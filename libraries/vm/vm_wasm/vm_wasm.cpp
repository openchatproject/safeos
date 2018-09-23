#include <stdint.h>
#include <eosiolib_native/vm_api.h>
#include <eosiolib/types.hpp>
#include "vm_wasm_api.h"

using namespace eosio;

int wasm_setcode(uint64_t account);
int wasm_apply(uint64_t receiver, uint64_t account, uint64_t act);
int wasm_preload(uint64_t account);
int wasm_unload(uint64_t account);

namespace eosio {
   namespace chain {
      void wasm_init_api();
   }
}

namespace eosio {
namespace chain {
   int  wasm_to_wast( const uint8_t* data, size_t size, uint8_t* wast, size_t wast_size, bool strip_names );
   int  wast_to_wasm( const uint8_t* data, size_t size, uint8_t* wasm, size_t wasm_size );

}
}


static struct vm_api s_api = {};

void vm_init(struct vm_api* api) {
   s_api = *api;
   api->wasm_to_wast = eosio::chain::wasm_to_wast;
   api->wast_to_wasm = eosio::chain::wast_to_wasm;

   eosio::chain::wasm_init_api();
}

void vm_deinit() {
   printf("vm_wasm vm_deinit\n");
}

struct vm_api* get_vm_api() {
   return &s_api;
}

int vm_setcode(uint64_t account) {
   size_t size = 0;
   const char* code = get_vm_api()->get_code(account, &size);
   if (size <= 0) {
      wasm_unload(account);
      return 1;
   }
   wasm_setcode(account);
   return 0;
}

int vm_apply(uint64_t receiver, uint64_t account, uint64_t act) {
   return wasm_apply(receiver, account, act);
}

int vm_call(uint64_t account, uint64_t func) {
   return 0;
}

int vm_preload(uint64_t account) {
   return wasm_preload(account);
}

int vm_unload(uint64_t account) {
   return wasm_unload(account);
}

#if 0
uint64_t _wasm_call(const char* act, uint64_t* args, int argc);
uint64_t vm_call(const char* act, uint64_t* args, int argc) {
   return _wasm_call(act, args, argc);
}
#endif

void resume_billing_timer() {
   get_vm_api()->resume_billing_timer();
}
void pause_billing_timer() {
   get_vm_api()->pause_billing_timer();
}

const char* get_code( uint64_t receiver, size_t* size ) {
   return get_vm_api()->get_code( receiver, size );
}

bool vm_is_account(uint64_t account) {
   return get_vm_api()->is_account( account );
}

int get_code_id( uint64_t account, char* code_id, size_t size ) {
   return get_vm_api()->get_code_id( account, code_id, size );
}

int db_api_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
   return get_vm_api()->rodb_find_i64(code, scope, table, id);
}

int32_t db_api_get_i64_ex( int iterator, uint64_t* primary, char* buffer, size_t buffer_size ) {
   return get_vm_api()->rodb_get_i64_ex(iterator, primary, buffer, buffer_size);
}

const char* db_api_get_i64_exex( int itr, size_t* buffer_size ) {
   return get_vm_api()->rodb_get_i64_exex(itr, buffer_size);
}

void vm_api_throw_exception(int type, const char* fmt, ...) {
   get_vm_api()->throw_exception(type, fmt);
}

