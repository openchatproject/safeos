/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>
#include <eoslib/db.hpp>

namespace proxy {
   
   //@abi action
   struct PACKED( set_owner ) {
      account_name owner;	
   };

   //@abi table
   struct config {
      config(){}
      const uint64_t      key = N(config);
      account_name        owner = 0;
      uint32_t            next_id = 0;
   };

   using configs = eosio::table<N(proxy),N(proxy),N(configs),config,uint64_t>;

} /// namespace proxy
