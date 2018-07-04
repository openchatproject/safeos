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

def gen_names(n):
    prefix = 'aaaa'
    chs = '12345abcdefghijklmnopqrstuvwxyz'
    names = []
    for i in range(n):
        name = prefix + chs[int(i/31/31)%31] + chs[int((i/31)%31)] + chs[i%31]
        names.append(name)
    return names

def _create_account(account):
    actions = []
    newaccount = {'creator': 'eosio',
     'name': account,
     'owner': {'threshold': 1,
               'keys': [{'key': initeos.key1,
                         'weight': 1}],
               'accounts': [],
               'waits': []},
     'active': {'threshold': 1,
                'keys': [{'key': initeos.key2,
                          'weight': 1}],
                'accounts': [],
                'waits': []}}
    if not eosapi.get_account(account):
        _newaccount = eosapi.pack_args('eosio', 'newaccount', newaccount)
        act = ['eosio', 'newaccount', {'eosio':'active'}, _newaccount]
        actions.append(act)
    '''
    args = {'payer':'eosio', 'receiver':account, 'quant':"1.0000 EOS"}
    args = eosapi.pack_args('eosio', 'buyram', args)
    act = ['eosio', 'buyram', {'eosio':'active'}, args]
    actions.append(act)
    '''

    args = {'payer':'eosio', 'receiver':account, 'bytes':128*1024*1024}
    args = eosapi.pack_args('eosio', 'buyrambytes', args)
    act = ['eosio', 'buyrambytes', {'eosio':'active'}, args]
    actions.append(act)

    args = {'from': 'eosio',
     'receiver': account,
     'stake_net_quantity': '1.0050 EOS',
     'stake_cpu_quantity': '1.0050 EOS',
     'transfer': 1}
    args = eosapi.pack_args('eosio', 'delegatebw', args)
    act = ['eosio', 'delegatebw', {'eosio':'active'}, args]
    actions.append(act)
    rr, cost = eosapi.push_actions(actions)
    assert_ret(rr)


def assert_ret(rr):
    for r in rr:
        if r['except']:
            print(r['except'])
        assert not r['except']

ACCOUNT_COUNT = 20 #100

def t():
    contracts_path = os.path.join(os.getcwd(), '..', 'contracts')
    sys.path.append(os.getcwd())
    account = 'eosio'
    path = 'eosio.system'
    _path = os.path.join(contracts_path, path, path)
    print('+++++++++code update', account)
    wast = _path + '.wast'
    abi = _path + '.abi'
    with producer:
        r = eosapi.set_contract(account, wast, abi, 0)

def t1():
    systemAccounts = [
        'eosio.bpay',
        'eosio.msig',
        'eosio.names',
        'eosio.ram',
        'eosio.ramfee',
        'eosio.saving',
        'eosio.stake',
        'eosio.token',
        'eosio.vpay',
    ]
    
    for account in systemAccounts:
        if not eosapi.get_account(account):
            _create_account(account)

def t2():
    contracts_path = os.path.join(os.getcwd(), '..', 'contracts')
    sys.path.append(os.getcwd())
    account = 'eosio'
    path = 'eosio.system'
    accounts = gen_names(ACCOUNT_COUNT)


    _path = os.path.join(contracts_path, path, path)
    wast = _path + '.wast'
    abi_file = _path + '.abi'

    with open(wast, 'rb') as f:
        wasm = eosapi.wast2wasm(f.read())
    code_hash = eosapi.sha256(wasm)
    with open(abi_file, 'rb') as f:
        abi = f.read()

    for account in accounts:
        _create_account(account)
        continue
        if not eosapi.get_account(account):
            _create_account(account)

    for account in accounts:
        print('+++++++++code update', account)
        actions = []
        _setcode = eosapi.pack_args('eosio', 'setcode', {'account':account,'vmtype':0, 'vmversion':0, 'code':wasm.hex()})
#        _setabi = eosapi.pack_args('eosio', 'setabi', {'account':account, 'abi':abi.hex()})
        _setabi = pack_setabi(abi_file, account)

        old_hash = eosapi.get_code_hash(account)
        print(old_hash, code_hash)
        if code_hash != old_hash:
            setcode = ['eosio', 'setcode', {account:'active'}, _setcode]
            actions.append(setcode)

        setabi = ['eosio', 'setabi', {account:'active'}, _setabi]
        actions.append(setabi)
        rr, cost = eosapi.push_actions(actions)
        assert_ret(rr)

def t3():
    accounts = gen_names(ACCOUNT_COUNT)
    actions = []
    for account in accounts:
        print('++++++boost account', account)
        msg = eosapi.pack_args('eosio', 'boost', {'account':account})
        act = ['eosio', 'boost', {'eosio':'active'}, msg]
        actions.append(act)
    rr, cost = eosapi.push_actions(actions)
    assert_ret(rr)

def t4():
    accounts = gen_names(ACCOUNT_COUNT)
    actions = []
    for account in accounts:
        print('++++++boost account', account)
        msg = eosapi.pack_args('eosio', 'cancelboost', {'account':account})
        act = ['eosio', 'cancelboost', {'eosio':'active'}, msg]
        actions.append(act)
    rr, cost = eosapi.push_actions(actions)
    assert_ret(rr)


def ca(account='aabb111'):
    actions = []
    newaccount = {'creator': 'eosio',
     'name': account,
     'owner': {'threshold': 1,
               'keys': [{'key': initeos.key1,
                         'weight': 1}],
               'accounts': [],
               'waits': []},
     'active': {'threshold': 1,
                'keys': [{'key': initeos.key2,
                          'weight': 1}],
                'accounts': [],
                'waits': []}}

    _newaccount = eosapi.pack_args('eosio', 'newaccount', newaccount)
    act = ['eosio', 'newaccount', {'eosio':'active'}, _newaccount]
    actions.append(act)

    args = {'payer':'eosio', 'receiver':account, 'bytes':819200000}
    args = eosapi.pack_args('eosio', 'buyrambytes', args)
    act = ['eosio', 'buyrambytes', {'eosio':'active'}, args]
    actions.append(act)

    args = {'from': 'eosio',
     'receiver': account,
     'stake_net_quantity': '3000.0050 EOS',
     'stake_cpu_quantity': '3000.0050 EOS',
     'transfer': 1}
    args = eosapi.pack_args('eosio', 'delegatebw', args)
    act = ['eosio', 'delegatebw', {'eosio':'active'}, args]
    actions.append(act)
    rr, cost = eosapi.push_actions(actions)
    assert_ret(rr)

def buyram():
    accounts = gen_names(ACCOUNT_COUNT)
    actions = []
    for account in accounts:
        print('buy ram', account)
        args = {'payer':'eosio', 'receiver':account, 'quant':"1.0000 EOS"}
        args = eosapi.pack_args('eosio', 'buyram', args)
        act = ['eosio', 'buyram', {'eosio':'active'}, args]
        actions.append(act)
    rr, cost = eosapi.push_actions(actions)
    assert_ret(rr)

def buyrambytes():
    accounts = gen_names(ACCOUNT_COUNT)
    actions = []
    for account in accounts:
        print('buy ram in bytes', account)
        args = {'payer':'eosio', 'receiver':account, 'bytes':1000000}
        args = eosapi.pack_args('eosio', 'buyrambytes', args)
        act = ['eosio', 'buyrambytes', {'eosio':'active'}, args]
        actions.append(act)
    rr, cost = eosapi.push_actions(actions)
    assert_ret(rr)

#eosapi.push_action('aaaa11t', 'boost', {'account':'aaaa11t'}, {'aaaa11t':'active'})


