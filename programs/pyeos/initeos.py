from imp import reload
import imp
import os
import signal
import socket
import sys

from code import InteractiveConsole
import debug
from eosapi import *
import eosapi
import net
import pickle
import re
import rodb
from tools import sketch
import traceback
import wallet


tests = os.path.join(os.path.dirname(__file__), 'tests')
sys.path.insert(0, tests)

libs = os.path.join(os.path.dirname(__file__), 'libs')
sys.path.insert(0, libs)

from console import PyEosConsole



#debug.add_trusted_account('hello')
#debug.add_trusted_account('counter')


config = '''
# Track only transactions whose scopes involve the listed accounts. Default is to track all transactions.
# filter_on_accounts = 

# Limits the maximum time (in milliseconds) processing a single get_transactions call.
#get-transactions-time-limit = 3

# File to read Genesis State from
#genesis-json = genesis.json


# Minimum size MB of database shared memory file
#shared-file-size = 1024

 # Enable production on a stale chain, since a single-node test chain is pretty much always stale
enable-stale-production = true
# Enable block production with the testnet producers
#producer-name = eosio

private-key = ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]


plugin = eosio::producer_plugin
#plugin = eosio::wallet_api_plugin
#plugin = eosio::chain_api_plugin
#plugin = eosio::history_api_plugin

'''



genesis = '''
{
  "initial_timestamp": "2018-06-08T08:08:08.888",
  "initial_key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
  "initial_configuration": {
    "max_block_net_usage": 1048576,
    "target_block_net_usage_pct": 1000,
    "max_transaction_net_usage": 524288,
    "base_per_transaction_net_usage": 12,
    "net_usage_leeway": 500,
    "context_free_discount_net_usage_num": 20,
    "context_free_discount_net_usage_den": 100,
    "max_block_cpu_usage": 350000,
    "target_block_cpu_usage_pct": 1000,
    "max_transaction_cpu_usage": 150000,
    "min_transaction_cpu_usage": 100,
    "max_transaction_lifetime": 3600,
    "deferred_trx_expiration_window": 600,
    "max_transaction_delay": 3888000,
    "max_inline_action_size": 4096,
    "max_inline_action_depth": 4,
    "max_authority_depth": 6
  }
}
'''

genesis = '''
{
  "initial_timestamp": "2018-06-08T08:08:08.888",
  "initial_key": "EOS7EarnUhcyYqmdnPon8rm7mBCTnBoot6o7fE2WzjvEX2TdggbL3",
  "initial_configuration": {
    "max_block_net_usage": 1048576,
    "target_block_net_usage_pct": 1000,
    "max_transaction_net_usage": 524288,
    "base_per_transaction_net_usage": 12,
    "net_usage_leeway": 500,
    "context_free_discount_net_usage_num": 20,
    "context_free_discount_net_usage_den": 100,
    "max_block_cpu_usage": 200000,
    "target_block_cpu_usage_pct": 1000,
    "max_transaction_cpu_usage": 150000,
    "min_transaction_cpu_usage": 100,
    "max_transaction_lifetime": 3600,
    "deferred_trx_expiration_window": 600,
    "max_transaction_delay": 3888000,
    "max_inline_action_size": 4096,
    "max_inline_action_depth": 4,
    "max_authority_depth": 6
  }
}
'''

if not hasattr(sys, 'argv'):
    sys.argv = ['']

key1 = 'EOS61MgZLN7Frbc2J7giU7JdYjy2TqnfWFjZuLXvpHJoKzWAj7Nst'
key2 = 'EOS5JuNfuZPATy8oPz9KMZV2asKf9m8fb2bSzftvhW55FKQFakzFL'
psw = None

def init_wallet():
    global psw
    
    data_dir = eosapi.get_opt('data-dir')
    if not data_dir:
        data_dir = 'data-dir'

    if not os.path.exists(data_dir):
        os.mkdir(data_dir)
    psw_file = os.path.join(data_dir, 'data.pkl')
    wallet.set_dir(data_dir)
    if not os.path.exists(os.path.join(data_dir, 'mywallet.wallet')):
        psw = wallet.create('mywallet')
        print('wallet password:', psw)
        with open(psw_file, 'wb') as f:
            print('test wallet password save to', psw_file)
            pickle.dump(psw, f)

    wallet.open('mywallet')
    if not psw:
        with open(psw_file, 'rb') as f:
            psw = pickle.load(f)
    wallet.unlock('mywallet',psw)
    wallet.set_timeout(60*60*24)

    priv_keys = [
                    '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
                    '5JEcwbckBCdmji5j8ZoMHLEUS8TqQiqBG1DRx1X9DN124GUok9s',
                    '5JbDP55GXN7MLcNYKCnJtfKi9aD2HvHAdY7g8m67zFTAFkY1uBB',
                    '5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p'
                ]

    keys = wallet.list_keys('mywallet', psw)
    exist_priv_keys = keys.values()
    for priv_key in priv_keys:
        if not priv_key in exist_priv_keys:
            print('import key:', priv_key)
            wallet.import_key('mywallet', priv_key)

def preinit():
    print('+++++++++++Initialize testnet.')
    config_dir = eosapi.get_opt('config-dir')
    print(config_dir)
    if not config_dir:
        config_dir = 'config-dir'

    if os.path.exists(os.path.join(config_dir,'.preinit')):
        return

    if not os.path.exists(config_dir):
        print('Creating config-dir')
        os.mkdir(config_dir)

    with open(os.path.join(config_dir, '.preinit'), 'w') as f:
        pass

    config_file = os.path.join(config_dir, 'config.ini')
    genesis_file = os.path.join(config_dir, 'genesis.json')


    print('Initialize ', config_file)
    with open(config_file, 'w') as f:
        f.write(config)

    print('Initialize ', genesis_file)
    with open(genesis_file, 'w') as f:
        f.write(genesis)

    print('Initialize finished, please restart pyeos')
    sys.exit(0)

try:
    from contracts.eosio_code import t as ct

    from python.apitest import t as at
    from python.cryptokitties import t as kt
    from python.backyard import t as bt
    from python.rpctest import t as rt
    from python.simpleauction import t as st
    from python.testcase import t as pt

    from wasm.lab import t as wt

    from python.vote import t as vt2
    from native.native import t as nt
    from python.vmstore import t as vt
    from python.inspector import t as it
    from python.deploytest import t as pd

    from eosio_token import t as tt
    from hello import t as ht
    from counter import t as ct1
    from credit import t as ct2

    from julia.hello import t as juh

    from lua.hello import t as lh
    from lua.testcase import t as lt
    from lua.eosio_token import t as le

    from java.hello import t as jh

    from evm.evm import t as ee
    from evm.testcase import t as et

#    from biosboot import t as bb
    import d
except Exception as e:
    traceback.print_exc()

def assert_ret(r):
    if r['except']:
        print(r['except'])
    assert not r['except']

def create_system_accounts():
    systemAccounts = [
        'eosio.token',
        'eosio.bpay',
        'eosio.msig',
        'eosio.names',
        'eosio.ram',
        'eosio.ramfee',
        'eosio.jit',
        'eosio.jitfee',
        'eosio.saving',
        'eosio.stake',
        'eosio.token',
        'eosio.vpay',
    ]
    newaccount = {'creator': 'eosio',
     'name': '',
     'owner': {'threshold': 1,
               'keys': [{'key': key1,
                         'weight': 1}],
               'accounts': [],
               'waits': []},
     'active': {'threshold': 1,
                'keys': [{'key': key2,
                          'weight': 1}],
                'accounts': [],
                'waits': []}}

    for account in systemAccounts:
        if not eosapi.get_account(account):
            actions = []
            print('+++++++++create account', account)
            newaccount['name'] = account
            _newaccount = eosapi.pack_args('eosio', 'newaccount', newaccount)
            act = ['eosio', 'newaccount', _newaccount, {'eosio':'active'}]
            actions.append(act)
            rr, cost = eosapi.push_actions(actions)
            assert_ret(rr)

def update_eosio():
    '''
    account = 'eosio'
    actions = []
    args = {'payer':'eosio', 'receiver':account, 'quant':"1000.0000 EOS"}
    args = eosapi.pack_args('eosio', 'buyram', args)
    act = ['eosio', 'buyram', {'eosio':'active'}, args]
    actions.append(act)
    
    args = {'from': 'eosio',
     'receiver': account,
     'stake_net_quantity': '1000.0050 EOS',
     'stake_cpu_quantity': '1000.0050 EOS',
     'transfer': 0}
    args = eosapi.pack_args('eosio', 'delegatebw', args)
    act = ['eosio', 'delegatebw', args, {'eosio':'active'}]
    actions.append(act)
    rr, cost = eosapi.push_actions(actions)
    '''

    contracts_path = os.path.join(os.getcwd(), '..', 'contracts')
    sys.path.append(os.getcwd())
    account = 'eosio'

    _path = os.path.join(contracts_path, 'eosio.system', 'eosio.system')
    wast = _path + '.wast'
    abi = _path + '.abi'
    r = eosapi.set_contract(account, wast, abi, 0)
    assert r

def publish_system_contracts(accounts_map):
    contracts_path = os.path.join(os.getcwd(), '..', 'contracts')
    sys.path.append(os.getcwd())
#    accounts_map = {'eosio.token':'eosio.token', 'eosio.msig':'eosio.msig', 'eosio':'eosio.system'}
    for account in accounts_map:
        print('account', account)
        if not eosapi.get_account(account):
            r = eosapi.create_account('eosio', account, key1, key2)
            assert r

        _path = os.path.join(contracts_path, accounts_map[account], accounts_map[account])
        wast = _path + '.wast'
        code = open(wast, 'rb').read()
        code = eosapi.wast2wasm(code)
        hash = eosapi.sha256(code)
        old_hash = eosapi.get_code_hash(account)
        if old_hash != hash:
            print('+++++++++code update', account)
            wast = _path + '.wast'
            abi = _path + '.abi'
            r = eosapi.set_contract(account, wast, abi, 0)
            print(wast, abi)
            time.sleep(1.0)
            if account == 'eosio.token' and eosapi.get_balance('eosio') <= 0.0:
                print('issue system token...')
#                msg = {"issuer":"eosio","maximum_supply":"1000000000.0000 EOS","can_freeze":0,"can_recall":0, "can_whitelist":0}
                msg = {"issuer":"eosio","maximum_supply":"11000000000000.0000 EOS"}
                r = eosapi.push_action('eosio.token', 'create', msg, {'eosio.token':'active'})
                assert r
                r = eosapi.push_action('eosio.token','issue',{"to":"eosio","quantity":"10000000000000.0000 EOS","memo":""},{'eosio':'active'})
                assert r
#                    msg = {"from":"eosio", "to":"hello", "quantity":"100.0000 EOS", "memo":"m"}
#                    r = eosapi.push_action('eosio.token', 'transfer', msg, {'eosio':'active'})

def init():
    if eosapi.is_replay():
        return

    src_dir = os.path.dirname(os.path.abspath(__file__))
    create_system_accounts()
    publish_system_contracts({'eosio.token':'eosio.token'})
    publish_system_contracts({'eosio.msig':'eosio.msig'})
    publish_system_contracts({'eosio':'eosio.system'})

    '''
    account = 'lab'
    for account in ['eosio', 'eosio.token']:
        print('++++++boost account', account)
        msg = eosapi.pack_args('eosio', 'boost', {'account':account})
        act = ['eosio', 'boost', msg, {'eosio':'active'}]
        rr, cost = eosapi.push_actions([act])
        assert_ret(rr)
    '''
    
#    from backyard import t
#    t.deploy_mpy()

#    net.connect('127.0.0.1:9101')
    #load common libraries
#    t.load_all()

peers = ("106.10.42.238:9876",
"178.49.174.48:9876",
"185.253.188.1:19876",
"807534da.eosnodeone.io:19872",
"api-full1.eoseoul.io:9876",
"api-full2.eoseoul.io:9876",
"api.eosuk.io:12000",
"boot.eostitan.com:9876",
"bp.antpool.com:443",
"bp.cryptolions.io:9876",
"bp.eosbeijing.one:8080",
"bp.libertyblock.io:9800",
"eos-seed-de.privex.io:9876",
"eos.nodepacific.com:9876",
"eos.staked.us:9870",
"eosapi.blockmatrix.network:13546",
"eosboot.chainrift.com:9876",
"eu-west-nl.eosamsterdam.net:9876",
"eu1.eosdac.io:49876",
"fn001.eossv.org:443",
"fullnode.eoslaomao.com:443",
"m.eosvibes.io:9876",
"mainnet-eos.wancloud.cloud:55576",
"mainnet.eoscalgary.io:5222",
"mainnet.eosoasis.io:9876",
"mainnet.eospay.host:19876",
"mars.fnp2p.eosbixin.com:443",
"node.eosflare.io:1883",
"node.eosio.lt:9878",
"node.eosmeso.io:9876",
"node1.eoscannon.io:59876",
"node1.eosnewyork.io:6987",
"node2.eosnewyork.io:6987",
"p.jeda.one:3322",
"p2p.eos.bitspace.no:9876",
"p2p.eosdetroit.io:3018",
"p2p.eosio.cr:1976",
"p2p.eosio.cr:5418",
"p2p.genereos.io:9876",
"p2p.mainnet.eosgermany.online:9876",
"p2p.mainnet.eospace.io:88",
"p2p.meet.one:9876",
"p2p.one.eosdublin.io:9876",
"p2p.two.eosdublin.io:9876",
"p2p.unlimitedeos.com:15555",
"peer.eosjrr.io:9876",
"peer.eosn.io:9876",
"peer.main.alohaeos.com:9876",
"peer2.mainnet.helloeos.com.cn:80",
"peering.mainnet.eoscanada.com:9876",
"peering1.mainnet.eosasia.one:80",
"peering2.mainnet.eosasia.one:80",
"pub0.eosys.io:6637",
"pub1.eostheworld.io:9876",
"pub1.eosys.io:6637",
"publicnode.cypherglass.com:9876",
"seed2.greymass.com:9876")

def connect_to_peers():
    for peer in peers:
        net.connect(peer)

def cleanup_peers():
    for n in net.connections():
        if n['last_handshake']['network_version'] == 0:
            net.disconnect(n['peer'])

cp = connect_to_peers
cup = cleanup_peers
pbs = produce_block_start
pbe = produce_block_end

original_sigint_handler = signal.getsignal(signal.SIGINT)

def info():
    print(eosapi.get_info())

def wd():
    print('disable native contract')
    debug.wasm_enable_native_contract(0)

pid = os.getpid()

def start_console():
    print("start console...")

    start_ipython = False

    if eosapi.has_opt('debug'):
        try:
            import IPython
            start_ipython = True
        except:
            pass

    if False: #start_ipython:
        IPython.start_ipython(user_ns=sys.modules['initeos'].__dict__)
    else:
        signal.signal(signal.SIGINT, original_sigint_handler)
        console = PyEosConsole(locals = globals())
        console.interact(banner='Welcome to PyEos')

def create_account_on_chain(from_account, new_account, balance, public_key):
    assert len(new_account) == 12
    assert balance <= 1.0
    assert len(public_key) == 53 and public_key[:3] == 'EOS'
    memo = '%s-%s'%(new_account, public_key)
    r = eosapi.transfer(from_account, 'signupeoseos', balance, memo)
    if r:
        print('success!')
    else:
        print('failure!')

def sellram(account, _bytes):
    r = eosapi.push_action('eosio', 'sellram', {'account':account, 'bytes':_bytes}, {account:'active'})
    print(r)

def check_connections():
    for peer in peers:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            addr, port = peer.split(':')
            port = int(port)
            s.connect((addr, port))
            s.close()
            print('"%s",'%(peer,))
        except Exception as ex:
            pass
#            print(ex)

def dlbw(_from, _to, net, cpu):
    args = {'from':_from, 
            'receiver':_to, 
            'stake_net_quantity':'%.4f EOS'%(net,), 
            'stake_cpu_quantity':'%.4f EOS'%(cpu,), 
            'transfer':False
            }
    eosapi.push_action('eosio', 'delegatebw', args, {_from:'active'})


def undlbw(_from, _to, net, cpu):
    args = {'from':_from, 
            'receiver':_to, 
            'unstake_net_quantity':'%.4f EOS'%(net,), 
            'unstake_cpu_quantity':'%.4f EOS'%(cpu,), 
            'transfer':False
            }
    eosapi.push_action('eosio', 'undelegatebw', args, {_from:'active'})
