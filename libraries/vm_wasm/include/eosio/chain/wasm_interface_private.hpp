#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/webassembly/wavm.hpp>
#include <eosio/chain/webassembly/binaryen.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <fc/scoped_exit.hpp>

#include "IR/Module.h"
#include "Runtime/Intrinsics.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "IR/Validate.h"

#include <dlfcn.h>


using namespace fc;
using namespace eosio::chain::webassembly;
using namespace IR;
using namespace Runtime;

#if defined(__APPLE__) && defined(__MACH__)
   #define NATIVE_PLATFORM 1
#elif defined(__linux__)
   #define NATIVE_PLATFORM 2
#elif defined(_WIN64)
   #define NATIVE_PLATFORM 3
#else
   #error Not Supported Platform
#endif

void resume_billing_timer();
void pause_billing_timer();
const char* get_code( uint64_t receiver, size_t* size );

namespace eosio { namespace chain {
   void register_vm_api(void* handle);
   typedef void (*fn_apply)(uint64_t receiver, uint64_t account, uint64_t act);

   struct native_code_cache {
         uint32_t version;
         void *handle;
         fn_apply apply;
   };

   struct wasm_interface_impl {
      wasm_interface_impl(wasm_interface::vm_type vm) {
         if(vm == wasm_interface::vm_type::wavm)
            runtime_interface = std::make_unique<webassembly::wavm::wavm_runtime>();
         else if(vm == wasm_interface::vm_type::binaryen)
            runtime_interface = std::make_unique<webassembly::binaryen::binaryen_runtime>();
         else
            FC_THROW("wasm_interface_impl fall through");
         //init_native_contract();
      }

      std::vector<uint8_t> parse_initial_memory(const Module& module) {
         std::vector<uint8_t> mem_image;

         for(const DataSegment& data_segment : module.dataSegments) {
            FC_ASSERT(data_segment.baseOffset.type == InitializerExpression::Type::i32_const);
            FC_ASSERT(module.memories.defs.size());
            const U32 base_offset = data_segment.baseOffset.i32;
            const Uptr memory_size = (module.memories.defs[0].type.size.min << IR::numBytesPerPageLog2);
            if(base_offset >= memory_size || base_offset + data_segment.data.size() > memory_size)
               FC_THROW_EXCEPTION(wasm_execution_error, "WASM data segment outside of valid memory range");
            if(base_offset + data_segment.data.size() > mem_image.size())
               mem_image.resize(base_offset + data_segment.data.size(), 0x00);
            memcpy(mem_image.data() + base_offset, data_segment.data.data(), data_segment.data.size());
         }

         return mem_image;
      }

      std::unique_ptr<wasm_instantiated_module_interface>& get_instantiated_module( const uint64_t& code_id )
      {
         size_t size = 0;
         const char* code = get_code( code_id, &size );

         auto it = instantiation_cache.find(code_id);
         if(it == instantiation_cache.end()) {
            auto timer_pause = fc::make_scoped_exit([&](){
               resume_billing_timer();
            });
            pause_billing_timer();
            IR::Module module;
            try {
               Serialization::MemoryInputStream stream((const U8*)code, size);
               WASM::serialize(stream, module);
               module.userSections.clear();
            } catch(const Serialization::FatalSerializationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            } catch(const IR::ValidationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            }

            wasm_injections::wasm_binary_injection injector(module);
            injector.inject();

            std::vector<U8> bytes;
            try {
               Serialization::ArrayOutputStream outstream;
               WASM::serialize(outstream, module);
               bytes = outstream.getBytes();
            } catch(const Serialization::FatalSerializationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            } catch(const IR::ValidationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            }
            it = instantiation_cache.emplace(code_id, runtime_interface->instantiate_module((const char*)bytes.data(), bytes.size(), parse_initial_memory(module))).first;
         }
         return it->second;
      }

      std::unique_ptr<wasm_runtime_interface> runtime_interface;
      map<uint64_t, std::unique_ptr<native_code_cache>> native_cache;
      map<uint64_t, std::unique_ptr<wasm_instantiated_module_interface>> instantiation_cache;
   };

} } // eosio::chain
