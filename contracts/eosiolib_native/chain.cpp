/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/chain.h>
#include "wasm_api.h"

uint32_t get_active_producers( account_name* producers, uint32_t datalen ) {
   return get_wasm_api()->get_active_producers(producers, datalen);
}

