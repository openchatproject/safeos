import os
import time
import struct

import wallet
import eosapi
import initeos

from eosapi import N
from tools import cpp2wast

from common import prepare, producer

print('please make sure you are running the following command before test')
print('./pyeos/pyeos --manual-gen-block --debug -i')

def init(wasm=True):
    def init_decorator(func):
        def func_wrapper(wasm=True, *args, **kwargs):
            if wasm:
                prepare('twitbot', 'twitbot.wast', 'twitbot.abi', __file__)
                return func(*args, **kwargs)
            else:
                prepare('twitbot', 'twitbot.py', 'twitbot.abi', __file__)
                return func(*args, **kwargs)
        return func_wrapper
    return init_decorator

@init()
def test(msg='hello,world'):
    '''
    with producer:
        r = eosapi.push_message('twitbot', 'sayhello', msg, {'twitbot':'active'})
        assert r
    '''
    msg = {"from":"eosio", "to":"hello", "quantity":"100.0000 EOS", "memo":"m"}
    r = eosapi.push_message('eosio.token', 'transfer', msg, {'eosio':'active'})

    with producer:
        msg = {"from":"hello", "to":"twitbot", "quantity":"1.0000 EOS", "memo":"m"}
        r = eosapi.push_message('eosio.token', 'transfer', msg, {'hello':'active'})



