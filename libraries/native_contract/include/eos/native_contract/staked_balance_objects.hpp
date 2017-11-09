/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/types.hpp>
#include <eos/chain/multi_index_includes.hpp>

#include <eos/types/types.hpp>

#include <chainbase/chainbase.hpp>

#include <fc/static_variant.hpp>

namespace native {
namespace eosio {

using namespace ::eosio::chain;
namespace config = ::eosio::config;
namespace chain = ::eosio::chain;
namespace types = ::eosio::types;

/**
 * @brief The ProducerSlate struct stores a list of producers voted on by an account
 */
struct ProducerSlate {
   std::array<types::account_name, config::max_producer_votes> votes;
   size_t size = 0;

   void add(types::account_name producer) {
      votes[size++] = producer;
      std::inplace_merge(votes.begin(), votes.begin() + size - 1, votes.begin() + size);
   }
   void remove(types::account_name producer) {
      auto itr = std::remove(votes.begin(), votes.begin() + size, producer);
      size = std::distance(votes.begin(), itr);
   }
   bool contains(types::account_name producer) const {
      return std::binary_search(votes.begin(), votes.begin() + size, producer);
   }

   auto range() { return boost::make_iterator_range_n(votes.begin(), size); }
   auto range() const { return boost::make_iterator_range_n(votes.begin(), size); }
};

/**
 * @brief The StakedBalanceObject class tracks the staked balance (voting balance) for accounts
 */
class StakedBalanceObject : public chainbase::object<chain::staked_balance_object_type, StakedBalanceObject> {
   OBJECT_CTOR(StakedBalanceObject)

   id_type id;
   types::account_name ownerName;

   types::share_type stakedBalance = 0;
   types::share_type unstakingBalance = 0;
   types::time      lastUnstakingTime = types::time::maximum();

   /// The account's vote on producers. This may either be a list of approved producers, or an account to proxy vote to
   fc::static_variant<ProducerSlate, types::account_name> producerVotes = ProducerSlate{};

   /**
    * @brief Add the provided stake to this balance, maintaining invariants
    * @param newStake The new stake to add to the balance
    * @param db Read-write reference to the database
    *
    * This method will update this object with the new stake, while maintaining invariants around the stake balance,
    * such as by updating vote tallies
    */
   void stakeTokens(types::share_type newStake, chainbase::database& db) const;
   /**
    * @brief Begin unstaking the specified amount of stake, maintaining invariants
    * @param amount The amount of stake to begin unstaking
    * @param db Read-write reference to the database
    *
    * This method will update this object's balances while maintaining invariants around the stake balances, such as by
    * updating vote tallies
    */
   void beginUnstakingTokens(types::share_type amount, chainbase::database& db) const;
   /**
    * @brief Finish unstaking the specified amount of stake
    * @param amount The amount of stake to finish unstaking
    * @param db Read-write reference to the database
    *
    * This method will update this object's balances. There aren't really any invariants to maintain on this call, as
    * the tokens are already unstaked and removed from vote tallies, but it is provided for completeness' sake.
    */
   void finishUnstakingTokens(types::share_type amount, chainbase::database& db) const;

   /**
    * @brief Propagate the specified change in stake to the producer votes or the proxy
    * @param stakeDelta The change in stake
    * @param db Read-write reference to the database
    *
    * This method will apply the provided delta in voting stake to the next stage in the producer voting pipeline,
    * whether that be the producers in the slate, or the account the votes are proxied to.
    *
    * This method will *not* update this object in any way. It will not adjust @ref stakedBalance, etc
    */
   void propagateVotes(types::share_type stakeDelta, chainbase::database& db) const;
};

struct byOwnerName;

using StakedBalanceMultiIndex = chainbase::shared_multi_index_container<
   StakedBalanceObject,
   indexed_by<
      ordered_unique<tag<by_id>,
         member<StakedBalanceObject, StakedBalanceObject::id_type, &StakedBalanceObject::id>
      >,
      ordered_unique<tag<byOwnerName>,
         member<StakedBalanceObject, types::account_name, &StakedBalanceObject::ownerName>
      >
   >
>;

} } // namespace native::eos

CHAINBASE_SET_INDEX_TYPE(native::eosio::StakedBalanceObject, native::eosio::StakedBalanceMultiIndex)
