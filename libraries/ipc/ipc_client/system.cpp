/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

extern "C" {

void eosio_abort() {
   edump(("abort() called"));
   FC_ASSERT( false, "abort() called");
}

void  eosio_assert( uint32_t test, const char* msg ) {
   if( BOOST_UNLIKELY( !test ) ) {
      std::string message( msg );
      edump((message));
      EOS_THROW( eosio_assert_message_exception, "assertion failure with message: ${s}", ("s",message) );
   }
}

void  eosio_assert_message( uint32_t test, const char* msg, uint32_t msg_len ) {
   if( BOOST_UNLIKELY( !test ) ) {
      std::string message( msg, msg_len );
      edump((message));
      EOS_THROW( eosio_assert_message_exception, "assertion failure with message: ${s}", ("s",message) );
   }
}

void  eosio_assert_code( uint32_t test, uint64_t error_code ) {
   if( BOOST_UNLIKELY( !test ) ) {
      edump((error_code));
      EOS_THROW( eosio_assert_code_exception,
                 "assertion failure with error code: ${error_code}", ("error_code", error_code) );
   }
}

void  eosio_exit( int32_t code ) {
   throw wasm_exit{code};
}

uint64_t  current_time() {
   FC_ASSERT(false, "not implemented!");
   return 0;
}

uint32_t  now() {
   FC_ASSERT(false, "not implemented!");
   return 0;
}

#warning FIXME: checktime
void checktime() {
//   FC_ASSERT(false, "not implemented!");
   return;
}

#warning FIXME: check_context_free
void check_context_free(bool context_free) {
//   FC_ASSERT(false, "not implemented!");
}

bool contracts_console() {
   return true;
}

}

