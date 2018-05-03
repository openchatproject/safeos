/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/types.h>

/**
 *  @defgroup chainapi Chain API
 *  @brief Define API for querying internal chain state
 *  @ingroup contractdev
 */

/**
 *  @defgroup chaincapi Chain C API
 *  @brief C API for querying internal chain state
 *  @ingroup chainapi
 *  @{
 */

extern "C" {
    /**
     *  Gets the set of active producers.
     *  @brief Gets the set of active producers.
     *
     *  @param producers - Pointer to a buffer of account names
     *  @param datalen - Byte length of buffer
     * 
     *  @return uint32_t - Number of bytes actually populated
     *  The passed in account_name pointer gets the array of active producers.
     * 
     *  Example:
     *  @code
     *  account_name producers[21];
     *  uint32_t bytes_populated = get_active_producers(producers, sizeof(account_name)*21);
     *  @endcode
     */

    uint32_t get_active_producers( account_name* producers, uint32_t datalen );

   ///@ } chaincapi
}
