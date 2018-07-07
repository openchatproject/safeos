#include <boost/process.hpp>
#include <boost/thread.hpp>

#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include <eosiolib_native/vm_api.h>

using namespace boost::process;

std::unique_ptr<boost::thread> client_monitor_thread;
std::unique_ptr<boost::thread> server_thread;
std::unique_ptr<boost::process::child> client_process;

extern "C" int _start_server(const char* ipc_path);
extern "C" int server_on_apply(uint64_t receiver, uint64_t account, uint64_t action, char** err, int* len);

void vm_init() {
   client_monitor_thread.reset(new boost::thread([]{
         do {
            ipstream pipe_stream;
            client_process.reset(new child("../libraries/ipc/ipc_client/ipc_client", std_out > pipe_stream));
            std::string line;
            while (pipe_stream && std::getline(pipe_stream, line) && !line.empty()) {
               std::cerr << line << std::endl;
            }
            client_process->wait();
            wlog("ipc_client exited.");
         } while(false);
   }));

   server_thread.reset(new boost::thread([]{
         _start_server("/tmp/pyeos.ipc");
   }));
}

void vm_deinit() {
   wlog("vm_deinit");
   client_process->terminate();
}

void vm_register_api(struct vm_api* api) {

}

int vm_setcode(uint64_t account) {
   wlog("vm_setcode");
   return -1;
}

int vm_apply(uint64_t receiver, uint64_t account, uint64_t act) {
   wlog("vm_apply");
   char *err;
   int len;
   int ret = server_on_apply(receiver, account, act, &err, &len);
   if (ret != 1) {
      std::string msg(err, len);
      free(err);
      throw fc::exception(0, "RPC", msg);
   }
   return 1;
}

uint64_t vm_call(const char* act, uint64_t* args, int argc) {
   return 0;
}

int vm_preload(uint64_t account) {
   return -1;
}

int vm_unload(uint64_t account) {
   return -1;
}
