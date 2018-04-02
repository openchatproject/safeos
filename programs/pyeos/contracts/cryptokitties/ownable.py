from backend import *
from basement import *
class Ownable:
    def __init__(self):
        self._owner = address(0)
        itr = db_find_i64(contract_owner, contract_owner, contract_owner, N('owner'))
        if itr >= 0:
            value = db_get_i64(itr)
            self._owner = address(value)
    '''FIXME: constructor
    function Ownable() {
      owner = msg.sender;
    }
    '''
    @property
    def owner(self):
        return self._owner
    
    @owner.setter
    def owner(self, addr):
        self._owner = addr

    @onlyOwner
    def transferOwnership(newOwner: address):
        if newOwner != address(0):
            self.owner = newOwner;
            