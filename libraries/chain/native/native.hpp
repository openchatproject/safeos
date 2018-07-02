#pragma once

#include <eosiolib/action.hpp>
#include <eosiolib/public_key.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/privileged.h>
#include <eosiolib/optional.hpp>
#include <eosiolib/producer_schedule.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/currency.hpp>

#define EOSIO_NATIVE_ABI( TYPE, MEMBERS ) \
extern "C" { \
   int native_apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if( code == self ) { \
         TYPE thiscontract( self ); \
         switch( action ) { \
            EOSIO_API( TYPE, MEMBERS ) \
            default:\
               return 0; \
         } \
         return 1;\
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
} \

using namespace eosio;

namespace eosiosystem {
   using eosio::permission_level;
   using eosio::public_key;

   typedef std::vector<char> bytes;

   struct boost_account {
      account_name    account;
      uint64_t        expiration;
      uint64_t primary_key()const { return account; }

      EOSLIB_SERIALIZE( boost_account, (account) )
   };

   typedef eosio::multi_index< N(boost), boost_account>  boost_table;


   /*
    * Method parameters commented out to prevent generation of code that parses input data.
    */
   class system_contract : public eosio::contract {
      private:
         boost_table            _boost;
      public:
         using eosio::contract::contract;

         system_contract( account_name s );
         ~system_contract();

         void boost(account_name account);
         void cancelboost(account_name account);
   };
}

