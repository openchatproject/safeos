#include "native.hpp"

extern "C" void vm_unload_account(uint64_t account);

namespace eosiosystem {

system_contract::system_contract( account_name s )
:contract(s), _boost(_self, _self)
{
}

system_contract::~system_contract() {

}

void system_contract::boost(account_name account) {
   require_auth( N(eosio) );
   eosio_assert(is_account(account), "account does not exist");
    eosio_assert(_boost.find(account) == _boost.end(), "account already accelerated");
   _boost.emplace( N(eosio), [&]( auto& p ) {
         p.account = account;
   });
}

void system_contract::cancelboost(account_name account) {
   require_auth( N(eosio) );
   eosio_assert(is_account(account), "account does not exist");
   auto itr = _boost.find(account);
   eosio_assert( itr != _boost.end(), "account not in list" );
   _boost.erase(itr);
   vm_unload_account(account);
}

}

extern "C" {
   int native_apply( uint64_t receiver, uint64_t code, uint64_t action ) {
      auto self = receiver;
      if( code == self ) {
         eosiosystem::system_contract thiscontract( self );
         switch( action ) {
         case N(boost):
            eosio::execute_action( &thiscontract, &eosiosystem::system_contract::boost );
            break;
         case N(cancelboost):
            eosio::execute_action( &thiscontract, &eosiosystem::system_contract::cancelboost );
            break;
         default:
               return 0;
         }
         return 1;
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */
      }
      return 0;
   }
}
/*
EOSIO_NATIVE_ABI( eosiosystem::system_contract,
     (boost)(cancelboost)
)
*/
