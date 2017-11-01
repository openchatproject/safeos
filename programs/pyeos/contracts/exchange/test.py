import time
import wallet
import eosapi

print('please make sure you are running the following command before test')
print('./pyeos/pyeos --manual_gen_block --debug -i')

class Wait(object):
    def __init__(self):
        pass
    
    def produce_block(self):
        for i in range(5):
            ret = eosapi.produce_block()
            if ret == 0:
                break
            time.sleep(1.0)
        count = 0
        while self.num == eosapi.get_info().head_block_num:  # wait for finish of create account
            time.sleep(0.2)
            count += 1
            if count >= 20:
                print('time out')
                return

    def __call__(self):
        self.num = eosapi.get_info().head_block_num
        self.produce_block()

    def __enter__(self):
        self.num = eosapi.get_info().head_block_num
    
    def __exit__(self, type, value, traceback):
        self.produce_block()

wait = Wait()

def init():
    psw = 'PW5Kd5tv4var9XCzvQWHZVyBMPjHEXwMjH1V19X67kixwxRpPNM4J'
    wallet.open('mywallet')
    wallet.unlock('mywallet', psw)
    
    key1 = 'EOS61MgZLN7Frbc2J7giU7JdYjy2TqnfWFjZuLXvpHJoKzWAj7Nst'
    key2 = 'EOS5JuNfuZPATy8oPz9KMZV2asKf9m8fb2bSzftvhW55FKQFakzFL'
    if not eosapi.get_account('currency'):
        with wait:
            r = eosapi.create_account('inita', 'currency', key1, key2)
            assert r

    if not eosapi.get_account('exchange'):
        with wait:
            r = eosapi.create_account('inita', 'exchange', key1, key2)
            assert r

    with wait:
        r = eosapi.set_contract('currency', '../../programs/pyeos/contracts/currency/currency.py', '../../contracts/currency/currency.abi', 1)
        assert r
        r = eosapi.set_contract('exchange', '../../programs/pyeos/contracts/exchange/exchange.py', '../../contracts/exchange/exchange.abi', 1)
        assert r

def test_deposit():
    messages = [
                [{"from":"currency", "to":"inita", "amount":1000}, ['currency', 'inita'], {'currency':'active'}],
                [{"from":"currency", "to":"initb", "amount":1000}, ['currency', 'initb'], {'currency':'active'}],
                [{"from":"inita", "to":"exchange", "amount":1000}, ['exchange', 'inita'], {'inita':'active'}],
                [{"from":"initb", "to":"exchange", "amount":1000}, ['exchange', 'initb'], {'initb':'active'}],
                ]
    for msg in messages:
        args, scopes, permissions = msg
        r = eosapi.push_message('currency', 'transfer', args, scopes, permissions)

    messages = [
                [ {"from":"inita", "to":"exchange", "amount":1000, "memo":"hello"}, ['exchange', 'inita'], {'inita':'active'}],
                [ {"from":"initb", "to":"exchange", "amount":1000, "memo":"hello"}, ['exchange', 'initb'], {'initb':'active'}],
                ]
    for msg in messages:
        args, scopes, permissions = msg
        r = eosapi.push_message('eos', 'transfer', args, scopes, permissions)
    wait()
    
def test_withdraw():
    messages = [
                [   
                    {"from":"exchange", "to":"inita", "amount":1, "memo":"hello"},
                    ['exchange', 'inita'],
                    {'exchange':'active', 'inita':'active'}
                 ],
                [
                    {"from":"exchange", "to":"initb", "amount":1, "memo":"hello"},
                    ['exchange', 'initb'],
                    {'exchange':'active', 'initb':'active'}
                 ],
                ]
    for msg in messages:
        args, scopes, permissions = msg
        r = eosapi.push_message('eos', 'transfer', args, scopes, permissions)
        
    
    messages = [
                [{"from":"exchange", "to":"inita", "amount":1, "memo":"hello"}, ['exchange', 'inita'], {'exchange':'active', 'inita':'active'}],
                [{"from":"exchange", "to":"initb", "amount":1, "memo":"hello"}, ['exchange', 'initb'], {'exchange':'active', 'initb':'active'}]
               ]

# r = eosapi.push_message('currency','transfer',{"from":"exchange","to":"initb","amount":1,"memo":"hello"},['exchange','initb'],{'exchange':'active','initb':'active'})

    for msg in messages:
        args, scopes, permissions = msg
        r = eosapi.push_message('currency', 'transfer', args, scopes, permissions)
    
    wait()
    
def test_deadlock():
# raise a "tx_missing_scope: missing required scope" exception
    r = eosapi.push_message('currency', 'transfer', {"from":"currency", "to":"inita", "amount":1, "memo":"hello"}, ['inita'], {'currency':'active'})

# raise a "tx_missing_auth: missing required authority" exception
    r = eosapi.push_message('currency', 'transfer', {"from":"currency", "to":"inita", "amount":1, "memo":"hello"}, ['currency', 'inita'], {})
    wait()

def test_bs():
    
    with wait:
        args = {"buyer" : {"name":"inita", "id":1}, "price" : "2", "quantity" : 4, "expiration" : "2017-11-11T13:12:28", "fill_or_kill":0}
        scopes = ['exchange', 'inita']
        permissions = {'inita':'active'}

        r = eosapi.push_message('exchange', 'buy', args, scopes, permissions)
        
        args = {"seller" : {"name":"initb", "id":1}, "price" : "2", "quantity" : 2, "expiration" : "2017-11-11T13:12:28", "fill_or_kill":0}
        scopes = ['exchange', 'inita']
        permissions = {'initb':'active'}
        r = eosapi.push_message('exchange', 'sell', args, scopes, permissions)

    r = eosapi.get_table('exchange', 'exchange', 'account')
    print(r)

def t2():
    args = {"buyer" : {"name":"inita", "id":1}, "price" : "2", "quantity" : 1, "expiration" : "2017-11-11T13:12:28", "fill_or_kill":1}
    scopes = ['exchange', 'inita']
    permissions = {'inita':'active'}
    with wait:
        r = eosapi.push_message('exchange', 'buy', args, scopes, permissions)
        assert r


'''
         "buyer" : "OrderID",
         "price" : "UInt128",
         "quantity" : "UInt64",
         "expiration" : "Time"
    fill_or_kill
'''
if __name__ == '__main__':
    import sys
    sys.path.insert(0, '..')
    from exchange import *
    import eoslib as eoslib
    from eoslib import N, readMessage, writeMessage
    
    init()
    writeMessage(bytes(512))
    order = BuyOrder()
    order.buyer = OrderID()
    order.buyer.name = eoslib.N(b'buyer')
    order.buyer.id = 0
    
    order.price = uint128(11)
    order.quantity = 12
    order.expiration = int(time.time()) + 100
    writeMessage(order())
    apply(exchange, N(b'buy'))
    
    order = SellOrder()
    order.seller = OrderID()
    order.seller.name = eoslib.N(b'seller')
    order.seller.id = 0
    
    order.price = uint128(11)
    order.quantity = 12
    order.expiration = int(time.time()) + 100
    writeMessage(order())
    
    apply(exchange, N(b'sell'))

    id = OrderID()
    id.name = eoslib.N(b'buyer')
    id.id = 0
    writeMessage(id())

    apply(exchange, N(b'cancelbuy'))
    apply(exchange, N(b'cancelsell'))

    msg = struct.pack('QQQ', 123, exchange, 1)
    msg += b'hello'
    writeMessage(msg)
    apply(currency, N(b'transfer'))

    msg = struct.pack('QQQ', 123, exchange, 1)
    msg += b'hello'
    writeMessage(msg)
    apply(N(b'eos'), N(b'transfer'))
    print('done!')
