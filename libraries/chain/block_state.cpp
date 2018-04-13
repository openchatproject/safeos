#include <eosio/chain/block_state.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {


   /**
    *  Perform context free validation of transaction state
    */
   void validate_transaction( const transaction& trx ) {
      EOS_ASSERT( !trx.actions.empty(), tx_no_action, "transaction must have at least one action" );

      // Check for at least one authorization in the context-aware actions
      bool has_auth = false;
      for( const auto& act : trx.actions ) {
         has_auth |= !act.authorization.empty();
         if( has_auth ) break;
      }
      EOS_ASSERT( has_auth, tx_no_auths, "transaction must have at least one authorization" );

      // Check that there are no authorizations in any of the context-free actions
      for (const auto &act : trx.context_free_actions) {
         EOS_ASSERT( act.authorization.empty(), cfa_irrelevant_auth, 
                     "context-free actions cannot require authorization" );
      }

      EOS_ASSERT( trx.max_kcpu_usage.value < UINT32_MAX / 1024UL, transaction_exception, "declared max_kcpu_usage overflows when expanded to max cpu usage" );
      EOS_ASSERT( trx.max_net_usage_words.value < UINT32_MAX / 8UL, transaction_exception, "declared max_net_usage_words overflows when expanded to max net usage" );
   } /// validate_transaction


   block_state::block_state( block_header_state h, signed_block_ptr b )
   :block_header_state( move(h) ), block( move(b) )
   {
      ilog("");
      if( block ) {
      ilog("");
         for( const auto& packed : block->transactions ) {
      ilog("");
#if 0
            auto meta_ptr = std::make_shared<transaction_metadata>( packed, chain_id_type(), block->timestamp );

            /** perform context-free validation of transactions */
            const auto& trx = meta_ptr->trx();
            FC_ASSERT( time_point(trx.expiration) > header.timestamp, "transaction is expired" );
            validate_transaction( trx );

            auto id = meta_ptr->id;
            input_transactions[id] = move(meta_ptr);
#endif
         }

      } // end if block
   } 

   void block_state::reset_trace() {
      trace = std::make_shared<block_trace>();
   }



} } /// eosio::chain
