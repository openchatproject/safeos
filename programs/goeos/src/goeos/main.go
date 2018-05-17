package main

import (
	"C"
	"bridge"
	"flag"
	"fmt"
	"os"
	"time"
	"unsafe"

	"git.apache.org/thrift.git/lib/go/thrift"
	"github.com/learnforpractice/panicwrap"
)

/*
#include "mpeoslib.h"
#include <stdio.h>
#include <stdint.h>

typedef int (*fn_rpc_apply)(uint64_t receiver, uint64_t code, uint64_t act, char** err, int* len);
void rpc_register_apply_call(fn_rpc_apply fn);
int call_onApply(uint64_t receiver, uint64_t account, uint64_t act, char** err, int* len);

void mp_init_eosapi();
void set_client_mode(int client_mode);

*/
import "C"

func Usage() {
	fmt.Fprint(os.Stderr, "Usage of ", os.Args[0], ":\n")
	flag.PrintDefaults()
	fmt.Fprint(os.Stderr, "\n")
}

func main() {

	flag.Usage = Usage
	server := flag.Bool("server", false, "Run server")
	rpc_server := flag.Bool("rpc-server", false, "Run rpc server")

	client := flag.Bool("client", false, "Run client")
	rpc_client := flag.Bool("rpc-client", false, "Run rpc client")
	replay := flag.Bool("replay", false, "Rebuild database")

	protocol := flag.String("P", "binary", "Specify the protocol (binary, compact, json, simplejson)")
	framed := flag.Bool("framed", false, "Use framed transport")
	buffered := flag.Bool("buffered", true, "Use buffered transport")
	addr := flag.String("addr", "localhost:9191", "Address to listen to")
	secure := flag.Bool("secure", false, "Use tls secure transport")
	interactive := flag.Bool("i", true, "Enter into interactive console")
	manual_gen_block := flag.Bool("manual-gen-block", true, "Generate blocks manually")
	debug := flag.Bool("debug", true, "Debug mode")

	data_dir := flag.String("data-dir", "data-dir", "data directory")

	_, _, _, _, _, _, _, _ = interactive, manual_gen_block, debug, rpc_client, data_dir, rpc_server, client, replay

	flag.Parse()

	var protocolFactory thrift.TProtocolFactory
	switch *protocol {
	case "compact":
		protocolFactory = thrift.NewTCompactProtocolFactory()
	case "simplejson":
		protocolFactory = thrift.NewTSimpleJSONProtocolFactory()
	case "json":
		protocolFactory = thrift.NewTJSONProtocolFactory()
	case "binary", "":
		protocolFactory = thrift.NewTBinaryProtocolFactoryDefault()
	default:
		fmt.Fprint(os.Stderr, "Invalid protocol specified", protocol, "\n")
		Usage()
		os.Exit(1)
	}

	var transportFactory thrift.TTransportFactory
	if *buffered {
		transportFactory = thrift.NewTBufferedTransportFactory(1024 * 1024)
	} else {
		transportFactory = thrift.NewTTransportFactory()
	}

	if *framed {
		transportFactory = thrift.NewTFramedTransportFactory(transportFactory)
	}

	if *server || (!*client && !*server) {
		go func() {
			if err := runServer(transportFactory, protocolFactory, *addr, *secure); err != nil {
				fmt.Println("error running server:", err)
			}
		}()
		C.rpc_register_apply_call((C.fn_rpc_apply)(unsafe.Pointer(C.call_onApply)))
		bridge.GoeosMain()
	} else {
		for i := 0; i < 0; i++ {
			wrap := panicwrap.WrapConfig{
				Handler: panicHandler,
				//                  ExePath: "ls",
				//                  Args	: []string{"-l"},
			}
			exitStatus, err := panicwrap.Wrap(&wrap)
			fmt.Println("panicwrap.Wrap return: ", exitStatus, err)
			if err != nil {
				// Something went wrong setting up the panic wrapper. Unlikely,
				// but possible.
				panic(err)
			}
			if exitStatus < 0 {
				break
			}
		}
		C.mp_init_eosapi()
		C.set_client_mode(C.int(1))
		if err := runClient(transportFactory, protocolFactory, *addr, *secure); err != nil {
			fmt.Println("error running client:", err)
		}
	}
	time.Sleep(0)
}

func panicHandler(output string) {
	// output contains the full output (including stack traces) of the
	// panic. Put it in a file or something.
	fmt.Printf("+++++++++++The child panicked:\n\n%s\n", output)
	//	os.Exit(1)
}
