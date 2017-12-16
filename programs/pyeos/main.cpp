#include <appbase/application.hpp>

#include <eos/account_history_api_plugin/account_history_api_plugin.hpp>
#include <eos/account_history_plugin/account_history_plugin.hpp>
#include <eos/chain_api_plugin/chain_api_plugin.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/http_plugin/http_plugin.hpp>
#include <eos/net_plugin/net_plugin.hpp>
#include <eos/producer_plugin/producer_plugin.hpp>
#include <eos/py_plugin/py_plugin.hpp>
#include <eos/wallet_api_plugin/wallet_api_plugin.hpp>
#include <eos/wallet_plugin/wallet_plugin.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/chrono.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/thread/thread.hpp>


#include <Python.h>

using namespace appbase;
using namespace eosio;

static bool init_finished = false;
static bool shutdown_finished = false;


void quit_app_() {
   app().quit();
   while (!shutdown_finished) {
      boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
   }
}

extern "C" {
   void PyInit_eosapi();
   PyObject* PyInit_eostypes();
   PyObject* PyInit_wallet();
   //extern "C" PyObject* PyInit_hello();
   PyObject* PyInit_database();
   PyObject* PyInit_blockchain();
   PyObject* PyInit_util();
   PyObject* PyInit_debug();

   PyThreadState *tiny_PyEval_SaveThread(void);
   void tiny_PyEval_RestoreThread(PyThreadState *tstate);
}

static int g_argc = 0;
static char** g_argv = NULL;



void eos_main() {
   try {
      app().register_plugin<net_plugin>();
      app().register_plugin<chain_api_plugin>();
      app().register_plugin<producer_plugin>();
      app().register_plugin<account_history_api_plugin>();
      app().register_plugin<wallet_api_plugin>();
      app().register_plugin<py_plugin>();
      if (!app().initialize<chain_plugin, http_plugin, net_plugin, py_plugin>(g_argc, g_argv)) {
         init_finished = true;
         shutdown_finished = true;
         return;
      }
//      init_debug();
      app().startup();
      init_finished = true;

      PyThreadState*  state = tiny_PyEval_SaveThread();
      app().exec();
      tiny_PyEval_RestoreThread(state);

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

void interactive_console() {
   Py_Initialize();
   PyEval_InitThreads();

   PyRun_SimpleString("import readline");
   PyInit_wallet();
   PyInit_eosapi();
   PyInit_eostypes();
   PyInit_database();
   PyInit_blockchain();
   PyInit_util();
   PyInit_debug();

   PyRun_SimpleString("import wallet");
   PyRun_SimpleString("import eosapi;");
   PyRun_SimpleString("import database;");
   PyRun_SimpleString("import util;");
   PyRun_SimpleString("import debug;");
   PyRun_SimpleString("from imp import reload;");
   PyRun_SimpleString("eosapi.register_signal_handler()");
   PyRun_SimpleString(
       "import sys;"
        "sys.path.append('../../programs/pyeos');"
        "sys.path.append('../../programs/pyeos/contracts');"
   );

   ilog("+++++++++++++interactive_console: init_finished ${n}", ("n", init_finished));

   while (!init_finished) {
      boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
   }

   if (shutdown_finished) {
      Py_Finalize();
      return;
   }

   PyRun_SimpleString("import initeos");
   PyRun_SimpleString("from main import chain_controller as ctrl");

   ilog("+++++++++++++interactive_console: ${n}", ("n", app().get_plugin<py_plugin>().interactive));

   PyThreadState*  state = tiny_PyEval_SaveThread();
   if (app().get_plugin<py_plugin>().interactive) {
      ilog("start interactive python.");
      PyRun_SimpleString("eosapi.register_signal_handler()");
      PyRun_InteractiveLoop(stdin, "<stdin>");
   }
   tiny_PyEval_RestoreThread(state);

   while (!shutdown_finished) {
      boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
   }

   Py_Finalize();
}

typedef void (*fn_eos_main)();
typedef void (*fn_interactive_console)();

void init_smart_contract(fn_eos_main eos_main, fn_interactive_console console);

int main(int argc, char** argv) {
   g_argc = argc;
   g_argv = argv;

   init_smart_contract(eos_main, interactive_console);

   return 0;
}


