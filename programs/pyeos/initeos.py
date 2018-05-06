import os
import re
import sys
import imp
import traceback

import wallet
import eosapi
from code import InteractiveConsole
from tools import sketch
from imp import reload

class PyEosConsole(InteractiveConsole):
    def __init__(self, locals):
        super(PyEosConsole, self).__init__(locals=locals, filename="<console>")

    def runcode(self, code):
        exc_type = None
        exc_value = None
        exc_traceback = None
        try:
            exec(code, self.locals)
        except SystemExit:
            raise
        except AttributeError as e:
            exc_type, exc_value, exc_traceback = sys.exc_info()
        except:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            traceback.print_exception(exc_type, exc_value, exc_traceback,
                                          limit=10, file=sys.stdout)
        if exc_value:
            result = re.search("'(.*?)' has no attribute '(.*?)'", str(exc_value))
            if result:
                package, attr = result.groups()
                mod = imp.reload(sys.modules[package])
                try:
                    exec(code, self.locals)
                except:
                    exc_type, exc_value, exc_traceback = sys.exc_info()
                    traceback.print_exception(exc_type, exc_value, exc_traceback,
                                                  limit=10, file=sys.stdout)

if not hasattr(sys, 'argv'):
    sys.argv = ['']

from apitest import test as at
from cryptokitties import test as kt
from currency import test as ct
from counter import test as cot
from hello import test as ht
from backyard import test as bt
from rpctest import test as rt
from vote import test as vt
from simpleauction import test as st
from lab import test as lt

key1 = 'EOS61MgZLN7Frbc2J7giU7JdYjy2TqnfWFjZuLXvpHJoKzWAj7Nst'
key2 = 'EOS5JuNfuZPATy8oPz9KMZV2asKf9m8fb2bSzftvhW55FKQFakzFL'

def init():

    psw = 'PW5K87AKbRvFFMJJm4dU7Zco4fi6pQtygEU4iyajwyTvmELUDnFBK'
    
    if not os.path.exists('data-dir/mywallet.wallet'):
        psw = wallet.create('mywallet')
        print('wallet password:', psw)
    
    wallet.open('mywallet')
    wallet.unlock('mywallet',psw)
    
    priv_keys = [   
                    '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
                    '5JEcwbckBCdmji5j8ZoMHLEUS8TqQiqBG1DRx1X9DN124GUok9s',
                    '5JbDP55GXN7MLcNYKCnJtfKi9aD2HvHAdY7g8m67zFTAFkY1uBB'
                ]
    
    keys = wallet.list_keys()
    exist_priv_keys = keys.values()
    for priv_key in priv_keys:
        if not priv_key in exist_priv_keys:
            wallet.import_key('mywallet', priv_key)

    if eosapi.is_replay():
        return

    src_dir = os.path.dirname(os.path.abspath(__file__))

    '''
    r = eosapi.push_message('eosio.token', 'create', {"to":"eosio", "quantity":"10000.0000 EOS", "memo":""},{'eosio':'active'})
    r = eosapi.push_message('eosio.token','issue',{"to":"hello","quantity":"1000.0000 EOS","memo":""},{'hello':'active'})
    assert r
    msg = {"from":"eosio", "to":"hello", "quantity":"25.0000 EOS", "memo":"m"}
    r = eosapi.push_message('eosio.token', 'transfer', msg, {'eosio':'active'})
    assert r
    '''

    contracts_path = os.path.join(src_dir, '../../build', 'contracts')
    sys.path.append(os.getcwd())
    for account in ['eosio.bios', 'eosio.msig', 'eosio.system', 'eosio.token']:
        print('account', account)
        if not eosapi.get_account(account).permissions:
            r = eosapi.create_account('eosio', account, key1, key2)
            assert r
            eosapi.produce_block()

        old_code = eosapi.get_code(account)[0]
        need_update = not old_code
        if False: #old_code:
            print('+++++++++old_code[:4]', old_code[:4])
            if old_code[:4] != b'\x00asm':
                old_code = eosapi.wast2wasm(old_code)

            wast = os.path.join(contracts_path, account, account+'.wast')
            code = open(wast, 'rb').read()
            code = eosapi.wast2wasm(code)

            print(len(code), len(old_code), old_code[:20])
            if code == old_code:
                need_update = False
        if need_update:
            wast = os.path.join(contracts_path, account, account+'.wast')
            abi = os.path.join(contracts_path, account, account+'.abi')
            r = eosapi.set_contract(account, wast, abi, 0)
            eosapi.produce_block()

            if False: #account == 'eosio.token':
                msg = {"issuer":"eosio","maximum_supply":"1000000000.0000 EOS","can_freeze":0,"can_recall":0, "can_whitelist":0}
                r = eosapi.push_message('eosio.token', 'create', msg, {'eosio.token':'active'})
                assert r
                r = eosapi.push_message('eosio.token','issue',{"to":"eosio","quantity":"1000.0000 EOS","memo":""},{'eosio':'active'})
                assert r
                eosapi.produce_block()

    from backyard import test
    test.deploy_mpy()
    #load common libraries
#    test.load_all()
    console = PyEosConsole(locals = globals())
    console.interact(banner='Welcome to pyeos')
