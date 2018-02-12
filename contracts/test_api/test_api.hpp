/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

typedef unsigned long long u64;
typedef unsigned int u32;
static constexpr u32 DJBH(const char* cp)
{
  u32 hash = 5381;
  while (*cp)
      hash = 33 * hash ^ (unsigned char) *cp++;
  return hash;
}

static constexpr u64 WASM_TEST_ACTION(const char* cls, const char* method)
{
  return u64(DJBH(cls)) << 32 | u64(DJBH(method));
}

#define WASM_TEST_HANDLER(CLASS, METHOD) \
  if( action == WASM_TEST_ACTION(#CLASS, #METHOD) ) { \
     CLASS::METHOD(); \
     return; \
  }
   
#pragma pack(push, 1)
struct dummy_action {
  char a; //1
  unsigned long long b; //8
  int  c; //4
};

struct u128_action {
  unsigned __int128  values[3]; //16*3
};
#pragma pack(pop)

static_assert( sizeof(dummy_action) == 13 , "unexpected packing" );
static_assert( sizeof(u128_action) == 16*3 , "unexpected packing" );

struct test_types {
  static void types_size();
  static void char_to_symbol();
  static void string_to_name();
  static void name_class();
};

struct test_print {
  static void test_prints();
  static void test_prints_l();
  static void test_printi();
  static void test_printi128();
  static void test_printn();
};

#define DUMMY_ACTION_DEFAULT_A 0x45
#define DUMMY_ACTION_DEFAULT_B 0xab11cd1244556677
#define DUMMY_ACTION_DEFAULT_C 0x7451ae12

struct test_action {

  static void read_action_normal();
  static void read_action_to_0();
  static void read_action_to_64k();
  static void require_notice();
  static void require_auth();
  static void assert_false();
  static void assert_true();
  static void now();
  static void test_abort();
  static void test_current_receiver();
  static void test_current_sender();
  static void test_publication_time();
};

struct test_math {
  static void test_multeq();
  static void test_diveq();
  static void test_diveq_by_0();
  static void test_double_api();
  static void test_double_api_div_0();
  static void test_i64_to_double();
  static void test_double_to_i64();
};

struct test_db {
   static void key_i64_general();
   static void key_i64_remove_all();
   static void key_i64_small_load();
   static void key_i64_small_store();
   static void key_i64_store_scope();
   static void key_i64_remove_scope();
   static void key_i64_not_found();
   static void key_i64_front_back();

   static void key_i128i128_general();
   static void key_i64i64i64_general();
   static void key_str_general();
   static void key_str_table();

   static void key_str_setup_limit();
   static void key_str_min_exceed_limit();
   static void key_str_under_limit();
   static void key_str_available_space_exceed_limit();
   static void key_str_another_under_limit();

   static void key_i64_setup_limit();
   static void key_i64_min_exceed_limit();
   static void key_i64_under_limit();
   static void key_i64_available_space_exceed_limit();
   static void key_i64_another_under_limit();

   static void key_i128i128_setup_limit();
   static void key_i128i128_min_exceed_limit();
   static void key_i128i128_under_limit();
   static void key_i128i128_available_space_exceed_limit();
   static void key_i128i128_another_under_limit();

   static void key_i64i64i64_setup_limit();
   static void key_i64i64i64_min_exceed_limit();
   static void key_i64i64i64_under_limit();
   static void key_i64i64i64_available_space_exceed_limit();
   static void key_i64i64i64_another_under_limit();
};

struct test_crypto {
   static void test_recover_key();
   static void test_recover_key_assert_true();
   static void test_recover_key_assert_false();
   static void test_sha1();
   static void test_sha256();
   static void test_sha512();
   static void test_ripemd160();
   static void sha1_no_data();
   static void sha256_no_data();
   static void sha512_no_data();
   static void ripemd160_no_data();
   static void sha256_null();
   static void assert_sha256_false();
   static void assert_sha1_false();
   static void assert_sha512_false();
   static void assert_ripemd160_false();
   static void assert_sha256_true();
   static void assert_sha1_true();
   static void assert_sha512_true();
   static void assert_ripemd160_true();
};

struct test_transaction {
  static void test_tapos_block_num();
  static void test_tapos_block_prefix();
  static void send_action();
  static void send_action_empty();
  static void send_action_max();
  static void send_action_large();
  static void send_action_recurse();
  static void send_action_inline_fail();
  static void test_read_transaction();
  static void test_transaction_size();
  static void send_transaction();
  static void send_transaction_empty();
  static void send_transaction_max();
  static void send_transaction_large();
  static void send_action_sender();
};

struct test_chain {
  static void test_activeprods();
};

struct test_string {
  static void construct_with_size();
  static void construct_with_data();
  static void construct_with_data_copied();
  static void construct_with_data_partially();
  static void copy_constructor();
  static void assignment_operator();
  static void index_operator();
  static void index_out_of_bound();
  static void substring();
  static void substring_out_of_bound();
  static void concatenation_null_terminated();
  static void concatenation_non_null_terminated();
  static void assign();
  static void comparison_operator();
  static void print_null_terminated();
  static void print_non_null_terminated();
  static void print_unicode();
  static void valid_utf8();
  static void invalid_utf8();
  static void string_literal();
};

struct test_fixedpoint {
   static void create_instances();
   static void test_addition();
   static void test_subtraction();
   static void test_multiplication();
   static void test_division();
   static void test_division_by_0();
};

struct test_real {
   static void create_instances();
   static void test_addition();
   static void test_multiplication();
   static void test_division();
   static void test_division_by_0();
};

struct test_compiler_builtins {
   static void test_multi3();
   static void test_divti3();
   static void test_divti3_by_0();
   static void test_udivti3();
   static void test_udivti3_by_0();
   static void test_modti3();
   static void test_modti3_by_0();
   static void test_umodti3();
   static void test_umodti3_by_0();
   static void test_lshlti3();
   static void test_lshrti3();
   static void test_ashlti3();
   static void test_ashrti3();
};

struct test_account {
   static void test_balance_acc1();
};

struct test_extended_memory {
	static void test_initial_buffer();
	static void test_page_memory();
	static void test_page_memory_exceeded();
	static void test_page_memory_negative_bytes();
};

struct test_memory {
   static void test_memory_allocs();
   static void test_memory_hunk();
   static void test_memory_hunks();
   static void test_memory_hunks_disjoint();
   static void test_memset_memcpy();
   static void test_memcpy_overlap_start();
   static void test_memcpy_overlap_end();
   static void test_memcmp();
};

struct test_checktime {
   static void checktime_pass();
   static void checktime_failure();
};
