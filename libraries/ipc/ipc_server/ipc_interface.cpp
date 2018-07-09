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
using namespace std;

unique_ptr<boost::thread> client_monitor_thread;
vector<shared_ptr<boost::thread>> server_threads;

vector<shared_ptr<boost::process::child>> client_processes;

static struct vm_api s_vm_api = {};
static const char* default_ipc_path = "/tmp";

extern "C" int _start_server(const char* ipc_file, int vm_type);
extern "C" int server_on_apply(uint64_t receiver, uint64_t account, uint64_t action, char** err, int* len);
static const char* vm_names[] = {
      "binaryen",
      "py",
      "eth",
      "wavm",
};

void vm_init() {
   client_monitor_thread.reset(new boost::thread([]{
         do {
            while (!s_vm_api.app_init_finished()) {
               boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            }
            for (int vm_type=0;vm_type<4;vm_type++) {
               char cmd[256];
               char option[128];
               static const char* format = "../libraries/ipc/ipc_client/ipc_client %s/%s.ipc %d";
               int ret = s_vm_api.get_option("ipc-path", option, sizeof(option));
               if (ret) {
                  snprintf(cmd, sizeof(cmd), format, option, vm_names[vm_type], vm_type);
               } else {
                  snprintf(cmd, sizeof(cmd), format, default_ipc_path, vm_names[vm_type], vm_type);
               }
               wlog("start ${n}", ("n", cmd));
               ipstream pipe_stream;
               shared_ptr<boost::process::child> client_process;
               client_process.reset(new child((const char*)cmd, std_out > pipe_stream));
               client_processes.push_back(client_process);
               /*
               std::string line;
               while (pipe_stream && std::getline(pipe_stream, line) && !line.empty()) {
                  std::cerr << line << std::endl;
               }
               */
            }
            for (auto& p: client_processes) {
               p->wait();
            }
         } while(false);
   }));

   for (int vm_type=0;vm_type<4;vm_type++) {
      shared_ptr<boost::thread> server_thread;
      server_thread.reset(new boost::thread([vm_type]{
            while (!s_vm_api.app_init_finished()) {
               boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            }

            char option[128];
            char ipc_file[128];
            int ret = s_vm_api.get_option("ipc-path", option, sizeof(option));
            if (ret) {
               snprintf(ipc_file, sizeof(ipc_file), "%s/%s.ipc", option, vm_names[vm_type]);
            } else {
               snprintf(ipc_file, sizeof(ipc_file), "%s/%s.ipc", default_ipc_path, vm_names[vm_type]);
            }
            _start_server(ipc_file, vm_type);
      }));
      server_threads.push_back(server_thread);
   }
}

void vm_deinit() {
   wlog("vm_deinit");
   for (auto& p: client_processes) {
      if (!p) {
         continue;
      }
      if (p->running()) {
         p->terminate();
      }
   }
}

void vm_register_api(struct vm_api* api) {
   s_vm_api = *api;
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
