/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/types.h>
extern "C" {
/**
 *  This method is implemented as:
 *
 *  checksum calc_hash;
 *  sha256( data, length, &calc_hash );
 *  eos_assert( calc_hash == hash, "invalid hash" );
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void eos_assert_sha256( char* data, uint32_t length, const checksum_base* hash );

/**
 *  This method is implemented as:
 *
 *  checksum calc_hash;
 *  sha1( data, length, &calc_hash );
 *  eos_assert( calc_hash == hash, "invalid hash" );
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void eos_assert_sha1( char* data, uint32_t length, const checksum_base* hash );

/**
 *  This method is implemented as:
 *
 *  checksum calc_hash;
 *  sha512( data, length, &calc_hash );
 *  eos_assert( calc_hash == hash, "invalid hash" );
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void eos_assert_sha512( char* data, uint32_t length, const checksum_base* hash );

/**
 *  This method is implemented as:
 *
 *  checksum calc_hash;
 *  ripemd160( data, length, &calc_hash );
 *  eos_assert( calc_hash == hash, "invalid hash" );
 *
 *  This method is optimized to a NO-OP when in fast evaluation mode
 */
void eos_assert_ripemd160( char* data, uint32_t length, const checksum_base* hash );

/**
 *  Calculates sha256( data,length) and stores result in memory pointed to by hash 
 *  `hash` should be checksum<256>
 */
void sha256( char* data, uint32_t length, checksum_base* hash );

/**
 *  Calculates sha1( data,length) and stores result in memory pointed to by hash 
 *  `hash` should be checksum<160>
 */
void sha1( char* data, uint32_t length, checksum_base* hash );

/**
 *  Calculates sha512( data,length) and stores result in memory pointed to by hash 
 *  `hash` should be checksum<512>
 */
void sha512( char* data, uint32_t length, checksum_base* hash );

/**
 *  Calculates ripemd160( data,length) and stores result in memory pointed to by hash 
 *  `hash` should be checksum<160>
 */
void ripemd160( char* data, uint32_t length, checksum_base* hash );

/**
 * Calculates the public key used for a given signature and hash used to create a message
 * `digest` should be checksum<256>
 */
int recover_key( checksum_base* digest, const char* sig, size_t siglen, char* pub, size_t publen );

}
