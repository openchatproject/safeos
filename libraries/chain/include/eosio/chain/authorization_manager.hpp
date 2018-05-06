/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/permission_object.hpp>

#include <utility>
#include <functional>

namespace eosio { namespace chain {

   class controller;
   struct updateauth;
   struct deleteauth;
   struct linkauth;
   struct unlinkauth;
   struct canceldelay;

   class authorization_manager {
      public:
         using permission_id_type = permission_object::id_type;

         explicit authorization_manager(controller& c, chainbase::database& d);

         void add_indices();
         void initialize_database();

         const permission_object& create_permission( account_name account,
                                                     permission_name name,
                                                     permission_id_type parent,
                                                     const authority& auth,
                                                     time_point initial_creation_time = time_point()
                                                   );

         const permission_object& create_permission( account_name account,
                                                     permission_name name,
                                                     permission_id_type parent,
                                                     authority&& auth,
                                                     time_point initial_creation_time = time_point()
                                                   );

         const permission_object*  find_permission( const permission_level& level )const;
         const permission_object&  get_permission( const permission_level& level )const;

         /**
          * @brief Find the lowest authority level required for @ref authorizer_account to authorize a message of the
          * specified type
          * @param authorizer_account The account authorizing the message
          * @param code_account The account which publishes the contract that handles the message
          * @param type The type of message
          */
         optional<permission_name> lookup_minimum_permission( account_name authorizer_account,
                                                              scope_name code_account,
                                                              action_name type
                                                            )const;

         /**
          * @param actions - the actions to check authorization across
          * @param provided_keys - the set of public keys which have authorized the transaction
          * @param provided_permissions - the set of permissions which have authorized the transaction (empty permission name acts as wildcard)
          * @param provided_delay - the delay satisfied by the transaction
          * @param checktime - the function that can be called to track CPU usage and time during the process of checking authorization
          * @param allow_unused_keys - true if method should not assert on unused keys
          *
          * @return the maximum delay among the authorities needed to satisfy the authorizations
          */
         fc::microseconds
         check_authorization( const vector<action>&                actions,
                              const flat_set<public_key_type>&     provided_keys,
                              const flat_set<permission_level>&    provided_permissions = flat_set<permission_level>(),
                              fc::microseconds                     provided_delay = fc::microseconds(0),
                              const std::function<void(uint32_t)>& checktime = std::function<void(uint32_t)>(),
                              bool                                 allow_unused_keys = false
                            )const;


         /**
          * @param account - the account owner of the permission
          * @param permission - the permission name to check for authorization
          * @param provided_keys - a set of public keys
          * @param checktime - the function that can be called to track CPU usage and time during the process of checking authorization
          * @param allow_unused_keys - true if method does not require all keys to be used
          *
          * @return true if the provided keys are sufficient to authorize the account permission
          */
         bool check_authorization( account_name                         account,
                                   permission_name                      permission,
                                   const flat_set<public_key_type>&     provided_keys,
                                   const std::function<void(uint32_t)>& checktime = std::function<void(uint32_t)>(),
                                   bool                                 allow_unused_keys = false
                                 )const;

         flat_set<public_key_type> get_required_keys( const transaction& trx,
                                                      const flat_set<public_key_type>& candidate_keys,
                                                      fc::microseconds delay_threshold = fc::microseconds(0)
                                                    )const;



      private:
         const controller&    _control;
         chainbase::database& _db;

         optional<fc::microseconds> check_updateauth_authorization( const updateauth& update, const vector<permission_level>& auths )const;
         fc::microseconds check_deleteauth_authorization( const deleteauth& del, const vector<permission_level>& auths )const;
         fc::microseconds check_linkauth_authorization( const linkauth& link, const vector<permission_level>& auths )const;
         fc::microseconds check_unlinkauth_authorization( const unlinkauth& unlink, const vector<permission_level>& auths )const;
         void             check_canceldelay_authorization( const canceldelay& cancel, const vector<permission_level>& auths )const;

         optional<permission_name> lookup_linked_permission( account_name authorizer_account,
                                                             scope_name code_account,
                                                             action_name type
                                                           )const;
   };

} } /// namespace eosio::chain
