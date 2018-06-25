import os
import re
import sys
import imp
import signal
import pickle
import traceback

import db
import net
import wallet
import eosapi
import debug

from eosapi import *

from code import InteractiveConsole
from tools import sketch
from imp import reload

producer = eosapi.Producer()

sys.path.insert(0, '/Applications/Eclipse.app/Contents//Eclipse/plugins/org.python.pydev_5.9.2.201708151115/pysrc')

config = '''
# Track only transactions whose scopes involve the listed accounts. Default is to track all transactions.
# filter_on_accounts = 

# Limits the maximum time (in milliseconds) processing a single get_transactions call.
#get-transactions-time-limit = 3

# File to read Genesis State from
genesis-json = genesis.json


# Minimum size MB of database shared memory file
shared-file-size = 1024

 # Enable production on a stale chain, since a single-node test chain is pretty much always stale
enable-stale-production = true
# Enable block production with the testnet producers
producer-name = eosio

private-key = ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]


plugin = eosio::producer_plugin
plugin = eosio::wallet_api_plugin
plugin = eosio::chain_api_plugin
plugin = eosio::history_api_plugin

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


class PyEosConsole(InteractiveConsole):
    def __init__(self, locals):
        super(PyEosConsole, self).__init__(locals=locals, filename="<console>")

    def check_module(self):
        for module in sys.modules.values():
            if not hasattr(module, '__file__'):
                continue
            if module.__file__.endswith('.py'):
                try:
                    t1 = os.path.getmtime(module.__file__)
                    t2 = os.path.getmtime(module.__cached__)
                except Exception as e:
                    continue
                try:
                    if t1 > t2:
                        print('Reloading ', module.__file__)
                        imp.reload(module)
                except Exception as e:
                    traceback.print_exc()

    def interact(self, banner=None, exitmsg=None):
        try:
            sys.ps1
        except AttributeError:
            sys.ps1 = ">>> "
        try:
            sys.ps2
        except AttributeError:
            sys.ps2 = "... "
        cprt = 'Type "help", "copyright", "credits" or "license" for more information.'
        if banner is None:
            self.write("Python %s on %s\n%s\n(%s)\n" %
                       (sys.version, sys.platform, cprt,
                        self.__class__.__name__))
        elif banner:
            self.write("%s\n" % str(banner))
        more = 0
        while 1:
            try:
                if more:
                    prompt = sys.ps2
                else:
                    prompt = sys.ps1
                try:
                    line = self.raw_input(prompt)
                except EOFError:
                    self.write("\n")
                    break
                else:
                    if line.strip() == 'exit()':
                        break
                    if line.strip():
                        self.check_module()
                    more = self.push(line)
            except KeyboardInterrupt:
                self.write("\nKeyboardInterrupt\n")
                self.resetbuffer()
                more = 0
                break
        if exitmsg is None:
            self.write('now exiting %s...\n' % self.__class__.__name__)
        elif exitmsg != '':
            self.write('%s\n' % exitmsg)

if not hasattr(sys, 'argv'):
    sys.argv = ['']

key1 = 'EOS61MgZLN7Frbc2J7giU7JdYjy2TqnfWFjZuLXvpHJoKzWAj7Nst'
key2 = 'EOS5JuNfuZPATy8oPz9KMZV2asKf9m8fb2bSzftvhW55FKQFakzFL'
psw = None

def init_wallet():
    global psw
    if not os.path.exists('data-dir/mywallet.wallet'):
        psw = wallet.create('mywallet')
        print('wallet password:', psw)

        with open('data-dir/data.pkl', 'wb') as f:
            print('test wallet password save to data-dir/data.pkl')
            pickle.dump(psw, f)

    wallet.open('mywallet')
    if not psw:
        with open('data-dir/data.pkl', 'rb') as f:
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

    if os.path.exists('config-dir/.preinit'):
        return

    if not os.path.exists('config-dir'):
        print('Creating config-dir')
        os.mkdir('config-dir')

    with open('config-dir/.preinit', 'w') as f:
        pass

    config_file = 'config-dir/config.ini'
    genesis_file = 'config-dir/genesis.json'


    print('Initialize ', config_file)
    with open(config_file, 'w') as f:
        f.write(config)

    print('Initialize ', genesis_file)
    with open(genesis_file, 'w') as f:
        f.write(genesis)

    print('Initialize finished, please restart pyeos')
    sys.exit(0)

try:
    from apitest import t as at
    from cryptokitties import t as kt
    from currency import t as ct
    from counter import t as ct1
    from credit import t as ct2
    from hello import t as ht
    from backyard import t as bt
    from rpctest import t as rt
    from simpleauction import t as st
    from lab import t as lt
    
    from vote import t as vt2
    from native import t as nt
    from vmstore import t as vt
    from biosboot import t as bb

except Exception as e:
    traceback.print_exc()

def publish_system_contract():
    contracts_path = os.path.join(os.getcwd(), '..', 'contracts')
    sys.path.append(os.getcwd())
    accounts_map = {'eosio.bios':'eosio.bios', 'eosio.msig':'eosio.msig', 'eosio':'eosio.system', 'eosio.token':'eosio.token'}
    for account in accounts_map:
        print('account', account)
        if not eosapi.get_account(account):
            with producer:
                r = eosapi.create_account('eosio', account, key1, key2)
                assert r

        old_code = eosapi.get_code(account)
        if old_code:
            old_code = old_code[0]
        need_update = not old_code

        _path = os.path.join(contracts_path, accounts_map[account], accounts_map[account])
        if old_code:
            print('+++++++++old_code[:4]', old_code[:4])
            if old_code[:4] != b'\x00asm':
                old_code = eosapi.wast2wasm(old_code)
            wast = _path + '.wast'
            code = open(wast, 'rb').read()
            code = eosapi.wast2wasm(code)

            print(len(code), len(old_code), old_code[:20])
            if code == old_code:
                need_update = False
        if need_update:
            print('+++++++++code update', account)
            wast = _path + '.wast'
            abi = _path + '.abi'
            with producer:
                r = eosapi.set_contract(account, wast, abi, 0)

            if account == 'eosio.token':
#                msg = {"issuer":"eosio","maximum_supply":"1000000000.0000 EOS","can_freeze":0,"can_recall":0, "can_whitelist":0}
                msg = {"issuer":"eosio","maximum_supply":"10000000000.0000 EOS"}
                r = eosapi.push_action('eosio.token', 'create', msg, {'eosio.token':'active'})
                assert r
                r = eosapi.push_action('eosio.token','issue',{"to":"eosio","quantity":"1000.0000 EOS","memo":""},{'eosio':'active'})
                assert r
#                    msg = {"from":"eosio", "to":"hello", "quantity":"100.0000 EOS", "memo":"m"}
#                    r = eosapi.push_action('eosio.token', 'transfer', msg, {'eosio':'active'})

def init():
    if eosapi.is_replay():
        return

    src_dir = os.path.dirname(os.path.abspath(__file__))

    publish_system_contract()

    from backyard import t
    t.deploy_mpy()
#    net.connect('127.0.0.1:9101')
    #load common libraries
#    t.load_all()

#{'peer': '185.253.188.1:19877', 'connecting': False, 'syncing': False, 'last_handshake': {'network_version': 1206, 'chain_id': 'aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906', 'node_id': '7bb4919c0ada83d3c58327df5dbd24eea3be107d6ddda9e0ddabce737a54a403', 'key': 'EOS8mTvPNb9Pwm94z4yrjC1pFetk3otHPuWDu7XFsp5ewmuFtQQDa', 'time': 1529892410318197237, 'token': 'f35be5448760792d74826de520f9265b83724c3cd90607c66de2d0369c68f6e6', 'sig': 'SIG_K1_KZCkqeFkNFZzs9wxQuXmwd5ajMsQhE9sm5a7iE7cXmrjLdFbDhUCsmAzWhyno9P8XMqG8PvjwcsCT1xipRqeW6hdMLVzW2', 'p2p_address': '0.0.0.0:9000 - 7bb4919', 'last_irreversible_block_num': 2489982, 'last_irreversible_block_id': '0025fe7e5ac6d86d4b0f67ba87f423ec1e4f6e748292c6dedc89d2bd8100d985', 'head_num': 2489982, 'head_id': '0025ffc7bc59caaa79cdf6d334806a63db7eb9c5b767df696d7e10d2dd45401e', 'os': 'linux', 'agent': 'EOSGen', 'generation': 1}}

peers = ('173.242.25.101:7115',
'18.191.33.148:59876',
'185.253.188.1:19877',
'40.114.68.16:9876',
'api-full1.eoseoul.io:9876',
'api-full2.eoseoul.io:9876',
'bp.antpool.com:443',
'bp.cryptolions.io:9876',
'dc1.eosemerge.io:9876',
'eno.eosvan.io:19866',
'eos.infinitystones.io:9876',
'eos.nodepacific.com:9876',
'eosapi.blockmatrix.network:13546',
'eosbp.eosvillage.io:8181',
'fn001.eossv.org:443',
'mainnet.eoscalgary.io:5222',
'mars.fnp2p.eosbixin.com:443',
'node.eosio.lt:9878',
'node1.eosnewyork.io:6987',
'node2.eosarmy.io:3330',
'node2.eosnewyork.io:6987',
'node2.eosphere.io:9876',
'p2p.eos.bitspace.no:9876',
'p2p.genereos.io:9876',
'p2p.meet.one:9876',
'p2p.one.eosdublin.io:9876',
'p2p.saltblock.io:19876',
'p2p.two.eosdublin.io:9876',
'peer.eosjrr.io:9876',
'peer.eosn.io:9876',
'peer.main.alohaeos.com:9876',
'peer1.eospalliums.org:9876',
'peer1.eosthu.com:8080',
'peer1.mainnet.eos.store:80',
'peer1.mainnet.helloeos.com.cn:80',
'peer2.eospalliums.org:9876',
'peer2.mainnet.helloeos.com.cn:80',
'peering.mainnet.eoscanada.com:9876',
'pub1.eostheworld.io:9876',
'pub2.eostheworld.io:9876',
'publicnode.cypherglass.com:9876',
'seed2.greymass.com:9876')

def peers_connect():
    for peer in peers:
        net.connect(peer)

def peers_cleanup():
    for n in net.connections():
        if n['last_handshake']['network_version'] == 0:
            net.disconnect(n['peer'])

pc = peers_connect
pcu = peers_cleanup


original_sigint_handler = signal.getsignal(signal.SIGINT)

def info():
    eosapi.get_info()

def start_console():
    init_wallet()
    signal.signal(signal.SIGINT, original_sigint_handler)
    console = PyEosConsole(locals = globals())
    console.interact(banner='Welcome to PyEos')

