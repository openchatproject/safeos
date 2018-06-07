from eoslib import *
code = N('vmstore')
def deploy(vm_name, src_code):
    print('++++++++++++deploy:vm_name', vm_name)
    itr = db_find_i64(code, code, code, vm_name)
    if itr < 0:
        db_store_i64(code, code, code, vm_name, src_code)
    else:
        db_update_i64(itr, code, src_code)

def apply(receiver, code, action):
    print(n2s(code), n2s(action))
    if action == N('deploy'):
        require_auth(code)
        msg = read_action()
        print('++++++len(msg):', len(msg))
        vm_name = int.from_bytes(msg[:8], 'little')
        src_code = msg[8:]
        deploy(vm_name, src_code)
