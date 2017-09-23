Hack on Eos. Have fun!

# what does this project for?

Eos will be a revolutionary technology. And I'm curious about the tech behind Eos. So I think I need to do something.
Python is the most powerfull language on the earth, it's easy to use and easy to understand.you can do a lot of things with python without paying too much time. But for the reason of permarmance,Eos is writing in C++. So I think maybe I can rewrite Eos in python? At least I can improve the usability of Eos with python. So here comes pyeos.

# what pyeos can do?

1. now you can call eos api with python
2. now you can write simple contract in python


# How to build
You have to export two environment,that's for compile python code.

```bash
export PYTHON_LIB="~/anaconda/lib/libpython3.6m.dylib"
export PYTHON_INC_DIR="~/anaconda/include/python3.6m"
```

Now following the instruction on [Building EOS and running a node](https://github.com/learnforpractice/eos#runanode)

currently only test with python 3.6 on macOS 0.12.6


# How to run

after your successful build of project,you can run the following command to start eos.

```
$ cd ~/dev/eos/build/programs/
$ ./pyeos/pyeos --skip-transaction-signatures
```

Then a python interactive console will show, you can input command in the python console.


# eos api

### >>> import eosapi
### >>> info = eosapi.get_info()
### >>> info
```
info = eosapi.get_info() json_str.size():336 info {'head_block_num': 2110, 'last_irreversible_block_num': 2092, 'head_block_id': '0000083ef40927dffaee69ff02a80beb7b21a2238e91300ac25d259179d7830d', 'head_block_time': '2017-08-27T09:22:00', 'head_block_producer': 'initp', 'recent_slots': '1111111111111111111111111111111111111111111111111111111111111111', 'participation_rate': '1.00000000000000000'}
{'head_block_num': 2110, 'last_irreversible_block_num': 2092, 'head_block_id': '0000083ef40927dffaee69ff02a80beb7b21a2238e91300ac25d259179d7830d', 'head_block_time': '2017-08-27T09:22:00', 'head_block_producer': 'initp', 'recent_slots': '1111111111111111111111111111111111111111111111111111111111111111', 'participation_rate': '1.00000000000000000'}
```
### >>> key = eosapi.create_key()
```
public: EOS5XnfJ2ahqxhmZz47mc1nceJdsumB4Mtpwu3tDDnnj9Pqobzn6o private: 5Ka18FuiqDTGHi6hRqqYRHeC8iabMzjLSbNegpvL9rosmRfC8qV
```
### >>> print(key)
```
(b'EOS5XnfJ2ahqxhmZz47mc1nceJdsumB4Mtpwu3tDDnnj9Pqobzn6o', b'5Ka18FuiqDTGHi6hRqqYRHeC8iabMzjLSbNegpvL9rosmRfC8qV')
```
### >>> key1 = 'EOS4toFS3YXEQCkuuw1aqDLrtHim86Gz9u3hBdcBw5KNPZcursVHq'
### >>> key2 = 'EOS6KdkmwhPyc2wxN9SAFwo2PU2h74nWs7urN1uRduAwkcns2uXsa'
### >>> eosapi.create_account('inita','tester5',key1,key2)
### >>> key = eosapi.create_key() 
```
public: EOS5XnfJ2ahqxhmZz47mc1nceJdsumB4Mtpwu3tDDnnj9Pqobzn6o private: 5Ka18FuiqDTGHi6hRqqYRHeC8iabMzjLSbNegpvL9rosmRfC8qV 
```
### >>> print(key) 
```
(b'EOS5XnfJ2ahqxhmZz47mc1nceJdsumB4Mtpwu3tDDnnj9Pqobzn6o', b'5Ka18FuiqDTGHi6hRqqYRHeC8iabMzjLSbNegpvL9rosmRfC8qV') 
```
### >>> r = eosapi.get_transaction('687d06e777bd8deedca0a7a92938b6fd44e85599d7ab8d351f4e98b42e8be082')
### >>> print(r)
```
r = eosapi.get_transaction('687d06e777bd8deedca0a7a92938b6fd44e85599d7ab8d351f4e98b42e8be082') print(r) {'transaction': {'refBlockNum': 2033, 'refBlockPrefix': 2586037830, 'expiration': '2017-08-27T09:19:49', 'scope': ['eos', 'inita'], 'signatures': [], 'messages': [{'code': 'eos', 'type': 'newaccount', 'authorization': [{'account': 'inita', 'permission': 'active'}], 'data': '000000008040934b000000e0cb4267a101000000010200b35ad060d629717bd3dbec82731094dae9cd7e9980c39625ad58fa7f9b654b010000010000000102bcca6347d828d4e1868b7dfa91692a16d5b20d0ee3d16a7ca2ddcc7f6dd03344010000010000000001000000008040934b00000000149be8080100010000000000000008454f5300000000'}], 'output': [{'notify': [], 'sync_transactions': [], 'async_transactions': []}]}}
{'transaction': {'refBlockNum': 2033, 'refBlockPrefix': 2586037830, 'expiration': '2017-08-27T09:19:49', 'scope': ['eos', 'inita'], 'signatures': [], 'messages': [{'code': 'eos', 'type': 'newaccount', 'authorization': [{'account': 'inita', 'permission': 'active'}], 'data': '000000008040934b000000e0cb4267a101000000010200b35ad060d629717bd3dbec82731094dae9cd7e9980c39625ad58fa7f9b654b010000010000000102bcca6347d828d4e1868b7dfa91692a16d5b20d0ee3d16a7ca2ddcc7f6dd03344010000010000000001000000008040934b00000000149be8080100010000000000000008454f5300000000'}], 'output': [{'notify': [], 'sync_transactions': [], 'async_transactions': []}]}}
```
### >>> eosapi.get_account('currency')
```
[b'currency', 0, 1, 0, b'2106-02-07T06:28:15']
```

### >>> eosapi.set_contract('currency','../../contracts/currency/currency.wast','../../contracts/currency/currency.abi',False)
```
Reading WAST...
Assembling WASM...
Publishing contract...
2103377ms thread-1   eos_contract.cpp:200          apply_eos_setcode    ] set code: 2390
2103377ms thread-1   wasm_interface.cpp:348        get                  ] Runtime::init
2103380ms thread-1   wasm_interface.cpp:522        load                 ] LOADING CODE
2103439ms thread-1   wasm_interface.cpp:532        load                 ] (end-start).count()/1000000.0: 0.05870400000000000 
2103439ms thread-1   wasm_interface.cpp:543        load                 ] INIT MEMORY: 189

2103439ms thread-1   wasm_interface.cpp:549        load                 ] state.code_version: 9b9db1a7940503a88535517049e64467a6e8f4e9e03af15e9968ec89dd794975 
2103439ms thread-1   wasm_interface.cpp:435        vm_onInit            ] on_init
b'"{\\"transaction_id\\":\\"19c3b23f43a90d3960a172d3195e4dd1726ce99fd95f311776bf37ac2558650b\\",\\"processed\\":{\\"refBlockNum\\":19565,\\"refBlockPrefix\\":3718592787,\\"expiration\\":\\"2017-09-08T14:36:43\\",\\"scope\\":[\\"currency\\",\\"eos\\"],\\"signatures\\":[],\\"messages\\":[{\\"code\\":\\"eos\\",\\"type\\":\\"setcode\\",\\"authorization\\":[{\\"account\\":\\"currency\\",\\"permission\\":\\"active\\"}],\\"data\\":\\"00000079b822651d0000d6120061736d0100000001390a60037e7e7f017f60047e7e7f7f017f60017e0060057e7e7e7f7f017f60027f7f0060027f7f017f60027e7f0060017f0060000060027e7e0002760703656e7606617373657274000403656e76086c6f61645f693634000303656e760b726561644d657373616765000503656e760a72656d6f76655f693634000003656e760b7265717569726541757468000203656e760d726571756972654e6f74696365000203656e760973746f72655f6936340001030504060708090404017000000503010001077e05066d656d6f727902002a5f5a4e3863757272656e6379313273746f72654163636f756e744579524b4e535f374163636f756e74450007355f5a4e3863757272656e637932336170706c795f63757272656e63795f7472616e7366657245524b4e535f385472616e7366657245000804696e69740009056170706c79000a0ab80c043400024020012903084200510d0020004280808080a8d7bee3082001411010061a0f0b20004280808080a8d7bee308200110031a0bc20504017e027f047e017f4100410028020441206b2208360204200029030021052000290308100520051005200029030010042000290300210142002105423b2104411021034200210603400240024002400240024020054206560d0020032c00002202419f7f6a41ff017141194b0d01200241a0016a21020c020b420021072005420b580d020c030b200241ea016a41002002414f6a41ff01714105491b21020b2002ad42388642388721070b2007421f83200442ffffffff0f838621070b200341016a2103200542017c2105200720068421062004427b7c2204427a520d000b420021052008420037031820082006370310200142808080c887d7c8b21d4280808080a8d7bee308200841106a411010011a200041086a2903002101423b2104411021034200210603400240024002400240024020054206560d0020032c00002202419f7f6a41ff017141194b0d01200241a0016a21020c020b420021072005420b580d020c030b200241ea016a41002002414f6a41ff01714105491b21020b2002ad42388642388721070b2007421f83200442ffffffff0f838621070b200341016a2103200542017c2105200720068421062004427b7c2204427a520d000b2008200637030020084200370308200142808080c887d7c8b21d4280808080a8d7bee3082008411010011a200841186a220329030020002903105a4120100020032003290300200029031022057d370300200520082903087c20055a41d00010002008200829030820002903107c370308200029030021050240024020032903004200510d0020054280808080a8d7bee308200841106a411010061a0c010b20054280808080a8d7bee308200841106a10031a0b200041086a290300210502400240200841086a2903004200510d0020054280808080a8d7bee3082008411010061a0c010b20054280808080a8d7bee308200810031a0b4100200841206a3602040b970303027f057e017f4100410028020441106b220736020442002103423b210241800121014200210403400240024002400240024020034207560d0020012c00002200419f7f6a41ff017141194b0d01200041a0016a21000c020b420021052003420b580d020c030b200041ea016a41002000414f6a41ff01714105491b21000b2000ad42388642388721050b2005421f83200242ffffffff0f838621050b200141016a2101200342017c2103200520048421042002427b7c2202427a520d000b42002103423b2102411021014200210603400240024002400240024020034206560d0020012c00002200419f7f6a41ff017141194b0d01200041a0016a21000c020b420021052003420b580d020c030b200041ea016a41002000414f6a41ff01714105491b21000b2000ad42388642388721050b2005421f83200242ffffffff0f838621050b200141016a2101200342017c2103200520068421062002427b7c2202427a520d000b2007428094ebdc033703082007200637030020044280808080a8d7bee3082007411010061a4100200741106a3602040ba30303027f047e017f4100410028020441206b220836020442002105423b210441800121034200210603400240024002400240024020054207560d0020032c00002202419f7f6a41ff017141194b0d01200241a0016a21020c020b420021072005420b580d020c030b200241ea016a41002002414f6a41ff01714105491b21020b2002ad42388642388721070b2007421f83200442ffffffff0f838621070b200341016a2103200542017c2105200720068421062004427b7c2204427a520d000b024020062000520d0042002105423b210441900121034200210603400240024002400240024020054207560d0020032c00002202419f7f6a41ff017141194b0d01200241a0016a21020c020b420021072005420b580d020c030b200241ea016a41002002414f6a41ff01714105491b21020b2002ad42388642388721070b2007421f83200442ffffffff0f838621070b200341016a2103200542017c2105200720068421062004427b7c2204427a520d000b20062001520d0020084200370318200841086a4118100241174b41a0011000200841086a10080b4100200841206a3602040b0bb601070041040b04c00400000041100b086163636f756e74000041200b2c696e746567657220756e646572666c6f77207375627472616374696e6720746f6b656e2062616c616e6365000041d0000b26696e7465676572206f766572666c6f7720616464696e6720746f6b656e2062616c616e636500004180010b0963757272656e637900004190010b097472616e73666572000041a0010b1e6d6573736167652073686f72746572207468616e20657870656374656400009202046e616d650b06617373657274020000086c6f61645f6936340500000000000b726561644d6573736167650200000a72656d6f76655f693634030000000b726571756972654175746801000d726571756972654e6f7469636501000973746f72655f69363404000000002a5f5a4e3863757272656e6379313273746f72654163636f756e744579524b4e535f374163636f756e74450201300131355f5a4e3863757272656e637932336170706c795f63757272656e63795f7472616e7366657245524b4e535f385472616e73666572450901300131013201330134013501360137013804696e69740801300131013201330134013501360137056170706c7909013001310132013301340135013601370138010b4163636f756e744e616d65044e616d6502087472616e7366657200030466726f6d0b4163636f756e744e616d6502746f0b4163636f756e744e616d6506616d6f756e740655496e743634076163636f756e740002076163636f756e74044e616d650762616c616e63650655496e74363401000000b298e982a4087472616e736665720100000080bafac6080369363401076163636f756e7400076163636f756e74\\"}],\\"output\\":[{\\"notify\\":[],\\"sync_transactions\\":[],\\"async_transactions\\":[]}]}}"'
2106007ms thread-0   eos_contract.cpp:200          apply_eos_setcode    ] set code: 2390
2106007ms thread-0   wasm_interface.cpp:435        vm_onInit            ] on_init
2106010ms thread-0   eos_contract.cpp:200          apply_eos_setcode    ] set code: 2390
2106010ms thread-0   wasm_interface.cpp:435        vm_onInit            ] on_init
```
### >>> eosapi.get_account('currency')
```
[b'currency', 0, 1, 0, b'2106-02-07T06:28:15', b'{"types":[{"newTypeName":"AccountName","type":"Name"}],"structs":[{"name":"transfer","base":"","fields":{"from":"AccountName","to":"AccountName","amount":"UInt64"}},{"name":"account","base":"","fields":{"account":"Name","balance":"UInt64"}}],"actions":[{"action":"transfer","type":"transfer"}],"tables":[{"table":"account","indextype":"i64","keynames":["account"],"keytype":[],"type":"account"}]}']
```

### >>> eosapi.push_message('currency','transfer','{"from":"currency","to":"inita","amount":50}',['currency','inita'],
```
{'currency':'active'},False)
175534ms thread-1   eosapi.cpp:327                push_message_        ] Converting argument to binary...
b'{"transaction_id":"7ea7e68ea661da58113dbcf1ef280a3959ba2d0bf194816bbb5ba2bca21a1909","processed":{"refBlockNum":21283,"refBlockPrefix":186295305,"expiration":"2017-09-09T10:04:34","scope":["currency","inita"],"signatures":[],"messages":[{"code":"currency","type":"transfer","authorization":[{"account":"currency","permission":"active"}],"data":{"from":"currency","to":"inita","amount":50},"hex_data":"00000079b822651d000000008040934b3200000000000000"}],"output":[{"notify":[{"name":"inita","output":{"notify":[],"sync_transactions":[],"async_transactions":[]}}],"sync_transactions":[],"async_transactions":[]}]}}'
```
### >>> eosapi.get_account('inita')
```
[b'inita', 9999999696, 0, 0, b'2106-02-07T06:28:15']
```
### >>> eosapi.get_table('inita','currency','account')

```
b'{"rows":[{"account":"account","balance":50}],"more":false}'
```

### >>> eosapi.get_table('currency','currency','account')
```
b'{"rows":[{"account":"account","balance":999999950}],"more":false}'
```

# wallet api

### >>> import wallet
### >>> wallet_name = 'mywallet'
### >>> password = wallet.create(wallet_name) 
```
2439175ms thread-1   wallet.cpp:182                save_wallet_file     ] saving wallet to file /Users/newworld/dev/eos/build/programs/data-dir/./mywallet.wallet
```
### >>> print(password)
```
b'PW5JGYdyzzZYTQpPNFU3bb83fQW3P8LUUQ5w2D8GL8zaB93riScSE'
```
### >>> wallet.open(wallet_name) 
```
True
```
### >>> wallet.unlock(wallet_name,password)
```
True
```
### >>> wallet.import_key(wallet_name,'5JaXKGt2J4kK4EabnPZ2DSS6UJFe8vZ9mgEBXDQ5j3G2m7HE5nQ')
```
2439178ms thread-1   wallet.cpp:182                save_wallet_file     ] saving wallet to file /Users/newworld/dev/eos/build/programs/data-dir/./mywallet.wallet
True
```
### >>> wallet.list_keys()
```
{b'EOS6V8Ho3VBuxNrGsqHWVXHfRPThPhqFDs9aPp1s9hJqAKZJ7NTJJ': b'5JaXKGt2J4kK4EabnPZ2DSS6UJFe8vZ9mgEBXDQ5j3G2m7HE5nQ'}
```
### >>> wallet.list_wallets()
```
[b'mywallet *']
```
### >>> eosapi.create_key()
```
(b'EOS7VMTHQiCKzynXZHvW2Ef1zi8w92G6ayM7bdh1kV6dgewyBB8i7', b'5HpsDntjXznKoFb13QVBeAD6FUE7hwRjCdGrAs3Wq8GxqK9m8Qs')
### >>> wallet.import_key(wallet_name,b'5HpsDntjXznKoFb13QVBeAD6FUE7hwRjCdGrAs3Wq8GxqK9m8Qs')
3519376ms thread-1   wallet.cpp:182                save_wallet_file     ] saving wallet to file /Users/newworld/dev/eos/build/programs/data-dir/./mywallet.wallet
True
```
### >>> wallet.list_keys()
```
{b'EOS6V8Ho3VBuxNrGsqHWVXHfRPThPhqFDs9aPp1s9hJqAKZJ7NTJJ': b'5JaXKGt2J4kK4EabnPZ2DSS6UJFe8vZ9mgEBXDQ5j3G2m7HE5nQ', b'EOS7VMTHQiCKzynXZHvW2Ef1zi8w92G6ayM7bdh1kV6dgewyBB8i7': b'5HpsDntjXznKoFb13QVBeAD6FUE7hwRjCdGrAs3Wq8GxqK9m8Qs'}
```
