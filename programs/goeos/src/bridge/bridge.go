package bridge

import "os"
import "unsafe"

// #cgo CFLAGS: -I/Users/newworld/dev/pyeos/programs/goeos/include
// #cgo LDFLAGS: -L/Users/newworld/dev/pyeos/build/programs/goeos /Users/newworld/dev/pyeos/build/programs/goeos/libgoeos.dylib
// #include <goeos.h>
// #include <stdlib.h>
import "C"

func GoeosMain() {
    C.arg_size(C.int(len(os.Args)))
    for _, v := range os.Args {
        arg := C.CString(v)
        C.arg_add(arg)
        C.free(unsafe.Pointer(arg))
    }

    C.goeos_main()
}

func ReadAction() (r []byte, err error) {
    var buffer [256]byte
    ret := C.read_action((*C.char)(unsafe.Pointer(&buffer[0])), C.size_t(len(buffer)))
    return buffer[:ret], nil
}

/*
int read_action_(char* memory, size_t size);
int read_action(char* memory, size_t size);

int db_store_i64( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const char* buffer, size_t buffer_size );
void db_update_i64( int itr, uint64_t payer, const char* buffer, size_t buffer_size );
void db_remove_i64( int itr );
int db_get_i64( int itr, char* buffer, size_t buffer_size );
int db_next_i64( int itr, uint64_t* primary );
int db_previous_i64( int itr, uint64_t* primary );
int db_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id );
int db_lowerbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id );
int db_upperbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id );
int db_end_i64( uint64_t code, uint64_t scope, uint64_t table );

*/
