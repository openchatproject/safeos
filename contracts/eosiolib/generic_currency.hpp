#pragma once
#include <eosiolib/multi_index.hpp>
#include <eosiolib/table.hpp>
#include <eosiolib/token.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/action.hpp>
#include <string>

namespace eosio {
   using std::string;

   template<typename Token>
   class generic_currency {
      public:
          typedef Token token_type;
          static const uint64_t code                = token_type::code;
          static const uint64_t symbol              = token_type::symbol;
          static const uint64_t accounts_table_name = N(account);
          static const uint64_t stats_table_name    = N(stat);

          ACTION( code, issue ) {
             typedef action_meta<code,N(issue)> meta;
             account_name to;
             asset        quantity;

             EOSLIB_SERIALIZE( issue, (to)(quantity) )
          };

          ACTION( code, transfer ) {
             transfer(){}
             transfer( account_name f, account_name t, token_type q ):from(f),to(t),quantity(q){}

             account_name from;
             account_name to;
             asset        quantity;

             template<typename DataStream>
             friend DataStream& operator << ( DataStream& ds, const transfer& t ){
                return ds << t.from << t.to << t.quantity;
             }
             template<typename DataStream>
             friend DataStream& operator >> ( DataStream& ds, transfer& t ){
                ds >> t.from >> t.to >> t.quantity;
                eosio_assert( t.quantity.symbol== token_type::symbol, "unexpected asset type" );
                return ds;
             }
          };

          struct transfer_memo : public transfer {
             transfer_memo(){}
             transfer_memo( account_name f, account_name t, token_type q, string m )
             :transfer( f, t, q ), memo( std::move(m) ){}

             string       memo;

             EOSLIB_SERIALIZE_DERIVED( transfer_memo, transfer, (memo) )
          };

          struct account {
             uint64_t   symbol = token_type::symbol;
             token_type balance;

             auto primary_key() const { return symbol; }

             EOSLIB_SERIALIZE( account, (symbol)(balance) )
          };

          struct currency_stats {
             uint64_t   symbol = token_type::symbol;
             token_type supply;

             auto primary_key() const { return symbol; }

             EOSLIB_SERIALIZE( currency_stats, (symbol)(supply) )
          };

          /**
           *  Each user stores their balance in the singleton table under the
           *  scope of their account name.
           */
          typedef eosio::multi_index<accounts_table_name, account> accounts;
         //typedef eosio::multi_index<stats_table_name, > stats;
          typedef table64<code, stats_table_name, code, currency_stats>  stats;

          static token_type get_balance( account_name owner ) {
             accounts t( code, owner );
             auto ptr = t.find( symbol );
             return ptr ? ptr->balance : token_type( asset(0, symbol) );
             //return accounts::get_or_create( token_type::symbol, owner ).balance;
          }

         static void set_balance( account_name owner, token_type balance, account_name bill_to_if_create ) {
             //accounts::set( account{token_type::symbol,balance}, owner );
             accounts t( code, owner );
             auto f = [&](account& acc) {
                      acc.symbol = symbol;
                      acc.balance = balance;
             };
             auto ptr = t.find( symbol );
             if (ptr) {
                t.update( *ptr, 0, f);
             } else {
                t.emplace( owner, f);
             }
          }

          static void on( const issue& act ) {
             require_auth( code );

             //stats t( code);
             auto s = stats::get_or_create(token_type::symbol);
             s.supply += act.quantity;
             stats::set(s);

             set_balance( code, get_balance( code ) + act.quantity, code );

             inline_transfer( code, act.to, act.quantity ); 
          }


          static void on( const transfer& act ) {
             require_auth( act.from );
             require_recipient(act.to,act.from);

             set_balance( act.from, get_balance( act.from ) - act.quantity, act.from );
             set_balance( act.to, get_balance( act.to ) + act.quantity, act.from );
          }

          static void inline_transfer( account_name from, account_name to, token_type quantity, 
                                       string memo = string() )
          {
             action act( permission_level(code,N(active)), transfer_memo( from, to, asset(quantity), move(memo) ));
             act.send();
          }


         static void apply( account_name c, action_name act) {
            eosio::dispatch<generic_currency, transfer, issue>(c,act);
         }
   };

} /// namespace eosio



