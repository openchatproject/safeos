#include <eosio/testing/tester.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/contracts/types.hpp>

#include <fc/utility.hpp>
#include <fc/io/json.hpp>

#include "WAST/WAST.h"
#include "WASM/WASM.h"
#include "IR/Module.h"
#include "IR/Validate.h"

namespace eosio { namespace testing {

   tester::tester() {
      cfg.block_log_dir      = tempdir.path() / "blocklog";
      cfg.shared_memory_dir  = tempdir.path() / "shared";
      cfg.shared_memory_size = 1024*1024*8;

      cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
      cfg.genesis.initial_accounts.resize( config::producer_count );
      cfg.genesis.initial_producers.resize( config::producer_count );

     // uint64_t init_name = N(inita);
      string init_name = "inita";
      for( uint32_t i = 0; i < config::producer_count; ++i ) {
         auto pubkey = get_public_key(init_name);
         cfg.genesis.initial_accounts[i].name               = string(account_name(init_name));
         cfg.genesis.initial_accounts[i].owner_key          = get_public_key(init_name,"owner");
         cfg.genesis.initial_accounts[i].active_key         = get_public_key(init_name,"active");
         cfg.genesis.initial_accounts[i].liquid_balance     = asset::from_string( "1000000.0000 EOS" );
         cfg.genesis.initial_accounts[i].staking_balance    = asset::from_string( "1000000.0000 EOS" );
         cfg.genesis.initial_producers[i].owner_name        = string(account_name(init_name));
         cfg.genesis.initial_producers[i].block_signing_key = get_public_key( init_name, "producer" );

         init_name[4]++;
      }
      open();
   }

   public_key_type  tester::get_public_key( name keyname, string role ) { 
      return get_private_key( keyname, role ).get_public_key(); 
   }

   private_key_type tester::get_private_key( name keyname, string role ) { 
      return private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(string(keyname)+role));
   }

   void tester::close() {
      control.reset();
      chain_transactions.clear();
   }
   void tester::open() {
      control.reset( new chain_controller(cfg) );
      chain_transactions.clear();
      // this will not index generated transactions
      control->applied_block.connect([this]( const signed_block& block ){
         for( const auto& region : block.regions) {
            for( const auto& cycle : region.cycles_summary ) {
               for ( const auto& shard : cycle ) {
                  for( const auto& receipt: shard ) {
                     chain_transactions.emplace(receipt.id, receipt);
                  }
               }
            }
         }
      });
   }

   signed_block tester::produce_block( fc::microseconds skip_time ) {
      auto head_time = control->head_block_time();
      auto next_time = head_time + skip_time;
      uint32_t slot  = control->get_slot_at_time( next_time );
      auto sch_pro   = control->get_scheduled_producer(slot);
      auto priv_key  = get_private_key( sch_pro, "producer" );

      return control->generate_block( next_time, sch_pro, priv_key );
   }


   void tester::produce_blocks( uint32_t n ) {
      for( uint32_t i = 0; i < n; ++i )
         produce_block();
   }

  void tester::set_tapos( signed_transaction& trx ) {
     trx.set_reference_block( control->head_block_id() );
  }


   void tester::create_account( account_name a, asset initial_balance, account_name creator ) {
      signed_transaction trx;
      set_tapos( trx );
      trx.write_scope = { creator, config::eosio_auth_scope };

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}}, 
                                contracts::newaccount{ 
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = authority( get_public_key( a, "owner" ) ),
                                   .active   = authority( get_public_key( a, "active" ) ),
                                   .recovery = authority( get_public_key( a, "recovery" ) ),
                                   .deposit  = initial_balance
                                });

      trx.sign( get_private_key( creator, "active" ), chain_id_type()  ); 

      control->push_transaction( trx );
   }

   void tester::push_transaction( signed_transaction& trx ) {
      set_tapos(trx);
      control->push_transaction( trx );
   }

   void tester::create_account( account_name a, string initial_balance, account_name creator ) {
      create_account( a, asset::from_string(initial_balance), creator );
   }
   

   void tester::transfer( account_name from, account_name to, string amount, string memo ) {
      transfer( from, to, asset::from_string(amount), memo );
   }
   void tester::transfer( account_name from, account_name to, asset amount, string memo ) {
      signed_transaction trx;
      trx.write_scope = {from,to};
      trx.actions.emplace_back( vector<permission_level>{{from,config::active_name}},
                                contracts::transfer{
                                   .from   = from,
                                   .to     = to,
                                   .amount = amount.amount,
                                   .memo   = memo
                                } );

      trx.sign( get_private_key( from, "active" ), chain_id_type()  ); 
      control->push_transaction( trx );
   }

   void tester::set_authority( account_name account,
                               permission_name perm,
                               authority auth,
                               permission_name parent ) { try {
      signed_transaction trx;
      trx.write_scope = {config::eosio_auth_scope};
      trx.actions.emplace_back( vector<permission_level>{{account,perm}},
                                contracts::updateauth{
                                   .account    = account,
                                   .permission = perm,
                                   .authority  = move(auth),
                                   .parent     = parent
                                });

      set_tapos( trx );
      trx.sign( get_private_key( account, "active" ), chain_id_type()  ); 
      control->push_transaction( trx );
   } FC_CAPTURE_AND_RETHROW( (account)(perm)(auth)(parent) ) }

   void tester::set_code( account_name account, const char* wast ) try {
      const auto assemble = [](const char* wast) -> vector<unsigned char> {
         using namespace IR;
         using namespace WAST;
         using namespace WASM;
         using namespace Serialization;

         Module module;
         vector<Error> parse_errors;
         parseModule(wast, fc::const_strlen(wast), module, parse_errors);
         if (!parse_errors.empty()) {
            fc::exception parse_exception(
               FC_LOG_MESSAGE(warn, "Failed to parse WAST"),
               fc::std_exception_code,
               "wast_parse_error",
               "Failed to parse WAST"
            );

            for (const auto& err: parse_errors) {
               parse_exception.append_log( FC_LOG_MESSAGE(error, ":${desc}: ${message}", ("desc", err.locus.describe())("message", err.message.c_str()) ) );
               parse_exception.append_log( FC_LOG_MESSAGE(error, string(err.locus.column(8), ' ') + "^" ));
            }

            throw parse_exception;
         }

         try {
            // Serialize the WebAssembly module.
            ArrayOutputStream stream;
            serialize(stream,module);
            return stream.getBytes();
         } catch(const FatalSerializationException& ex) {
            fc::exception serialize_exception (
               FC_LOG_MESSAGE(warn, "Failed to serialize wasm: ${message}", ("message", ex.message)),
               fc::std_exception_code,
               "wasm_serialization_error",
               "Failed to serialize WASM"
            );
            throw serialize_exception;
         }
      };

      auto wasm = assemble(wast);

      signed_transaction trx;
      trx.write_scope = {config::eosio_auth_scope};
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                                contracts::setcode{
                                   .account    = account,
                                   .vmtype     = 0,
                                   .vmversion  = 0,
                                   .code       = bytes(wasm.begin(), wasm.end())
                                });

      set_tapos( trx );
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );
      control->push_transaction( trx );
   } FC_CAPTURE_AND_RETHROW( (account)(wast) )

   void tester::set_abi( account_name account, const char* abi_json) {
      auto abi = fc::json::from_string(abi_json).template as<contracts::abi_def>();
      signed_transaction trx;
      trx.write_scope = {config::eosio_auth_scope};
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                                contracts::setabi{
                                   .account    = account,
                                   .abi        = abi
                                });

      set_tapos( trx );
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );
      control->push_transaction( trx );
   }

   bool tester::chain_has_transaction( const transaction_id_type& txid ) const {
      return chain_transactions.count(txid) != 0;
   }

   const transaction_receipt& tester::get_transaction_receipt( const transaction_id_type& txid ) const {
      return chain_transactions.at(txid);
   }


} }  /// eosio::test
