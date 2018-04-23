#include <eosio/chain/fork_database.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <fc/io/fstream.hpp>
#include <fstream>

namespace eosio { namespace chain {
   using boost::multi_index_container;
   using namespace boost::multi_index;


   struct by_block_id;
   struct by_block_num;
   struct by_lib_block_num;
   struct by_prev;
   typedef multi_index_container<
      block_state_ptr,
      indexed_by<
         hashed_unique< tag<by_block_id>, member<block_header_state, block_id_type, &block_header_state::id>, std::hash<block_id_type>>,
         hashed_non_unique< tag<by_prev>, const_mem_fun<block_header_state,
                                         const block_id_type&, &block_header_state::prev>,
                                         std::hash<block_id_type>>,
         ordered_non_unique< tag<by_block_num>,
            composite_key< block_state,
               member<block_header_state,uint32_t,&block_header_state::block_num>,
               member<block_state,bool,&block_state::in_current_chain>
            >,
            composite_key_compare< std::less<uint32_t>, std::greater<bool> >
         >,
         ordered_non_unique< tag<by_lib_block_num>,
            composite_key< block_header_state,
                member<block_header_state,uint32_t,&block_header_state::dpos_last_irreversible_blocknum>,
                member<block_header_state,uint32_t,&block_header_state::block_num>
            >,
            composite_key_compare< std::greater<uint32_t>, std::greater<uint32_t> >
         >
      >
   > fork_multi_index_type;


   struct fork_database_impl {
      fork_multi_index_type index;
      block_state_ptr       head;
      fc::path              datadir;
   };


   fork_database::fork_database( const fc::path& data_dir ):my( new fork_database_impl() ) {
      my->datadir = data_dir;

      if (!fc::is_directory(my->datadir))
         fc::create_directories(my->datadir);

      auto fork_db_dat = my->datadir / "forkdb.dat";
      if( fc::exists( fork_db_dat ) ) {
         string content;
         fc::read_file_contents( fork_db_dat, content );

         fc::datastream<const char*> ds( content.data(), content.size() );
         vector<block_state>  states;
         fc::raw::unpack( ds, states );

         for( auto& s : states ) {
            set( std::make_shared<block_state>( move( s ) ) );
         }
         block_id_type head_id;
         fc::raw::unpack( ds, head_id );

         my->head = get_block( head_id );
      }
   }

   fork_database::~fork_database() {
      fc::datastream<size_t> ps;
      vector<block_state>  states;
      states.reserve( my->index.size() );
      for( const auto& s : my->index ) {
         states.push_back( *s );
      }

      auto fork_db_dat = my->datadir / "forkdb.dat";
      std::ofstream out( fork_db_dat.generic_string().c_str(), std::ios::out | std::ios::binary | std::ofstream::trunc );
      fc::raw::pack( out, states );
      if( my->head )
         fc::raw::pack( out, my->head->id );
      else
         fc::raw::pack( out, block_id_type() );
      idump((states.size()));
   }

   void fork_database::set( block_state_ptr s ) {
      auto result = my->index.insert( s );
      FC_ASSERT( s->id == s->header.id() );

         //FC_ASSERT( s->block_num == s->header.block_num() );

      FC_ASSERT( result.second, "unable to insert block state, duplicate state detected" );
      if( !my->head ) {
         my->head =  s;
      } else if( my->head->block_num < s->block_num ) {
         my->head =  s;
      }
   }

   block_state_ptr fork_database::add( block_state_ptr n ) {
      auto inserted = my->index.insert(n);
      FC_ASSERT( inserted.second, "duplicate block added?" );

      my->head = *my->index.get<by_lib_block_num>().begin();
      return n;
   }

   block_state_ptr fork_database::add( signed_block_ptr b ) {
      FC_ASSERT( my->head, "no head block set" );

      const auto& by_id_idx = my->index.get<by_block_id>();
      auto existing = by_id_idx.find( b->id() );
      FC_ASSERT( existing == by_id_idx.end(), "we already know about this block" );

      auto prior = by_id_idx.find( b->previous );
      FC_ASSERT( prior != by_id_idx.end(), "unlinkable block" );

      auto result = std::make_shared<block_state>( **prior, move(b) );
      return add(result);
   }

   const block_state_ptr& fork_database::head()const { return my->head; }

   /**
    *  Given two head blocks, return two branches of the fork graph that
    *  end with a common ancestor (same prior block)
    */
   pair< branch_type, branch_type >  fork_database::fetch_branch_from( const block_id_type& first,
                                                                       const block_id_type& second )const {
      pair<branch_type,branch_type> result;
      auto first_branch = get_block(first);
      auto second_branch = get_block(second);

      while( first_branch->block_num > second_branch->block_num )
      {
         result.first.push_back(first_branch);
         first_branch = get_block( first_branch->header.previous );
         FC_ASSERT( first_branch );
      }

      while( second_branch->block_num > first_branch->block_num )
      {
         result.second.push_back( second_branch );
         second_branch = get_block( second_branch->header.previous );
         FC_ASSERT( second_branch );
      }

      while( first_branch->header.previous != second_branch->header.previous )
      {
         result.first.push_back(first_branch);
         result.second.push_back(second_branch);
         first_branch = get_block( first_branch->header.previous );
         second_branch = get_block( second_branch->header.previous );
         FC_ASSERT( first_branch && second_branch );
      }

      if( first_branch && second_branch )
      {
         result.first.push_back(first_branch);
         result.second.push_back(second_branch);
      }
      return result;
   } /// fetch_branch_from

   /// remove all of the invalid forks built of this id including this id
   void fork_database::remove( const block_id_type& id ) {
      vector<block_id_type> remove_queue{id};

      for( uint32_t i = 0; i < remove_queue.size(); ++i ) {
         auto itr = my->index.find( remove_queue[i] );
         if( itr != my->index.end() )
            my->index.erase(itr);

         auto& previdx = my->index.get<by_prev>();
         auto  previtr = previdx.find(id);
         while( previtr != previdx.end() ) {
            remove_queue.push_back( (*previtr)->id );
            previdx.erase(previtr);
            previtr = previdx.find(id);
         }
      }
   }

   void fork_database::set_validity( const block_state_ptr& h, bool valid ) {
      if( !valid ) {
         remove( h->id );
      } else {
         /// remove older than irreversible and mark block as valid
         h->validated = true;
      }
   }

   void fork_database::mark_in_current_chain( const block_state_ptr& h, bool in_current_chain ) {
      if( h->in_current_chain == in_current_chain )
         return;

      auto& by_id_idx = my->index.get<by_block_id>();
      auto itr = by_id_idx.find( h->id );
      FC_ASSERT( itr != by_id_idx.end(), "could not find block in fork database" );

      by_id_idx.modify( itr, [&]( auto& bsp ) { // Need to modify this way rather than directly so that Boost MultiIndex can re-sort
         bsp->in_current_chain = in_current_chain;
      });
   }

   void fork_database::prune( const block_state_ptr& h ) {
      auto num = h->block_num;

      auto itr = my->index.find( h->id );
      if( itr != my->index.end() )
         my->index.erase(itr);

      auto& numidx = my->index.get<by_block_num>();
      for( auto nitr = numidx.lower_bound( num ); nitr != numidx.end() && (*nitr)->block_num == num; ++nitr ) {
         remove( (*nitr)->id );
      }
   }

   block_state_ptr   fork_database::get_block(const block_id_type& id)const {
      auto itr = my->index.find( id );
      if( itr != my->index.end() )
         return *itr;
      return block_state_ptr();
   }

   block_state_ptr   fork_database::get_block_in_current_chain_by_num( uint32_t n )const {
      const auto& numidx = my->index.get<by_block_num>();
      auto nitr = numidx.lower_bound( n );
      FC_ASSERT( nitr != numidx.end() && (*nitr)->block_num == n,
                 "could not find block in fork database with block number ${block_num}", ("block_num", n) );
      FC_ASSERT( (*nitr)->in_current_chain == true,
                 "block (with block number ${block_num}) found in fork database is not in the current chain", ("block_num", n) );
      return *nitr;
   }

} } /// eosio::chain
