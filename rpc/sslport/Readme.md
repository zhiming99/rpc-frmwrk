. sslfido.cpp, sslfido.h and sslutils.cpp implemented a filter port, as inserted between the upper `Native Protocol Fdo port` and the bottom `Tcp Stream Pdo port`. It encrypts the outbound packets and decrypts the inbound packets as well as performs the `SSL handshake` and `SSL shutdown`, plus the `SSL renegotiation` occationally.   

. The `sslfido` requires OpenSSL-1.1 to work properly. And I have not tested whether it works with lower version so far. And the `SSL renegotioation` is probably not working since there is no test case for it yet.

. OpenSSL requires to have the key file and certificate file ready before it can work.   

. Here is a command line example. Note it generates a self-signed key with no password protection.   
```
# command line
openssl req -x509 -newkey rsa:4096 -nodes -keyout key.pem -out cert.pem -days 365
```

* The `sslfido` related settings are all added to the [driver.json](https://github.com/zhiming99/rpc-frmwrk/blob/master/ipc/driver.json). You can find the detail in this file.

* To enable SSL on the server side, edit the `driver.json`, in which, and find the `RpcTcpBusPort` option block, where lists the attributes of all the listening ports. 
```
        {
            "PortType"      : "Fdo",
            "PortClass"     : "RpcTcpBusPort",
            "PortDriver"    : "RpcTcpBusDriver",
            "InternalClass" : "CTcpRpcBusPort",
            "Parameters" :
            [
                {
                    "comment-1" : "remove it or change it according to your environment",
                    "AddrFormat" : "ipv4",
                    "Protocol" : "native",
                    "comment-2" : "port 4132 will accept native non-websocket connection",
                    "PortNumber": "4132",
                    "BindAddr" : "127.0.0.1",
                    "comment"  : "inter-changable with 'TcpStreamPdo' when in a trusted and stable environment",
                    "PdoClass" : "TcpStreamPdo2",
                    "Compression" : "true",
                    "EnableWS":  "false",
                    "EnableSSL": "true"
                }
            ]
        }
```
* To enable SSL on the proxy side, edit the proxy's description file. For example, in the [`hwdesc.json`](https://github.com/zhiming99/rpc-frmwrk/blob/master/test/helloworld/hwdesc.json) for [`helloworld`](https://github.com/zhiming99/rpc-frmwrk/tree/master/test/helloworld) test, notice the attribute `"EnableSSL":"true"`
```
         {
            "ObjectName": "CEchoServer",
            "BusName" : "DBusBusPort_0",
            "PortClass": "DBusLocalPdo",
            "PortId" : "0",
            ......
            "comment-1" : "IpAddress is the ip address of the server",
            "IpAddress" : "127.0.0.1",
            "Compression" : "true",
            "EnableSSL" : "true",
            "EnableWS" : "false",
            "RouterPath-1" : "/rasp1",
            ......
        }
```
