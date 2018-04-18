#pragma once
#include <eosio/chain/controller.hpp>

namespace eosio { namespace chain {


   class transaction_context {
      public:
         transaction_context( controller& c, const transaction_metadata_ptr& trx ) 
         :control(c),
          trx_meta(trx),
          undo_session(c.db().start_undo_session(true))
         {
            executed.reserve( trx_meta->total_actions() );
         }

         void exec();
         void dispatch_action( const action& a, account_name receiver, bool context_free = false );

         vector<action_receipt>            executed;

         transaction_trace                 trace;

         controller&                       control;
         const transaction_metadata_ptr&   trx_meta;
         chainbase::database::session      undo_session;
   };

} }
