#include "vm_manager.hpp"
#include "micropython/db_api.hpp"
#include <dlfcn.h>

using namespace eosio::chain;

static uint64_t vm_names[] = {N(vm.wasm), N(vm.py), N(vm.eth)};

vm_manager& vm_manager::get() {
   static vm_manager *mngr = nullptr;
   if (!mngr) {
      mngr = new vm_manager();
   }
   return *mngr;
}

vm_manager::vm_manager() {
   const char* vm_libs_path[] = {
#ifdef DEBUG
   "../libraries/vm_wasm/libvm_wasmd.dylib",
   "../libraries/vm_py/libvm_py-1d.dylib",
   "../libraries/vm_eth/libvm_ethd.dylib",
#else
   "../libraries/vm_wasm/libvm_wasm.dylib",
   "../libraries/vm_py/libvm_py.dylib",
   "../libraries/vm_eth/libvm_eth.dylib",
#endif
   };

   for (int i=0;i<sizeof(vm_names)/sizeof(vm_names[0]);i++) {
      if (load_vm(i, vm_names[i])) {
         continue;
      }
      load_vm_default(i, vm_libs_path[i]);
   }
}

int vm_manager::load_vm_default(int vm_type, const char* vm_path) {
   void *handle = dlopen(vm_path, RTLD_LAZY | RTLD_LOCAL);
   if (handle == NULL) {
      return 0;
   }

   fn_setcode setcode = (fn_setcode)dlsym(handle, "setcode");
   if (setcode == NULL) {
      return 0;
   }

   fn_apply apply = (fn_apply)dlsym(handle, "apply");
   if (apply == NULL) {
      return 0;
   }

   std::unique_ptr<vm_calls> calls = std::make_unique<vm_calls>();
   calls->version = 0;
   calls->handle = handle;
   calls->setcode = setcode;
   calls->apply = apply;
   printf("loading %s %p %p\n", vm_path, setcode, apply);
   vm_map[vm_type] = std::move(calls);
   return 1;
}

int vm_manager::check_new_version(int vm_type, uint64_t vm_name) {
   uint64_t vm_store = N(vmstore);

   int itr = db_api::get().db_find_i64(vm_store, vm_store, vm_store, vm_name);
   if (itr < 0) {
      return 0;
   }

   size_t native_size = 0;
   const char* code = db_api::get().db_get_i64_exex(itr, &native_size);
   uint32_t type = *(uint32_t*)code;
   uint32_t version = *(uint32_t*)&code[4];
   if (vm_type != type) {
      wlog("+++++++++type not match");
      return 0; //type not match
   }
   auto _itr = vm_map.find(vm_type);
   if (_itr == vm_map.end()) {
      return 1;
   }

   if (version > _itr->second->version) {
      return 1;
   }
   return 0;
}

int vm_manager::load_vm(int vm_type, uint64_t vm_name) {
   uint64_t vm_store = N(vmstore);

   int itr = db_api::get().db_find_i64(vm_store, vm_store, vm_store, vm_name);
   if (itr < 0) {
      return 0;
   }

   size_t native_size = 0;
   const char* code = db_api::get().db_get_i64_exex(itr, &native_size);
   uint32_t type = *(uint32_t*)code;
   uint32_t version = *(uint32_t*)&code[4];

   char vm_path[128];
   sprintf(vm_path, "%s.%d",name(vm_name).to_string().c_str(), version);

   wlog("loading vm ${n1}: ${n2}", ("n1", name(vm_name).to_string())("n2", vm_path));

   struct stat _s;
   if (stat(vm_path, &_s) == 0) {
      //
   } else {
      std::ofstream out(vm_path, std::ios::binary | std::ios::out);
      out.write(&code[8], native_size - 8);
      out.close();
   }

   void *handle = dlopen(vm_path, RTLD_LAZY | RTLD_LOCAL);
   if (handle == NULL) {
      return 0;
   }

   fn_setcode setcode = (fn_setcode)dlsym(handle, "setcode");
   if (setcode == NULL) {
      return 0;
   }

   fn_apply apply = (fn_apply)dlsym(handle, "apply");
   if (apply == NULL) {
      return 0;
   }

   std::unique_ptr<vm_calls> calls = std::make_unique<vm_calls>();
   calls->handle = handle;
   calls->version = version;
   calls->setcode = setcode;
   calls->apply = apply;
   printf("loading %s %p %p\n", vm_path, setcode, apply);
   vm_map[vm_type] = std::move(calls);

   return 1;
}

int vm_manager::setcode(int type, uint64_t account) {

   if (check_new_version(type, vm_names[type])) {
      load_vm(type, vm_names[type]);
   }

   auto itr = vm_map.find(type);
   if (itr == vm_map.end()) {
      return -1;
   }
   return itr->second->setcode(account);
}

int vm_manager::apply(int type, uint64_t receiver, uint64_t account, uint64_t act) {

   if (check_new_version(type, vm_names[type])) {
      load_vm(type, vm_names[type]);
   }

   auto itr = vm_map.find(type);
   if (itr == vm_map.end()) {
      return -1;
   }
   return itr->second->apply(receiver, account, act);
}

