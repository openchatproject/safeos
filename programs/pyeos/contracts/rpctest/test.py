import time
import wallet
import eosapi
import initeos

from common import init_, producer

def init(func):
    def func_wrapper(*args):
        init_('rpctest', 'rpctest.py', 'rpctest.abi', __file__)
        return func(*args)
    return func_wrapper

@init
def test(name=None):
    with producer:
        print('rpctest')
        r = eosapi.push_message('rpctest','sayhello', 'hello,wwww',{'rpctest':'active'},rawargs=True)
        assert r

@init
def test2(count):
    import time
    import json
    
    contracts = []
    functions = []
    args = []
    per = []
    for i in range(count):
        functions.append('sayhello')
        arg = str(i)
        args.append(arg)
        contracts.append('rpctest')
        per.append({'rpctest':'active'})
    ret = eosapi.push_messages(contracts, functions, args, per, True, rawargs=True)
    assert ret
    cost = ret['cost_time']
    print('total cost time:%.3f s, cost per action: %.3f ms, actions per second: %.3f'%(cost/1e6, cost/count/1000, 1*1e6/(cost/count)))
    eosapi.produce_block()


