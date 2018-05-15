package main

import (
    "fmt"
    "rpc"
    _"time"
    "unsafe"
    "context"
    "crypto/tls"
    "encoding/binary"
    "git.apache.org/thrift.git/lib/go/thrift"
)

/*
#include <goeos.h>
#include <stdlib.h>
#include <stdio.h>
*/
import "C"

var applyClient *rpc.RpcInterfaceClient;

var applyStart = make(chan rpc.Apply, 1)
var applyFinish = make(chan bool, 1)

//export onApply
func onApply(receiver uint64, account uint64, act uint64) int {
//    initApplyClient()
//    fmt.Println("+++++++onApply", receiver, account, act, applyClient)
    apply := rpc.Apply{int64(receiver), int64(account), int64(act)}
    applyStart <- apply
    _ = <- applyFinish
//    fmt.Println("+++++++onApply finished")

    /*
    if applyClient != nil {
        r, _ := applyClient.Apply(ctx, int64(receiver), int64(account), int64(act))
        return int(r)
    }
    */
    return 0;
}

type RpcServiceImpl struct {
}

func (p *RpcServiceImpl)  ApplyRequest(ctx context.Context) (r *rpc.Apply, err error) {
    apply := <-applyStart
    return &apply, nil
}

func (p *RpcServiceImpl)  ApplyFinish(ctx context.Context) (err error) {
    applyFinish <- true;
    return nil
}

func (this *RpcServiceImpl) FunCall(ctx context.Context, callTime int64, funCode string, paramMap map[string]string) (r []string, err error) {
//  fmt.Println("-->FunCall:", callTime, funCode, paramMap)
    for k, v := range paramMap {
        r = append(r, k+v)
    }
    return
}

func (p *RpcServiceImpl) ReadAction(ctx context.Context) (r []byte, err error) {
    var buffer [256]byte
    ret  := 0//:= C.read_action((*C.char)(unsafe.Pointer(&buffer[0])), C.size_t(len(buffer)))
    return buffer[:ret], nil
}

// Parameters:
//  - Scope
//  - Table
//  - Payer
//  - ID
//  - Buffer
func (p *RpcServiceImpl) DbStoreI64(ctx context.Context, scope int64, table int64, payer int64, id int64, buffer []byte) (r int32, err error) {
    if buffer == nil || len(buffer) == 0 {
        fmt.Println("+++++++++invalide buffer", buffer);
        return -1, nil
    }
    ret := C.db_store_i64(C.uint64_t(scope), C.uint64_t(table), C.uint64_t(payer), C.uint64_t(id), (*C.char)(unsafe.Pointer(&buffer[0])), C.size_t(len(buffer)))
    return int32(ret), nil
}

// Parameters:
//  - Itr
//  - Payer
//  - Buffer
func (p *RpcServiceImpl) DbUpdateI64(ctx context.Context, itr int32, payer int64, buffer []byte) (err error) {
    C.db_update_i64( C.int(itr), C.uint64_t(payer), (*C.char)(unsafe.Pointer(&buffer[0])), C.size_t(len(buffer)));
    return nil
}

// Parameters:
//  - Itr
func (p *RpcServiceImpl) DbRemoveI64(ctx context.Context, itr int32) (err error) {
    C.db_remove_i64(C.int(itr));
    return nil
}

// Parameters:
//  - Itr
func (p *RpcServiceImpl) DbGetI64(ctx context.Context, itr int32) ([]byte, error) {
    size := C.db_get_i64(C.int(itr), nil, C.size_t(0));
    if size <= 0 {
        return nil, nil
    }

    buffer := C.malloc(C.size_t(size))

    ret := C.db_get_i64(C.int(itr), (*C.char)(buffer), C.size_t(size))
    defer C.free(buffer)
    r := C.GoBytes(buffer,ret)
    return r, nil
}

func Int64ToBytes(i uint64) []byte {
    var buf = make([]byte, 8)
    binary.BigEndian.PutUint64(buf, uint64(i))
    return buf
}

func BytesToInt64(buf []byte) uint64 {
    return uint64(binary.BigEndian.Uint64(buf))
}

// Parameters:
//  - Itr
func (p *RpcServiceImpl) DbNextI64(ctx context.Context, itr int32) (r *rpc.Result_, err error) {
    var primary uint64
    var result rpc.Result_
    ret := C.db_next_i64( C.int(itr), (*C.uint64_t)(&primary) )
    result.Status = int32(ret)
    result.Value =  Int64ToBytes(primary)
    return &result, nil;
}

// Parameters:
//  - Itr
func (p *RpcServiceImpl) DbPreviousI64(ctx context.Context, itr int32) (r *rpc.Result_, err error) {
    var primary uint64
    var result rpc.Result_

    ret := C.db_previous_i64(C.int(itr), (*C.uint64_t)(&primary))
    result.Status = int32(ret)
    result.Value =  Int64ToBytes(primary)
    return &result, nil
}

// Parameters:
//  - Code
//  - Scope
//  - Table
//  - ID
func (p *RpcServiceImpl) DbFindI64(ctx context.Context, code int64, scope int64, table int64, id int64) (r int32, err error) {
    ret := C.db_find_i64(C.uint64_t(code), C.uint64_t(scope), C.uint64_t(table), C.uint64_t(id))
    return int32(ret), nil
}


// Parameters:
//  - Code
//  - Scope
//  - Table
//  - ID
func (p *RpcServiceImpl) DbLowerboundI64(ctx context.Context, code int64, scope int64, table int64, id int64) (r int32, err error) {
    ret := C.db_lowerbound_i64(C.uint64_t(code), C.uint64_t(scope), C.uint64_t(table), C.uint64_t(id))
    return int32(ret), nil
}

// Parameters:
//  - Code
//  - Scope
//  - Table
//  - ID
func (p *RpcServiceImpl) DbUpperboundI64(ctx context.Context, code int64, scope int64, table int64, id int64) (r int32, err error) {
    ret := C.db_upperbound_i64(C.uint64_t(code), C.uint64_t(scope), C.uint64_t(table), C.uint64_t(id))
    return int32(ret), nil
}


// Parameters:
//  - Code
//  - Scope
//  - Table
func (p *RpcServiceImpl) DbEndI64(ctx context.Context, code int64, scope int64, table int64) (int32, error) {
    ret := C.db_end_i64(C.uint64_t(code), C.uint64_t(scope), C.uint64_t(table))
    return int32(ret), nil
}


func (p *RpcServiceImpl) DbUpdateI64Ex(ctx context.Context, scope int64, payer int64, table int64, id int64, buffer []byte) (err error) {
    C.db_update_i64_ex(C.uint64_t(scope), C.uint64_t(payer), C.uint64_t(table), C.uint64_t(id), (*C.char)(unsafe.Pointer(&buffer[0])), C.size_t(len(buffer)))
    return nil
}

func (p *RpcServiceImpl) DbRemoveI64Ex(ctx context.Context, scope int64, payer int64, table int64, id int64) (err error) {
    C.db_remove_i64_ex(C.uint64_t(scope), C.uint64_t(payer), C.uint64_t(table), C.uint64_t(id))
    return nil
}


var client *rpc.RpcInterfaceClient
var __transportFactory thrift.TTransportFactory
var __protocolFactory thrift.TProtocolFactory

func runServer(transportFactory thrift.TTransportFactory, protocolFactory thrift.TProtocolFactory, addr string, secure bool) error {
    var transport thrift.TServerTransport
    var err error

    __transportFactory = transportFactory
    __protocolFactory = protocolFactory

    if secure {
        cfg := new(tls.Config)
        if cert, err := tls.LoadX509KeyPair("server.crt", "server.key"); err == nil {
            cfg.Certificates = append(cfg.Certificates, cert)
        } else {
            return err
        }
        transport, err = thrift.NewTSSLServerSocket(addr, cfg)
    } else {
        transport, err = thrift.NewTServerSocket(addr)
    }
    
    if err != nil {
        return err
    }
    fmt.Printf("%T\n", transport)

    handler := &RpcServiceImpl{}
    processor := rpc.NewRpcServiceProcessor(handler)

//  handler := NewEoslibServiceHandler()
//  processor := idl.EoslibServiceProcessor(handler)
    server := thrift.NewTSimpleServer4(processor, transport, transportFactory, protocolFactory)

    fmt.Println("Starting the simple server... on ", addr)
    return server.Serve()
}

func initApplyClient() error {
    var transport thrift.TTransport
    var err error

    addr := "localhost:9192"
    secure := false
    if applyClient != nil {
        return nil
    }
    
    if secure {
        cfg := new(tls.Config)
        cfg.InsecureSkipVerify = true
        transport, err = thrift.NewTSSLSocket(addr, cfg)
    } else {
        transport, err = thrift.NewTSocket(addr)
    }
    if err != nil {
        fmt.Println("Error opening socket:", err)
        return nil
    }
    transport, err = __transportFactory.GetTransport(transport)
    if err != nil {
        return nil
    }
//    defer transport.Close()
    if err := transport.Open(); err != nil {
        return nil
    }

    iprot := __protocolFactory.GetProtocol(transport)
    oprot := __protocolFactory.GetProtocol(transport)

    applyClient = rpc.NewRpcInterfaceClient(thrift.NewTStandardClient(iprot, oprot))
    return nil
}





