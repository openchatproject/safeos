/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eoslib/types.h>

/**
 *  @defgroup accountcapi Account API
 *  @brief Define API for querying account data
 *  @ingroup contractdev
 */

extern "C" {
   /**
    * @brief Return the balance for the provided account
    *
    * @param balance -  a pointer to a range of memory to store balance data
    * @param len     -  length of the range of memory to store balance data
    *
    * @pre data is a valid pointer to a range of memory at least datalen bytes long
    * @pre data is a pointer to a balance object
    * @pre *((uint64_t*)data) stores the primary key
    *
    *  Example:
    *  @code
    *  balance b;
    *  b.account = n(myaccount);
    *  balance(b, sizeof(balance));
    *  @endcode
    */

   void account_balance_get( void* balance, uint32_t len );
   ///@ } accountcapi
}
