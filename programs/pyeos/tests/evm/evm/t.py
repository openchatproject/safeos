import os
import json
import web3

from web3 import Web3, HTTPProvider, TestRPCProvider, EthereumTesterProvider
from solc import compile_source
from web3.contract import ConciseContract
import eosapi
import wallet
import initeos
from common import prepare
from eosapi import N

def init(func):
    def func_wrapper(*args, **kwargs):
        if not eosapi.get_account('evm'):
            print('evm account not exist, create it.')
            r = eosapi.create_account('eosio', 'evm', initeos.key1, initeos.key2)
            assert r
        func(*args, **kwargs)
    return func_wrapper


from eth_utils import (
    to_dict,
)

class LocalProvider(web3.providers.base.JSONBaseProvider):
    endpoint_uri = None
    _request_args = None
    _request_kwargs = None

    def __init__(self, request_kwargs=None):
        self._request_kwargs = request_kwargs or {}
        super(LocalProvider, self).__init__()

    def __str__(self):
        return "RPC connection {0}".format(self.endpoint_uri)

    @to_dict
    def get_request_kwargs(self):
        if 'headers' not in self._request_kwargs:
            yield 'headers', self.get_request_headers()
        for key, value in self._request_kwargs.items():
            yield key, value

    def request_func_(self, method, params):
        print('----request_func', method, params)
        if method == 'eth_sendTransaction':
            print(params)
            if 'to' in params[0]:
                r, cost = eosapi.push_evm_action(params[0]['to'], params[0]['data'], {params[0]['from']:'active'}, True)
            else:
                r, cost  = eosapi.set_evm_contract(params[0]['from'], params[0]['data'])
            if r:
                return {'result':r}
        elif method == 'eth_call':
            return {"id":0,"jsonrpc":"2.0","result":123}
        elif method == 'eth_estimateGas':
            return {"id":0,"jsonrpc":"2.0","result":88}
        elif method == 'eth_blockNumber':
            return {"id":0,"jsonrpc":"2.0","result":15}
        elif method == 'eth_getBlock':
            result = {'author': '0x4b8823fda79d1898bd820a4765a94535d90babf3', 'extraData': '0xdc809a312e332e302b2b313436372a4444617277692f6170702f496e74', 'gasLimit': 3141592, 'gasUsed': 0, 'hash': '0x259d3ac184c567e4e3aa3fb0aa6c89d39dd172f6dad2c7e26265b40dce2f8893', 'logsBloom': '0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000', 'miner': '0x4b8823fda79d1898bd820a4765a94535d90babf3', 'number': 138, 'parentHash': '0x7ed0cdae409d5b785ea671e24408ab34b25cb450766e501099ad3050afeff71a', 'receiptsRoot': '0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421', 'sha3Uncles': '0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347', 'stateRoot': '0x1a0789d0d895011034cda1007a4be75faee0b91093c784ebf246c8651dbf699b', 'timestamp': 1521704325, 'totalDifficulty': 131210, 'transactions': [], 'transactionsRoot': '0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421', 'uncles': []}
            return {"id":0,"jsonrpc":"2.0","result":result}
        elif method == 'eth_getBlockByNumber':
            result = {'author': '0x4b8823fda79d1898bd820a4765a94535d90babf3', 'extraData': '0xdc809a312e332e302b2b313436372a4444617277692f6170702f496e74', 'gasLimit': 3141592, 'gasUsed': 0, 'hash': '0x259d3ac184c567e4e3aa3fb0aa6c89d39dd172f6dad2c7e26265b40dce2f8893', 'logsBloom': '0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000', 'miner': '0x4b8823fda79d1898bd820a4765a94535d90babf3', 'number': 138, 'parentHash': '0x7ed0cdae409d5b785ea671e24408ab34b25cb450766e501099ad3050afeff71a', 'receiptsRoot': '0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421', 'sha3Uncles': '0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347', 'stateRoot': '0x1a0789d0d895011034cda1007a4be75faee0b91093c784ebf246c8651dbf699b', 'timestamp': 1521704325, 'totalDifficulty': 131210, 'transactions': [], 'transactionsRoot': '0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421', 'uncles': []}
            return {"id":0,"jsonrpc":"2.0","result":result}
        elif method == 'eth_blockNumber':
            return {"id":0,"jsonrpc":"2.0","result":'100'}

    def request_func(self, web3, outer_middlewares):
        '''
        @param outer_middlewares is an iterable of middlewares, ordered by first to execute
        @returns a function that calls all the middleware and eventually self.make_request()
        '''
        return self.request_func_
    
    def get_request_headers(self):
        return {
            'Content-Type': 'application/json',
            'User-Agent': construct_user_agent(str(type(self))),
        }


TEST = False
DEPLOY = True

contract_interface = None
w3 = None
contract = None
contract_address = None

provider = LocalProvider()

if TEST:
    w3 = Web3(EthereumTesterProvider())
else:
    w3 = Web3(provider)


def compile(contract_source_code, main_class):

    compiled_sol = compile_source(contract_source_code) # Compiled source code
#    print(compiled_sol)
    
#    s = json.dumps(compiled_sol[main_class], sort_keys=False, indent=4, separators=(',', ': '))
    for _class in compiled_sol.keys():
        print(_class)

    contract_interface = compiled_sol[main_class]
    return contract_interface

def deploy(contract_interface):
    contract = w3.eth.contract(contract_interface['abi'], bytecode=contract_interface['bin'])
#    print(contract_interface['bin'])
    json.dumps(contract_interface['abi'], sort_keys=False, indent=4, separators=(',', ': '))

    address = eosapi.eos_name_to_eth_address('evm')
    tx_hash = contract.deploy(transaction={'from': address, 'gas': 2000001350})
    print('tx_hash:', tx_hash)
    print('=========================deploy end======================')

def call_contract(contract_interface):
#    contract = w3.eth.contract(contract_interface['abi'], bytecode=contract_interface['bin'])
    contract_address = eosapi.eos_name_to_eth_address('evm')

    # Contract instance in concise mode
    contract_instance = w3.eth.contract(contract_address, abi=contract_interface['abi'], bytecode=contract_interface['bin'], ContractFactoryClass=ConciseContract)


#    r = contract_instance.getValue(transact={'from': address})
#    r = contract_instance.getValue(call={'from': address})
    if 0:
        r = contract_instance.getValue(transact={'from': contract_address})
        print('++++++++++getValue:', r)

    address = eosapi.eos_name_to_eth_address('evm')
    r = contract_instance.setValue(119000, transact={'from': contract_address})
    print('++++++++++++setValue:', r)

#    r = contract_instance.getValue(transact={'from': address})
#    r = contract_instance.getValue(call={'from': address})
    if 0:
        r = contract_instance.getValue(transact={'from': contract_address})
        print('++++++++++getValue:', r)


def kitties_test(contract_interface):
    contract = w3.eth.contract(contract_interface['abi'], bytecode=contract_interface['bin'])
    contract_address = eosapi.eos_name_to_eth_address('evm')

    # Contract instance in concise mode
    contract_instance = w3.eth.contract(contract_interface['abi'], contract_address, ContractFactoryClass=ConciseContract)

#    r = contract_instance.getValue(transact={'from': address})
#    r = contract_instance.getValue(call={'from': address})
    r = contract_instance.getValue(transact={'from': contract_address})
    print('++++++++++getValue:', r)

    address = eosapi.eos_name_to_eth_address('evm')
    r = contract_instance.setValue(119000, transact={'from': contract_address})
    print('++++++++++++setValue:', r)

#    r = contract_instance.getValue(transact={'from': address})
#    r = contract_instance.getValue(call={'from': address})
    r = contract_instance.getValue(transact={'from': contract_address})
    print('++++++++++getValue:', r)

contract_source_code = '''

'''

@init
def test():
    main_class = '<stdin>:Greeter'
    greeter = os.path.join(os.path.dirname(__file__), 'greeter.sol')
    with open(greeter, 'r') as f:
        contract_source_code = f.read()
        contract_interface = compile(contract_source_code, main_class)
#        deploy(contract_interface)
        bin = contract_interface['bin']
        print(bin)

        account = 'evm'
        actions = []

        _src_dir = os.path.dirname(__file__)
        abi_file = os.path.join(_src_dir, 'evm.abi')
        setabi = eosapi.pack_setabi(abi_file, eosapi.N(account))
        act = ['eosio', 'setabi', setabi, {account:'active'}]
        actions.append(act)

#        bin = '608060405234801561001057600080fd5b506103e76000803373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055506122b86000803373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002081905550610221806100aa6000396000f300608060405260043610610057576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063209652551461005c5780635524107714610087578063a5b248d9146100b4575b600080fd5b34801561006857600080fd5b5061007161010b565b6040518082815260200191505060405180910390f35b34801561009357600080fd5b506100b260048036038101908080359060200190929190505050610151565b005b3480156100c057600080fd5b506100f5600480360381019080803573ffffffffffffffffffffffffffffffffffffffff1690602001909291905050506101dd565b6040518082815260200191505060405180910390f35b60008060003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054905090565b806000803373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002081905550606381016000803373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000208190555050565b600060205280600052604060002060009150905054815600a165627a7a7230582027ddd8a5407c4fad4a070848746c3d9e2313600c2a052b6e11b3416b35d72cda0029'
        args = eosapi.pack_args("eosio", 'setcode', {'account':account,'vmtype':2, 'vmversion':0, 'code':bin})
        act = ['eosio', 'setcode', args, {'evm':'active'}]
        actions.append(act)

        r, cost = eosapi.push_actions(actions)
        print(r['except'])
        print(r['elapsed'])
#        call_contract(contract_interface)

@init
def setValue(v=119000):
    main_class = '<stdin>:Greeter'
    greeter = os.path.join(os.path.dirname(__file__), 'greeter.sol')
    with open(greeter, 'r') as f:
        contract_source_code = f.read()
        contract_interface = compile(contract_source_code, main_class)
#        deploy(contract_interface)
        contract_abi = contract_interface['abi']

    fn_identifier = 'setValue'

    for abi in contract_abi:
        if 'name' in abi and abi['name'] == fn_identifier:
            fn_abi = abi
            break
    args = (v,)
    kwargs = {}

    data = web3.utils.contracts.encode_transaction_data(
            web3,
            fn_identifier,
            contract_abi,
            fn_abi,
            args,
            kwargs)
    print(data)
    data = data[2:]
    args = {'from':'eosio', 'to':'evm', 'amount':123, 'data':data}
#    args = eosapi.pack_args('evm', 'transfer', args)
#    print(args)
    r = eosapi.push_action('evm', 'transfer', args, {'eosio':'active'})
    print(r['except'])
    print(r['elapsed'])


@init
def getValue():
    main_class = '<stdin>:Greeter'
    greeter = os.path.join(os.path.dirname(__file__), 'greeter.sol')
    with open(greeter, 'r') as f:
        contract_source_code = f.read()
        contract_interface = compile(contract_source_code, main_class)
#        deploy(contract_interface)
        contract_abi = contract_interface['abi']

    fn_identifier = 'getValue'

    for abi in contract_abi:
        if 'name' in abi and abi['name'] == fn_identifier:
            fn_abi = abi
            break
    args = ()
    kwargs = {}

    data = web3.utils.contracts.encode_transaction_data(
            web3,
            fn_identifier,
            contract_abi,
            fn_abi,
            args,
            kwargs)
    print(data)
    data = data[2:]
    '''
    ret = eosapi.transfer('eosio', 'evm', 0.2, data)
    assert ret
    '''

    args = {'from':'eosio', 'to':'evm', 'amount':2000, 'data':data}
    r = eosapi.push_action('evm', 'transfer', args, {'eosio':'active'})
    print(r['elapsed'])

@init
def test2(count=200):
    main_class = '<stdin>:Greeter'
    greeter = os.path.join(os.path.dirname(__file__), 'greeter.sol')
    with open(greeter, 'r') as f:
        contract_source_code = f.read()
        contract_interface = compile(contract_source_code, main_class)
#        deploy(contract_interface)
        contract_abi = contract_interface['abi']

    fn_identifier = 'getValue'

    for abi in contract_abi:
        if 'name' in abi and abi['name'] == fn_identifier:
            fn_abi = abi
            break
    args = ()
    kwargs = {}

    data = web3.utils.contracts.encode_transaction_data(
            web3,
            fn_identifier,
            contract_abi,
            fn_abi,
            args,
            kwargs)
    print(data)
    data = data[2:]
    args = {'from':'eosio', 'to':'evm', 'amount':10, 'data':data}

    actions = []
    for i in range(count):
        action = ['evm', 'transfer', args, {'eosio':'active'}]
        actions.append(action)

    ret, cost = eosapi.push_actions(actions)
    cost = ret['elapsed']
    print(ret['except'])
    assert ret and not ret['except']
    print('total cost time:%.3f s, cost per action: %.3f ms, actions per second: %.3f'%(cost/1e6, cost/count/1000, 1*1e6/(cost/count)))

@init
def test3(count=200):
    main_class = '<stdin>:Greeter'
    greeter = os.path.join(os.path.dirname(__file__), 'greeter.sol')
    with open(greeter, 'r') as f:
        contract_source_code = f.read()
        contract_interface = compile(contract_source_code, main_class)
#        deploy(contract_interface)
        contract_abi = contract_interface['abi']

    fn_identifier = 'getValue'

    for abi in contract_abi:
        if 'name' in abi and abi['name'] == fn_identifier:
            fn_abi = abi
            break
    args = ()
    kwargs = {}

    data = web3.utils.contracts.encode_transaction_data(
            web3,
            fn_identifier,
            contract_abi,
            fn_abi,
            args,
            kwargs)
    print(data)
    data = data[2:]
    transactions = []
    for i in range(count):
        args = {'from':'eosio', 'to':'evm', 'amount':i, 'data':data}
        args = eosapi.pack_args('evm', 'transfer', args)
        action = ['evm', 'transfer', args, {'eosio':'active'}]
        transactions.append([action,])
    ret, cost = eosapi.push_transactions(transactions)
    assert ret
    print('total cost time:%.3f s, cost per action: %.3f ms, actions per second: %.3f'%(cost/1e6, cost/count/1000, 1*1e6/(cost/count)))

@init
def test4():
    main_class = '<stdin>:KittyCore'
    with open('../../programs/pyeos/contracts/evm/cryptokitties.sol', 'r') as f:
        contract_source_code = f.read()
        contract_interface = compile(contract_source_code, main_class)
        deploy(contract_interface)
        kitties_test(contract_interface)


if __name__ == '__main__':
    import pydevd
    pydevd.settrace(suspend=False)
    test2()



