#include <appbase/application.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <eosio/history_plugin/history_plugin.hpp>
#include <eosio/net_plugin/net_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/chain_api_plugin/chain_api_plugin.hpp>
#include <eosio/wallet_api_plugin/wallet_api_plugin.hpp>
#include <eosio/history_api_plugin/history_api_plugin.hpp>

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <Python.h>

#include <mutex>
#include <condition_variable>

#include <config.hpp>

//std::shared_ptr<std::mutex> m(new std::mutex());


using namespace appbase;
using namespace eosio;

static bool init_finished = false;
static bool shutdown_finished = false;
static std::condition_variable cv;
static std::mutex cv_m;

static int g_argc = 0;
static char** g_argv = NULL;

//eosapi.pyx
void py_exit();

extern "C" {
   void init_api();
   PyObject* PyInit_eosapi();
   PyObject* PyInit_wallet();
   PyObject* PyInit_net();

//only used in debug
   PyObject* PyInit_eoslib();
   PyObject* PyInit_db();
   PyObject* PyInit_rodb();
   PyObject* PyInit_debug();
   PyObject* PyInit_python_contract();
   PyObject* PyInit__struct();
#if 0
   PyObject* PyInit__ssl();
   PyObject* PyInit__posixsubprocess();
   PyObject* PyInit_readline();
   PyObject* PyInit_select();
   PyObject* PyInit_math();
   PyObject* PyInit__socket();
   PyObject* PyInit_array();
#endif

   //vm_manager.cpp
   void vm_deinit_all();
   void vm_api_init();
}

bool is_init_finished() {
   return init_finished;
}


void init_console() {
//   init_api();

#if 0
   PyImport_AppendInittab("_struct", PyInit__struct);
   PyImport_AppendInittab("_ssl", PyInit__ssl);
   PyImport_AppendInittab("_socket", PyInit__socket);
   PyImport_AppendInittab("_posixsubprocess", PyInit__posixsubprocess);

   PyImport_AppendInittab("readline", PyInit_readline);
   PyImport_AppendInittab("select", PyInit_select);
   PyImport_AppendInittab("math", PyInit_math);
   PyImport_AppendInittab("array", PyInit_array);
#endif

   Py_InitializeEx(0);

#if 0
   PyImport_ImportModule("_struct");
   PyImport_ImportModule("_ssl");
   PyImport_ImportModule("_socket");
   PyImport_ImportModule("_posixsubprocess");
   PyImport_ImportModule("readline");
   PyImport_ImportModule("select");
   PyImport_ImportModule("math");
   PyImport_ImportModule("array");
#endif

#ifdef WITH_THREAD
   PyEval_InitThreads();
#endif
   PyInit_wallet();
   PyInit_eosapi();
   PyInit_net();

   PyInit_rodb();
   PyInit_debug();
//   PyInit_python_contract();

   PyRun_SimpleString(
       "import sys;"
        "sys.path.append('../../programs/pyeos');"
        "sys.path.append('../../programs/pyeos/contracts');"
   );
   PyRun_SimpleString("import readline");
   PyRun_SimpleString("import wallet");
   PyRun_SimpleString("import eosapi;");
   PyRun_SimpleString("import debug;");
   PyRun_SimpleString("from imp import reload;");
   PyRun_SimpleString("import initeos;initeos.preinit()");

}

void start_eos() {
   try {
      app().startup();
      wlog("+++++++++++app().startup() done!");
      {
          //std::lock_guard<std::mutex> lk(cv_m);
      }
      cv.notify_all();
      init_finished = true;
      app().exec();
   } FC_LOG_AND_DROP();
   wlog("+++++++++++app exec done!!");
   {
       std::lock_guard<std::mutex> lk(cv_m);
       init_finished = true;
       shutdown_finished = true;
   }
   cv.notify_all();
}

int main(int argc, char** argv) {
   g_argc = argc;
   g_argv = argv;

   vm_api_init();

   app().init_args(argc, argv);
   init_console();

   app().set_version(eosio::nodeos::config::version);
   app().register_plugin<history_plugin>();
   app().register_plugin<history_api_plugin>();

   app().register_plugin<chain_api_plugin>();
   app().register_plugin<wallet_api_plugin>();
   app().register_plugin<producer_plugin>();


//      if(!app().initialize_ex(g_argc, g_argv, "chain_plugin", "http_plugin", "net_plugin", "producer_plugin")) {
   try {
      try {
         if(!app().initialize<chain_plugin, http_plugin, net_plugin, history_plugin, producer_plugin>(g_argc, g_argv)) {
            return -1;
         }
      } FC_LOG_AND_RETHROW();
   } catch (...) {
      return -1;
   }

   bool readonly = app().has_option("read-only");
   if (readonly) {
      wlog("+++++++++read only mode");
      PyRun_SimpleString("import initeos");
      PyRun_SimpleString("initeos.start_console()");
      py_exit();
      return 0;
   }

   std::unique_lock<std::mutex> lk(cv_m);
   boost::thread t(start_eos);
   cv.wait(lk);

   wlog("running console...");
   if (app().interactive_mode()) {
      PyRun_SimpleString("import initeos");
      PyRun_SimpleString("initeos.start_console()");
      wlog("quit app...");
      appbase::app().quit();
   }

   cv.wait(lk, [](){return shutdown_finished;});
   wlog("exiting...");
   vm_deinit_all();
   py_exit();

   return 0;
}


