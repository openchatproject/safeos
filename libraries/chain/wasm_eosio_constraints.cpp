#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/wasm_eosio_validators.hpp>
#include <eosio/chain/wasm_binary_ops.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include "IR/Module.h"
#include "IR/Operators.h"
#include "WASM/WASM.h"

namespace eosio { namespace chain {

namespace wasm_rewriter {
   uint32_t instruction_counter::icnt = 0;
   int32_t checktime_injector::checktime_idx = -1;
} // namespace wasm_rewriter

using namespace IR;

namespace wasm_constraints {

void noop_validation_visitor::validate( Module& m ) {
   // just pass
}

void blacklist_validation_visitor::validate( Module& m ) {
    
}

void memories_validation_visitor::validate( Module& m ) {
   if ( m.memories.defs.size() && m.memories.defs[0].type.size.min > wasm_constraints::maximum_linear_memory/(64*1024) )
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract initial memory size must be less than or equal to ${k}KiB", 
            ("k", wasm_constraints::maximum_linear_memory/1024));
}

void data_segments_validation_visitor::validate( Module& m ) {
   for ( const DataSegment& ds : m.dataSegments ) {
      if ( ds.baseOffset.type != InitializerExpression::Type::i32_const )
         FC_THROW_EXCEPTION( wasm_execution_error, "Smart contract has unexpected memory base offset type" );

      if ( static_cast<uint32_t>( ds.baseOffset.i32 ) + ds.data.size() > wasm_constraints::maximum_linear_memory_init )
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract data segments must lie in first ${k}KiB", 
               ("k", wasm_constraints::maximum_linear_memory_init/1024));
   }
}

void tables_validation_visitor::validate( Module& m ) {
   if ( m.tables.defs.size() && m.tables.defs[0].type.size.min > wasm_constraints::maximum_table_elements )
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract table limited to ${t} elements", 
            ("t", wasm_constraints::maximum_table_elements));
}

void globals_validation_visitor::validate( Module& m ) {
   unsigned mutable_globals_total_size = 0;
   for(const GlobalDef& global_def : m.globals.defs) {
      if(!global_def.type.isMutable)
         continue;
      switch(global_def.type.valueType) {
         case ValueType::any:
         case ValueType::num:
            FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract has unexpected global definition value type");
         case ValueType::i64:
         case ValueType::f64:
            mutable_globals_total_size += 4;
         case ValueType::i32:
         case ValueType::f32:
            mutable_globals_total_size += 4;
      }
   }
   if(mutable_globals_total_size > wasm_constraints::maximum_mutable_globals)
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract has more than ${k} bytes of mutable globals",
            ("k", wasm_constraints::maximum_mutable_globals));
}
/*
struct wasm_opcode_no_disposition_exception {
   string opcode_name;
};
*/
struct nop_opcode_visitor {
   typedef void Result;

   #define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
         virtual void name(Imm) {throw wasm_opcode_no_disposition_exception{nameString}; }
   ENUM_OPERATORS(VISIT_OPCODE)
   #undef VISIT_OPCODE

   void unknown(Opcode) {
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract encountered unknown opcode");
   }
};

struct eosio_constraints_visitor : public nop_opcode_visitor {
   //While it's possible to access beyond 1MiB by giving an offset that's 1MiB-1 and
   // an 8 byte data type, that's fine. There will be enough of a guard on the end
   // of 1MiB where it's not a problem
   void fail_large_offset(U32 offset) {
      if(offset >= wasm_constraints::maximum_linear_memory)
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract used an invalid large memory store/load offset");
   }
   void i32_load     (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void i64_load     (LoadOrStoreImm<3> imm) override { fail_large_offset(imm.offset); }
   void i32_load8_s  (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i32_load8_u  (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i32_load16_s (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i32_load16_u (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_load8_s  (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i64_load8_u  (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i64_load16_s (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_load16_u (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_load32_s (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void i64_load32_u (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void i32_store    (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void i64_store    (LoadOrStoreImm<3> imm) override { fail_large_offset(imm.offset); }
   void i32_store8   (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i32_store16  (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_store8   (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i64_store16  (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_store32  (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }

#if 0  //These are caught by ENUM_FLOAT_NONCONTROL_NONPARAMETRIC_OPERATORS below, but should be placed back when float is supported
   void f32_load     (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void f64_load     (LoadOrStoreImm<3> imm) override { fail_large_offset(imm.offset); }
   void f32_store    (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void f64_store    (LoadOrStoreImm<3> imm) override { fail_large_offset(imm.offset); }
#endif

   #define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
      void name(Imm) override { FC_THROW_EXCEPTION(wasm_execution_error, "Smart contracts may not use WASM memory operators"); }
   ENUM_MEMORY_OPERATORS(VISIT_OPCODE);
   #undef VISIT_OPCODE

   #define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
      void name(Imm) override { FC_THROW_EXCEPTION(wasm_execution_error, "Smart contracts may not use any floating point opcodes"); }
   ENUM_FLOAT_NONCONTROL_NONPARAMETRIC_OPERATORS(VISIT_OPCODE);
   #undef VISIT_OPCODE

   //Allow all these through untouched/////////////////////////
   #define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
      void name(Imm) override {}
   ENUM_CONTROL_OPERATORS(VISIT_OPCODE);
   ENUM_PARAMETRIC_OPERATORS(VISIT_OPCODE);
   //annoyingly the rest need to be defined manually given current design
   VISIT_OPCODE(0x01,nop,"nop",NoImm,NULLARY(none));
   VISIT_OPCODE(0x41,i32_const,"i32.const",LiteralImm<I32>,NULLARY(i32));
   VISIT_OPCODE(0x42,i64_const,"i64.const",LiteralImm<I64>,NULLARY(i64));

   VISIT_OPCODE(0x45,i32_eqz,"i32.eqz",NoImm,UNARY(i32,i32));
   VISIT_OPCODE(0x46,i32_eq,"i32.eq",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x47,i32_ne,"i32.ne",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x48,i32_lt_s,"i32.lt_s",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x49,i32_lt_u,"i32.lt_u",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x4a,i32_gt_s,"i32.gt_s",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x4b,i32_gt_u,"i32.gt_u",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x4c,i32_le_s,"i32.le_s",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x4d,i32_le_u,"i32.le_u",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x4e,i32_ge_s,"i32.ge_s",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x4f,i32_ge_u,"i32.ge_u",NoImm,BINARY(i32,i32));

   VISIT_OPCODE(0x50,i64_eqz,"i64.eqz",NoImm,UNARY(i64,i32));
   VISIT_OPCODE(0x51,i64_eq,"i64.eq",NoImm,BINARY(i64,i32));
   VISIT_OPCODE(0x52,i64_ne,"i64.ne",NoImm,BINARY(i64,i32));
   VISIT_OPCODE(0x53,i64_lt_s,"i64.lt_s",NoImm,BINARY(i64,i32));
   VISIT_OPCODE(0x54,i64_lt_u,"i64.lt_u",NoImm,BINARY(i64,i32));
   VISIT_OPCODE(0x55,i64_gt_s,"i64.gt_s",NoImm,BINARY(i64,i32));
   VISIT_OPCODE(0x56,i64_gt_u,"i64.gt_u",NoImm,BINARY(i64,i32));
   VISIT_OPCODE(0x57,i64_le_s,"i64.le_s",NoImm,BINARY(i64,i32));
   VISIT_OPCODE(0x58,i64_le_u,"i64.le_u",NoImm,BINARY(i64,i32));
   VISIT_OPCODE(0x59,i64_ge_s,"i64.ge_s",NoImm,BINARY(i64,i32));
   VISIT_OPCODE(0x5a,i64_ge_u,"i64.ge_u",NoImm,BINARY(i64,i32));

   VISIT_OPCODE(0x67,i32_clz,"i32.clz",NoImm,UNARY(i32,i32));
   VISIT_OPCODE(0x68,i32_ctz,"i32.ctz",NoImm,UNARY(i32,i32));
   VISIT_OPCODE(0x69,i32_popcnt,"i32.popcnt",NoImm,UNARY(i32,i32));

   VISIT_OPCODE(0x6a,i32_add,"i32.add",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x6b,i32_sub,"i32.sub",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x6c,i32_mul,"i32.mul",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x6d,i32_div_s,"i32.div_s",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x6e,i32_div_u,"i32.div_u",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x6f,i32_rem_s,"i32.rem_s",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x70,i32_rem_u,"i32.rem_u",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x71,i32_and,"i32.and",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x72,i32_or,"i32.or",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x73,i32_xor,"i32.xor",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x74,i32_shl,"i32.shl",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x75,i32_shr_s,"i32.shr_s",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x76,i32_shr_u,"i32.shr_u",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x77,i32_rotl,"i32.rotl",NoImm,BINARY(i32,i32));
   VISIT_OPCODE(0x78,i32_rotr,"i32.rotr",NoImm,BINARY(i32,i32));

   VISIT_OPCODE(0x79,i64_clz,"i64.clz",NoImm,UNARY(i64,i64));
   VISIT_OPCODE(0x7a,i64_ctz,"i64.ctz",NoImm,UNARY(i64,i64));
   VISIT_OPCODE(0x7b,i64_popcnt,"i64.popcnt",NoImm,UNARY(i64,i64));

   VISIT_OPCODE(0x7c,i64_add,"i64.add",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x7d,i64_sub,"i64.sub",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x7e,i64_mul,"i64.mul",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x7f,i64_div_s,"i64.div_s",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x80,i64_div_u,"i64.div_u",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x81,i64_rem_s,"i64.rem_s",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x82,i64_rem_u,"i64.rem_u",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x83,i64_and,"i64.and",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x84,i64_or,"i64.or",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x85,i64_xor,"i64.xor",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x86,i64_shl,"i64.shl",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x87,i64_shr_s,"i64.shr_s",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x88,i64_shr_u,"i64.shr_u",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x89,i64_rotl,"i64.rotl",NoImm,BINARY(i64,i64));
   VISIT_OPCODE(0x8a,i64_rotr,"i64.rotr",NoImm,BINARY(i64,i64));

   VISIT_OPCODE(0xa7,i32_wrap_i64,"i32.wrap/i64",NoImm,UNARY(i64,i32));
   VISIT_OPCODE(0xac,i64_extend_s_i32,"i64.extend_s/i32",NoImm,UNARY(i32,i64));
   VISIT_OPCODE(0xad,i64_extend_u_i32,"i64.extend_u/i32",NoImm,UNARY(i32,i64));
   #undef VISIT_OPCODE
   
};
/*
void check_wasm_opcode_dispositions() {
   eosio_constraints_visitor visitor;
   vector<string> opcodes_without_disposition;
   vector<string> opcodes_allowed;
   #define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
         try { \
         visitor.name(Imm{}); \
         opcodes_allowed.push_back(nameString); \
         } \
         catch(wasm_execution_error& e) {} \
         catch(wasm_opcode_no_disposition_exception& e) { \
            opcodes_without_disposition.push_back(e.opcode_name); \
         }
   ENUM_OPERATORS(VISIT_OPCODE)
   #undef VISIT_OPCODE

   if(opcodes_without_disposition.size()) {
      elog("WASM opcode disposition not defined for opcodes:");
      for(const string& g : opcodes_without_disposition)
         elog("  ${p}", ("p", g));
      FC_THROW("All opcodes must have a constraint");
   }
}

void validate_eosio_wasm_constraints(const Module& m) {
   if(m.memories.defs.size() && m.memories.defs[0].type.size.min > wasm_constraints::maximum_linear_memory/(64*1024))
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract initial memory size must be less than or equal to ${k}KiB", ("k", wasm_constraints::maximum_linear_memory/1024));

   for(const DataSegment& ds : m.dataSegments) {
      if(ds.baseOffset.type != InitializerExpression::Type::i32_const)
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract has unexpected memory base offset type");
      if(static_cast<uint32_t>(ds.baseOffset.i32) + ds.data.size() > wasm_constraints::maximum_linear_memory_init)
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract data segments must lie in first ${k}KiB", ("k", wasm_constraints::maximum_linear_memory_init/1024));
   }

   if(m.tables.defs.size() && m.tables.defs[0].type.size.min > wasm_constraints::maximum_table_elements)
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract table limited to ${t} elements", ("t", wasm_constraints::maximum_table_elements));

   unsigned mutable_globals_total_size = 0;
   for(const GlobalDef& global_def : m.globals.defs) {
      if(!global_def.type.isMutable)
         continue;
      switch(global_def.type.valueType) {
         case ValueType::any:
         case ValueType::num:
            FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract has unexpected global definition value type");
         case ValueType::i64:
         case ValueType::f64:
            mutable_globals_total_size += 4;
         case ValueType::i32:
         case ValueType::f32:
            mutable_globals_total_size += 4;
      }
   }
   if(mutable_globals_total_size > wasm_constraints::maximum_mutable_globals)
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract has more than ${k} bytes of mutable globals", ("k", wasm_constraints::maximum_mutable_globals));

   //Now check for constraints on each opcode.
   //Some of the OperatorDecoderStream users inside of WAVM track the control stack and quit parsing from
   // OperatorDecoderStream when the control stack is empty (since that would indicate unreachable code).
   // Not doing that here, yet, since it's not clear it's required for the purpose of the validation
   eosio_constraints_visitor visitor;
   for(const FunctionDef& fd : m.functions.defs) {
      OperatorDecoderStream decoder(fd.code);
      while(decoder) {
         decoder.decodeOp(visitor);
      }
   }
   */
}
}}
