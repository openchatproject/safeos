/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/crypto/sha256.hpp>

namespace eosio {

   class net_plugin_impl;
   struct handshake_message;

   namespace chain_apis {
      class read_only;
   }

namespace chain {

   struct chain_id_type : public fc::sha256 {
      using fc::sha256::sha256;

      template<typename T>
      inline friend T& operator<<( T& ds, const chain_id_type& cid ) {
        ds.write( cid.data(), cid.data_size() );
        return ds;
      }

      template<typename T>
      inline friend T& operator>>( T& ds, chain_id_type& cid ) {
        ds.read( cid.data(), cid.data_size() );
        return ds;
      }

      void reflector_verify()const;

      private:
         chain_id_type() = default;

         template<typename T>
         friend T fc::variant::as()const;

         friend class eosio::chain_apis::read_only;

         // Eventually net_plugin should be updated to not default construct chain_id. Then these two lines can be removed.
         friend class eosio::net_plugin_impl;
         friend struct eosio::handshake_message;
   };

} }  // namespace eosio::chain

namespace fc {
  class variant;
  void to_variant(const eosio::chain::chain_id_type& cid, fc::variant& v);
  void from_variant(const fc::variant& v, eosio::chain::chain_id_type& cid);
} // fc
