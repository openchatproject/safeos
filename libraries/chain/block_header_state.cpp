#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {

  uint32_t block_header_state::calc_dpos_last_irreversible()const {
    if( producer_to_last_produced.size() == 0 ) 
       return 0;

    vector<uint32_t> irb;
    irb.reserve( producer_to_last_produced.size() );
    for( const auto& item : producer_to_last_produced ) 
       irb.push_back(item.second);

    size_t offset = EOS_PERCENT(irb.size(), config::percent_100- config::irreversible_threshold_percent);
    std::nth_element( irb.begin(), irb.begin() + offset, irb.end() );

    return irb[offset];
  }


  bool block_header_state::is_active_producer( account_name n )const {
    return producer_to_last_produced.find(n) != producer_to_last_produced.end();
  }

  producer_key block_header_state::scheduled_producer( block_timestamp_type t )const {
    auto index = t.slot % (active_schedule.producers.size() * config::producer_repetitions);
    index /= config::producer_repetitions;
    return active_schedule.producers[index];
  }


  /**
   *  Generate a template block header state for a given block time, it will not
   *  contain a transaction mroot, action mroot, or new_producers as those components
   *  are derived from chain state.
   */
  block_header_state block_header_state::generate_next( block_timestamp_type when )const {
     block_header_state result;

     if( when != block_timestamp_type() ) {
        FC_ASSERT( when > header.timestamp, "next block must be in the future" );
     } else {
        (when = header.timestamp).slot++;
     }
     result.header.timestamp = when;
     result.header.previous  = id;


    auto prokey                                  = scheduled_producer(when);
    result.block_signing_key                     = prokey.block_signing_key;
    result.header.producer                       = prokey.producer_name;
    result.prior_pending_schedule_hash           = pending_schedule_hash;
    result.pending_schedule_hash                 = pending_schedule_hash;
    result.block_num                             = block_num + 1;
    result.producer_to_last_produced             = producer_to_last_produced;
    result.producer_to_last_produced[prokey.producer_name] = result.block_num;
    result.dpos_last_irreversible_blocknum       = result.calc_dpos_last_irreversible();
    result.blockroot_merkle = blockroot_merkle;
    result.blockroot_merkle.append( id );
    result.header.block_mroot = result.blockroot_merkle.get_root();

    if( result.dpos_last_irreversible_blocknum >= pending_schedule_lib_num ) {
      result.active_schedule = pending_schedule; 

      flat_map<account_name,uint32_t> new_producer_to_last_produced;
      for( const auto& pro : result.active_schedule.producers ) {
        auto existing = producer_to_last_produced.find( pro.producer_name );
        if( existing != producer_to_last_produced.end() ) {
          new_producer_to_last_produced[pro.producer_name] = existing->second;
        } else {
          new_producer_to_last_produced[pro.producer_name] = result.dpos_last_irreversible_blocknum;
        }
      }
      result.producer_to_last_produced = move( new_producer_to_last_produced );
      result.producer_to_last_produced[prokey.producer_name] = result.block_num;
    } else {
      result.active_schedule  = active_schedule;
      result.pending_schedule = pending_schedule;
    }

    return result;
  } /// generate_next



  /**
   *  Transitions the current header state into the next header state given the supplied signed block header.
   *
   *  Given a signed block header, generate the expected template based upon the header time,
   *  then validate that the provided header matches the template. 
   *
   *  If the header specifies new_producers then apply them accordingly. 
   */
  block_header_state block_header_state::next( const signed_block_header& h )const {
    FC_ASSERT( h.timestamp != block_timestamp_type() );

    auto result = generate_next( h.timestamp );
    FC_ASSERT( result.header.producer = h.producer, "wrong producer specified" );
    FC_ASSERT( h.previous == id, "block must link to current state" );
    FC_ASSERT( h.timestamp > header.timestamp, "block must be later in time" );

    FC_ASSERT( result.block_signing_key == h.signee( pending_schedule_hash ), "block not signed by expected key" );
    result.id = h.id();

    FC_ASSERT( result.header.block_mroot == h.block_mroot, "mistmatch block merkle root" );

    if( h.new_producers ) {
      FC_ASSERT( h.new_producers->version == result.active_schedule.version + 1, "wrong producer schedule version specified" );
      FC_ASSERT( result.pending_schedule.producers.size() == 0, 
                 "cannot set new pending producers until last pending is confirmed" );
      result.pending_schedule         = *h.new_producers;
      result.pending_schedule_hash    = digest_type::hash( result.pending_schedule );
      result.pending_schedule_lib_num = h.block_num();
    } 

    result.header = h;

    return result;
  } /// next

} } /// namespace eosio::chain
