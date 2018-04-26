#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester_network.hpp>
#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>
#include <currency/currency.wast.hpp>
#include <currency/currency.abi.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;


BOOST_AUTO_TEST_SUITE(delay_tests)

BOOST_FIXTURE_TEST_CASE( delay_create_account, validating_tester) { try {

   produce_blocks(2);
   signed_transaction trx;

   account_name a = N(newco);
   account_name creator = config::system_account_name;

   auto owner_auth =  authority( get_public_key( a, "owner" ) );
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             newaccount{
                                .creator  = creator,
                                .name     = a,
                                .owner    = owner_auth,
                                .active   = authority( get_public_key( a, "active" ) ),
                                .recovery = authority( get_public_key( a, "recovery" ) ),
                             });
   set_transaction_headers(trx);
   trx.delay_sec = 3;
   trx.sign( get_private_key( creator, "active" ), chain_id_type()  );

   auto trace = push_transaction( trx );

   produce_blocks(8);

} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE( delay_error_create_account, validating_tester) { try {

   produce_blocks(2);
   signed_transaction trx;

   account_name a = N(newco);
   account_name creator = config::system_account_name;

   auto owner_auth =  authority( get_public_key( a, "owner" ) );
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             newaccount{
                                .creator  = N(bad), /// a does not exist, this should error when execute
                                .name     = a,
                                .owner    = owner_auth,
                                .active   = authority( get_public_key( a, "active" ) ),
                                .recovery = authority( get_public_key( a, "recovery" ) ),
                             });
   set_transaction_headers(trx);
   trx.delay_sec = 3;
   trx.sign( get_private_key( creator, "active" ), chain_id_type()  );

   ilog( fc::json::to_pretty_string(trx) );
   auto trace = push_transaction( trx );
   edump((*trace));

   produce_blocks(8);

} FC_LOG_AND_RETHROW() }


asset get_currency_balance(const TESTER& chain, account_name account) {
   return chain.get_currency_balance(N(currency), symbol(SY(4,CUR)), account);
}

// test link to permission with delay directly on it
BOOST_AUTO_TEST_CASE( link_delay_direct_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" ),
       20, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks(18);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test link to permission with delay on permission which is parent of min permission (special logic in permission_object::satisfies)
BOOST_AUTO_TEST_CASE( link_delay_direct_parent_permission_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "active")
           ("parent", "owner")
           ("auth",  authority(chain.get_public_key(tester_account, "active"), 15))
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" ),
       20, 15
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks(28);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test link to permission with delay on permission between min permission and authorizing permission it
BOOST_AUTO_TEST_CASE( link_delay_direct_walk_parent_permissions_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first")))
   );
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "first")
           ("auth",  authority(chain.get_public_key(tester_account, "second")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "second"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 20))
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "3.0000 CUR")
       ("memo", "hi" ),
       30, 20
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks(38);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test removing delay on permission
BOOST_AUTO_TEST_CASE( link_delay_permission_change_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"))),
           30, 10);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // delayed update auth removing the delay will finally execute
   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks(15);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   // second transfer finally is performed
   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(0, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test removing delay on permission based on heirarchy delay
BOOST_AUTO_TEST_CASE( link_delay_permission_change_with_delay_heirarchy_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "first")
           ("auth",  authority(chain.get_public_key(tester_account, "second")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "second"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"))),
           30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks(15);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test moving link with delay on permission
BOOST_AUTO_TEST_CASE( link_delay_link_change_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "second")))
   );

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "second"),
           30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks(15);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test moving link with delay on permission's parent
BOOST_AUTO_TEST_CASE( link_delay_link_change_heirarchy_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "second")
           ("parent", "first")
           ("auth",  authority(chain.get_public_key(tester_account, "first")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "second"));
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "third")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "third")))
   );

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "third"),
           30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // first transfer will finally be performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks(15);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("89.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("11.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("84.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("16.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() } /// link_delay_link_change_heirarchy_test

// test delay_sec field imposing unneeded delay
BOOST_AUTO_TEST_CASE( mindelay_test ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" )
   );

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   // send transfer with delay_sec set to 10
   const auto& acnt = chain.control->db().get<account_object,by_name>(N(currency));
   const auto abi = acnt.get_abi();
   chain::abi_serializer abis(abi);
   const auto a = chain.control->db().get<account_object,by_name>(N(currency)).get_abi();

   string action_type_name = abis.get_action_type(name("transfer"));

   action act;
   act.account = N(currency);
   act.name = name("transfer");
   act.authorization.push_back(permission_level{N(tester), config::active_name});
   act.data = abis.variant_to_binary(action_type_name, fc::mutable_variant_object()
      ("from", "tester")
      ("to", "tester2")
      ("quantity", "3.0000 CUR")
      ("memo", "hi" )
   );

   signed_transaction trx;
   trx.actions.push_back(act);

   chain.set_transaction_headers(trx, 30, 10);
   trx.sign(chain.get_private_key(N(tester), "active"), chain_id_type());
   trace = chain.push_transaction(trx);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, gen_size);
   BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks(18);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("99.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 CUR"), liquid_balance);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("96.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 CUR"), liquid_balance);

} FC_LOG_AND_RETHROW() }/// schedule_test

// test canceldelay action cancelling a delayed transaction
BOOST_AUTO_TEST_CASE( canceldelay_test ) { try {
   TESTER chain;
   const auto& tester_account = N(tester);
   std::vector<transaction_id_type> ids;
   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks(10);

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks(10);

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 10))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();
   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "1.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   //wdump((fc::json::to_pretty_string(trace)));
   ids.push_back(trace->id);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   const auto& idx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
   auto itr = idx.find( trace->id );
   BOOST_CHECK_EQUAL( (itr != idx.end()), true );

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"))),
           30, 10
   );
   //wdump((fc::json::to_pretty_string(trace)));
   ids.push_back(trace->id);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   chain.produce_blocks(16);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transaction will be delayed 20 blocks
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "5.0000 CUR")
       ("memo", "hi" ),
       30, 10
   );
   //wdump((fc::json::to_pretty_string(trace)));
   ids.push_back(trace->id);
   BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(3, gen_size);
   BOOST_CHECK_EQUAL(0, trace->action_traces.size());

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // send canceldelay for first delayed transaction
   signed_transaction trx;
   trx.actions.emplace_back(vector<permission_level>{{N(tester), config::active_name}},
                            chain::canceldelay{{N(tester), config::active_name}, ids[0]});

   chain.set_transaction_headers(trx);
   trx.sign(chain.get_private_key(N(tester), "active"), chain_id_type());
   trace = chain.push_transaction(trx);
   //wdump((fc::json::to_pretty_string(trace)));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   const auto& cidx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
   auto citr = cidx.find( ids[0] );
   BOOST_CHECK_EQUAL( (citr == cidx.end()), true );

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(2, gen_size);

   chain.produce_blocks();
   // update auth will finally be performed

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

   // this transfer is performed right away since delay is removed
   trace = chain.push_action(N(currency), name("transfer"), N(tester), fc::mutable_variant_object()
       ("from", "tester")
       ("to", "tester2")
       ("quantity", "10.0000 CUR")
       ("memo", "hi" )
   );
   //wdump((fc::json::to_pretty_string(trace)));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   chain.produce_blocks();

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("90.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("10.0000 CUR"), liquid_balance);

   chain.produce_blocks(15);

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(1, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("90.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("10.0000 CUR"), liquid_balance);

   // second transfer finally is performed
   chain.produce_blocks();

   gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_CHECK_EQUAL(0, gen_size);

   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("85.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester2));
   BOOST_REQUIRE_EQUAL(asset::from_string("15.0000 CUR"), liquid_balance);
} FC_LOG_AND_RETHROW() }

// test canceldelay action under different permission levels
BOOST_AUTO_TEST_CASE( canceldelay_test2 ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);
   std::vector<transaction_id_type> ids;
   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.produce_blocks();

   chain.set_code(N(currency), currency_wast);
   chain.set_abi(N(currency), currency_abi);

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks();

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 5))
   );
   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
          ("account", "tester")
          ("permission", "second")
          ("parent", "first")
          ("auth",  authority(chain.get_public_key(tester_account, "second")))
   );
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "first"));

   chain.produce_blocks();
   chain.push_action(N(currency), N(create), N(currency), mutable_variant_object()
           ("issuer", "currency" )
           ("maximum_supply", "9000000.0000 CUR" )
           ("can_freeze", 0)
           ("can_recall", 0)
           ("can_whitelist", 0)
   );

   chain.push_action(N(currency), name("issue"), N(currency), fc::mutable_variant_object()
           ("to",       "currency")
           ("quantity", "1000000.0000 CUR")
           ("memo", "for stuff")
   );

   auto trace = chain.push_action(N(currency), name("transfer"), N(currency), fc::mutable_variant_object()
       ("from", "currency")
       ("to", "tester")
       ("quantity", "100.0000 CUR")
       ("memo", "hi" )
   );
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
   auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, gen_size);

   chain.produce_blocks();
   auto liquid_balance = get_currency_balance(chain, N(currency));
   BOOST_REQUIRE_EQUAL(asset::from_string("999900.0000 CUR"), liquid_balance);
   liquid_balance = get_currency_balance(chain, N(tester));
   BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);

   ilog("attempting first delayed transfer");

   {
      // this transaction will be delayed 10 blocks
      trace = chain.push_action(N(currency), name("transfer"), vector<permission_level>{{N(tester), N(first)}}, fc::mutable_variant_object()
          ("from", "tester")
          ("to", "tester2")
          ("quantity", "1.0000 CUR")
          ("memo", "hi" ),
          30, 5
      );
      auto trx_id = trace->id;
      BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(1, gen_size);
      BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

      const auto& idx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto itr = idx.find( trace->id );
      if (itr == idx.end()) BOOST_TEST(false);
      const auto sender_id_to_cancel = itr->sender_id;

      chain.produce_blocks();

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

      // attempt canceldelay with wrong canceling_auth for delayed transfer of 1.0000 CUR
      {
         signed_transaction trx;
         trx.actions.emplace_back(vector<permission_level>{{N(tester), config::active_name}},
                                  chain::canceldelay{{N(tester), config::active_name}, trx_id});
         chain.set_transaction_headers(trx);
         trx.sign(chain.get_private_key(N(tester), "active"), chain_id_type());
         BOOST_REQUIRE_THROW( chain.push_transaction(trx), transaction_exception );
      }

      // attempt canceldelay with "second" permission for delayed transfer of 1.0000 CUR
      {
         signed_transaction trx;
         trx.actions.emplace_back(vector<permission_level>{{N(tester), N(second)}},
                                  chain::canceldelay{{N(tester), N(first)}, trx_id});
         chain.set_transaction_headers(trx);
         trx.sign(chain.get_private_key(N(tester), "second"), chain_id_type());
         BOOST_REQUIRE_THROW( chain.push_transaction(trx), tx_irrelevant_auth );
      }

      // canceldelay with "active" permission for delayed transfer of 1.0000 CUR
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(tester), config::active_name}},
                               chain::canceldelay{{N(tester), N(first)}, trx_id});
      chain.set_transaction_headers(trx);
      trx.sign(chain.get_private_key(N(tester), "active"), chain_id_type());
      trace = chain.push_transaction(trx);

      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(1, gen_size);

      const auto& cidx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto citr = cidx.find( ids[0] );
      if (citr == cidx.end()) BOOST_TEST(false);
      const auto sender_id_canceled = citr->sender_id;
      BOOST_REQUIRE_EQUAL(std::string(uint128(sender_id_to_cancel)), std::string(uint128(sender_id_canceled)));

      chain.produce_blocks(10);

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);
   }

   ilog("reset minimum permission of transfer to second permission");

   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "currency")
           ("type", "transfer")
           ("requirement", "second"),
           30, 5
   );

   chain.produce_blocks(10);


   ilog("attempting second delayed transfer");
   {
      // this transaction will be delayed 10 blocks
      trace = chain.push_action(N(currency), name("transfer"), vector<permission_level>{{N(tester), N(second)}}, fc::mutable_variant_object()
          ("from", "tester")
          ("to", "tester2")
          ("quantity", "5.0000 CUR")
          ("memo", "hi" ),
          30, 5
      );
      auto trx_id = trace->id;
      BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
      auto gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_CHECK_EQUAL(1, gen_size);
      BOOST_CHECK_EQUAL(0, trace->action_traces.size());

      const auto& idx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto itr = idx.find( trace->id );
      if (itr == idx.end()) BOOST_TEST(false);
      const auto sender_id_to_cancel = itr->sender_id;

      chain.produce_blocks();

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

      // canceldelay with "first" permission for delayed transfer of 5.0000 CUR
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(tester), N(first)}},
                               chain::canceldelay{{N(tester), N(second)}, trx_id});
      chain.set_transaction_headers(trx);
      trx.sign(chain.get_private_key(N(tester), "first"), chain_id_type());
      trace = chain.push_transaction(trx);

      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(1, gen_size);

      const auto& cidx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto citr = cidx.find( ids[0] );
      if (citr == cidx.end()) BOOST_TEST(false);
      const auto sender_id_canceled = citr->sender_id;
      BOOST_REQUIRE_EQUAL(std::string(uint128(sender_id_to_cancel)), std::string(uint128(sender_id_canceled)));

      chain.produce_blocks(10);

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);
   }

   ilog("attempting third delayed transfer");

   {
      // this transaction will be delayed 10 blocks
      trace = chain.push_action(N(currency), name("transfer"), vector<permission_level>{{N(tester), config::owner_name}}, fc::mutable_variant_object()
          ("from", "tester")
          ("to", "tester2")
          ("quantity", "10.0000 CUR")
          ("memo", "hi" ),
          30, 5
      );
      auto trx_id = trace->id;
      BOOST_REQUIRE_EQUAL(transaction_receipt::delayed, trace->receipt.status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(1, gen_size);
      BOOST_REQUIRE_EQUAL(0, trace->action_traces.size());

      const auto& idx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto itr = idx.find( trace->id );
      if (itr == idx.end()) BOOST_TEST(false);
      const auto sender_id_to_cancel = itr->sender_id;

      chain.produce_blocks();

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);

      // attempt canceldelay with "active" permission for delayed transfer of 10.0000 CUR
      {
         signed_transaction trx;
         trx.actions.emplace_back(vector<permission_level>{{N(tester), N(active)}},
                                  chain::canceldelay{{N(tester), config::owner_name}, trx_id});
         chain.set_transaction_headers(trx);
         trx.sign(chain.get_private_key(N(tester), "active"), chain_id_type());
         BOOST_REQUIRE_THROW( chain.push_transaction(trx), tx_irrelevant_auth );
      }

      // canceldelay with "owner" permission for delayed transfer of 10.0000 CUR
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(tester), config::owner_name}},
                               chain::canceldelay{{N(tester), config::owner_name}, trx_id});
      chain.set_transaction_headers(trx);
      trx.sign(chain.get_private_key(N(tester), "owner"), chain_id_type());
      trace = chain.push_transaction(trx);

      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt.status);
      gen_size = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_REQUIRE_EQUAL(1, gen_size);

      const auto& cidx = chain.control->db().get_index<generated_transaction_multi_index,by_trx_id>();
      auto citr = cidx.find( ids[0] );
      if (citr == cidx.end()) BOOST_TEST(false);
      const auto sender_id_canceled = citr->sender_id;
      BOOST_REQUIRE_EQUAL(std::string(uint128(sender_id_to_cancel)), std::string(uint128(sender_id_canceled)));

      chain.produce_blocks(10);

      liquid_balance = get_currency_balance(chain, N(tester));
      BOOST_REQUIRE_EQUAL(asset::from_string("100.0000 CUR"), liquid_balance);
      liquid_balance = get_currency_balance(chain, N(tester2));
      BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 CUR"), liquid_balance);
   }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
