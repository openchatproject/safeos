#include <eosiolib_native/vm_api.h>
#include <eosiolib/types.hpp>
#include "vm_wasm_api.h"

using namespace eosio;

int wasm_setcode(uint64_t account);
int wasm_apply(uint64_t receiver, uint64_t account, uint64_t act);

namespace eosio {
   namespace chain {
      void wasm_init_api();
   }
}

namespace eosio {
namespace chain {
   void register_vm_api(void* handle);
   int  wasm_to_wast( const uint8_t* data, size_t size, uint8_t* wast, size_t wast_size );
   int  wast_to_wasm( const uint8_t* data, size_t size, uint8_t* wasm, size_t wasm_size );

}
}


static struct vm_api s_api;
struct vm_wasm_api s_vm_wasm_api;

void vm_init() {
   s_vm_wasm_api.wasm_to_wast = eosio::chain::wasm_to_wast;
   s_vm_wasm_api.wast_to_wasm = eosio::chain::wast_to_wasm;
   eosio::chain::wasm_init_api();
}

void vm_deinit() {
   printf("vm_wasm finalize\n");
}

extern "C" struct vm_wasm_api* vm_get_wasm_api() {
   return &s_vm_wasm_api;
}

void vm_register_api(struct vm_api* api) {
   s_api = *api;
}

struct vm_api* get_vm_api() {
   return &s_api;
}

int vm_setcode(uint64_t account) {
   wasm_setcode(account);
   return 0;
}

int vm_apply(uint64_t receiver, uint64_t account, uint64_t act) {
   wasm_apply(receiver, account, act);
   return 0;
}
uint64_t _wasm_call(const char* act, uint64_t* args, int argc);
uint64_t vm_call(const char* act, uint64_t* args, int argc) {
   return _wasm_call(act, args, argc);
}

void resume_billing_timer() {
   get_vm_api()->resume_billing_timer();
}
void pause_billing_timer() {
   get_vm_api()->pause_billing_timer();
}

const char* get_code( uint64_t receiver, size_t* size ) {
   return get_vm_api()->get_code( receiver, size );
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

