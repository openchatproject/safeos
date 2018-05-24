#include <appbase/application.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/history_plugin.hpp>
#include <eosio/net_plugin/net_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/chain_api_plugin/chain_api_plugin.hpp>
#include <eosio/wallet_api_plugin/wallet_api_plugin.hpp>
#include <eosio/history_api_plugin/history_api_plugin.hpp>


#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <boost/exception/diagnostic_information.hpp>


#include <Python.h>

using namespace appbase;
using namespace eosio;

static bool init_finished = false;
static bool shutdown_finished = false;
static bool rpc_server = false;
static bool rpc_client = false;
static bool eos_started = false;

//rpc_interface.cpp

bool is_init_finished() {
   return init_finished;
}

//eosapi_.cpp
int produce_block_();

void quit_app_() {
   if (eos_started) {
      produce_block_();
      app().quit();
      while (!shutdown_finished) {
         boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
      }
   }
}


extern "C" {
   void PyInit_eosapi();
   PyObject* PyInit_wallet();
   PyObject* PyInit_eoslib();
//only used in debug
   PyObject* PyInit_eoslib();
   PyObject* PyInit_db();
   PyObject* PyInit_debug();
   PyObject* PyInit_python_contract();

   int main_micropython(int argc, char **argv);
}

static int g_argc = 0;
static char** g_argv = NULL;


typedef void (*fn_init)();

void start_eos() {
   try {
      eos_started = true;
      app().register_plugin<net_plugin>();
      app().register_plugin<chain_api_plugin>();
      app().register_plugin<producer_plugin>();
      app().register_plugin<history_plugin>();
      app().register_plugin<history_api_plugin>();
      app().register_plugin<wallet_api_plugin>();

      if (!app().initialize<chain_plugin, http_plugin, net_plugin>(g_argc, g_argv)) {
         init_finished = true;
         shutdown_finished = true;
         return;
      }
      app().startup();
      init_finished = true;
      app().exec();

   } catch (const fc::exception& e) {
      elog("${e}", ("e", e.to_detail_string()));
   } catch (const boost::exception& e) {
      elog("${e}", ("e", boost::diagnostic_information(e)));
   } catch (const std::exception& e) {
      elog("${e}", ("e", e.what()));
   } catch (...) {
      elog("unknown exception");
   }
   init_finished = true;
   shutdown_finished = true;
}

extern "C" void init_api();

void init_console() {
   init_api();

   Py_Initialize();

#ifdef WITH_THREAD
   PyEval_InitThreads();
#endif
   PyRun_SimpleString("import readline");
   PyInit_wallet();
   PyInit_eosapi();
   PyInit_eoslib();
   PyInit_db();
   PyInit_debug();
   PyInit_python_contract();

   PyRun_SimpleString(
       "import sys;"
        "sys.path.append('../../programs/pyeos');"
        "sys.path.append('../../programs/pyeos/contracts');"
   );
   PyRun_SimpleString("import wallet");
   PyRun_SimpleString("import eosapi;");
   PyRun_SimpleString("import eoslib;");
   //   PyRun_SimpleString("import util;");
   PyRun_SimpleString("import debug;");
   PyRun_SimpleString("from imp import reload;");
   PyRun_SimpleString("eosapi.register_signal_handler()");
   PyRun_SimpleString("import initeos;initeos.preinit()");

}

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void py_exit();
void my_handler(int s){
   printf("Caught signal %d, exiting... \n",s);
   py_exit();
}

int install_ctrl_c_handler()
{
   struct sigaction sigIntHandler;

   sigIntHandler.sa_handler = my_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;

   sigaction(SIGINT, &sigIntHandler, NULL);
   return 0;
}

void interactive_console() {

   while (!init_finished) {
         boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
   }

   if (shutdown_finished) {
      return;
   }

   PyRun_SimpleString("from initeos import *");
   PyRun_SimpleString("init()");

   if (true) {//(app().interactive_mode()) {
      ilog("Starting interactive Python.");
//      PyRun_SimpleString("eosapi.register_signal_handler()");
      PyRun_InteractiveLoop(stdin, "<stdin>");
      Py_Finalize();
   } else {
      while (!shutdown_finished) {
         boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
      }
   }

}

typedef void (*fn_start_eos)();
typedef void (*fn_interactive_console)();

extern "C" void* micropy_load(const char *mod_name, const char *data, size_t len);
void init() {
   boost::thread eos( start_eos );
//   boost::thread console( interactive_console );
   wlog("+++++++=start console");
   interactive_console();
}

extern "C" int eos_main(int argc, char** argv) {
   g_argc = argc;
   g_argv = argv;

   install_ctrl_c_handler();

   init_console();

   boost::thread t( start_eos );
   interactive_console();
   return 0;
}


