/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <proxy/proxy.hpp>
#include <currency/currency.hpp>

namespace proxy {
   using namespace eosio;

   template<typename T>
   void apply_transfer(account_name code, const T& transfer) {
      config code_config;
      const auto self = current_receiver();
      auto get_res = configs::get(code_config, self);
      assert(get_res, "Attempting to use unconfigured proxy");
      if (transfer.from == self) {
         assert(transfer.to == code_config.owner,  "proxy may only pay its owner" );
      } else {
         assert(transfer.to == self, "proxy is not involved in this transfer");
         T new_transfer = T(transfer);
         new_transfer.from = self;
         new_transfer.to = code_config.owner;

         auto id = code_config.next_id++;
         configs::store(code_config, self);

         auto out_act = action<>(code, N(transfer), new_transfer, self, N(active));

         transaction<> out;
         out.add_action(out_act);
         out.add_write_scope(self);
         out.add_write_scope(code_config.owner);
         out.send(id);
      }
   }

   void apply_setowner(set_owner params) {
      const auto self = current_receiver();
      config code_config;
      configs::get(code_config, self);
      code_config.owner = params.owner;
      configs::store(code_config, self);
   }

   template<size_t ...Args>
   void apply_onerror( const deferred_transaction<Args...>& failed_dtrx ) {
      const auto self = current_receiver();
      config code_config;
      assert(configs::get(code_config, self), "Attempting to use unconfigured proxy");

      auto id = code_config.next_id++;
      configs::store(code_config, self);

      eosio::print("Resending Transaction: ", failed_dtrx._sender_id, " as ", id, "\n");
      failed_dtrx.send(id);
   }
}

using namespace proxy;
using namespace eosio;

extern "C" {
    void init()  {
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(currency) ) {
          if( action == N(transfer) ) {
             apply_transfer(code, current_action<currency::transfer>());
          }
       } else if ( code == N(eosio) ) {
          if( action == N(transfer) ) {
             apply_transfer(code, current_action<eosio::transfer>());
          }
       } else if (code == N(proxy) ) {
          if ( action == N(setowner)) {
             apply_setowner(current_action<set_owner>());
          }
       } else if (code == N(eosio) ) {
          if ( action == N(onerror)) {
             apply_onerror(deferred_transaction<>::from_current_action());
          }
       }
    }
}
