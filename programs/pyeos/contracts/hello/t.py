import os
import re
import sys
import imp
import time
import struct

import debug
import wallet
import eosapi
import initeos
import traceback
from eosapi import N, mp_compile, pack_bytes, pack_setabi, push_transactions
from common import prepare, producer

def init(func):
    def func_wrapper(*args, **kwargs):
        prepare('hello', 'hello.py', 'hello.abi', __file__)
        func(*args, **kwargs)
    return func_wrapper

@init
def test(name='mike'):
    debug.mp_set_max_execution_time(1000_000)

    r = eosapi.push_action('hello','sayhello', name, {'hello':'active'})
    assert r and not r['except']
    print('cost time:', r['cost'])
#    print(eosapi.JsonStruct(r[0]))
#    eosapi.produce_block()

@init
def play():
    r = eosapi.push_action('hello', 'play', '', {'hello':'active'})
    assert r

@init
def test2(count=100):
    actions = []
    for i in range(count):
        action = ['hello', 'sayhello', {'hello':'active'}, str(i)]
        actions.append(action)

    ret, cost = eosapi.push_actions(actions, True)
    assert ret
    print('total cost time:%.3f s, cost per action: %.3f ms, actions per second: %.3f'%(cost/1e6, cost/count/1000, 1*1e6/(cost/count)))

@init
def test3(count=100):
    actions = []
    for i in range(count):
        act = [N('hello'), N('sayhello'), [[N('hello'), N('active')]], b'hello,world%d'%(i,)]
        actions.append([act])
    r, cost_time = eosapi.push_transactions(actions, True)
    print(1e6/(cost_time/count), cost_time)

@init
def deploy_mpy():
    src_dir = os.path.dirname(os.path.abspath(__file__))
    file_name = 'hello.py'
    
    src_code = eosapi.mp_compile(os.path.join(src_dir, file_name))
    file_name = file_name.replace('.py', '.mpy')
    mod_name = file_name
    msg = int.to_bytes(len(mod_name), 1, 'little')
    msg += mod_name.encode('utf8')
    msg += int.to_bytes(1, 1, 'little') # compiled code
    msg += src_code

    print('++++++++++++++++deply:', file_name)
    r = eosapi.push_action('kitties','deploy',msg,{'kitties':'active'})
    assert r
