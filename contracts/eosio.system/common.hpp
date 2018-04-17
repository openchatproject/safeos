#pragma once
#include <eosiolib/eosio.hpp>

//#include <eosiolib/generic_currency.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   eosio::token _system_token(N(eosio));

   template<account_name SystemAccount>
   class common {
      public:
         static constexpr account_name system_account = SystemAccount;
         //         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;
         //         typedef typename currency::token_type                                    system_token_type;

         //         static constexpr eosio::symbol_type system_symbol(S(4,EOS));

         static constexpr uint32_t   max_inflation_rate = 5;                        // 5% annual inflation

         static constexpr uint32_t   blocks_per_producer = 12;
         static constexpr uint32_t   seconds_per_day = 24 * 3600;
         static constexpr uint32_t   days_per_4years = 1461;

         struct eosio_parameters : eosio::blockchain_parameters {
            uint64_t          max_storage_size = 10 * 1024 * 1024;
            uint32_t          percent_of_max_inflation_rate = 0;
            uint32_t          storage_reserve_ratio = 1000;      // ratio * 1000

            EOSLIB_SERIALIZE_DERIVED( eosio_parameters, eosio::blockchain_parameters, (percent_of_max_inflation_rate)(storage_reserve_ratio) )
         };

         struct eosio_global_state : eosio_parameters {
            uint64_t             total_storage_bytes_reserved = 0;
            eosio::asset         total_storage_stake;
            eosio::asset         payment_per_block;
            eosio::asset         payment_to_eos_bucket;
            time                 first_block_time_in_cycle = 0;
            uint32_t             blocks_per_cycle = 0;
            time                 last_bucket_fill_time = 0;
            eosio::asset         eos_bucket;

            EOSLIB_SERIALIZE_DERIVED( eosio_global_state, eosio_parameters, (total_storage_bytes_reserved)(total_storage_stake)
                                      (payment_per_block)(payment_to_eos_bucket)(first_block_time_in_cycle)(blocks_per_cycle)
                                      (last_bucket_fill_time)(eos_bucket) )
         };

         typedef eosio::singleton<SystemAccount, N(inflation), SystemAccount, eosio_global_state> global_state_singleton;

         static eosio_global_state& get_default_parameters() {
            static eosio_global_state dp;
            get_blockchain_parameters(dp);
            return dp;
         }
   };

}
