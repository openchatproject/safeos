/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/transaction.h>

#include "vm_api.h"

#ifdef __cplusplus
void send_deferred(const uint128_t& sender_id, uint64_t payer, const char *serialized_transaction, size_t size, uint32_t replace_existing) {
   get_vm_api()->send_deferred(sender_id, payer,serialized_transaction, size, replace_existing);
}

int cancel_deferred(const uint128_t& sender_id) {
   return get_vm_api()->cancel_deferred(sender_id);
}
#else
void send_deferred(const uint128_t* sender_id, uint64_t payer, const char *serialized_transaction, size_t size, uint32_t replace_existing) {
   get_vm_api()->send_deferred(sender_id, payer,serialized_transaction, size, replace_existing);
}

int cancel_deferred(const uint128_t* sender_id) {
   return get_vm_api()->cancel_deferred(sender_id);
}
#endif

size_t read_transaction(char *buffer, size_t size) {
   return get_vm_api()->read_transaction(buffer, size);
}

size_t transaction_size() {
   return get_vm_api()->transaction_size();
}

int tapos_block_num() {
   return get_vm_api()->tapos_block_num();
}

int tapos_block_prefix() {
   return get_vm_api()->tapos_block_prefix();
}

time expiration() {
   return get_vm_api()->expiration();
}

int get_action( uint32_t type, uint32_t index, char* buff, size_t size ) {
   return get_vm_api()->get_action( type, index, buff, size );
}

int get_context_free_data( uint32_t index, char* buff, size_t size ) {
   return get_vm_api()->get_context_free_data( index, buff, size );
}
