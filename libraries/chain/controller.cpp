#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/fork_database.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/scope_sequence_object.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/action_objects.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/permission_link_object.hpp>

#include <eosio/chain/resource_limits.hpp>

#include <chainbase/chainbase.hpp>
#include <fc/io/json.hpp>

#include <eosio/chain/eosio_contract.hpp>

namespace eosio { namespace chain {

using resource_limits::resource_limits_manager;


struct pending_state {
   pending_state( database::session&& s )
   :_db_session( move(s) ){}

   database::session                  _db_session;
   vector<transaction_metadata_ptr>   _applied_transaction_metas;

   block_state_ptr                    _pending_block_state;

   vector<action_receipt>             _actions;


   void push() {
      _db_session.push();
   }
};

struct controller_impl {
   chainbase::database            db;
   block_log                      blog;
   optional<pending_state>        pending;
   block_state_ptr                head; 
   fork_database                  fork_db;            
   wasm_interface                 wasmif;
   resource_limits_manager        resource_limits;
   controller::config             conf;
   controller&                    self;

   typedef pair<scope_name,action_name>                   handler_key;
   map< account_name, map<handler_key, apply_handler> >   apply_handlers;

   /**
    *  Transactions that were undone by pop_block or abort_block, transactions
    *  are removed from this list if they are re-applied in other blocks. Producers
    *  can query this list when scheduling new transactions into blocks.
    */
   map<transaction_id_type, transaction_metadata_ptr>     unapplied_transactions;

   block_id_type head_block_id()const {
      return head->id;
   }
   time_point head_block_time()const {
      return head->header.timestamp;
   }
   const block_header& head_block_header()const {
      return head->header;
   }

   void pop_block() {
      for( const auto& t : head->trxs )
         unapplied_transactions[t->id] = t;
      head = fork_db.get( head->previous );
      db.undo();
   }


   void set_apply_handler( account_name contract, scope_name scope, action_name action, apply_handler v ) {
      apply_handlers[contract][make_pair(scope,action)] = v;
   }

   controller_impl( const controller::config& cfg, controller& s  )
   :db( cfg.shared_memory_dir, 
        cfg.read_only ? database::read_only : database::read_write,
        cfg.shared_memory_size ),
    blog( cfg.block_log_dir ),
    fork_db( cfg.shared_memory_dir ),
    wasmif( cfg.wasm_runtime ),
    resource_limits( db ),
    conf( cfg ),
    self( s )
   {
      head = fork_db.head();


#define SET_APP_HANDLER( contract, scope, action, nspace ) \
   set_apply_handler( #contract, #scope, #action, &BOOST_PP_CAT(apply_, BOOST_PP_CAT(contract, BOOST_PP_CAT(_,action) ) ) )
   SET_APP_HANDLER( eosio, eosio, newaccount, eosio );
   SET_APP_HANDLER( eosio, eosio, setcode, eosio );
   SET_APP_HANDLER( eosio, eosio, setabi, eosio );
   SET_APP_HANDLER( eosio, eosio, updateauth, eosio );
   SET_APP_HANDLER( eosio, eosio, deleteauth, eosio );
   SET_APP_HANDLER( eosio, eosio, linkauth, eosio );
   SET_APP_HANDLER( eosio, eosio, unlinkauth, eosio );
   SET_APP_HANDLER( eosio, eosio, onerror, eosio );
   SET_APP_HANDLER( eosio, eosio, postrecovery, eosio );
   SET_APP_HANDLER( eosio, eosio, passrecovery, eosio );
   SET_APP_HANDLER( eosio, eosio, vetorecovery, eosio );
   SET_APP_HANDLER( eosio, eosio, canceldelay, eosio );


   }

   void init() {
      // ilog( "${c}", ("c",fc::json::to_pretty_string(cfg)) );
      initialize_indicies();

      initialize_fork_db();
      /**
       * The undable state contains state transitions from blocks
       * in the fork database that could be reversed. Because this
       * is a new startup and the fork database is empty, we must
       * unwind that pending state. This state will be regenerated
       * when we catch up to the head block later.
       */
      //clear_all_undo(); 
   }

   ~controller_impl() {
      pending.reset();

      edump((db.revision())(head->block_num));

      db.flush();
   }

   void initialize_indicies() {
      db.add_index<account_index>();
      db.add_index<account_sequence_index>();
      db.add_index<permission_index>();
      db.add_index<permission_usage_index>();
      db.add_index<permission_link_index>();
      db.add_index<action_permission_index>();

      db.add_index<table_id_multi_index>();
      db.add_index<key_value_index>();
      db.add_index<index64_index>();
      db.add_index<index128_index>();
      db.add_index<index256_index>();
      db.add_index<index_double_index>();

      db.add_index<global_property_multi_index>();
      db.add_index<dynamic_global_property_multi_index>();
      db.add_index<block_summary_multi_index>();
      db.add_index<transaction_multi_index>();
      db.add_index<generated_transaction_multi_index>();
      db.add_index<scope_sequence_multi_index>();

      resource_limits.initialize_database();
   }

   void abort_pending_block() {
      pending.reset();
   }

   void clear_all_undo() {
      // Rewind the database to the last irreversible block
      db.with_write_lock([&] {
         db.undo_all();
         /*
         FC_ASSERT(db.revision() == self.head_block_num(), 
                   "Chainbase revision does not match head block num",
                   ("rev", db.revision())("head_block", self.head_block_num()));
                   */
      });
   }

   /**
    *  The fork database needs an initial block_state to be set before
    *  it can accept any new blocks. This initial block state can be found
    *  in the database (whose head block state should be irreversible) or
    *  it would be the genesis state.
    */
   void initialize_fork_db() {
      head = fork_db.head();
      if( !head ) {
         wlog( " Initializing new blockchain with genesis state                  " );
         producer_schedule_type initial_schedule{ 0, {{N(eosio), conf.genesis.initial_key}} };

         block_header_state genheader;
         genheader.active_schedule       = initial_schedule;
         genheader.pending_schedule      = initial_schedule;
         genheader.pending_schedule_hash = fc::sha256::hash(initial_schedule);
         genheader.header.timestamp      = conf.genesis.initial_timestamp;
         genheader.header.action_mroot   = conf.genesis.compute_chain_id();
         genheader.id                    = genheader.header.id();
         genheader.block_num             = genheader.header.block_num();

         head = std::make_shared<block_state>( genheader );
         signed_block genblock(genheader.header);

         edump((genheader.header));
         edump((genblock));
         blog.append( genblock );

         db.set_revision( head->block_num );
         fork_db.set( head );
  
         initialize_database();
      }
      FC_ASSERT( db.revision() == head->block_num, "fork database is inconsistant with shared memory",
                 ("db",db.revision())("head",head->block_num) );
   }

   void create_native_account( account_name name ) {
      db.create<account_object>([&](auto& a) {
         a.name = name;
         a.creation_date = conf.genesis.initial_timestamp;
         a.privileged = true;

         if( name == config::system_account_name ) {
            a.set_abi(eosio_contract_abi(abi_def()));
         }
      });
      db.create<account_sequence_object>([&](auto & a) {
        a.name = name;
      });

      const auto& owner = db.create<permission_object>([&](auto& p) {
         p.owner = name;
         p.name = "owner";
         p.auth.threshold = 1;
         p.auth.keys.push_back( key_weight{ .key = conf.genesis.initial_key, .weight = 1 } );
      });
      db.create<permission_object>([&](auto& p) {
         p.owner = name;
         p.parent = owner.id;
         p.name = "active";
         p.auth.threshold = 1;
         p.auth.keys.push_back( key_weight{ .key = conf.genesis.initial_key, .weight = 1 } );
      });

      resource_limits.initialize_account(name);
   }

   void initialize_database() {
      // Initialize block summary index
      for (int i = 0; i < 0x10000; i++)
         db.create<block_summary_object>([&](block_summary_object&) {});

      db.create<global_property_object>([](auto){});
      db.create<dynamic_global_property_object>([](auto){});
      db.create<permission_object>([](auto){}); /// reserve perm 0 (used else where)

      resource_limits.initialize_chain();

      create_native_account( config::system_account_name );

      // Create special accounts
      auto create_special_account = [&](account_name name, const auto& owner, const auto& active) {
         db.create<account_object>([&](account_object& a) {
            a.name = name;
            a.creation_date = conf.genesis.initial_timestamp;
         });
         db.create<account_sequence_object>([&](auto & a) {
           a.name = name;
         });
         const auto& owner_permission = db.create<permission_object>([&](permission_object& p) {
            p.name = config::owner_name;
            p.parent = 0;
            p.owner = name;
            p.auth = move(owner);
         });
         db.create<permission_object>([&](permission_object& p) {
            p.name = config::active_name;
            p.parent = owner_permission.id;
            p.owner = owner_permission.owner;
            p.auth = move(active);
         });
      };

      auto empty_authority = authority(0, {}, {});
      auto active_producers_authority = authority(0, {}, {});
      active_producers_authority.accounts.push_back({{config::system_account_name, config::active_name}, 1});

      create_special_account(config::nobody_account_name, empty_authority, empty_authority);
      create_special_account(config::producers_account_name, empty_authority, active_producers_authority);
   }


   void commit_block( bool add_to_fork_db ) {
      if( add_to_fork_db ) {
         pending->_pending_block_state->validated = true;
         head = fork_db.add( pending->_pending_block_state );
      } 
      pending->push();
      pending.reset();
      self.accepted_block( head );
   }

   transaction_trace_ptr push_transaction( const transaction_metadata_ptr& trx ) {
      return db.with_write_lock( [&](){ 
         return apply_transaction( trx );
      });
   }

   transaction_trace_ptr apply_transaction( const transaction_metadata_ptr& trx, bool implicit = false ) {
      transaction_context trx_context( self, trx );
      trx_context.processing_deadline = fc::time_point::now() + conf.limits.max_push_transaction_us;
      trx_context.exec();

      auto& acts = pending->_actions;
      fc::move_append( acts, move(trx_context.executed) );

      if( !implicit ) {
         pending->_pending_block_state->block->transactions.emplace_back( trx->packed_trx );
         pending->_pending_block_state->trxs.emplace_back(trx);
      }

      return move(trx_context.trace);
   }


   void record_transaction( const transaction_metadata_ptr& trx ) {
      try {
          db.create<transaction_object>([&](transaction_object& transaction) {
              transaction.trx_id = trx->id;
              transaction.expiration = trx->trx.expiration;
          });
      } catch ( ... ) {
          EOS_ASSERT( false, transaction_exception,
                     "duplicate transaction ${id}", ("id", trx->id ) );
      }
   } /// record_transaction

   void start_block( block_timestamp_type when ) {
     FC_ASSERT( !pending );

     FC_ASSERT( db.revision() == head->block_num );

     pending = db.start_undo_session(true);
     pending->_pending_block_state = std::make_shared<block_state>( *head, when );

     try {
        auto onbtrx = std::make_shared<transaction_metadata>( get_on_block_transaction() );
        apply_transaction( onbtrx, true );
     } catch ( ... ) {
        ilog( "on block transaction failed, but shouldn't impact block generation, system contract needs update" );
     }
   } // start_block



   void sign_block( const std::function<signature_type( const digest_type& )>& signer_callback ) {
      auto p = pending->_pending_block_state;
      p->sign( signer_callback );
      static_cast<signed_block_header&>(*p->block) = p->header;
   } /// sign_block

   void apply_block( const signed_block_ptr& b ) { 
      try {
         start_block( b->timestamp );

         for( const auto& receipt : b->transactions ) {
            if( receipt.trx.contains<packed_transaction>() ) {
               auto& pt = receipt.trx.get<packed_transaction>();
               auto mtrx = std::make_shared<transaction_metadata>(pt);
               push_transaction( mtrx );
            }
         }

         finalize_block();
         sign_block( [&]( const auto& ){ return b->producer_signature; } );

         // this is implied by the signature passing
         //FC_ASSERT( b->id() == pending->_pending_block_state->block->id(),
         //           "applying block didn't produce expected block id" );

         commit_block(false);
         return;
      } catch ( const fc::exception& e ) {
         edump((e.to_detail_string()));
         abort_block();
         throw;
      }
   } /// apply_block

   void push_block( const signed_block_ptr& b ) {

      auto new_header_state = fork_db.add( b );
      self.accepted_block_header( new_header_state );

      auto new_head = fork_db.head();

      if( new_head->header.previous == head->id ) {
         try {
            abort_block();
            apply_block( b );
            fork_db.set_validity( new_head, true );
            head = new_head;
         } catch ( const fc::exception& e ) {
            fork_db.set_validity( new_head, false );
            throw;
         }
      } else {
         auto branches = fork_db.fetch_branch_from( new_head->id, head->id );

         while( head_block_id() != branches.second.back()->header.previous )
            pop_block();

         for( auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr) {
            optional<fc::exception> except;
            try {
               apply_block( (*ritr)->block );
            }
            catch (const fc::exception& e) { except = e; }
            if (except) {
               wlog("exception thrown while switching forks ${e}", ("e",except->to_detail_string()));
         
               while (ritr != branches.first.rend() ) {
                  fork_db.set_validity( *ritr, false );
                  ++ritr;
               }
         
               // pop all blocks from the bad fork
               while( head_block_id() != branches.second.back()->header.previous )
                  pop_block();
               
               // re-apply good blocks
               for( auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr ) {
                  apply_block( (*ritr)->block );
               }
               throw *except;
            } // end if exception
         } /// end for each block in branch
      }
   } /// push_block

   vector<transaction_metadata_ptr> abort_block() {
      vector<transaction_metadata_ptr> pushed;
      if( pending ) {
         pushed = move(pending->_applied_transaction_metas);
         pending.reset();
      }
      return pushed;
   }


   bool should_enforce_runtime_limits()const {
      return false;
   }

   /*
   void validate_net_usage( const transaction& trx, uint32_t min_net_usage ) {
      uint32_t net_usage_limit = trx.max_net_usage_words.value * 8; // overflow checked in validate_transaction_without_state
      EOS_ASSERT( net_usage_limit == 0 || min_net_usage <= net_usage_limit,
                  transaction_exception,
                  "Packed transaction and associated data does not fit into the space committed to by the transaction's header! [usage=${usage},commitment=${commit}]",
                  ("usage", min_net_usage)("commit", net_usage_limit));
   } /// validate_net_usage
   */


   void set_action_merkle() {
      vector<digest_type> action_digests;
      action_digests.reserve( pending->_actions.size() );
      for( const auto& a : pending->_actions )
         action_digests.emplace_back( a.digest() );

      pending->_pending_block_state->header.action_mroot = merkle( move(action_digests) );
   }

   void set_trx_merkle() {
      vector<digest_type> trx_digests;
      const auto& trxs = pending->_pending_block_state->block->transactions;
      trx_digests.reserve( trxs.size() );
      for( const auto& a : trxs )
         trx_digests.emplace_back( a.digest() );

      pending->_pending_block_state->header.transaction_mroot = merkle( move(trx_digests) );
   }


   void finalize_block() 
   { try {
      ilog( "finalize block ${p} ${t} schedule_version: ${v} lib: ${lib} ${np}  ${signed}", 
            ("p",pending->_pending_block_state->header.producer)
            ("t",pending->_pending_block_state->header.timestamp)
            ("v",pending->_pending_block_state->header.schedule_version)
            ("lib",pending->_pending_block_state->dpos_last_irreversible_blocknum)
            ("np",pending->_pending_block_state->header.new_producers)
            ("signed", pending->_pending_block_state->block_signing_key) 
            );

      set_action_merkle();
      set_trx_merkle();

      auto p = pending->_pending_block_state;
      p->id = p->header.id();

      create_block_summary();


      /* TODO RESTORE 
      const auto& b = trace.block;
      update_global_properties( b );
      update_global_dynamic_data( b );
      update_signing_producer(signing_producer, b);

      create_block_summary(b);
      clear_expired_transactions();

      update_last_irreversible_block();

      resource_limits.process_account_limit_updates();

      const auto& chain_config = self.get_global_properties().configuration;
      resource_limits.set_block_parameters(
         {EOS_PERCENT(chain_config.max_block_cpu_usage, chain_config.target_block_cpu_usage_pct), chain_config.max_block_cpu_usage, config::block_cpu_usage_average_window_ms / config::block_interval_ms, 1000, {99, 100}, {1000, 999}},
         {EOS_PERCENT(chain_config.max_block_net_usage, chain_config.target_block_net_usage_pct), chain_config.max_block_net_usage, config::block_size_average_window_ms / config::block_interval_ms, 1000, {99, 100}, {1000, 999}}
      );

      */
   } FC_CAPTURE_AND_RETHROW() }


   void create_block_summary() {
      auto p = pending->_pending_block_state;
      auto sid = p->block_num & 0xffff;
      db.modify( db.get<block_summary_object,by_id>(sid), [&](block_summary_object& bso ) {
          bso.block_id = p->id;
      });
   }

   /**
    *  This method only works for blocks within the TAPOS range, (last 65K blocks). It
    *  will return block_id_type() for older blocks.
    */
   block_id_type get_block_id_for_num( uint32_t block_num ) {
      auto sid = block_num & 0xffff;
      auto id  = db.get<block_summary_object,by_id>(sid).block_id;
      auto num = block_header::num_from_id( id );
      if( num == block_num ) return id;
      return block_id_type();
   }

   void clear_expired_transactions() {
      //Look for expired transactions in the deduplication list, and remove them.
      auto& transaction_idx = db.get_mutable_index<transaction_multi_index>();
      const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
      while( (!dedupe_index.empty()) && (head_block_time() > fc::time_point(dedupe_index.begin()->expiration) ) ) {
         transaction_idx.remove(*dedupe_index.begin());
      }

      // Look for expired transactions in the pending generated list, and remove them.
      // TODO: expire these by sending error to handler
      auto& generated_transaction_idx = db.get_mutable_index<generated_transaction_multi_index>();
      const auto& generated_index = generated_transaction_idx.indices().get<by_expiration>();
      while( (!generated_index.empty()) && (head_block_time() > generated_index.begin()->expiration) ) {
      // TODO:   destroy_generated_transaction(*generated_index.begin());
      }
   }

   /*
   bool should_check_tapos()const { return true; }

   void validate_tapos( const transaction& trx )const {
      if( !should_check_tapos() ) return;

      const auto& tapos_block_summary = db.get<block_summary_object>((uint16_t)trx.ref_block_num);

      //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
      EOS_ASSERT(trx.verify_reference_block(tapos_block_summary.block_id), invalid_ref_block_exception,
                 "Transaction's reference block did not match. Is this transaction from a different fork?",
                 ("tapos_summary", tapos_block_summary));
   }
   */


   /**
    *  At the start of each block we notify the system contract with a transaction that passes in
    *  the block header of the prior block (which is currently our head block)
    */
   signed_transaction get_on_block_transaction()
   {
      action on_block_act;
      on_block_act.account = config::system_account_name;
      on_block_act.name = N(onblock);
      on_block_act.authorization = vector<permission_level>{{config::system_account_name, config::active_name}};
      on_block_act.data = fc::raw::pack(head_block_header());

      signed_transaction trx;
      trx.actions.emplace_back(std::move(on_block_act));
      trx.set_reference_block(head_block_id());
      trx.expiration = head_block_time() + fc::seconds(1);
      return trx;
   }

}; /// controller_impl

const resource_limits_manager&   controller::get_resource_limits_manager()const 
{
   return my->resource_limits;
}
resource_limits_manager&         controller::get_mutable_resource_limits_manager() 
{
   return my->resource_limits;
}


controller::controller( const controller::config& cfg )
:my( new controller_impl( cfg, *this ) )
{
   my->init();
}

controller::~controller() {
}


void controller::startup() {
   my->head = my->fork_db.head();
   if( !my->head ) {
   }

   /*
   auto head = my->blog.read_head();
   if( head && head_block_num() < head->block_num() ) {
      wlog( "\nDatabase in inconsistant state, replaying block log..." );
      //replay();
   }
   */
}

chainbase::database& controller::db()const { return my->db; }


void controller::start_block( block_timestamp_type when ) {
   my->start_block(when);
}

void controller::finalize_block() {
   my->finalize_block();
}

void controller::sign_block( const std::function<signature_type( const digest_type& )>& signer_callback ) {
   my->sign_block( signer_callback );
}

void controller::commit_block() {
   my->commit_block(true);
}

block_state_ptr controller::head_block_state()const {
   return my->head;
}

block_state_ptr controller::pending_block_state()const {
   if( my->pending ) return my->pending->_pending_block_state;
   return block_state_ptr();
}

vector<transaction_metadata_ptr> controller::abort_block() {
   return my->abort_block();
}

void controller::push_block( const signed_block_ptr& b ) {
   my->push_block( b );
}

transaction_trace_ptr controller::push_transaction( const transaction_metadata_ptr& trx ) { 
   return my->push_transaction(trx);
}

transaction_trace_ptr controller::push_next_scheduled_transaction() {
   return transaction_trace_ptr();
}
transaction_trace_ptr controller::push_scheduled_transaction( const transaction_id_type& trxid ) {
   /// lookup scheduled trx and then apply it...
   return transaction_trace_ptr();
}

uint32_t controller::head_block_num()const {
   return my->head->block_num;
}
block_id_type controller::head_block_id()const {
   return my->head->id;
}

time_point controller::head_block_time()const {
   return my->head->header.timestamp;
}

void     controller::record_transaction( const transaction_metadata_ptr& trx ) {
   my->record_transaction( trx );
}

const dynamic_global_property_object& controller::get_dynamic_global_properties()const {
  return my->db.get<dynamic_global_property_object>();
}
const global_property_object& controller::get_global_properties()const {
  return my->db.get<global_property_object>();
}

/**
 *  This method reads the current dpos_irreverible block number, if it is higher
 *  than the last block number of the log, it grabs the next block from the
 *  fork database, saves it to disk, then removes the block from the fork database. 
 *
 *  Any forks built off of a different block with the same number are also pruned.
 */
void controller::log_irreversible_blocks() {
   if( !my->blog.head() ) 
      my->blog.read_head();
 
   const auto& log_head = my->blog.head();
   auto lib = my->head->dpos_last_irreversible_blocknum;

   if( lib > 1 ) {
      while( log_head && log_head->block_num() < lib ) {
         auto lhead = log_head->block_num();
         auto blk_id = my->get_block_id_for_num( lhead + 1 );
         auto blk = my->fork_db.get_block( blk_id );
         FC_ASSERT( blk, "unable to find block state", ("id",blk_id));
         irreversible_block( blk );
         my->blog.append( *blk->block );
         my->fork_db.prune( blk );
         my->db.commit( lhead );
      }
   }
}
signed_block_ptr controller::fetch_block_by_id( block_id_type id )const {
   auto state = my->fork_db.get_block(id);
   if( state ) return state->block;
   auto bptr = fetch_block_by_number( block_header::num_from_id(id) );
   if( bptr->id() == id ) return bptr;
   return signed_block_ptr();
}

signed_block_ptr controller::fetch_block_by_number( uint32_t block_num )const  {
   optional<signed_block> b = my->blog.read_block_by_num(block_num);
   if( b ) return std::make_shared<signed_block>( move(*b) );

   auto blk_id = my->get_block_id_for_num( block_num );
   auto blk_state =  my->fork_db.get_block( blk_id );
   if( blk_state ) return blk_state->block;
   return signed_block_ptr();
}

void controller::pop_block() {
   auto prev = my->fork_db.get_block( my->head->header.previous );
   FC_ASSERT( prev, "attempt to pop beyond last irreversible block" );
   my->db.undo();
   my->head = prev;
}


/**
 * @param actions - the actions to check authorization across
 * @param provided_keys - the set of public keys which have authorized the transaction
 * @param allow_unused_signatures - true if method should not assert on unused signatures
 * @param provided_accounts - the set of accounts which have authorized the transaction (presumed to be owner)
 *
 * @return fc::microseconds set to the max delay that this authorization requires to complete
 */
fc::microseconds controller::check_authorization( const vector<action>& actions,
                                      const flat_set<public_key_type>& provided_keys,
                                      bool                             allow_unused_signatures,
                                      flat_set<account_name>           provided_accounts,
                                      flat_set<permission_level>       provided_levels
                                    )const {
   return fc::microseconds();
}

/**
 * @param account - the account owner of the permission
 * @param permission - the permission name to check for authorization
 * @param provided_keys - a set of public keys
 *
 * @return true if the provided keys are sufficient to authorize the account permission
 */
bool controller::check_authorization( account_name account, permission_name permission,
                       flat_set<public_key_type> provided_keys,
                       bool allow_unused_signatures)const {
   return true;
}

void controller::set_active_producers( const producer_schedule_type& sch ) {
   FC_ASSERT( !my->pending->_pending_block_state->header.new_producers, "this block has already set new producers" );
   FC_ASSERT( !my->pending->_pending_block_state->pending_schedule.producers.size(), "there is already a pending schedule, wait for it to become active" );
   my->pending->_pending_block_state->set_new_producers( sch );
}
const producer_schedule_type& controller::active_producers()const {
   return my->pending->_pending_block_state->active_schedule;
}

const producer_schedule_type& controller::pending_producers()const {
   return my->pending->_pending_block_state->pending_schedule;
}

const apply_handler* controller::find_apply_handler( account_name receiver, account_name scope, action_name act ) const
{
   auto native_handler_scope = my->apply_handlers.find( receiver );
   if( native_handler_scope != my->apply_handlers.end() ) {
      auto handler = native_handler_scope->second.find( make_pair( scope, act ) );
      if( handler != native_handler_scope->second.end() )
         return &handler->second;
   }
   return nullptr;
}
wasm_interface& controller::get_wasm_interface() {
   return my->wasmif;
}

const account_object& controller::get_account( account_name name )const 
{ try {
   return my->db.get<account_object, by_name>(name);
} FC_CAPTURE_AND_RETHROW( (name) ) }

   
} } /// eosio::chain
