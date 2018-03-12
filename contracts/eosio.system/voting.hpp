/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include "common.hpp"

#include <eosiolib/eosio.hpp>
#include <eosiolib/token.hpp>
#include <eosiolib/print.hpp>

#include <eosiolib/generic_currency.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>

#include <array>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::member;
   using eosio::bytes;
   using eosio::print;
   using eosio::singleton;
   using eosio::transaction;


   template<account_name SystemAccount>
   class voting {
      public:
         static constexpr account_name system_account = SystemAccount;
         using currency = typename common<SystemAccount>::currency;
         using system_token_type = typename common<SystemAccount>::system_token_type;
         using eosio_parameters = typename common<SystemAccount>::eosio_parameters;
         using global_state_singleton = typename common<SystemAccount>::global_state_singleton;

         static const uint32_t max_inflation_rate = common<SystemAccount>::max_inflation_rate;
         static constexpr uint32_t max_unstake_requests = 10;
         static constexpr uint32_t unstake_pay_period = 7*24*3600; // one per week
         static constexpr uint32_t unstake_payments = 26; // during 26 weeks
         static constexpr uint32_t blocks_per_year = 52*7*24*2*3600; // half seconds per year

         struct producer_info {
            account_name      owner;
            uint64_t          padding = 0;
            uint128_t         total_votes = 0;
            eosio_parameters  prefs;
            eosio::bytes      packed_key; /// a packed public key object
            system_token_type per_block_payments;
            time              last_rewards_claim = 0;
            time              time_became_active = 0;
            time              last_produced_block_time = 0;

            uint64_t    primary_key()const { return owner;       }
            uint128_t   by_votes()const    { return total_votes; }
            bool active() const { return packed_key.size() == sizeof(public_key); }

            EOSLIB_SERIALIZE( producer_info, (owner)(total_votes)(prefs)(packed_key)
                              (per_block_payments)(last_rewards_claim)
                              (time_became_active)(last_produced_block_time) )
         };

         typedef eosio::multi_index< N(producerinfo), producer_info,
                                     indexed_by<N(prototalvote), const_mem_fun<producer_info, uint128_t, &producer_info::by_votes>  >
                                     >  producers_table;


         struct voter_info {
            account_name                owner = 0;
            account_name                proxy = 0;
            uint32_t                    last_update = 0;
            uint32_t                    is_proxy = 0;
            system_token_type           staked;
            system_token_type           unstaking;
            system_token_type           unstake_per_week;
            uint128_t                   proxied_votes = 0;
            std::vector<account_name>   producers;
            uint32_t                    deferred_trx_id = 0;
            time                        last_unstake_time = 0; //uint32

            uint64_t primary_key()const { return owner; }

            EOSLIB_SERIALIZE( voter_info, (owner)(proxy)(last_update)(is_proxy)(staked)(unstaking)(unstake_per_week)(proxied_votes)(producers)(deferred_trx_id)(last_unstake_time) )
         };

         typedef eosio::multi_index< N(voters), voter_info>  voters_table;

         ACTION( SystemAccount, regproducer ) {
            account_name     producer;
            bytes            producer_key;
            eosio_parameters prefs;

            EOSLIB_SERIALIZE( regproducer, (producer)(producer_key)(prefs) )
         };

         /**
          *  This method will create a producer_config and producer_info object for 'producer' 
          *
          *  @pre producer is not already registered
          *  @pre producer to register is an account
          *  @pre authority of producer to register 
          *  
          */
         static void on( const regproducer& reg ) {
            require_auth( reg.producer );

            producers_table producers_tbl( SystemAccount, SystemAccount );
            const auto* prod = producers_tbl.find( reg.producer );

            if ( prod ) {
               producers_tbl.update( *prod, reg.producer, [&]( producer_info& info ){
                     info.prefs = reg.prefs;
                     info.packed_key = reg.producer_key;
                  });
            } else {
               producers_tbl.emplace( reg.producer, [&]( producer_info& info ){
                     info.owner       = reg.producer;
                     info.total_votes = 0;
                     info.prefs       = reg.prefs;
                     info.packed_key  = reg.producer_key;
                  });
            }
         }

         ACTION( SystemAccount, unregprod ) {
            account_name producer;

            EOSLIB_SERIALIZE( unregprod, (producer) )
         };

         static void on( const unregprod& unreg ) {
            require_auth( unreg.producer );

            producers_table producers_tbl( SystemAccount, SystemAccount );
            const auto* prod = producers_tbl.find( unreg.producer );
            eosio_assert( bool(prod), "producer not found" );

            producers_tbl.update( *prod, 0, [&]( producer_info& info ){
                  info.packed_key.clear();
               });
         }

         static void increase_voting_power( account_name acnt, system_token_type amount ) {
            voters_table voters_tbl( SystemAccount, SystemAccount );
            const auto* voter = voters_tbl.find( acnt );

            if( !voter ) {
               voter = &voters_tbl.emplace( acnt, [&]( voter_info& a ) {
                     a.owner = acnt;
                     a.last_update = now();
                     a.staked = amount;
                  });
            } else {
               voters_tbl.update( *voter, 0, [&]( auto& av ) {
                     av.last_update = now();
                     av.staked += amount;
                  });
            }

            const std::vector<account_name>* producers = nullptr;
            if ( voter->proxy ) {
               auto proxy = voters_tbl.find( voter->proxy );
               voters_tbl.update( *proxy, 0, [&](voter_info& a) { a.proxied_votes += amount.quantity; } );
               if ( proxy->is_proxy ) { //only if proxy is still active. if proxy has been unregistered, we update proxied_votes, but don't propagate to producers
                  producers = &proxy->producers;
               }
            } else {
               producers = &voter->producers;
            }

            if ( producers ) {
               producers_table producers_tbl( SystemAccount, SystemAccount );
               for( auto p : *producers ) {
                  auto prod = producers_tbl.find( p );
                  eosio_assert( bool(prod), "never existed producer" ); //data corruption
                  producers_tbl.update( *prod, 0, [&]( auto& v ) {
                        v.total_votes += amount.quantity;
                     });
               }
            }
         }

         static void decrease_voting_power( account_name acnt, system_token_type amount ) {
            require_auth( acnt );
            voters_table voters_tbl( SystemAccount, SystemAccount );
            const auto* voter = voters_tbl.find( acnt );
            eosio_assert( bool(voter), "stake not found" );

            if ( 0 < amount.quantity ) {
               eosio_assert( amount <= voter->staked, "cannot unstake more than total stake amount" );
               /*
               if (voter->deferred_trx_id) {
                  //XXX cancel_deferred_transaction(voter->deferred_trx_id);
               }

               unstake_vote_deferred dt;
               dt.voter = acnt;
               uint32_t new_trx_id = 0;//XXX send_deferred(dt);

               avotes.update( *voter, 0, [&](voter_info& a) {
                     a.staked -= amount;
                     a.unstaking += a.unstaking + amount;
                     //round up to guarantee that there will be no unpaid balance after 26 weeks, and we are able refund amount < unstake_payments
                     a.unstake_per_week = system_token_type( a.unstaking.quantity /unstake_payments + a.unstaking.quantity % unstake_payments );
                     a.deferred_trx_id = new_trx_id;
                     a.last_update = now();
                  });
               */

               // Temporary code: immediate unstake
               voters_tbl.update( *voter, 0, [&](voter_info& a) {
                     a.staked -= amount;
                     a.last_update = now();
                  });
               //currency::inline_transfer( SystemAccount, acnt, amount, "unstake voting" );
               // end of temporary code

               const std::vector<account_name>* producers = nullptr;
               if ( voter->proxy ) {
                  auto proxy = voters_tbl.find( voter->proxy );
                  voters_tbl.update( *proxy, 0, [&](voter_info& a) { a.proxied_votes -= amount.quantity; } );
                  if ( proxy->is_proxy ) { //only if proxy is still active. if proxy has been unregistered, we update proxied_votes, but don't propagate to producers
                     producers = &proxy->producers;
                  }
               } else {
                  producers = &voter->producers;
               }

               if ( producers ) {
                  producers_table producers_tbl( SystemAccount, SystemAccount );
                  for( auto p : *producers ) {
                     auto prod = producers_tbl.find( p );
                     eosio_assert( bool(prod), "never existed producer" ); //data corruption
                     producers_tbl.update( *prod, 0, [&]( auto& v ) {
                           v.total_votes -= amount.quantity;
                        });
                  }
               }
            } else {
               if (voter->deferred_trx_id) {
                  //XXX cancel_deferred_transaction(voter->deferred_trx_id);
               }
               voters_tbl.update( *voter, 0, [&](voter_info& a) {
                     a.staked += a.unstaking;
                     a.unstaking.quantity = 0;
                     a.unstake_per_week.quantity = 0;
                     a.deferred_trx_id = 0;
                     a.last_update = now();
                  });
            }
         }

         static system_token_type payment_per_block(uint32_t percent_of_max_inflation_rate) {
            const system_token_type token_supply = currency::get_total_supply();
            const auto inflation_rate = max_inflation_rate * percent_of_max_inflation_rate;
            const auto& inflation_ratio = int_logarithm_one_plus(inflation_rate);
            return (token_supply * inflation_ratio.first) / (inflation_ratio.second * blocks_per_year);
         }

         static void update_elected_producers(time cycle_time) {
            producers_table producers_tbl( SystemAccount, SystemAccount );
            auto idx = producers_tbl.template get_index<N(prototalvote)>();

            std::array<uint32_t, 21> target_block_size;
            std::array<uint32_t, 21> max_block_size;
            std::array<uint32_t, 21> target_block_acts_per_scope;
            std::array<uint32_t, 21> max_block_acts_per_scope;
            std::array<uint32_t, 21> target_block_acts;
            std::array<uint32_t, 21> max_block_acts;
            std::array<uint64_t, 21> max_storage_size;
            std::array<uint32_t, 21> max_transaction_lifetime;
            std::array<uint16_t, 21> max_authority_depth;
            std::array<uint32_t, 21> max_transaction_exec_time;
            std::array<uint16_t, 21> max_inline_depth;
            std::array<uint32_t, 21> max_inline_action_size;
            std::array<uint32_t, 21> max_generated_transaction_size;
            std::array<uint32_t, 21> percent_of_max_inflation_rate;
            std::array<uint32_t, 21> storage_reserve_ratio;

            eosio::producer_schedule schedule;
            schedule.producers.reserve(21);

            auto it = idx.end();
            if (it == idx.begin()) {
                  return;
            }
            --it;
            size_t n = 0;
            while ( n < 21 ) {
               if ( it->active() ) {
                  schedule.producers.emplace_back();
                  schedule.producers.back().producer_name = it->owner;
                  eosio_assert( sizeof(schedule.producers.back().block_signing_key) == it->packed_key.size(), "size mismatch" );
                  std::copy(it->packed_key.begin(), it->packed_key.end(), schedule.producers.back().block_signing_key.data);

                  target_block_size[n] = it->prefs.target_block_size;
                  max_block_size[n] = it->prefs.max_block_size;

                  target_block_acts_per_scope[n] = it->prefs.target_block_acts_per_scope;
                  max_block_acts_per_scope[n] = it->prefs.max_block_acts_per_scope;

                  target_block_acts[n] = it->prefs.target_block_acts;
                  max_block_acts[n] = it->prefs.max_block_acts;

                  max_storage_size[n] = it->prefs.max_storage_size;
                  max_transaction_lifetime[n] = it->prefs.max_transaction_lifetime;
                  max_authority_depth[n] = it->prefs.max_authority_depth;
                  max_transaction_exec_time[n] = it->prefs.max_transaction_exec_time;
                  max_inline_depth[n] = it->prefs.max_inline_depth;
                  max_inline_action_size[n] = it->prefs.max_inline_action_size;
                  max_generated_transaction_size[n] = it->prefs.max_generated_transaction_size;

                  storage_reserve_ratio[n] = it->prefs.storage_reserve_ratio;
                  percent_of_max_inflation_rate[n] = it->prefs.percent_of_max_inflation_rate;
                  ++n;
               }

               if (it == idx.begin()) {
                  break;
               }
               --it;
            }
            // should use producer_schedule_type from libraries/chain/include/eosio/chain/producer_schedule.hpp
            bytes packed_schedule = pack(schedule);
            set_active_producers( packed_schedule.data(),  packed_schedule.size() );
            size_t median = n/2;

            auto parameters = global_state_singleton::exists() ? global_state_singleton::get()
                  : common<SystemAccount>::get_default_parameters();

            parameters.target_block_size = target_block_size[median];
            parameters.max_block_size = max_block_size[median];
            parameters.target_block_acts_per_scope = target_block_acts_per_scope[median];
            parameters.max_block_acts_per_scope = max_block_acts_per_scope[median];
            parameters.target_block_acts = target_block_acts[median];
            parameters.max_block_acts = max_block_acts[median];
            parameters.max_storage_size = max_storage_size[median];
            parameters.max_transaction_lifetime = max_transaction_lifetime[median];
            parameters.max_transaction_exec_time = max_transaction_exec_time[median];
            parameters.max_authority_depth = max_authority_depth[median];
            parameters.max_inline_depth = max_inline_depth[median];
            parameters.max_inline_action_size = max_inline_action_size[median];
            parameters.max_generated_transaction_size = max_generated_transaction_size[median];
            parameters.storage_reserve_ratio = storage_reserve_ratio[median];
            parameters.percent_of_max_inflation_rate = percent_of_max_inflation_rate[median];

            // not voted on
            parameters.first_block_time_in_cycle = cycle_time;

            // derived parameters
            auto half_of_percentage = parameters.percent_of_max_inflation_rate / 2;
            auto other_half_of_percentage = parameters.percent_of_max_inflation_rate - half_of_percentage;
            parameters.payment_per_block = payment_per_block(half_of_percentage);
            parameters.payment_to_eos_bucket = payment_per_block(other_half_of_percentage);

            if ( parameters.max_storage_size < parameters.total_storage_bytes_reserved ) {
               parameters.max_storage_size = parameters.total_storage_bytes_reserved;
            }

            set_blockchain_parameters(&parameters);

            global_state_singleton::set( parameters );
         }

         ACTION(  SystemAccount, unstake_vote_deferred ) {
            account_name                voter;

            EOSLIB_SERIALIZE( unstake_vote_deferred, (voter) )
         };

         static void on( const unstake_vote_deferred& usv) {
            require_auth( usv.voter );
            voters_table voters_tbl( SystemAccount, SystemAccount );
            const auto* voter = voters_tbl.find( usv.voter );
            eosio_assert( bool(voter), "stake not found" );

            auto weeks = (now() - voter->last_unstake_time) / unstake_pay_period;
            eosio_assert( 0 == weeks, "less than one week passed since last transfer or unstake request" );
            eosio_assert( 0 < voter->unstaking.quantity, "no unstaking money to transfer" );

            auto unstake_amount = std::min(weeks * voter->unstake_per_week, voter->unstaking);
            uint32_t new_trx_id = unstake_amount < voter->unstaking ? /* XXX send_deferred() */ 0 : 0;

            currency::inline_transfer( SystemAccount, usv.voter, unstake_amount, "unstake voting" );

            voters_tbl.update( *voter, 0, [&](voter_info& a) {
                  a.unstaking -= unstake_amount;
                  a.deferred_trx_id = new_trx_id;
                  a.last_unstake_time = a.last_unstake_time + weeks * unstake_pay_period;
               });
         }

         ACTION( SystemAccount, voteproducer ) {
            account_name                voter;
            account_name                proxy;
            std::vector<account_name>   producers;

            EOSLIB_SERIALIZE( voteproducer, (voter)(proxy)(producers) )
         };

         /**
          *  @pre vp.producers must be sorted from lowest to highest
          *  @pre if proxy is set then no producers can be voted for
          *  @pre every listed producer or proxy must have been previously registered
          *  @pre vp.voter must authorize this action
          *  @pre voter must have previously staked some EOS for voting
          */
         static void on( const voteproducer& vp ) {
            require_auth( vp.voter );

            //validate input
            if ( vp.proxy ) {
               eosio_assert( vp.producers.size() == 0, "cannot vote for producers and proxy at same time" );
               require_recipient( vp.proxy );
            } else {
               eosio_assert( vp.producers.size() <= 30, "attempt to vote for too many producers" );
               eosio_assert( std::is_sorted( vp.producers.begin(), vp.producers.end() ), "producer votes must be sorted" );
            }

            voters_table voters_tbl( SystemAccount, SystemAccount );
            auto voter = voters_tbl.find( vp.voter );

            eosio_assert( bool(voter), "no stake to vote" );
            if ( voter->is_proxy ) {
               eosio_assert( vp.proxy == 0 , "accounts elected to be proxy are not allowed to use another proxy" );
            }

            //find old producers, update old proxy if needed
            const std::vector<account_name>* old_producers = nullptr;
            if( voter->proxy ) {
               if ( voter->proxy == vp.proxy ) {
                  return; // nothing changed
               }
               auto old_proxy = voters_tbl.find( voter->proxy );
               voters_tbl.update( *old_proxy, 0, [&](auto& a) { a.proxied_votes -= voter->staked.quantity; } );
               if ( old_proxy->is_proxy ) { //if proxy stoped being proxy, the votes were already taken back from producers by on( const unregister_proxy& )
                  old_producers = &old_proxy->producers;
               }
            } else {
               old_producers = &voter->producers;
            }

            //find new producers, update new proxy if needed
            const std::vector<account_name>* new_producers = nullptr;
            if ( vp.proxy ) {
               auto new_proxy = voters_tbl.find( vp.proxy );
               eosio_assert( new_proxy->is_proxy, "selected proxy has not elected to be a proxy" );
               voters_tbl.update( *new_proxy, 0, [&](auto& a) { a.proxied_votes += voter->staked.quantity; } );
               new_producers = &new_proxy->producers;
            } else {
               new_producers = &vp.producers;
            }

            producers_table producers_tbl( SystemAccount, SystemAccount );

            if ( old_producers ) { //old_producers == 0 if proxy has stoped being a proxy and votes were taken back from producers at that moment
               //revoke votes only from no longer elected
               std::vector<account_name> revoked( old_producers->size() );
               auto end_it = std::set_difference( old_producers->begin(), old_producers->end(), new_producers->begin(), new_producers->end(), revoked.begin() );
               for ( auto it = revoked.begin(); it != end_it; ++it ) {
                  auto prod = producers_tbl.find( *it );
                  eosio_assert( bool(prod), "never existed producer" ); //data corruption
                  producers_tbl.update( *prod, 0, [&]( auto& pi ) { pi.total_votes -= voter->staked.quantity; });
               }
            }

            //update newly elected
            std::vector<account_name> elected( new_producers->size() );
            auto end_it = std::set_difference( new_producers->begin(), new_producers->end(), old_producers->begin(), old_producers->end(), elected.begin() );
            for ( auto it = elected.begin(); it != end_it; ++it ) {
               auto prod = producers_tbl.find( *it );
               eosio_assert( bool(prod), "never existed producer" ); //data corruption
               if ( vp.proxy == 0 ) { //direct voting, in case of proxy voting update total_votes even for inactive producers
                  eosio_assert( prod->active(), "can vote only for active producers" );
               }
               producers_tbl.update( *prod, 0, [&]( auto& pi ) { pi.total_votes += voter->staked.quantity; });
            }

            // save new values to the account itself
            voters_tbl.update( *voter, 0, [&](voter_info& a) {
                  a.proxy = vp.proxy;
                  a.last_update = now();
                  a.producers = vp.producers;
               });
         }

         ACTION( SystemAccount, register_proxy ) {
            account_name proxy_to_register;

            EOSLIB_SERIALIZE( register_proxy, (proxy_to_register) )
         };

         static void on( const register_proxy& reg ) {
            require_auth( reg.proxy_to_register );

            voters_table voters_tbl( SystemAccount, SystemAccount );
            auto voter = voters_tbl.find( reg.proxy_to_register );
            if ( voter ) {
               eosio_assert( voter->is_proxy == 0, "account is already a proxy" );
               eosio_assert( voter->proxy == 0, "account that uses a proxy is not allowed to become a proxy" );
               voters_tbl.update( *voter, 0, [&](voter_info& a) {
                     a.is_proxy = 1;
                     a.last_update = now();
                     //a.proxied_votes may be > 0, if the proxy has been unregistered, so we had to keep the value
                  });
            } else {
               voters_tbl.emplace( reg.proxy_to_register, [&]( voter_info& a ) {
                     a.owner = reg.proxy_to_register;
                     a.last_update = now();
                     a.proxy = 0;
                     a.is_proxy = 1;
                     a.proxied_votes = 0;
                     a.staked.quantity = 0;
                  });
            }
         }

         ACTION( SystemAccount, unregister_proxy ) {
            account_name proxy_to_unregister;

            EOSLIB_SERIALIZE( unregister_proxy, (proxy_to_unregister) )
         };

         static void on( const unregister_proxy& reg ) {
            require_auth( reg.proxy_to_unregister );

            voters_table voters_tbl( SystemAccount, SystemAccount );
            auto proxy = voters_tbl.find( reg.proxy_to_unregister );
            eosio_assert( bool(proxy), "proxy not found" );
            eosio_assert( proxy->is_proxy == 1, "account is already a proxy" );

            producers_table producers_tbl( SystemAccount, SystemAccount );
            for ( auto p : proxy->producers ) {
               auto prod = producers_tbl.find( p );
               eosio_assert( bool(prod), "never existed producer" ); //data corruption
               producers_tbl.update( *prod, 0, [&]( auto& pi ) { pi.total_votes -= proxy->proxied_votes; });
            }

            voters_tbl.update( *proxy, 0, [&](voter_info& a) {
                     a.is_proxy = 0;
                     a.last_update = now();
                     //a.proxied_votes should be kept in order to be able to reenable this proxy in the future
               });
         }

   };
}
