/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <appbase/channel.hpp>
#include <appbase/method.hpp>

#include <eosio/chain/block.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain { namespace plugin_interface {
   using namespace eosio::chain;
   using namespace appbase;

   struct chain_plugin_interface;
   
   namespace channels {
      using accepted_block_header  = channel_decl<struct accepted_block_header_tag, block_state_ptr>;
      using accepted_block         = channel_decl<struct accepted_block_tag,        block_state_ptr>;
      using irreversible_block     = channel_decl<struct irreversible_block_tag,    block_state_ptr>;
      using accepted_transaction   = channel_decl<struct accepted_transaction_tag,  transaction_metadata_ptr>;
      using applied_transaction    = channel_decl<struct applied_transaction_tag,   transaction_trace_ptr>;
      using accepted_confirmation  = channel_decl<struct accepted_confirmation_tag, header_confirmation>;
   }

   namespace methods {
      using get_block_by_number    = method_decl<chain_plugin_interface, const signed_block&(uint32_t block_num)>;
      using get_block_by_id        = method_decl<chain_plugin_interface, const signed_block&(const block_id_type& block_id)>;
      using get_head_block_id      = method_decl<chain_plugin_interface, const block_id_type& ()>;

      using get_last_irreversible_block_number = method_decl<chain_plugin_interface, uint32_t ()>;
   }

} } }