import struct

import db
from eoslib import N
import eoslib

_code = N('eosio.token')
#_code = N('mytoken')

def to_symbol_name(s):
    n = int.from_bytes(s, 'little')
    return n>>8

class asset(object):
    def __init__(self, amount=0, symbol=None):
        self.amount = amount
        self.symbol = symbol

    def pack(self):
        buffer = bytearray(16)
        struct.pack_into('Q8s', buffer, 0, self.amount, self.symbol)
        return bytes(buffer)

    @classmethod
    def unpack(cls, data):
        a = asset()
        a.amount, a.symbol = struct.unpack('Q8s', data)
        return a

class multi_index(object):
    def __init__(self, code, scope, table_id):
        self.code = code
        self.scope = scope
        self.table_id = table_id

    def store(self, payer):
        it = db.find_i64(self.code, self.scope, self.table_id, self.get_primary_key())
        if it >= 0:
            db.update_i64(it, payer, self.pack())
        else:
            db.store_i64(self.scope, self.table_id, payer, self.get_primary_key(), self.pack())

    def load(self):
        it = db.find_i64(self.code, self.scope, self.table_id, self.get_primary_key())
        if it >= 0:
            data = db.get_i64(it)
            return self.unpack(data)
        return False

    def get_primary_key(self):
        raise Exception('should be implemented by subclass')

    def pack(self):
        raise Exception('should be implemented by subclass')

    def unpack(self):
        raise Exception('should be implemented by subclass')

class currency_stats(multi_index):
    def __init__(self, symbol):
        table_id = N('stat')
        self.primary_key = scope = to_symbol_name(symbol)
        multi_index.__init__(self, _code, scope, table_id)

        self.supply = asset(0, symbol)
        self.max_supply =asset(0, symbol)
        self.issuer = 0

        self.load()

    def get_primary_key(self):
        return self.primary_key

    def pack(self):
        result = bytearray(40)
        s1 = self.supply
        s2 = self.max_supply
        struct.pack_into('Q8sQ8sQ', result, 0, s1.amount, s1.symbol, s2.amount, s2.symbol, self.issuer)
        return bytes(result)

    def unpack(self, data):
        s1 = self.supply
        s2 = self.max_supply
        s1.amount, s1.symbol, s2.amount, s2.symbol, self.issuer = struct.unpack_from('Q8sQ8sQ', data)
        return True

class transfer(object):
    def __init__(self):
        self._from, self.to, self.amount = 0, 0, 0

        self.symbol = None
        self.memo = None

    def unpack(self, msg):
        self._from, self.to, self.amount, self.symbol = struct.unpack('QQQ8s', msg)
        self.memo = eoslib.unpack_bytes(msg[32:])

    def pack(self):
        result = struct.pack('QQQ8s', self._from, self.to, self.amount, self.symbol)
        result += eoslib.pack_bytes(self.memo)
        return result

'''
    {"name":"to", "type":"account_name"},
    {"name":"quantity", "type":"asset"},
    {"name":"memo", "type":"string"}
'''

class issue(object):
    def __init__(self):
        self.unpack()

    def pack(self):
        pass

    def unpack(self):
        msg = eoslib.read_action()
        self.to, self.amount = struct.unpack('QQ', msg)
        self.symbol = msg[16:24]
        self.memo = eoslib.unpack_bytes(msg[24:])

'''
    {"name":"issuer", "type":"account_name"},
    {"name":"maximum_supply", "type":"asset"}
'''
class create():
    def __init__(self):
        self.unpack()

    def pack(self):
        pass
    
    def unpack(self):
        msg = eoslib.read_action()
        self.issuer, self.amount, self.symbol = struct.unpack('QQ8s', msg)

'''
{"name":"to", "type":"account_name"},
{"name":"quantity", "type":"asset"},
{"name":"memo", "type":"string"}
'''

class Balance(multi_index):
    def __init__(self, owner, symbol):
        self.owner = owner
        self.a = asset(0, symbol)
        self.symbol_name = to_symbol_name(symbol)

        table_id = N('accounts')
        multi_index.__init__(self, _code, self.owner, table_id)

        self.load()

    def add(self, amount, payer):
        self.a.amount += amount
        self.store(payer)

    def sub(self, amount):
        assert(self.a.amount >= amount)
        self.a.amount -= amount
        self.store(self.owner)

    def get_primary_key(self):
        return self.symbol_name

    def pack(self):
        buffer = bytearray(16)
        struct.pack_into('Q8s', buffer, 0, self.a.amount, self.a.symbol)
        return bytes(buffer)

    def unpack(self, data):
        self.a.amount, self.a.symbol = struct.unpack('Q8s', data)


def apply(receiver, account, act):
    if act == N('create'):
        eoslib.require_auth(_code)
        msg = eoslib.read_action()
        issuer, amount, symbol = struct.unpack('QQ8s', msg)

        cs = currency_stats(symbol)
        if cs.issuer:
            raise Exception('currency has already been created')
        cs.issuer = issuer
        cs.max_supply.amount = amount
        cs.store()

    elif act == N('issue'):
        msg = eoslib.read_action()
        _to, amount, symbol = struct.unpack('QQ8s', msg[:24])
        memo = eoslib.unpack_bytes(msg[24:])
        cs = currency_stats(symbol)
        assert cs.issuer, 'currency does not exists'
        eoslib.require_auth(cs.issuer)
        cs.supply.amount += amount
        assert cs.supply.amount < cs.max_supply.amount
        cs.store(cs.issuer)

        acc = Balance(_to, symbol)
        acc.add(amount, cs.issuer)

    elif act == N('transfer'):
        msg = eoslib.read_action()
        _from, to, amount, symbol = struct.unpack('QQQ8s', msg[:32])
        eoslib.require_auth(_from)
        memo = eoslib.unpack_bytes(msg[32:])
        a1 = Balance(_from, symbol)
        a2 = Balance(to, symbol)
        a1.sub(amount)
        a2.add(amount, _from)

