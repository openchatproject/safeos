#include "native_interface.hpp"

#include <eosio/chain/types.hpp>
#include <eosio/chain/db_api.hpp>

#include <fc/ext_string.h>
#include <dlfcn.h>

using namespace fc;



namespace eosio { namespace chain {

   native_interface::native_interface() {
      init_native_contract();
   };

   native_interface& native_interface::get() {
      static native_interface* native = nullptr;
      if (!native) {
        native = new native_interface();
      }
      return *native;
   };

   void native_interface::init_native_contract() {
//      uint64_t native_account[] = {N(eosio.bios), N(eosio.msig), N(eosio.token), N(eosio)/*eosio.system*/, N(exchange)};
      uint64_t native_account[] = {N(eosio.token)};
      fn_apply* native_applies[] = {&_bios_apply, &_msig_apply, &_token_apply, &_eosio_apply, &_exchange_apply};
      for (int i=0; i<sizeof(native_account)/sizeof(native_account[0]); i++) {
         if (!load_native_contract(native_account[i])) {
            *native_applies[i] = load_native_contract_default(native_account[i]);
         }
      }
   }

   fn_apply native_interface::load_native_contract_default(uint64_t _account) {
      string contract_path;
      uint32_t version = 0;
      uint64_t native = N(native);
      void *handle = nullptr;

      if (get_vm_api()->is_debug_mode()) {
         uint64_t debug_account = 0;
         const char* contract_path = get_vm_api()->vm_get_debug_contract(&debug_account);
         if (_account == debug_account && get_vm_api()->get_code_type(_account) == 0) {
            handle = dlopen(contract_path, RTLD_LAZY | RTLD_LOCAL);
         }
      }

      if (!handle) {
         char _path[128];
         ext_string s = name(_account).to_string();
         s.replace(".", "_");
         snprintf(_path, sizeof(_path), "../libs/lib%s_native%s", s.c_str(), DYLIB_SUFFIX);
//         wlog("loading native contract: ${n1}", ("n1", string(_path)));
         handle = dlopen(_path, RTLD_LAZY | RTLD_LOCAL);
         if (!handle) {
          return nullptr;
         }
      }

      fn_apply _apply = (fn_apply)dlsym(handle, "apply");

      std::unique_ptr<native_code_cache> _cache = std::make_unique<native_code_cache>();
      _cache->version = version;
      _cache->handle = handle;
      _cache->apply = _apply;
//      native_cache.emplace(_account, std::move(_cache));
      native_cache[_account] =  std::move(_cache);
      return _apply;
   }

   fn_apply native_interface::load_native_contract(uint64_t _account) {
      if (!db_api::get().is_account(_account)) {
         return nullptr;
      }
      string contract_path;
      uint32_t version = 0;
      uint64_t native = N(native);
      void *handle = nullptr;
      char _name[64];
      snprintf(_name, sizeof(_name), "%s.%d", name(_account).to_string().c_str(), NATIVE_PLATFORM);
      uint64_t __account = NN(_name);
//         wlog("++++++++++try to load native contract: ${n}", ("n", (string(_name))));

      int itr = db_api::get().db_find_i64(native, native, native, __account);
      if (itr < 0) {
         auto _itr = native_cache.find(_account);
         if (_itr != native_cache.end()) {
            return _itr->second->apply;
         }
         return nullptr;
      }

      size_t native_size = 0;
      const char* code = db_api::get().db_get_i64_exex(itr, &native_size);
      version = *(uint32_t*)code;

      auto _itr = native_cache.find(_account);
      if (_itr != native_cache.end()) {
         if (version <= _itr->second->version) {
            return _itr->second->apply;
         }
      }

      char native_path[64];
      sprintf(native_path, "%s.%d",name(__account).to_string().c_str(), version);

      wlog("loading native contract:\t ${n}", ("n", native_path));

      struct stat _s;
      if (stat(native_path, &_s) == 0) {
         //
      } else {
         std::ofstream out(native_path, std::ios::binary | std::ios::out);
         out.write(&code[4], native_size - 4);
         out.close();
      }
      contract_path = native_path;

      handle = dlopen(contract_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
      if (!handle) {
         return nullptr;
      }

      fn_apply _apply = (fn_apply)dlsym(handle, "apply");

      std::unique_ptr<native_code_cache> _cache = std::make_unique<native_code_cache>();
      _cache->version = version;
      _cache->handle = handle;
      _cache->apply = _apply;
//      native_cache.emplace(_account, std::move(_cache));
      native_cache[_account] =  std::move(_cache);
      return _apply;
   }

   int native_interface::apply(uint64_t receiver, uint64_t account, uint64_t act) {
      auto it = native_cache.find(receiver);
      if (it == native_cache.end()) {
         load_native_contract_default(receiver);
         auto _it = native_cache.find(receiver);
         if (_it == native_cache.end()) {
            return 0;
         }
      }
      it->second->apply(receiver, account, act);
      return 1;
   }

} } // eosio::chain


