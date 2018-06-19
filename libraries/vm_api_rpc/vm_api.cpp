#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>
/*
#include <eosio/chain/wasm_interface_private.hpp>
#include <eosio/chain/wasm_eosio_validation.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
*/
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/account_object.hpp>

#include <fc/exception/exception.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/io/raw.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <fstream>

#include <fc/crypto/xxhash.h>
#include <dlfcn.h>

typedef struct { uint16_t v; } float16_t;
typedef struct { uint32_t v; } float32_t;
typedef struct { uint64_t v; } float64_t;
typedef struct { uint64_t v[2]; } float128_t;


using namespace fc;

namespace eosio {
namespace chain {


#include "vm_api.h"

#include "action.cpp"
#include "chain.cpp"
#include "system.cpp"
#include "crypto.cpp"
#include "db.cpp"
#include "privileged.cpp"
#include "transaction.cpp"
#include "print.cpp"
#include "permission.cpp"


#define API() get_vm_api()

#if defined(assert)
   #undef assert
#endif

void eosio_assert_( bool condition, char* msg ) {
   if( BOOST_UNLIKELY( !condition ) ) {
      std::string message( msg );
      edump((message));
      EOS_THROW( eosio_assert_message_exception, "assertion failure with message: ${s}", ("s",message) );
   }
}

void eosio_assert( bool condition) {
   eosio_assert_( condition, "" );
}

const char* get_code( uint64_t receiver, size_t* size ) {
   return 0;
#if 0
   const shared_string& src = db_api::get().get_code(receiver);
   *size = src.size();
   return src.data();
#endif
}

extern "C" {
   int split_path(const char* str_path, char *path1, size_t path1_size, char *path2, size_t path2_size);
   uint64_t get_action_account();
   uint64_t string_to_uint64_(const char* str);
}

int32_t uint64_to_string_(uint64_t n, char* out, int size) {
#if 0
   if (out == NULL || size == 0) {
      return 0;
   }

   std::string s = name(n).to_string();
   if (s.length() < size) {
      size = s.length();
   }
   memcpy(out, s.c_str(), size);
   return size;
#endif
   return 0;
}

void resume_billing_timer() {
//   ctx().trx_context.resume_billing_timer();
}

void pause_billing_timer() {
//   ctx().trx_context.pause_billing_timer();
}

static struct vm_api _vm_api = {
//action.cpp
   .read_action_data = read_action_data,
   .action_data_size = action_data_size,
   .require_recipient = require_recipient,
   .require_auth = require_auth,
   .require_auth2 = require_auth2,
   .has_auth = has_auth,
   .is_account = is_account,

   .send_inline = send_inline,
   .send_context_free_inline = send_context_free_inline,
   .publication_time = publication_time,

   .current_receiver = current_receiver,
   .get_active_producers = get_active_producers,
   .assert_sha256 = assert_sha256,
   .assert_sha1 = assert_sha1,
   .assert_sha512 = assert_sha512,
   .assert_ripemd160 = assert_ripemd160,
   .sha256 = sha256,
   .sha1 = sha1,
   .sha512 = sha512,
   .ripemd160 = ripemd160,
   .recover_key = recover_key,
   .assert_recover_key = assert_recover_key,
   .db_store_i64 = db_store_i64,
   .db_update_i64 = db_update_i64,
   .db_remove_i64 = db_remove_i64,
   .db_get_i64 = db_get_i64,
   .db_get_i64_ex = db_get_i64_ex,
   .db_get_i64_exex = db_get_i64_exex,

   .db_next_i64 = db_next_i64,

   .db_previous_i64 = db_previous_i64,
   .db_find_i64 = db_find_i64,
   .db_lowerbound_i64 = db_lowerbound_i64,

   .db_upperbound_i64 = db_upperbound_i64,
   .db_end_i64 = db_end_i64,

#if 0
   .db_idx64_store = db_idx64_store,
   .db_idx64_update = db_idx64_update,

   .db_idx64_remove = db_idx64_remove,
   .db_idx64_next = db_idx64_next,
   .db_idx64_previous = db_idx64_previous,
   .db_idx64_find_primary = db_idx64_find_primary,
   .db_idx64_find_secondary = db_idx64_find_secondary,
   .db_idx64_lowerbound = db_idx64_lowerbound,
   .db_idx64_upperbound = db_idx64_upperbound,
   .db_idx64_end = db_idx64_end,
   .db_idx128_store = db_idx128_store,

   .db_idx128_update = db_idx128_update,
   .db_idx128_remove = db_idx128_remove,
   .db_idx128_next = db_idx128_next,
   .db_idx128_previous = db_idx128_previous,
   .db_idx128_find_primary = db_idx128_find_primary,
   .db_idx128_find_secondary = db_idx128_find_secondary,
   .db_idx128_lowerbound = db_idx128_lowerbound,
   .db_idx128_upperbound = db_idx128_upperbound,

   .db_idx128_end = db_idx128_end,
   .db_idx256_store = db_idx256_store,
   .db_idx256_update = db_idx256_update,
   .db_idx256_remove = db_idx256_remove,
   .db_idx256_next = db_idx256_next,

   .db_idx256_previous = db_idx256_previous,
   .db_idx256_find_primary = db_idx256_find_primary,
   .db_idx256_find_secondary = db_idx256_find_secondary,
   .db_idx256_lowerbound = db_idx256_lowerbound,
   .db_idx256_upperbound = db_idx256_upperbound,
   .db_idx256_end = db_idx256_end,
   .db_idx_double_store = db_idx_double_store,
   .db_idx_double_update = db_idx_double_update,
   .db_idx_double_remove = db_idx_double_remove,
   .db_idx_double_next = db_idx_double_next,
   .db_idx_double_previous = db_idx_double_previous,
   .db_idx_double_find_primary = db_idx_double_find_primary,
   .db_idx_double_find_secondary = db_idx_double_find_secondary,
   .db_idx_double_lowerbound = db_idx_double_lowerbound,
   .db_idx_double_upperbound = db_idx_double_upperbound,
   .db_idx_double_end = db_idx_double_end,
   .db_idx_long_double_store = db_idx_long_double_store,
   .db_idx_long_double_update = db_idx_long_double_update,
   .db_idx_long_double_remove = db_idx_long_double_remove,
   .db_idx_long_double_next = db_idx_long_double_next,
   .db_idx_long_double_previous = db_idx_long_double_previous,
   .db_idx_long_double_find_primary = db_idx_long_double_find_primary,
   .db_idx_long_double_find_secondary = db_idx_long_double_find_secondary,
   .db_idx_long_double_lowerbound = db_idx_long_double_lowerbound,
   .db_idx_long_double_upperbound = db_idx_long_double_upperbound,
   .db_idx_long_double_end = db_idx_long_double_end,
#endif

   .check_transaction_authorization = check_transaction_authorization,
   .check_permission_authorization = check_permission_authorization,
   .get_permission_last_used = get_permission_last_used,
   .get_account_creation_time = get_account_creation_time,



   .prints = prints,
   .prints_l = prints_l,
   .printi = printi,
   .printui = printui,
   .printi128 = printi128,
   .printui128 = printui128,
   .printsf = printsf,
   .printdf = printdf,
   .printqf = printqf,
   .printn = printn,
   .printhex = printhex,

   .set_resource_limits = set_resource_limits,
   .get_resource_limits = get_resource_limits,
   .set_proposed_producers = set_proposed_producers,
   .is_privileged = is_privileged,
   .set_privileged = set_privileged,
   .set_blockchain_parameters_packed = set_blockchain_parameters_packed,
   .get_blockchain_parameters_packed = get_blockchain_parameters_packed,
   .activate_feature = activate_feature,

   .abort = abort,
   .eosio_assert = eosio_assert,
   .eosio_assert_message = eosio_assert_message,
   .eosio_assert_code = eosio_assert_code,
   .eosio_exit = eosio_exit,
   .current_time = current_time,
   .now = now,

   .checktime = checktime,
   .check_context_free = check_context_free,
   .contracts_console = contracts_console,

   .send_deferred = send_deferred,
   .cancel_deferred = cancel_deferred,
   .read_transaction = read_transaction,
   .transaction_size = transaction_size,

   .tapos_block_num = tapos_block_num,
   .tapos_block_prefix = tapos_block_prefix,
   .expiration = expiration,
   .get_action = get_action,

   .assert_privileged = assert_privileged,
   .assert_context_free = assert_context_free,
   .get_context_free_data = get_context_free_data,
   .get_code = get_code,
#if 0
   .rodb_remove_i64 = db_api_remove_i64,

   .rodb_find_i64 = db_api_find_i64,
   .rodb_get_i64_ex = db_api_get_i64_ex,
   .rodb_get_i64_exex = db_api_get_i64_exex,

   .rodb_next_i64 = db_api_next_i64,
   .rodb_previous_i64 = db_api_previous_i64,
   .rodb_lowerbound_i64 = db_api_lowerbound_i64,
   .rodb_upperbound_i64 = db_api_upperbound_i64,
   .rodb_end_i64 = db_api_end_i64,

   .split_path = split_path,
   .get_action_account = get_action_account,
   .string_to_uint64 = string_to_uint64_,
   .uint64_to_string = uint64_to_string_,
   .string_to_symbol = string_to_symbol_c,
   .resume_billing_timer = resume_billing_timer,
   .pause_billing_timer = pause_billing_timer
#endif

};

struct vm_api* get_vm_api() {
   return &_vm_api;
}

void register_vm_api(void* handle) {
   fn_register_vm_api _register_vm_api = (fn_register_vm_api)dlsym(handle, "register_vm_api");
   _register_vm_api(&_vm_api);
}

}}
