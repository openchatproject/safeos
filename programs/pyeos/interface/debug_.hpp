#pragma once
#include <string>
//#include <micropython/mpeoslib.h>
#include <Python.h>

using namespace std;

void debug_test();

void set_debug_mode(int mode);
int get_debug_mode();

void run_code_(std::string code);

//mpeoslib.cpp
struct mpapi& get_mpapi();

void py_debug_enable_(int enable);
bool py_debug_enabled_();

void wasm_debug_enable_(int enable);
bool wasm_debug_enabled_();

void set_debug_contract_(string& _account, string& path);
uint64_t get_debug_contract_();

int mp_is_account2(string& account);

void wasm_enable_native_contract_(bool b);
bool wasm_is_native_contract_enabled_();

//mpeoslib.cpp
void mp_set_max_execution_time_(int _max);

//application.cpp
void app_set_debug_mode_(bool d);

uint64_t wasm_test_action_(const char* cls, const char* method);

void block_log_test_(string& path, int start_block, int end_block);
void block_log_get_raw_actions_(string& path, int start, int end);

int block_on_action(int block, PyObject* trx);
PyObject* block_log_get_block_(string& path, int block_num);

bool hash_option_(const char* option);


uint64_t usage_accumulator_new_();
void usage_accumulator_add_(uint64_t p, uint64_t units, uint32_t ordinal, uint32_t window_size);
void usage_accumulator_get_(uint64_t p, uint64_t& value_ex, uint64_t& consumed);
void usage_accumulator_release_(uint64_t p);

uint64_t acc_get_used_(uint64_t value_ex);


void add_trusted_account_(uint64_t account);
void remove_trusted_account_(uint64_t account);

