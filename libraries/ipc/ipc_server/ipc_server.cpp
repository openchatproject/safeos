#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

#include <fc/log/logger.hpp>

#include <softfloat.hpp>
#include <eosiolib_native/vm_api.h>

#include <eosio/chain/db_api.hpp>
#include "thrift/blockingconcurrentqueue.h"
#include "thrift/readerwriterqueue.h"
#include "thrift/RpcService.h"
#include <mutex>

using namespace eosio::chain;

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;


using namespace  ::cpp;

using namespace moodycamel;

struct ApplyFinish {
   int32_t status;
   std::string errMsg;
};

class RpcServiceHandler : virtual public RpcServiceIf {
private:
   BlockingConcurrentQueue<Apply> apply_request_queue;
   BlockingConcurrentQueue<ApplyFinish> apply_finish_queue;
public:
  RpcServiceHandler() {
  }

  bool apply(Apply& apply) {
     apply_request_queue.enqueue(apply);
     return true;
  }

  bool apply_wait(ApplyFinish& finish) {
     if (!apply_finish_queue.wait_dequeue_timed(finish, std::chrono::milliseconds(100))) {
        return false;
     }
     return true;
  }

  void apply_request(Apply& _return) {
     wlog("++++++++++++++apply_request");
     apply_request_queue.wait_dequeue(_return);
     wlog("++++++++++++++apply_request return");
  }

  void apply_finish(const int32_t status, const std::string& errMsg) {
     wlog("apply_finish");
     apply_finish_queue.enqueue({status, errMsg});
  }

  void funCall(std::vector<std::string> & _return, const int64_t callTime, const std::string& funCode, const std::map<std::string, std::string> & paramMap) {
    // Your implementation goes here
    printf("funCall\n");
  }

  void read_action(std::string& _return) {
    // Your implementation goes here
    printf("read_action\n");
  }

  int32_t db_store_i64(const int64_t scope, const int64_t table, const int64_t payer, const int64_t id, const std::string& buffer) {
    return ::db_store_i64(scope, table, payer, id, buffer.c_str(), buffer.length());
  }

  void db_update_i64(const int32_t itr, const int64_t payer, const std::string& buffer) {
     ::db_update_i64(itr, payer, buffer.c_str(), buffer.length());
  }

  void db_remove_i64(const int32_t itr) {
     ::db_remove_i64(itr);
  }

  void db_get_i64(std::string& _return, const int32_t itr) {
     int size = ::db_get_i64(itr, nullptr, 0);
     char* buffer = new char[size];
     ::db_get_i64(itr, buffer, size);
     _return = std::string(buffer, size);
     delete[] buffer;
  }

  void db_next_i64(Result& _return, const int32_t itr) {
     uint64_t primary = 0;
     _return.status = ::db_next_i64(itr, &primary);
     _return.value = std::string((char*)&primary, sizeof(primary));
  }

  void db_previous_i64(Result& _return, const int32_t itr) {
     uint64_t primary = 0;
     _return.status = ::db_previous_i64(itr, &primary);
     _return.value = std::string((char*)&primary, sizeof(primary));
  }

  int32_t db_find_i64(const int64_t code, const int64_t scope, const int64_t table, const int64_t id) {
     return ::db_find_i64(code, scope, table, id);
  }

  int32_t db_lowerbound_i64(const int64_t code, const int64_t scope, const int64_t table, const int64_t id) {
     return ::db_lowerbound_i64(code, scope, table, id);
  }

  int32_t db_upperbound_i64(const int64_t code, const int64_t scope, const int64_t table, const int64_t id) {
     return ::db_upperbound_i64(code, scope, table, id);
  }

  int32_t db_end_i64(const int64_t code, const int64_t scope, const int64_t table) {
     return ::db_end_i64(code, scope, table);
  }

  void db_update_i64_ex(const int64_t scope, const int64_t payer, const int64_t table, const int64_t id, const std::string& buffer) {
     ::db_update_i64_ex(scope, payer, table, id, buffer.c_str(), buffer.length());
  }

  void db_remove_i64_ex(const int64_t scope, const int64_t payer, const int64_t table, const int64_t id) {
     ::db_remove_i64_ex(scope, payer, table, id);
  }

};


static std::map<int, ::apache::thrift::stdcxx::shared_ptr<RpcServiceHandler>> handler_map;
static std::mutex m1;

extern "C" int server_on_apply(uint64_t receiver, uint64_t account, uint64_t action, char** err, int* len) {
   Apply apply;
   apply.receiver = receiver;
   apply.account = account;
   apply.action = action;
   wlog("++++++++++++on_apply");

   int vm_type = db_api::get().get_code_type(receiver);

   auto& handler = handler_map[vm_type];
   handler->apply(apply);

   ApplyFinish finish = {};
   if (!handler->apply_wait(finish)) {
      std::string errMsg("++++on_apply: execution time out!");
      wlog(errMsg);
      *err = (char*)malloc(errMsg.length());
      memcpy(*err, errMsg.c_str(), errMsg.length());
      *len = errMsg.length();
      return 911;
   }

   *err = (char*)malloc(finish.errMsg.length());
   memcpy(*err, finish.errMsg.c_str(), finish.errMsg.length());
   *len = finish.errMsg.length();
   return finish.status;
}

extern "C" int _start_server(const char* ipc_path, int vm_type) {
//   rpc_register_cpp_apply_call();
   if (access(ipc_path, F_OK) != -1) {
      unlink(ipc_path);
   }

   if (access(ipc_path, F_OK) != -1) {
      elog("start server failed, ${n} already in use", ("n", ipc_path));
      return 0;
   }

   ::apache::thrift::stdcxx::shared_ptr<RpcServiceHandler> handler;
   {
      std::lock_guard<std::mutex> lock(m1);
      handler.reset(new RpcServiceHandler());
      handler_map[vm_type] = handler;
   }
//   ::apache::thrift::stdcxx::shared_ptr<RpcServiceHandler> handler(handler);
   ::apache::thrift::stdcxx::shared_ptr<TProcessor> processor(new RpcServiceProcessor(handler));
   ::apache::thrift::stdcxx::shared_ptr<TServerTransport> serverTransport(new TServerSocket(ipc_path));
   ::apache::thrift::stdcxx::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
   ::apache::thrift::stdcxx::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

   TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
   wlog("ipc server ready to go ${n}", ("n", ipc_path));
   server.serve();

  return 1;
}

