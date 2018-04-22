import os
import time
import wallet
import eosapi
import initeos
import database_api
from common import init_, producer

print('please make sure you are running the following command before test')
print('./pyeos/pyeos --manual-gen-block --debug -i')

def init(func):
    def func_wrapper(*args):
        init_('backyard', 'backyard.py', 'backyard.abi', __file__)
        return func(*args)
    return func_wrapper

@init
def test(name=None):
    with producer:
        if not name:
            name = 'mike'
        r = eosapi.push_message('backyard','sayhello',name,{'backyard':'active'},rawargs=True)
        assert r

@init
def deploy():
    src_dir = os.path.dirname(os.path.abspath(__file__))
    libs = ('asset.py', 'cache.py', 'storage.py', 'garden.py')

    code = eosapi.N('backyard')
    for file_name in libs:
        src_code = open(os.path.join(src_dir, file_name), 'rb').read()
        src_id = eosapi.hash64(file_name, 0)
        itr = database_api.find_i64(code, code, code, src_id)
        if itr >= 0:
            old_src = database_api.get_i64(itr)
            if old_src[1:] == src_code:
                continue
        mod_name = file_name
        msg = int.to_bytes(len(mod_name), 1, 'little')
        msg += mod_name.encode('utf8')
        msg += int.to_bytes(0, 1, 'little') # source code
        msg += src_code
    
        print('++++++++++++++++deply:', file_name)
        r = eosapi.push_message('backyard','deploy',msg,{'backyard':'active'},rawargs=True)
        assert r

    producer.produce_block()

@init
def deploy_mpy():
    src_dir = os.path.dirname(os.path.abspath(__file__))
    libs = ('asset.py', 'cache.py', 'storage.py', 'garden.py')
    code = eosapi.N('backyard')
    for file_name in libs:
        src_code = eosapi.mp_compile(os.path.join(src_dir, file_name))
        src_id = eosapi.hash64(file_name, 0)
        itr = database_api.find_i64(code, code, code, src_id)
        if itr >= 0:
            old_src = database_api.get_i64(itr)
            if old_src[1:] == src_code:
                continue

        file_name = file_name.replace('.py', '.mpy')
        mod_name = file_name
        msg = int.to_bytes(len(mod_name), 1, 'little')
        msg += mod_name.encode('utf8')
        msg += int.to_bytes(1, 1, 'little') # compiled code
        msg += src_code
    
        print('++++++++++++++++deply:', file_name)
        r = eosapi.push_message('backyard','deploy',msg,{'backyard':'active'},rawargs=True)
        assert r

    producer.produce_block()



