#include <eosiolib/types.h>
#include <eosiolib/privileged.h>

#include "wasm_api.h"

void set_resource_limits( account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight ) {
   get_wasm_api()->set_resource_limits( account, ram_bytes, net_weight, cpu_weight );
}


int64_t set_proposed_producers( char *producer_data, uint32_t producer_data_size ) {
   return get_wasm_api()->set_proposed_producers( producer_data, producer_data_size );
}

bool is_privileged( account_name account )  {
   return get_wasm_api()->is_privileged( account );
}

void set_privileged( account_name account, bool is_priv ) {
   get_wasm_api()->set_privileged( account, is_priv );
}

void set_blockchain_parameters_packed(char* data, uint32_t datalen) {
   get_wasm_api()->set_blockchain_parameters_packed(data, datalen);
}

uint32_t get_blockchain_parameters_packed(char* data, uint32_t datalen) {
   return get_wasm_api()->get_blockchain_parameters_packed(data, datalen);
}

void activate_feature( int64_t f ) {
   get_wasm_api()->activate_feature( f );
}


