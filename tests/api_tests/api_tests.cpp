/**
 *  @file api_tests.cpp
 *  @copyright defined in eos/LICENSE.txt
 */
#include <algorithm>
#include <random>
#include <iostream>
#include <vector>
#include <iterator>
#include <sstream>
#include <numeric>

#include <boost/test/unit_test.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/contracts/contract_table_objects.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/wasm_interface.hpp>

//TODO this should be in eosio not eos
#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <test_api/test_api.wast.hpp>
#include <test_api_mem/test_api_mem.wast.hpp>
#include <test_api/test_api.hpp>


#include <eosio/chain/staked_balance_objects.hpp>

FC_REFLECT( dummy_action, (a)(b)(c) );
FC_REFLECT( u128_action, (values) );

using namespace eosio;
using namespace eosio::testing;
using namespace chain;
using namespace chain::contracts;
using namespace fc;

namespace bio = boost::iostreams;

template<uint64_t NAME>
struct test_api_action {
	static scope_name get_scope() {
		return N(testapi);
	}

	static action_name get_name() {
		return action_name(NAME);
	}
};
FC_REFLECT_TEMPLATE((uint64_t T), test_api_action<T>, BOOST_PP_SEQ_NIL);

struct assert_message_is {
	assert_message_is(string expected) 
		: expected(expected) {}
	
	bool operator()( const fc::assert_exception& ex) {
		auto act = ex.get_log().at(0).get_message();
		return boost::algorithm::ends_with(act, expected);
	}

	string expected;
};

constexpr uint64_t TEST_METHOD(const char* CLASS, const char *METHOD) {
  //std::cerr << CLASS << "::" << METHOD << std::endl;
  return ( (uint64_t(DJBH(CLASS))<<32) | uint32_t(DJBH(METHOD)) );
}

string U64Str(uint64_t i)
{
	std::stringstream ss;
	ss << i;
	return ss.str();
}

string U128Str(unsigned __int128 i)
{
   return fc::variant(fc::uint128_t(i)).get_string();
}


// run the `METHOD` and capture it's output then check some common parts that all the test_print methods have
#define CAPTURE_AND_PRE_TEST_PRINT(METHOD) \
	{ \
		BOOST_TEST_MESSAGE( "Running test_print::" << METHOD ); \
		CAPTURE( cerr, CALL_TEST_FUNCTION( *this, "test_print", METHOD, {} ) ); \
		BOOST_CHECK_EQUAL( capture.size(), 7 ); \
		captured = capture[3]; \
	}

BOOST_AUTO_TEST_SUITE(api_tests)

template <typename T>
void CallFunction(tester& test, T tm, const vector<char>& data, const vector<account_name>& scope = {N(testapi)}) {
	{
		signed_transaction trx;
		trx.write_scope = scope;

      auto pl = vector<permission_level>{{scope[0], config::active_name}};
      if (scope.size() > 1)
         for (int i=1; i < scope.size(); i++)
            pl.push_back({scope[i], config::active_name});

      action act(pl, tm);
      vector<char>& dest = *(vector<char> *)(&act.data);
      std::copy(data.begin(), data.end(), std::back_inserter(dest));
      trx.actions.push_back(act);

		test.set_tapos(trx);
		trx.sign(test.get_private_key(scope[0], "active"), chain_id_type());
		auto res = test.control->push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
      /*
		BOOST_TEST_MESSAGE("ACTION TRACE SIZE : " << res.action_traces.size());
		BOOST_TEST_MESSAGE("ACTION TRACE RECEIVER : " << res.action_traces.at(0).receiver.to_string());
		BOOST_TEST_MESSAGE("ACTION TRACE SCOPE : " << res.action_traces.at(0).act.scope.to_string());
		BOOST_TEST_MESSAGE("ACTION TRACE NAME : " << res.action_traces.at(0).act.name.to_string());
		BOOST_TEST_MESSAGE("ACTION TRACE AUTH SIZE : " << res.action_traces.at(0).act.authorization.size());
		BOOST_TEST_MESSAGE("ACTION TRACE ACTOR : " << res.action_traces.at(0).act.authorization.at(0).actor.to_string());
		BOOST_TEST_MESSAGE("ACTION TRACE PERMISSION : " << res.action_traces.at(0).act.authorization.at(0).permission.to_string());
      */
		test.produce_block();
	}
}

#define CALL_TEST_FUNCTION(TESTER, CLS, MTH, DATA) CallFunction(TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA)
#define CALL_TEST_FUNCTION_SCOPE(TESTER, CLS, MTH, DATA, ACCOUNT) CallFunction(TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA, ACCOUNT)

bool is_access_violation(fc::unhandled_exception const & e) {
   try {
      std::rethrow_exception(e.get_inner_exception());
    }
    catch (const eosio::chain::wasm_execution_error& e) {
       return true;
    } catch (...) {

    }
   return false;
}

bool is_access_violation(const Runtime::Exception& e) {
   return true;
}

bool is_assert_exception(fc::assert_exception const & e) { return true; }
bool is_page_memory_error(page_memory_error const &e) { return true; }
bool is_tx_missing_auth(tx_missing_auth const & e) { return true; }
bool is_tx_missing_recipient(tx_missing_recipient const & e) { return true;}
bool is_tx_missing_sigs(tx_missing_sigs const & e) { return true;}
bool is_wasm_execution_error(eosio::chain::wasm_execution_error const& e) {return true;}
bool is_tx_resource_exhausted(const tx_resource_exhausted& e) { return true; }

/*
bool is_tx_missing_recipient(tx_missing_recipient const & e) { return true;}
bool is_tx_missing_auth(tx_missing_auth const & e) { return true; }
bool is_tx_missing_scope(tx_missing_scope const& e) { return true; }
bool is_tx_resource_exhausted(const tx_resource_exhausted& e) { return true; }
bool is_tx_unknown_argument(const tx_unknown_argument& e) { return true; }
bool is_assert_exception(fc::assert_exception const & e) { return true; }
bool is_tx_resource_exhausted_or_checktime(const transaction_exception& e) {
   return (e.code() == tx_resource_exhausted::code_value) || (e.code() == checktime_exceeded::code_value);
}
*/

std::vector<std::string> capture;

struct MySink : public bio::sink
{

   std::streamsize write(const char* s, std::streamsize n)
   {
      std::string tmp;
      tmp.assign(s, n);
      capture.push_back(tmp);
      std::cout << "stream : [" << tmp << "]" << std::endl;
      return n;
   }
};
uint32_t last_fnc_err = 0;

#define CAPTURE(STREAM, EXEC) \
   {\
      capture.clear(); \
      bio::stream_buffer<MySink> sb; sb.open(MySink()); \
      std::streambuf *oldbuf = std::STREAM.rdbuf(&sb); \
      EXEC; \
      std::STREAM.rdbuf(oldbuf); \
   }

/*
#define TEST_PRINT_METHOD( METHOD , OUT) \
	{ \
		auto U64Str = [](uint64_t v) -> std::string { std::stringstream s; s << v; return s.str(); }; \
		CAPTURE( cerr, CALL_TEST_FUNCTION(*this, "test_print", METHOD, {}) ); \
		BOOST_TEST_MESSAGE( "Running test_print::" << METHOD ); \
		BOOST_CHECK_EQUAL( capture.size(), 7 ); \
		BOOST_TEST_MESSAGE( "Captured Output : " << capture[3] ); \
		BOOST_CHECK_EQUAL( capture[3].substr(0,1), U64Str(0) ); \
		BOOST_CHECK_EQUAL( capture[3].substr(1,6), U64Str(556644) ); \
		BOOST_CHECK_EQUAL( capture[3].substr(7, capture[3].size()), U64Str(-1) ); \
	}
*/

// TODO missing intrinsic account_balance_get
/*************************************************************************************
 * account_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(account_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	create_account( N(acc1), asset::from_string("0.0000 EOS") );
	create_account( N(acc2), asset::from_string("0.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	//transfer( N(inita), N(acc1), "1000.0000 EOS", "test");
#if 0
	eosio::chain::signed_transaction trx;
	trx.write_scope = { N(acc1), N(inita) };

	trx.actions.emplace_back(vector<permission_level>{{N(inita), config::active_name}}, 
									 contracts::transfer{N(inita), N(acc1), 1000, "memo"});


	trx.expiration = control->head_block_time() + fc::seconds(100);
	trx.set_reference_block(control->head_block_id());

	trx.sign(get_private_key(N(inita), "active"), chain_id_type());
	push_transaction(trx);
#endif
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);

   transfer( N(inita), N(acc1), "24.0000 EOS", "memo" );
	produce_blocks(1000);
   BOOST_CHECK_EQUAL(get_balance(N(acc1)), 240000);

   transfer( N(inita), N(acc1), "1.0000 EOS", "memo" );
	produce_blocks(1000);
   BOOST_CHECK_EQUAL(get_balance(N(acc1)), 250000);

   transfer( N(inita), N(acc1), "5000.0000 EOS", "memo" );
   BOOST_CHECK_EQUAL(get_balance(N(acc1)), 50250000);
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * action_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("100000.0000 EOS") );
   create_account( N(acc1), asset::from_string("100000.0000 EOS") );
   create_account( N(acc2), asset::from_string("100000.0000 EOS") );
   create_account( N(acc3), asset::from_string("100000.0000 EOS") );
   create_account( N(acc4), asset::from_string("100000.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	set_code( N(acc1), test_api_wast );
	set_code( N(acc2), test_api_wast );
	produce_blocks(1);

	CALL_TEST_FUNCTION( *this, "test_action", "assert_true", {});
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "assert_false", {}), fc::assert_exception, is_assert_exception);

   dummy_action dummy13{DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   CALL_TEST_FUNCTION( *this, "test_action", "read_action_normal", fc::raw::pack(dummy13));

   std::vector<char> raw_bytes((1<<16));
	CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_0", raw_bytes );

   std::vector<char> raw_bytes2((1<<16)+1);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_0", raw_bytes2), 
      eosio::chain::wasm_execution_error, is_wasm_execution_error);

   raw_bytes.resize(1);
	CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_64k", raw_bytes );

   raw_bytes.resize(2);
	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_64k", raw_bytes ),
         eosio::chain::wasm_execution_error, is_wasm_execution_error);

   CALL_TEST_FUNCTION( *this, "test_action", "require_notice", raw_bytes );
   
   auto scope = std::vector<account_name>{N(testapi)};
   auto test_require_notice = [](auto& test, std::vector<char>& data, std::vector<account_name>& scope){
      signed_transaction trx;
		trx.write_scope = scope; 
      auto tm = test_api_action<TEST_METHOD("test_action", "require_notice")>{};

      action act(std::vector<permission_level>{{N(testapi), config::active_name}}, tm);
      vector<char>& dest = *(vector<char> *)(&act.data);
      std::copy(data.begin(), data.end(), std::back_inserter(dest));
      trx.actions.push_back(act);

		test.set_tapos(trx);
		trx.sign(test.get_private_key(N(inita), "active"), chain_id_type());
		auto res = test.control->push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
   };

   BOOST_CHECK_EXCEPTION(test_require_notice(*this, raw_bytes, scope), tx_missing_sigs, is_tx_missing_sigs);


   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", {}), tx_missing_auth, is_tx_missing_auth);

   auto a3only = std::vector<permission_level>{{N(acc3), config::active_name}};
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", fc::raw::pack(a3only)), tx_missing_auth, is_tx_missing_auth);

   auto a4only = std::vector<permission_level>{{N(acc4), config::active_name}};
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", fc::raw::pack(a4only)), tx_missing_auth, is_tx_missing_auth);

   auto a3a4 = std::vector<permission_level>{{N(acc3), config::active_name}, {N(acc4), config::active_name}};
   auto a3a4_scope = std::vector<account_name>{N(acc3), N(acc4)};
   {
      signed_transaction trx;
		trx.write_scope = a3a4_scope;
      auto tm = test_api_action<TEST_METHOD("test_action", "require_auth")>{};
      auto pl = a3a4;
      if (a3a4_scope.size() > 1)
         for (int i=1; i < a3a4_scope.size(); i++)
            pl.push_back({a3a4_scope[i], config::active_name});

      //action act(vector<permission_level>{{scope[0], config::active_name}}, tm);
      action act(pl, tm);
      auto dat = fc::raw::pack(a3a4);
      vector<char>& dest = *(vector<char> *)(&act.data);
      std::copy(dat.begin(), dat.end(), std::back_inserter(dest));
      trx.actions.push_back(act);

		set_tapos(trx);
		trx.sign(get_private_key(N(acc3), "active"), chain_id_type());
		trx.sign(get_private_key(N(acc4), "active"), chain_id_type());
		auto res = control->push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
   }

   uint32_t now = control->head_block_time().sec_since_epoch();
   CALL_TEST_FUNCTION( *this, "test_action", "now", fc::raw::pack(now));
   produce_block();
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "now", fc::raw::pack(now)), fc::assert_exception, is_assert_exception);
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * compiler_builtins_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(compiler_builtins_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("100000.0000 EOS") );
   create_account( N(acc1), asset::from_string("1.0000 EOS") );
   create_account( N(acc2), asset::from_string("1.0000 EOS") );
   create_account( N(acc3), asset::from_string("1.0000 EOS") );
   create_account( N(acc4), asset::from_string("1.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);
   
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_multi3", {});
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_divti3", {});
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_divti3_by_0", {}), fc::assert_exception, is_assert_exception);
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_lshlti3", {});
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_lshrti3", {});
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_ashlti3", {});
   //CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_ashrti3", {});
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * transaction_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(transaction_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("100000.0000 EOS") );
   create_account( N(acc1), asset::from_string("1.0000 EOS") );
   create_account( N(acc2), asset::from_string("1.0000 EOS") );
   create_account( N(acc3), asset::from_string("1.0000 EOS") );
   create_account( N(acc4), asset::from_string("1.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);
   
   CALL_TEST_FUNCTION(*this, "test_transaction", "send_action", {});
   CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_empty", {});
   CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_max", {});
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_large", {}), fc::assert_exception, is_assert_exception);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_recurse", {}), fc::assert_exception, is_assert_exception);
   CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_inline_fail", {});

} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * chain_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(chain_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	create_account( N(acc1), asset::from_string("0.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );

	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);
   
   auto& gpo = control->get_global_properties();   
   std::vector<account_name> prods(gpo.active_producers.producers.size());
   for ( int i=0; i < gpo.active_producers.producers.size(); i++ )
      prods[i] = gpo.active_producers.producers[i].producer_name;

	CALL_TEST_FUNCTION( *this, "test_chain", "test_activeprods", fc::raw::pack(prods));
} FC_LOG_AND_RETHROW() }


// (Bucky) TODO got to fix macros in test_db.cpp
#if 0
/*************************************************************************************
 * db_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(db_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("100000.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_mem_wast );
	produce_blocks(1);

	CALL_TEST_FUNCTION( *this, "test_db", "key_i64_general", {});
} FC_LOG_AND_RETHROW() }
#endif


/*************************************************************************************
 * fixedpoint_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(fixedpoint_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);

	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "create_instances", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_addition", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_subtraction", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_multiplication", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_division", {});
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * real_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(real_tests, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);

   CALL_TEST_FUNCTION( *this, "test_real", "create_instances", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_real", "test_addition", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_real", "test_multiplication", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_real", "test_division", {} );
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * crypto_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(crypto_tests, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_crypto", "test_sha256", {} );
   produce_blocks(1000);
   // TODO should this throw an exception
   //BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_crypto", "sha256_no_data", {} ), fc::assert_exception, is_assert_exception);
   CALL_TEST_FUNCTION( *this, "test_crypto", "sha256_no_data", {} );
   produce_blocks(1000);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_crypto", "asert_sha256_false", {} ), fc::assert_exception, is_assert_exception);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_crypto", "asert_sha256_true", {} );
   produce_blocks(1000);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_crypto", "asert_no_data", {} ), fc::assert_exception, is_assert_exception);
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * memory_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(memory_test, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_allocs", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_hunk", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_hunks", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_hunks_disjoint", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memset_memcpy", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memcpy_overlap_start", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memcpy_overlap_end", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memcmp", {} );
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * extended_memory_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(extended_memory_test_initial_memory, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_initial_buffer", {} );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(extended_memory_test_page_memory, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_page_memory", {} );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(extended_memory_test_page_memory_exceeded, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_page_memory_exceeded", {} ),
                         page_memory_error, is_page_memory_error);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(extended_memory_test_page_memory_negative_bytes, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_page_memory_negative_bytes", {} ),
                         page_memory_error, is_page_memory_error);
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * string_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(string_tests, tester) { try {
	produce_blocks(1000);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	create_account( N(testextmem), asset::from_string("100.0000 EOS") );
	produce_blocks(1000);

	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_mem_wast );
	produce_blocks(1000);

	CALL_TEST_FUNCTION( *this, "test_string", "construct_with_size", {});
	CALL_TEST_FUNCTION( *this, "test_string", "construct_with_data", {});
	CALL_TEST_FUNCTION( *this, "test_string", "construct_with_data_partially", {});
	CALL_TEST_FUNCTION( *this, "test_string", "construct_with_data_copied", {});
	CALL_TEST_FUNCTION( *this, "test_string", "copy_constructor", {});
	CALL_TEST_FUNCTION( *this, "test_string", "assignment_operator", {});
	CALL_TEST_FUNCTION( *this, "test_string", "index_operator", {});
	BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_string", "index_out_of_bound", {}), fc::assert_exception, is_assert_exception );
	CALL_TEST_FUNCTION( *this, "test_string", "substring", {});
	CALL_TEST_FUNCTION( *this, "test_string", "concatenation_null_terminated", {});
	BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_string", "substring_out_of_bound", {}), fc::assert_exception, is_assert_exception );
	CALL_TEST_FUNCTION( *this, "test_string", "concatenation_non_null_terminated", {});
	CALL_TEST_FUNCTION( *this, "test_string", "assign", {});
	CALL_TEST_FUNCTION( *this, "test_string", "comparison_operator", {});
	CALL_TEST_FUNCTION( *this, "test_string", "print_null_terminated", {});
	CALL_TEST_FUNCTION( *this, "test_string", "print_non_null_terminated", {});
	CALL_TEST_FUNCTION( *this, "test_string", "print_unicode", {});
	CALL_TEST_FUNCTION( *this, "test_string", "string_literal", {});
	CALL_TEST_FUNCTION( *this, "test_string", "valid_utf8", {});
	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_string", "invalid_utf8", {}), fc::assert_exception, is_assert_exception );
   
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * print_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(print_tests, tester) { try {
	produce_blocks(2);
	create_account(N(testapi), asset::from_string("100000.0000 EOS"));
	create_account(N(another), asset::from_string("1.0000 EOS"));
	create_account(N(acc1), asset::from_string("1.0000 EOS"));
	create_account(N(acc2), asset::from_string("1.0000 EOS"));
	create_account(N(acc3), asset::from_string("1.0000 EOS"));
	create_account(N(acc4), asset::from_string("1.0000 EOS"));
	produce_blocks(1000);

	//Set test code
	transfer(N(inita), N(testapi), "10.0000 EOS", "memo");
	set_code(N(testapi), test_api_wast); 
	produce_blocks(1000);
	string captured = "";

	// test prints
	CAPTURE_AND_PRE_TEST_PRINT("test_prints");
	BOOST_CHECK_EQUAL(captured == "abcefg", true);

	// test printi
	CAPTURE_AND_PRE_TEST_PRINT("test_printi");
	BOOST_CHECK_EQUAL( captured.substr(0,1), U64Str(0) );  						// "0"
	BOOST_CHECK_EQUAL( captured.substr(1,6), U64Str(556644) );					// "556644" 
	BOOST_CHECK_EQUAL( captured.substr(7, capture[3].size()), U64Str(-1) ); // "18446744073709551615"

	//TODO come back to this, no native uint128_t
	// test printi128

	// test printn
	CAPTURE_AND_PRE_TEST_PRINT("test_printn");
	BOOST_CHECK_EQUAL( captured.substr(0,5), "abcde" );
	BOOST_CHECK_EQUAL( captured.substr(5, 5), "ab.de" );
	BOOST_CHECK_EQUAL( captured.substr(10, 6), "1q1q1q");
	BOOST_CHECK_EQUAL( captured.substr(16, 11), "abcdefghijk");
	BOOST_CHECK_EQUAL( captured.substr(27, 12), "abcdefghijkl");
	BOOST_CHECK_EQUAL( captured.substr(39, 13), "abcdefghijkl1");
	BOOST_CHECK_EQUAL( captured.substr(52, 13), "abcdefghijkl1");
	BOOST_CHECK_EQUAL( captured.substr(65, 13), "abcdefghijkl1");

	// test printi128
	CAPTURE_AND_PRE_TEST_PRINT("test_printi128");
	BOOST_CHECK_EQUAL( captured.substr(0, 39), U128Str(-1) );
	BOOST_CHECK_EQUAL( captured.substr(39, 1), U128Str(0) );
	BOOST_CHECK_EQUAL( captured.substr(40, 11), U128Str(87654323456) );
      
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * math_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(math_tests, tester) { try {
	produce_blocks(1000);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	produce_blocks(1000);

	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1000);

   std::random_device rd;
   std::mt19937_64 gen(rd());
   std::uniform_int_distribution<unsigned long long> dis;

   // test mult_eq with 10 random pairs of 128 bit numbers
   for (int i=0; i < 10; i++) {
      u128_action act;
      act.values[0] = dis(gen); act.values[0] <<= 64; act.values[0] |= dis(gen);
      act.values[1] = dis(gen); act.values[1] <<= 64; act.values[1] |= dis(gen);
      act.values[2] = act.values[0] * act.values[1];
      CALL_TEST_FUNCTION( *this, "test_math", "test_multeq", fc::raw::pack(act));
   }
   // test div_eq with 10 random pairs of 128 bit numbers
   for (int i=0; i < 10; i++) {
      u128_action act;
      act.values[0] = dis(gen); act.values[0] <<= 64; act.values[0] |= dis(gen);
      act.values[1] = dis(gen); act.values[1] <<= 64; act.values[1] |= dis(gen);
      act.values[2] = act.values[0] / act.values[1];
      CALL_TEST_FUNCTION( *this, "test_math", "test_diveq", fc::raw::pack(act));
   }
   // test diveq for divide by zero
	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_math", "test_diveq_by_0", {}), fc::assert_exception, is_assert_exception);
	CALL_TEST_FUNCTION( *this, "test_math", "test_double_api", {});
	//BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_math", "test_double_api_div_0", {}), fc::assert_exception, is_assert_exception);
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * types_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(types_tests, tester) { try {
	produce_blocks(1000);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	produce_blocks(1000);

	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1000);

	CALL_TEST_FUNCTION( *this, "test_types", "types_size", {});
	CALL_TEST_FUNCTION( *this, "test_types", "char_to_symbol", {});
	CALL_TEST_FUNCTION( *this, "test_types", "string_to_name", {});
	CALL_TEST_FUNCTION( *this, "test_types", "name_class", {});
} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
