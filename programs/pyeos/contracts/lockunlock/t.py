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

def init(func):
    def func_wrapper(wasm=False, *args, **kwargs):
        if wasm:
            prepare('lockunlock', 'lockunlock.wast', 'lockunlock.abi', __file__)
            return func(*args, **kwargs)
        else:
            prepare('lockunlock', 'lockunlock.py', 'lockunlock.abi', __file__)
            return func(*args, **kwargs)
    return func_wrapper

@init
def setabi():
    _src_dir = os.path.dirname(__file__)

    setabi = eosapi.pack_setabi(os.path.join(_src_dir, 'lockunlock.abi'), eosapi.N('lockunlock'))
    action = [N('eosio'), N('setabi'), [[N('lockunlock'), N('active')]], setabi]

    cost_time = eosapi.push_transactions2([[action]], sign)
    eosapi.produce_block()

@init
def setcode(sign=True):
    _src_dir = os.path.dirname(__file__)
    code = struct.pack('QBB', N('lockunlock'), 1, 0)
    with open(os.path.join(_src_dir, 'lockunlock.py'), 'rb') as f:
        code += eosapi.pack_bytes(b'\x00'+f.read())
    act = [N('eosio'), N('setcode'), [[N('lockunlock'), N('active')]], code]

    cost_time = eosapi.push_transactions2([[act]], sign)
    eosapi.produce_block()

def lock(msg='hello,world', wasm=False):
    with producer:
        r = eosapi.push_message('eosio', 'lockcode', {'account':'lockunlock'}, {'lockunlock':'active'})
        assert r

def unlock(msg='hello,world', wasm=False):
    with producer:
        r = eosapi.push_message('eosio', 'unlockcode', {'unlocker':'eosio', 'account':'lockunlock'}, {'eosio':'active'})
        assert r
