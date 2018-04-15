#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/context.hpp>


namespace eosio { namespace chain {

   void transaction_context::exec() {
      control.record_transaction( trx_meta );

      for( const auto& act : trx_meta->trx.context_free_actions ) {
         dispatch_action( act, act.account, true );
      }

      for( const auto& act : trx_meta->trx.actions ) {
         dispatch_action( act, act.account, false );
      }

      undo_session.squash();
   }

   void transaction_context::dispatch_action( const action& a, account_name receiver, bool context_free ) {
      apply_context  acontext( control, a, *trx_meta );
      acontext.context_free = context_free;
      acontext.receiver     = receiver;
      acontext.exec();

      fc::move_append(executed, move(acontext.executed) );
   }


} }
