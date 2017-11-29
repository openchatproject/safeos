/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/types.hpp>

#include <numeric>

namespace eosio { namespace chain {

   struct permission_level {
      account_name    actor;
      permission_name permission;
   };

   /**
    *  An action is performed by an actor, aka an account. It may
    *  be created explicitly and authorized by signatures or might be
    *  generated implicitly by executing application code. 
    *
    *  This follows the design pattern of React Flux where actions are
    *  named and then dispatched to one or more action handlers (aka stores).
    *  In the context of eosio, every action is dispatched to the handler defined
    *  by account 'scope' and function 'name', but the default handler may also
    *  forward the action to any number of additional handlers. Any application
    *  can write a handler for "scope::name" that will get executed if and only if
    *  this action is forwarded to that application.
    *
    *  Each action may require the permission of specific actors. Actors can define
    *  any number of permission levels. The actors and their respective permission
    *  levels are declared on the action and validated independently of the executing
    *  application code. An application code will check to see if the required authorization
    *  were properly declared when it executes.
    */
   struct action {
      scope_name                 scope;
      action_name                name;
      vector<permission_level>   authorization;
      vector<char>               data;

      action(){}

      template<typename T>
      action( vector<permission_level> auth, const T& value ) {
         scope       = T::get_scope();
         name        = T::get_name();
         authorization = move(auth);
         data        = fc::raw::pack(value);
      }

      template<typename T>
      T as()const {
         FC_ASSERT( scope == T::get_scope() );
         FC_ASSERT( name  == T::get_name()  );
         return fc::raw::unpack<T>(data);
      }
   };

   struct action_notice : public action {
      account_name receiver;
   };


   /**
    * When a transaction is referenced by a block it could imply one of several outcomes which 
    * describe the state-transition undertaken by the block producer. 
    */
   struct transaction_receipt {
      enum status_enum {
         generated = 0, ///< created, not yet known to be valid or invalid (no state transition)
         executed  = 1, ///< succeed, no error handler executed
         soft_fail = 2, ///< objectively failed (not executed), error handler executed 
         hard_fail = 3  ///< objectively failed and error handler objectively failed thus no state change
      };

      transaction_receipt( transaction_id_type tid ):status(executed),id(tid){}

      fc::enum_type<uint8_t,status_enum>  status;
      transaction_id_type                 id;
   };

   /**
    *  The transaction header contains the fixed-sized data
    *  associated with each transaction. It is separated from
    *  the transaction body to facilitate partial parsing of
    *  transactions without requiring dynamic memory allocation.
    *
    *  All transactions have an expiration time after which they
    *  may no longer be included in the blockchain. Once a block
    *  with a block_header::timestamp greater than expiration is 
    *  deemed irreversible, then a user can safely trust the transaction
    *  will never be included. 
    *
    
    *  Each region is an independent blockchain, it is included as routing
    *  information for inter-blockchain communication. A contract in this
    *  region might generate or authorize a transaction intended for a foreign
    *  region.
    */
   struct transaction_header {
      time_point_sec         expiration;   ///< the time at which a transaction expires
      uint16_t               region        = 0; ///< the computational memory region this transaction applies to.
      uint16_t               ref_block_num = 0; ///< specifies a block num in the last 2^16 blocks.
      uint32_t               ref_block_prefix = 0; ///< specifies the lower 32 bits of the blockid at get_ref_blocknum

      /**
       * @return the absolute block number given the relative ref_block_num
       */
      block_num_type get_ref_blocknum( block_num_type head_blocknum )const {
         return ((head_blocknum/0xffff)*0xffff) + head_blocknum%0xffff;
      }
      void set_reference_block( const block_id_type& reference_block );
      bool verify_reference_block( const block_id_type& reference_block )const;
   };

   /**
    *  A transaction consits of a set of messages which must all be applied or
    *  all are rejected. These messages have access to data within the given
    *  read and write scopes.
    */
   struct transaction : public transaction_header {
      vector<account_name>   read_scope;
      vector<account_name>   write_scope;
      vector<action>         actions;

      transaction_id_type id()const;
      digest_type         sig_digest( const chain_id_type& chain_id )const;
   };

   struct signed_transaction : public transaction {
      vector<signature_type> signatures;

      const signature_type&     sign(const private_key_type& key, const chain_id_type& chain_id);
      signature_type            sign(const private_key_type& key, const chain_id_type& chain_id)const;
      flat_set<public_key_type> get_signature_keys( const chain_id_type& chain_id )const;
   };


   /**
    *  When a transaction is generated it can be scheduled to occur
    *  in the future. It may also fail to execute for some reason in
    *  which case the sender needs to be notified. When the sender
    *  sends a transaction they will assign it an ID which will be
    *  passed back to the sender if the transaction fails for some
    *  reason.
    */
   struct deferred_transaction : public transaction
   {
      uint32_t       sender_id; /// ID assigned by sender of generated, accessible via WASM api when executing normal or error
      account_name   sender; /// receives error handler callback
      time_point_sec execute_after; /// delayed exeuction
   };

   struct action_result {
      account_name               receiver;
      action                     act;
      vector<string>             console;
   };

   struct transaction_result : transaction_receipt {
      using transaction_receipt::transaction_receipt;

      vector<action_result>         action_results;
      vector<deferred_transaction>  deferred_transactions;
   };

} } // eosio::chain

FC_REFLECT( eosio::chain::permission_level, (actor)(permission) )
FC_REFLECT( eosio::chain::action, (scope)(name)(authorization)(data) )
FC_REFLECT( eosio::chain::transaction_header, (expiration)(region)(ref_block_num)(ref_block_prefix) )
FC_REFLECT_DERIVED( eosio::chain::transaction, (eosio::chain::transaction_header), (read_scope)(write_scope)(actions) )
FC_REFLECT_DERIVED( eosio::chain::signed_transaction, (eosio::chain::transaction), (signatures) )
FC_REFLECT_DERIVED( eosio::chain::deferred_transaction, (eosio::chain::transaction), (sender_id)(sender)(execute_after) )
FC_REFLECT( eosio::chain::action_result, (receiver)(act)(console) )
FC_REFLECT( eosio::chain::transaction_receipt, (status)(id))
FC_REFLECT_ENUM( eosio::chain::transaction_receipt::status_enum, (generated)(executed)(soft_fail)(hard_fail))
FC_REFLECT_DERIVED( eosio::chain::transaction_result, (eosio::chain::transaction_receipt), (action_results)(deferred_transactions) )



