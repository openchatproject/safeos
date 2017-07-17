#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>

/**
 * Make it easy to change the account name the currency is deployed to.
 */
#ifndef TOKEN_NAME
#define TOKEN_NAME currency
#endif

namespace TOKEN_NAME {

   typedef eos::token<uint64_t,N(currency)> CurrencyTokens;

   /**
    *  Transfer requires that the sender and receiver be the first two
    *  accounts notified and that the sender has provided authorization.
    */
   struct Transfer {
      AccountName       from;
      AccountName       to;
      CurrencyTokens    quantity;
   };

   struct Account {
      const uint64_t     key = N(account);
      CurrencyTokens     balance;

      bool  isEmpty()const  { return balance.quantity == 0; }
   };
   static_assert( sizeof(Account) == sizeof(uint64_t)+sizeof(CurrencyTokens), "unexpected packing" );

   using Accounts = Table<N(currency),N(currency),N(account),Account,uint64_t>;

   /**
    *  Accounts information for owner is stored:
    *
    *  owner/TOKEN_NAME/account/account -> Account
    *
    *  This API is made available for 3rd parties wanting read access to
    *  the users balance. If the account doesn't exist a default constructed
    *  account will be returned.
    */
   inline Account getAccount( AccountName owner ) {
      Account account;
      ///      scope, record
      Accounts::get( account, owner );
      return account;
   }

} /// namespace TOKEN_NAME

