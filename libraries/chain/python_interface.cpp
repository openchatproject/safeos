#include <boost/function.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/thread/thread.hpp>
#include <eos/chain/python_interface.hpp>
#include <eos/chain/wasm_interface.hpp>
#include <eos/chain/chain_controller.hpp>
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "Runtime/Runtime.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"
#include "IR/Module.h"
#include "IR/Operators.h"
#include "IR/Validate.h"
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/account_object.hpp>
#include <chrono>
#include <appbase/application.hpp>

using namespace std;
int python_load(string& name, string& code, string* error);
int python_call(std::string &name, std::string &function,std::vector<uint64_t> args, string* error);
void stop_tracemalloc();

namespace eos {
namespace chain {
using namespace IR;
using namespace Runtime;
typedef boost::multiprecision::cpp_bin_float_50 DOUBLE;


int python_load_with_gil(string& name, string& code) {
   int ret = -1;
   string error;
   bool need_hold_gil;
   PyGILState_STATE save;

   need_hold_gil = boost::this_thread::get_id() == appbase::app().get_thread_id();

   if (need_hold_gil) {
      save = PyGILState_Ensure();
   }

   try {
      ret = python_load(name, code, &error);
   } catch (...) {
      if (need_hold_gil) {
         PyGILState_Release(save);
      }
      stop_tracemalloc();
      throw;
   }

   if (need_hold_gil) {
      PyGILState_Release(save);
   }

   if (ret < 0) {
      throw fc::exception(19, name, error);
   }
   return ret;
}

int python_call_with_gil(std::string &name, std::string &function, std::vector<uint64_t> args) {
   int ret = 0;
   string error;
   PyGILState_STATE save;
   bool need_hold_gil;
   boost::this_thread::get_id();

   need_hold_gil = boost::this_thread::get_id() == appbase::app().get_thread_id();

   if (need_hold_gil) {
      save = PyGILState_Ensure();
   }

   try {
      ret = python_call(name, function, args, &error);
   } catch (...) {
      if (need_hold_gil) {
         PyGILState_Release(save);
      }
      stop_tracemalloc();
      throw;
   }

   if (need_hold_gil) {
      PyGILState_Release(save);
   }

   if (ret < 0) {
      throw fc::exception(19, name, error);
   }
   return ret;
}

int wasm_call_function(uint64_t code, uint64_t function,
      std::vector<uint64_t> args) {
//   ilog("wasm_call_function args.size:${n}",("n",args.size()));
   for (auto& n : args) {
      ilog("${n}",("n",n));
   }
   apply_context* c = python_interface::get().current_apply_context;
   return wasm_interface::get().call_function(*c, code, function, args);

}

python_interface::python_interface() {
}

template<typename Function, typename KeyType, int numberOfKeys>
int32_t validate(int32_t valueptr, int32_t valuelen, Function func) {

   static const uint32_t keylen = numberOfKeys * sizeof(KeyType);

   FC_ASSERT(valuelen >= keylen, "insufficient data passed");

   auto& wasm = python_interface::get();
   FC_ASSERT(wasm.current_apply_context, "no apply context found");

   char* value = memoryArrayPtr<char>(wasm.current_memory, valueptr, valuelen);
   KeyType* keys = reinterpret_cast<KeyType*>(value);

   valuelen -= keylen;
   value += keylen;

   return func(wasm.current_apply_context, keys, value, valuelen);
}

python_interface& python_interface::get() {
   static python_interface* python = nullptr;
   if (!python) {
      wlog("python_interface::init");
      python = new python_interface();
   }
   return *python;
}

int64_t python_interface::current_execution_time() {
   return (fc::time_point::now() - checktimeStart).count();
}

char* python_interface::vm_allocate(int bytes) {
   return 0;
}

U32 python_interface::vm_pointer_to_offset(char* ptr) {
   return U32(ptr - &memoryRef<char>(current_memory, 0));
}

void python_interface::vm_call(const char* function_name) {
   std::vector<uint64_t> args = { current_validate_context->msg.code.value,
         current_validate_context->msg.type.value };

   string module_name = current_module;
   string function_name_ = function_name;

   python_call_with_gil(module_name, function_name_, args);
}

void python_interface::vm_apply() {
   vm_call("apply");
}

void python_interface::vm_onInit() {
   vector<uint64_t> args;
   string module_name = current_module;
   string function_name = "init";
   python_call_with_gil(module_name, function_name, args);
}

void python_interface::validate(apply_context& c) {
   /*
    current_validate_context       = &c;
    current_precondition_context   = nullptr;
    current_apply_context          = nullptr;

    load( c.code, c.db );
    vm_validate();
    */
}

void python_interface::precondition(apply_context& c) {
   try {

      /*
       current_validate_context       = &c;
       current_precondition_context   = &c;

       load( c.code, c.db );
       vm_precondition();
       */

   }FC_CAPTURE_AND_RETHROW()
}

void python_interface::apply(apply_context& c) {
   try {
      current_validate_context = &c;
      current_precondition_context = &c;
      current_apply_context = &c;

      load(c.code, c.db);
      vm_apply();

   }FC_CAPTURE_AND_RETHROW()
}

void python_interface::init(apply_context& c) {
   try {
      current_validate_context = &c;
      current_precondition_context = &c;
      current_apply_context = &c;
      load(c.code, c.db);
      vm_onInit();
   }FC_CAPTURE_AND_RETHROW()
}

void python_interface::load(const AccountName& name, const chainbase::database& db) {
   const auto& recipient = db.get<account_object, by_name>(name);
   string module_name = string(name);
   string code = string((const char*) recipient.code.data(),
         recipient.code.size());
   current_module = module_name;
//   ilog("python_interface::load");
   python_load_with_gil(module_name, code);
}

void python_interface::set_current_context(apply_context& c) {
   current_validate_context = &c;
   current_precondition_context = &c;
   current_apply_context = &c;
}


uint32_t transactionCreate_() {
   auto& ptrx = python_interface::get().current_apply_context->create_pending_transaction();
   return ptrx.handle;
}

static void emplace_scope(const Name& scope, std::vector<Name>& scopes) {
   auto i = std::upper_bound( scopes.begin(), scopes.end(), scope);
   if (i == scopes.begin() || *(i - 1) != scope ) {
     scopes.insert(i, scope);
   }
}

void transactionRequireScope_(uint32_t handle, string& scope, uint32_t readOnly) {
   auto& ptrx = python_interface::get().current_apply_context->get_pending_transaction(handle);
   if(readOnly == 0) {
      emplace_scope(Name(scope), ptrx.scope);
   } else {
      emplace_scope(Name(scope), ptrx.readscope);
   }

   ptrx.check_size();
}


void transactionAddMessage_(uint32_t handle,uint32_t msg_handle) {
   auto apply_context  = python_interface::get().current_apply_context;
   auto& ptrx = apply_context->get_pending_transaction(handle);
   auto& pmsg = apply_context->get_pending_message(msg_handle);
   ptrx.messages.emplace_back(pmsg);
   ptrx.check_size();
   apply_context->release_pending_message(msg_handle);
}


void transactionSend_(uint32_t handle) {
   auto apply_context  = python_interface::get().current_apply_context;
   auto& ptrx = apply_context->get_pending_transaction(handle);

   EOS_ASSERT(ptrx.messages.size() > 0, tx_unknown_argument,
      "Attempting to send a transaction with no messages");

   apply_context->deferred_transactions.emplace_back(ptrx);
   apply_context->release_pending_transaction(handle);
}


void transactionDrop_(uint32_t handle) {
   python_interface::get().current_apply_context->release_pending_transaction(handle);
}


uint32_t messageCreate_(string& code, string& type, string& data) {
   auto& wasm  = python_interface::get();
   auto  mem   = wasm.current_memory;

   Bytes payload;
   if (data.size() > 0) {
      try {
         // memoryArrayPtr checks that the entire array of bytes is valid and
         // within the bounds of the memory segment so that transactions cannot pass
         // bad values in attempts to read improper memory
         payload.insert(payload.end(), data.c_str(), data.c_str()+data.size());
      } catch( const Runtime::Exception& e ) {
         FC_THROW_EXCEPTION(tx_unknown_argument, "Message data is not a valid memory range");
      }
   }

   auto& pmsg = wasm.current_apply_context->create_pending_message(Name(code), Name(type), payload);
   return pmsg.handle;
}


void messageRequirePermission_(uint32_t handle, string& account,string& permission) {
   auto apply_context  = python_interface::get().current_apply_context;
   // if this is not sent from the code account with the permission of "code" then we must
   // presently have the permission to add it, otherwise its a failure
   if (!(Name(account).value == apply_context->code.value && Name(permission) == Name("code"))) {
      apply_context->require_authorization(Name(account), Name(permission));
   }
   auto& pmsg = apply_context->get_pending_message(handle);
   pmsg.authorization.emplace_back(Name(account), Name(permission));
}


void messageSend_(uint32_t handle) {
   auto apply_context  = python_interface::get().current_apply_context;
   auto& pmsg = apply_context->get_pending_message(handle);

   apply_context->inline_messages.emplace_back(pmsg);
   apply_context->release_pending_message(handle);
}


void messageDrop_(uint32_t handle) {
   python_interface::get().current_apply_context->release_pending_message(handle);
}

/**
 * @} Transaction C API implementation
 */

}
}

typedef void (*fn_init_modules)(void);

static wchar_t env_home[MAXPATHLEN+1];

typedef void (*fn_init_modules)(void);
extern "C" int init_tinypy(fn_init_modules init_modules);

extern "C" PyObject* PyInit_eoslib();
extern "C" PyObject* PyInit_python_contract();

void init_modules(void) {
   PyInit_eoslib();
   PyInit_python_contract();
   PyRun_SimpleString("import eoslib");
   PyRun_SimpleString("import struct");
   PyRun_SimpleString("import logging");
   PyRun_SimpleString("import pickle");
   PyRun_SimpleString("import numpy");
   PyRun_SimpleString("import tracemalloc");
   PyRun_SimpleString("tracemalloc.set_max_malloc_size(2000*1024)");
}

void init_smart_contract() {
//   Py_NoSiteFlag = 1;
#if 0
   const char *chome = "../../libraries/tinypy/dist";
   mbstowcs(env_home, chome, sizeof(env_home)/sizeof(env_home[0]));
   Py_SetPythonHome(env_home);
#endif
   Py_InitializeEx(0);
   PyEval_InitThreads();
   init_modules();

}


extern "C" PyThreadState *tiny_PyEval_SaveThread(void) {
    return PyEval_SaveThread();
}

extern "C" void tiny_PyEval_RestoreThread(PyThreadState *tstate) {
    PyEval_RestoreThread(tstate);
}

