import json
from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.map cimport map
from eostypes_ cimport *
#import eostypes_
from typing import Dict, Tuple, List

class JsonStruct:
    def __init__(self, **entries):
        self.__dict__.update(entries)
    def __str__(self):
        return str(self.__dict__)
    def __repr__(self):
        return str(self.__dict__)

cdef extern from "eosapi.h":
    ctypedef int bool
    object get_info_ ()
    object get_block_(char *num_or_id)
    object get_account_(char *name)
    object get_accounts_(char *public_key)
    string create_account_(string creator, string newaccount, string owner, string active, int sign)
    object get_controlled_accounts_(char *account_name);
    void create_key_(string& pub,string& priv)

    string get_transaction_(string id);
    string get_transactions_(string account_name,int skip_seq,int num_seq);

    string push_transaction( SignedTransaction& trx, bool sign )
    int get_transaction_(char *id,char* result,int length)
    
    string transfer_(string& sender,string&recipient,int amount,string memo,bool sign);
    string push_message_(string& contract,string& action,string& args,vector[string] scopes,map[string,string]& permissions,bool sign);
    int set_contract_(string& account,string& wastPath,string& abiPath,bool sign,string& result);
    int get_code_(string& name,string& wast,string& abi,string& code_hash);
    int get_table_(string& scope,string& code,string& table,string& result);

    int setcode_(char *account_,char *wast_file,char *abi_file,char *ts_buffer,int length) 
    int exec_func_(char *code_,char *action_,char *json_,char *scope,char *authorization,char *ts_result,int length)

cdef extern object py_new_bool(int b):
    if b:
        return True
    return False

cdef extern object py_new_none():
    return None

cdef extern object py_new_string(string& s):
    return s

cdef extern object py_new_int(int n):
    return n

cdef extern object py_new_int64(long long n):
    return n

cdef extern object array_create():
    return []

cdef extern void array_append(object arr,object v):
    arr.append(v)

cdef extern void array_append_string(object arr,string& s):
    arr.append(s)

cdef extern void array_append_int(object arr,int n):
    arr.append(n)

cdef extern void array_append_double(object arr,double n):
    arr.append(n)

cdef extern void array_append_uint64(object arr,unsigned long long n):
    arr.append(n)

cdef extern object dict_create():
    return {}

cdef extern void dict_add(object d,object key,object value):
    d[key] = value


def toobject(bstr):
    bstr = json.loads(bstr.decode('utf8'))
    return JsonStruct(**bstr)

def tobytes(ustr:str):
    if type(ustr) == str:
        ustr = bytes(ustr,'utf8')
    return ustr

def get_info()->str:
    return get_info_()

def get_block(id:str)->str:
    if type(id) == int:
        id = bytes(id)
    if type(id) == str:
        id = bytes(id,'utf8')
    return get_block_(id)

def get_account(name:str)->str:
    if type(name) == str:
        name = bytes(name,'utf8')
    return get_account_(name)

def get_accounts(public_key:str)->List[str]:
    if type(public_key) == str:
        public_key = bytes(public_key,'utf8')
    return get_accounts_(public_key)

def get_controlled_accounts(account_name:str)->List[str]:
    if type(account_name) == str:
        account_name = bytes(account_name,'utf8')

    return get_controlled_accounts_(account_name);

def create_account(creator:str,newaccount:str,owner_key:str,active_key:str,sign)->str:
    if type(creator) == str:
        creator = bytes(creator,'utf8')
    
    if type(newaccount) == str:
        newaccount = bytes(newaccount,'utf8')
    
    if type(owner_key) == str:
        owner_key = bytes(owner_key,'utf8')
    
    if type(active_key) == str:
        active_key = bytes(active_key,'utf8')

    if sign:
        return create_account_(creator,newaccount,owner_key,active_key, 1)
    else:
        return create_account_(creator,newaccount,owner_key,active_key, 0)

def create_key()->Tuple[bytes]:
    cdef string pub
    cdef string priv
    create_key_(pub,priv)
    return(pub,priv)

def get_transaction(id:str)->str:
    if type(id) == int:
        id = str(id)
    if type(id) == str:
        id = bytes(id,'utf8')
    return get_transaction_(id)

def get_transactions(account_name:str,skip_seq:int,num_seq:int)->str:
    if type(account_name) == str:
        account_name = bytes(account_name,'utf8')
    return get_transactions_(account_name,skip_seq,num_seq)

def transfer(sender:str,recipient:str,int amount,memo:str,sign)->str:
    if type(sender) == str:
        sender = bytes(sender,"utf8");
    if type(recipient) == str:
        recipient = bytes(recipient,"utf8");
    if type(memo) == str:
        memo = bytes(memo,"utf8");

    if sign:
        return transfer_(sender,recipient,amount,memo,1)
    else:
        return transfer_(sender,recipient,amount,memo,0)

def push_message(contract:str,action:str,args:str,scopes:List[str],permissions:Dict,sign):
    if type(contract) == str:
        contract = bytes(contract,"utf8");
    if type(action) == str:
        action = bytes(contract,"utf8");
    if type(args) == str:
        args = bytes(args,"utf8")

    cdef vector[string] scopes_;
    cdef map[string,string] permissions_;

    for scope in scopes:
        scopes_.push_back(scope)
    for per in permissions:
        permissions_[per] = permissions[per]
    if sign:
        return push_message(contract,action,args,scopes,permissions,1)
    else:
        return push_message(contract,action,args,scopes,permissions,0)

def set_contract(account:str,wast_file:str,abi_file:str,sign)->str:
    cdef string result
    account = tobytes(account)
    wast_file = tobytes(wast_file)
    abi_file = tobytes(abi_file)
    if sign:
        sign = 1
    else:
        sign = 0

    if 0 == set_contract_(account,wast_file,abi_file,sign,result):
        return result
    return None

def get_contract(name:str):
    cdef string wast
    cdef string abi
    cdef string code_hash
    name = tobytes(name)
    if 0 == get_code_(name,wast,abi,code_hash):
        return [wast,abi,code_hash]
    return []

def get_table(scope,code,table):
    cdef string result
    scope = tobytes(scope)
    code = tobytes(code)
    table = tobytes(table)

    if 0 == get_table_(scope,code,table,result):
        return result
    return None

def exec_func(code_:str,action_:str,json_:str,scope_:str,authorization_:str)->str:
    pass




'''
cdef class PyMessage:
    cdef Message* msg      # hold a C++ instance which we're wrapping
    def __cinit__(self,code,funcName,authorization,data):
#        cdef AccountName code_
#        cdef FuncName funcName_
        cdef Vector[AccountPermission] authorization_
        cdef Bytes data_
        for a in authorization:
            account = bytes(a[0],'utf8')
            permission = bytes(a[1],'utf8')
            authorization_.push_back(AccountPermission(Name(account),Name(permission)))
        for d in bytearray(data,'utf8'):
            data_.push_back(<char>d)
        self.msg = new Message(AccountName(bytes(code,'utf8')),FuncName(bytes(funcName,'utf8')),authorization_,data_)
    def __dealloc__(self):
        del self.msg
'''

