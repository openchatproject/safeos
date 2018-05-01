/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/action.hpp>
#include <eosiolib/public_key.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/privileged.h>

namespace eosiosystem {
   using eosio::permission_level;
   using eosio::public_key;

   typedef std::vector<char> bytes;

   struct permission_level_weight {
      permission_level  permission;
      weight_type       weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };

   struct key_weight {
      public_key   key;
      weight_type  weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( key_weight, (key)(weight) )
   };

   struct authority {
      uint32_t                              threshold;
      uint32_t                              delay_sec;
      std::vector<key_weight>               keys;
      std::vector<permission_level_weight>  accounts;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( authority, (threshold)(delay_sec)(keys)(accounts) )
   };

   /*
    * Empty handlers for native messages.
    * Method parameters commented out to prevent generation of code that parses input data.
    */
   class native {
      public:

      void newaccount( account_name     creator,
                       account_name     newact
                              /*
                              const authority& owner,
                              const authority& active,
                              const authority& recovery*/ ) {
         eosio::print( eosio::name{creator}, " created ", eosio::name{newact});
         set_resource_limits( newact, 3000, 0, 0 );
         // TODO: The 3000 initial ram usage is a hack to get tests to work for now.
         //       When we add support in the system contract to buy storage for another user, we will need to replace the
         //       3000 with 0 and modify the tester to gift the necessary storage amount to all created accounts.
      }

      void updateauth( /*account_name     account,
                              permission_name  permission,
                              permission_name  parent,
                              const authority& data*/ ) {}

      void deleteauth( /*account_name account, permission_name permission*/ ) {}

      void linkauth( /*account_name    account,
                            account_name    code,
                            action_name     type,
                            permission_name requirement*/ ) {}

      void unlinkauth( /*account_name account,
                              account_name code,
                              action_name  type*/ ) {}

      void postrecovery( /*account_name       account,
                                const authority&   data,
                                const std::string& memo*/ ) {}

      void passrecovery( /*account_name account*/ ) {}

      void vetorecovery( /*account_name account*/ ) {}

      void onerror( /*const bytes&*/ ) {}

      void canceldelay( /*permission_level canceling_auth, transaction_id_type trx_id*/ ) {}

   };
}
