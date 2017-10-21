from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "eoslib_.hpp":
    ctypedef unsigned long long uint64_t
    ctypedef int int32_t
    ctypedef unsigned int uint32_t
    cdef cppclass Name:
        Name()
        Name(uint64_t)
    uint64_t string_to_uint64_( string str );
    string uint64_to_string_( uint64_t n );
    
    void requireAuth_( uint64_t account );
    int readMessage_( string& buffer );
    void requireScope_(uint64_t account);
    void requireNotice_( uint64_t account );
    uint64_t currentCode_();
    uint32_t now_();

    int32_t store_(Name scope, Name code, Name table, void *keys, int key_type, char* value, uint32_t valuelen);
    int32_t update_( Name scope, Name code, Name table, void *keys, int key_type,char* value, uint32_t valuelen );
    int32_t remove_( Name scope, Name code, Name table, void *keys, int key_type, char* value, uint32_t valuelen );
    int32_t load_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen );
    int32_t front_( Name scope, Name code, Name table, void *keys,int key_type,int scope_index, char* value, uint32_t valuelen )
    int32_t back_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )
    int32_t next_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )
    int32_t previous_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )
    int32_t lower_bound_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )
    int32_t upper_bound_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )

    void unpack_(string& raw,string& out)
    void pack_(string& raw,string& out)

def pack(bytes bs):
    cdef string out
    pack_(bs,out)
    return out

def unpack(bytes bs):
    cdef string out
    unpack_(bs,out)
    return out

def now():
    return now_()

cdef uint64_t toname(name):
    if isinstance(name,int):
        return name
    if isinstance(name,str):
        name = bytes(name,'utf8')
    return string_to_uint64_(name)

def s2n(name):
    return toname(name)

def N(name):
    return s2n(name)

def n2s(uint64_t n):
    return uint64_to_string_(n)

def requireAuth(name):
    cdef uint64_t name_
    name_ = toname(name)
    requireAuth_(name_)

def readMessage():
    cdef string buffer
    cdef int ret
    ret = readMessage_(buffer)
    return buffer

def requireScope(account):
    cdef uint64_t account_
    account_ = toname(account)
    requireScope_(account_)

def requireNotice(account):
    cdef uint64_t account_
    account_ = toname(account)
    requireNotice_(account_)

def currentCode():
    return currentCode_()

def store(scope, code, table, bytes keys, int key_type, bytes value):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    return store_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, value, len(value))

def store_u64(scope, code, table, uint64_t key, uint64_t value):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    return store_(Name(scope_), Name(code_), Name(table_), <void*>&key, 0, <char*>&value, sizeof(value))

def update(scope,  code,  table, bytes keys, int key_type, bytes value):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    return update_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, value, len(value))

def remove(scope, code, table, bytes keys, int key_type, bytes value):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    return remove_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, value, len(value))

def load(scope, code, table, bytes keys, int key_type, int scope_index,bytes values):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    cdef char *values_ = values
    
    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    return load_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, scope_index,values_, len(values))

def load_u64(scope, code, table, uint64_t key):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef uint64_t value
    cdef int value_length

    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    value = 0
    value_length = load_(Name(scope_), Name(code_), Name(table_), <void*>&key, 0, 0,<char*>&value, sizeof(value))
    return value
'''
int32_t front_( Name scope, Name code, Name table, void *keys,int key_type,int scope_index, char* value, uint32_t valuelen )
int32_t back_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )
int32_t next_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )
int32_t previous_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )
int32_t lower_bound_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )
int32_t upper_bound_( Name scope, Name code, Name table, void *keys,int key_type, int scope_index, char* value, uint32_t valuelen )
'''
cdef int get_key_size(int key_type):
    if key_type == 0:
        return 64/8
    elif key_type == 1:
        return 128*2/8
    elif key_type == 2:
        return 64*3/8
    return 0

def front(scope, code, table, bytes keys,int key_type,int scope_index,bytes values):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    cdef char *values_ = values
    cdef int value_length
    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    if get_key_size(key_type) != len(keys):
        return None
    if key_type > 2:
        return None
    return front_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, scope_index,values_, len(values))

def back(scope, code, table, bytes keys,int key_type,int scope_index,bytes values):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    cdef char *values_ = values
    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    if get_key_size(key_type) != len(keys):
        return None
    if key_type > 2:
        return None
    return back_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, scope_index,values_, len(values))

def next(scope, code, table, bytes keys,int key_type, int scope_index,bytes values):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    cdef char *values_ = values

    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    return next_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, scope_index,values_, len(values))

def previous(scope, code, table, bytes keys,int key_type, int scope_index,bytes values):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    cdef char *values_ = values

    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    return previous_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, scope_index,values_, len(values))

def lower_bound(scope, code, table, bytes keys,int key_type, int scope_index,bytes values):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    cdef char *values_ = values

    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    return lower_bound_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, scope_index,values_, len(values))

def upper_bound(scope, code, table, bytes keys,int key_type, int scope_index,bytes values):
    cdef uint64_t scope_
    cdef uint64_t code_
    cdef uint64_t table_
    cdef char *keys_ = keys
    cdef char *values_ = values

    scope_ = toname(scope)
    code_ = toname(code)
    table_ = toname(table)
    return upper_bound_(Name(scope_), Name(code_), Name(table_), <void*>keys_, key_type, scope_index,values_, len(values))

