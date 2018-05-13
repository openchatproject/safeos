// Autogenerated by Thrift Compiler (0.11.0)
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING

package main

import (
        "context"
        "flag"
        "fmt"
        "math"
        "net"
        "net/url"
        "os"
        "strconv"
        "strings"
        "git.apache.org/thrift.git/lib/go/thrift"
        "rpc"
)


func Usage() {
  fmt.Fprintln(os.Stderr, "Usage of ", os.Args[0], " [-h host:port] [-u url] [-f[ramed]] function [arg1 [arg2...]]:")
  flag.PrintDefaults()
  fmt.Fprintln(os.Stderr, "\nFunctions:")
  fmt.Fprintln(os.Stderr, "  Apply apply_request()")
  fmt.Fprintln(os.Stderr, "  void apply_finish()")
  fmt.Fprintln(os.Stderr, "   funCall(i64 callTime, string funCode,  paramMap)")
  fmt.Fprintln(os.Stderr, "  string read_action()")
  fmt.Fprintln(os.Stderr, "  i32 db_store_i64(i64 scope, i64 table, i64 payer, i64 id, string buffer)")
  fmt.Fprintln(os.Stderr, "  void db_update_i64(i32 itr, i64 payer, string buffer)")
  fmt.Fprintln(os.Stderr, "  void db_remove_i64(i32 itr)")
  fmt.Fprintln(os.Stderr, "  string db_get_i64(i32 itr)")
  fmt.Fprintln(os.Stderr, "  Result db_next_i64(i32 itr)")
  fmt.Fprintln(os.Stderr, "  Result db_previous_i64(i32 itr)")
  fmt.Fprintln(os.Stderr, "  i32 db_find_i64(i64 code, i64 scope, i64 table, i64 id)")
  fmt.Fprintln(os.Stderr, "  i32 db_lowerbound_i64(i64 code, i64 scope, i64 table, i64 id)")
  fmt.Fprintln(os.Stderr, "  i32 db_upperbound_i64(i64 code, i64 scope, i64 table, i64 id)")
  fmt.Fprintln(os.Stderr, "  i32 db_end_i64(i64 code, i64 scope, i64 table)")
  fmt.Fprintln(os.Stderr)
  os.Exit(0)
}

func main() {
  flag.Usage = Usage
  var host string
  var port int
  var protocol string
  var urlString string
  var framed bool
  var useHttp bool
  var parsedUrl *url.URL
  var trans thrift.TTransport
  _ = strconv.Atoi
  _ = math.Abs
  flag.Usage = Usage
  flag.StringVar(&host, "h", "localhost", "Specify host and port")
  flag.IntVar(&port, "p", 9090, "Specify port")
  flag.StringVar(&protocol, "P", "binary", "Specify the protocol (binary, compact, simplejson, json)")
  flag.StringVar(&urlString, "u", "", "Specify the url")
  flag.BoolVar(&framed, "framed", false, "Use framed transport")
  flag.BoolVar(&useHttp, "http", false, "Use http")
  flag.Parse()
  
  if len(urlString) > 0 {
    var err error
    parsedUrl, err = url.Parse(urlString)
    if err != nil {
      fmt.Fprintln(os.Stderr, "Error parsing URL: ", err)
      flag.Usage()
    }
    host = parsedUrl.Host
    useHttp = len(parsedUrl.Scheme) <= 0 || parsedUrl.Scheme == "http"
  } else if useHttp {
    _, err := url.Parse(fmt.Sprint("http://", host, ":", port))
    if err != nil {
      fmt.Fprintln(os.Stderr, "Error parsing URL: ", err)
      flag.Usage()
    }
  }
  
  cmd := flag.Arg(0)
  var err error
  if useHttp {
    trans, err = thrift.NewTHttpClient(parsedUrl.String())
  } else {
    portStr := fmt.Sprint(port)
    if strings.Contains(host, ":") {
           host, portStr, err = net.SplitHostPort(host)
           if err != nil {
                   fmt.Fprintln(os.Stderr, "error with host:", err)
                   os.Exit(1)
           }
    }
    trans, err = thrift.NewTSocket(net.JoinHostPort(host, portStr))
    if err != nil {
      fmt.Fprintln(os.Stderr, "error resolving address:", err)
      os.Exit(1)
    }
    if framed {
      trans = thrift.NewTFramedTransport(trans)
    }
  }
  if err != nil {
    fmt.Fprintln(os.Stderr, "Error creating transport", err)
    os.Exit(1)
  }
  defer trans.Close()
  var protocolFactory thrift.TProtocolFactory
  switch protocol {
  case "compact":
    protocolFactory = thrift.NewTCompactProtocolFactory()
    break
  case "simplejson":
    protocolFactory = thrift.NewTSimpleJSONProtocolFactory()
    break
  case "json":
    protocolFactory = thrift.NewTJSONProtocolFactory()
    break
  case "binary", "":
    protocolFactory = thrift.NewTBinaryProtocolFactoryDefault()
    break
  default:
    fmt.Fprintln(os.Stderr, "Invalid protocol specified: ", protocol)
    Usage()
    os.Exit(1)
  }
  iprot := protocolFactory.GetProtocol(trans)
  oprot := protocolFactory.GetProtocol(trans)
  client := rpc.NewRpcServiceClient(thrift.NewTStandardClient(iprot, oprot))
  if err := trans.Open(); err != nil {
    fmt.Fprintln(os.Stderr, "Error opening socket to ", host, ":", port, " ", err)
    os.Exit(1)
  }
  
  switch cmd {
  case "apply_request":
    if flag.NArg() - 1 != 0 {
      fmt.Fprintln(os.Stderr, "ApplyRequest requires 0 args")
      flag.Usage()
    }
    fmt.Print(client.ApplyRequest(context.Background()))
    fmt.Print("\n")
    break
  case "apply_finish":
    if flag.NArg() - 1 != 0 {
      fmt.Fprintln(os.Stderr, "ApplyFinish requires 0 args")
      flag.Usage()
    }
    fmt.Print(client.ApplyFinish(context.Background()))
    fmt.Print("\n")
    break
  case "funCall":
    if flag.NArg() - 1 != 3 {
      fmt.Fprintln(os.Stderr, "FunCall requires 3 args")
      flag.Usage()
    }
    argvalue0, err33 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err33 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    argvalue1 := flag.Arg(2)
    value1 := argvalue1
    arg35 := flag.Arg(3)
    mbTrans36 := thrift.NewTMemoryBufferLen(len(arg35))
    defer mbTrans36.Close()
    _, err37 := mbTrans36.WriteString(arg35)
    if err37 != nil { 
      Usage()
      return
    }
    factory38 := thrift.NewTSimpleJSONProtocolFactory()
    jsProt39 := factory38.GetProtocol(mbTrans36)
    containerStruct2 := rpc.NewRpcServiceFunCallArgs()
    err40 := containerStruct2.ReadField3(jsProt39)
    if err40 != nil {
      Usage()
      return
    }
    argvalue2 := containerStruct2.ParamMap
    value2 := argvalue2
    fmt.Print(client.FunCall(context.Background(), value0, value1, value2))
    fmt.Print("\n")
    break
  case "read_action":
    if flag.NArg() - 1 != 0 {
      fmt.Fprintln(os.Stderr, "ReadAction requires 0 args")
      flag.Usage()
    }
    fmt.Print(client.ReadAction(context.Background()))
    fmt.Print("\n")
    break
  case "db_store_i64":
    if flag.NArg() - 1 != 5 {
      fmt.Fprintln(os.Stderr, "DbStoreI64 requires 5 args")
      flag.Usage()
    }
    argvalue0, err41 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err41 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    argvalue1, err42 := (strconv.ParseInt(flag.Arg(2), 10, 64))
    if err42 != nil {
      Usage()
      return
    }
    value1 := argvalue1
    argvalue2, err43 := (strconv.ParseInt(flag.Arg(3), 10, 64))
    if err43 != nil {
      Usage()
      return
    }
    value2 := argvalue2
    argvalue3, err44 := (strconv.ParseInt(flag.Arg(4), 10, 64))
    if err44 != nil {
      Usage()
      return
    }
    value3 := argvalue3
    argvalue4 := []byte(flag.Arg(5))
    value4 := argvalue4
    fmt.Print(client.DbStoreI64(context.Background(), value0, value1, value2, value3, value4))
    fmt.Print("\n")
    break
  case "db_update_i64":
    if flag.NArg() - 1 != 3 {
      fmt.Fprintln(os.Stderr, "DbUpdateI64 requires 3 args")
      flag.Usage()
    }
    tmp0, err46 := (strconv.Atoi(flag.Arg(1)))
    if err46 != nil {
      Usage()
      return
    }
    argvalue0 := int32(tmp0)
    value0 := argvalue0
    argvalue1, err47 := (strconv.ParseInt(flag.Arg(2), 10, 64))
    if err47 != nil {
      Usage()
      return
    }
    value1 := argvalue1
    argvalue2 := []byte(flag.Arg(3))
    value2 := argvalue2
    fmt.Print(client.DbUpdateI64(context.Background(), value0, value1, value2))
    fmt.Print("\n")
    break
  case "db_remove_i64":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "DbRemoveI64 requires 1 args")
      flag.Usage()
    }
    tmp0, err49 := (strconv.Atoi(flag.Arg(1)))
    if err49 != nil {
      Usage()
      return
    }
    argvalue0 := int32(tmp0)
    value0 := argvalue0
    fmt.Print(client.DbRemoveI64(context.Background(), value0))
    fmt.Print("\n")
    break
  case "db_get_i64":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "DbGetI64 requires 1 args")
      flag.Usage()
    }
    tmp0, err50 := (strconv.Atoi(flag.Arg(1)))
    if err50 != nil {
      Usage()
      return
    }
    argvalue0 := int32(tmp0)
    value0 := argvalue0
    fmt.Print(client.DbGetI64(context.Background(), value0))
    fmt.Print("\n")
    break
  case "db_next_i64":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "DbNextI64 requires 1 args")
      flag.Usage()
    }
    tmp0, err51 := (strconv.Atoi(flag.Arg(1)))
    if err51 != nil {
      Usage()
      return
    }
    argvalue0 := int32(tmp0)
    value0 := argvalue0
    fmt.Print(client.DbNextI64(context.Background(), value0))
    fmt.Print("\n")
    break
  case "db_previous_i64":
    if flag.NArg() - 1 != 1 {
      fmt.Fprintln(os.Stderr, "DbPreviousI64 requires 1 args")
      flag.Usage()
    }
    tmp0, err52 := (strconv.Atoi(flag.Arg(1)))
    if err52 != nil {
      Usage()
      return
    }
    argvalue0 := int32(tmp0)
    value0 := argvalue0
    fmt.Print(client.DbPreviousI64(context.Background(), value0))
    fmt.Print("\n")
    break
  case "db_find_i64":
    if flag.NArg() - 1 != 4 {
      fmt.Fprintln(os.Stderr, "DbFindI64 requires 4 args")
      flag.Usage()
    }
    argvalue0, err53 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err53 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    argvalue1, err54 := (strconv.ParseInt(flag.Arg(2), 10, 64))
    if err54 != nil {
      Usage()
      return
    }
    value1 := argvalue1
    argvalue2, err55 := (strconv.ParseInt(flag.Arg(3), 10, 64))
    if err55 != nil {
      Usage()
      return
    }
    value2 := argvalue2
    argvalue3, err56 := (strconv.ParseInt(flag.Arg(4), 10, 64))
    if err56 != nil {
      Usage()
      return
    }
    value3 := argvalue3
    fmt.Print(client.DbFindI64(context.Background(), value0, value1, value2, value3))
    fmt.Print("\n")
    break
  case "db_lowerbound_i64":
    if flag.NArg() - 1 != 4 {
      fmt.Fprintln(os.Stderr, "DbLowerboundI64 requires 4 args")
      flag.Usage()
    }
    argvalue0, err57 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err57 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    argvalue1, err58 := (strconv.ParseInt(flag.Arg(2), 10, 64))
    if err58 != nil {
      Usage()
      return
    }
    value1 := argvalue1
    argvalue2, err59 := (strconv.ParseInt(flag.Arg(3), 10, 64))
    if err59 != nil {
      Usage()
      return
    }
    value2 := argvalue2
    argvalue3, err60 := (strconv.ParseInt(flag.Arg(4), 10, 64))
    if err60 != nil {
      Usage()
      return
    }
    value3 := argvalue3
    fmt.Print(client.DbLowerboundI64(context.Background(), value0, value1, value2, value3))
    fmt.Print("\n")
    break
  case "db_upperbound_i64":
    if flag.NArg() - 1 != 4 {
      fmt.Fprintln(os.Stderr, "DbUpperboundI64 requires 4 args")
      flag.Usage()
    }
    argvalue0, err61 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err61 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    argvalue1, err62 := (strconv.ParseInt(flag.Arg(2), 10, 64))
    if err62 != nil {
      Usage()
      return
    }
    value1 := argvalue1
    argvalue2, err63 := (strconv.ParseInt(flag.Arg(3), 10, 64))
    if err63 != nil {
      Usage()
      return
    }
    value2 := argvalue2
    argvalue3, err64 := (strconv.ParseInt(flag.Arg(4), 10, 64))
    if err64 != nil {
      Usage()
      return
    }
    value3 := argvalue3
    fmt.Print(client.DbUpperboundI64(context.Background(), value0, value1, value2, value3))
    fmt.Print("\n")
    break
  case "db_end_i64":
    if flag.NArg() - 1 != 3 {
      fmt.Fprintln(os.Stderr, "DbEndI64 requires 3 args")
      flag.Usage()
    }
    argvalue0, err65 := (strconv.ParseInt(flag.Arg(1), 10, 64))
    if err65 != nil {
      Usage()
      return
    }
    value0 := argvalue0
    argvalue1, err66 := (strconv.ParseInt(flag.Arg(2), 10, 64))
    if err66 != nil {
      Usage()
      return
    }
    value1 := argvalue1
    argvalue2, err67 := (strconv.ParseInt(flag.Arg(3), 10, 64))
    if err67 != nil {
      Usage()
      return
    }
    value2 := argvalue2
    fmt.Print(client.DbEndI64(context.Background(), value0, value1, value2))
    fmt.Print("\n")
    break
  case "":
    Usage()
    break
  default:
    fmt.Fprintln(os.Stderr, "Invalid function ", cmd)
  }
}
