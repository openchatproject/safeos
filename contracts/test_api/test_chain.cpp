/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/action.h>
#include <eoslib/chain.h>
#include <eoslib/eos.hpp>
#include "test_api.hpp"

#pragma pack(push, 1)
struct producers {
   char len;
   account_name producers[21];
};
#pragma pack(pop)

void test_chain::test_activeprods() {
  producers act_prods;
  read_action(&act_prods, sizeof(producers));

  assert(act_prods.len == 21, "producers.len != 21");

  producers api_prods;
  get_active_producers(api_prods.producers, sizeof(account_name)*21);


  for( int i = 0; i < 21 ; ++i )
      assert(api_prods.producers[i] == act_prods.producers[i], "Active producer");
}
