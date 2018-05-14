import os
import time
import struct

import wallet
import eosapi
import initeos
import initeos
from eosapi import N
from tools import cpp2wast

from common import prepare, producer

print('please make sure you are running the following command before test')
print('./pyeos/pyeos --manual-gen-block --debug -i')

def init(wasm=False):
    if not eosapi.get_account('test2').permissions:
        r = eosapi.create_account('eosio', 'test2', initeos.key1, initeos.key2)
        assert r
        eosapi.produce_block()
    def init_decorator(func):
        def func_wrapper(*args, **kwargs):
            if wasm:
                prepare('credit', 'credit.wast', 'credit.abi', __file__)
            else:
                prepare('credit', 'credit.py', 'credit.abi', __file__)
            return func(*args, **kwargs)
        return func_wrapper
    return init_decorator

@init()
def addtowl(account):
    with producer:
        r = eosapi.push_message('credit', 'addtowl', struct.pack('Q', N(account)), {'credit':'active'})
        assert r

@init()
def rmfromwl(account):
    with producer:
        r = eosapi.push_message('credit', 'rmfromwl', struct.pack('Q', N(account)), {'credit':'active'})
        assert r

@init()
def addtobl(account):
    with producer:
        r = eosapi.push_message('credit', 'addtobl', struct.pack('Q', N(account)), {'credit':'active'})
        assert r

@init()
def rmfrombl(account):
    with producer:
        r = eosapi.push_message('credit', 'rmfrombl', struct.pack('Q', N(account)), {'credit':'active'})
        assert r

def testall():
    addtowl('test')
    rmfromwl('test')
    addtobl('test')
    rmfrombl('test')
    
    
    