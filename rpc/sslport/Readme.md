[中文](./README_cn.md)
. **The SSLPort** has implemented a filter port, as can be inserted to the port stack with the upper port to be `Native Protocol Fdo port`, `web socket Fido` or `secfido` and the bottom to be `Tcp Stream Pdo port`. It performs the `SSL handshake`, encrypts the outbound packets, decrypts the inbound packets and performs`SSL shutdown`, and the `SSL renegotiation` occationally through the SSL connection's lifecycle.   

. The `SSLPort` requires OpenSSL-1.1 or higer version to work properly. It is recommended to choose TLS1.2 or TLS1.3 as the communication protocol, depending on peer's capability.

. There are two approaches to deploy the keys and certificates as necessary to estabilish a SSL connection.

1. Generating the keys manually, and importing keys by telling the `rpc-frmwrk` the path of the keys with the config tool `rpcfg.py`. And repeat the steps on each target platform. The keys obtained from public CA applies the same way.
```
# an example command to generate keys and certs
openssl req -x509 -newkey rsa:4096 -nodes -keyout key.pem -out cert.pem -days 365
```
2. Using `Gen Demo Key` from the `rpcfg.py` to generate the requested number of keys and certs, and using `Save As` button to generate a config installer, which can run on the target host to deploy the keys and update the configurations automatically. The `Demo` keys and certificates are 2048-bit RSA keys in PEM format.

. The following section gives the detail SSL config options, that can be found in the global [driver.json](../../ipc/driver.json). 

* In the config block for `RpcTcpBusPort` , Enable using SSL by setting 'EnableSSL' to 'true'. This config block applies to server only.
```
        {
            "PortType"      : "Fdo",
            "PortClass"     : "RpcTcpBusPort",
            "PortDriver"    : "RpcTcpBusDriver",
            "InternalClass" : "CTcpRpcBusPort",
            "Parameters" :
            [
                {
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
* In the config block for `RpcOpenSSLFido`, set the directory path where keys and certs are stored on the server side. The attribute `VerifyPeer` specifies whether or not the SSLPort verify the certs from the peer are trusted. It is a useful way to control the SSL connections are from trusted users. This applies to both server and client. The attribute `SecretFile` can be empty if the key file is not password protected, or an absolute path to a file storing the password, or the string `console` to prompt for password when the `rpcrouter` starts.
```
        {
            "PortType": "Fido",
            "PortClass": "RpcOpenSSLFido",
            "InternalClass": "CRpcOpenSSLFido",
            "PortDriver": "RpcOpenSSLFidoDrv",
            "Parameters": {
                "comments": "Certificate and private key file paths",
                "CertFile": "/root/.rpcf/openssl/signcert.pem",
                "KeyFile": "/root/.rpcf/openssl/signkey.pem",
                "CACertFile": "/root/.rpcf/openssl/certs.pem",
                "SecretFile": "",
                "VerifyPeer": "true"
            }
        },  
```
* Then in the config block for `TcpStreamPdo2`'s driver probe sequence, add `RpcOpenSSLFidoDrv` to the beginning of the array `ProbeSequence`. This config block applies to both server and client.
```
        {
            "PortClass": "TcpStreamPdo2",
            "comment": "ProbeSequence specify the order of the drivers' Probe to call to build the port stack",
            "ProbeSequence": [
                "RpcOpenSSLFidoDrv",
                "RpcWebSockFidoDrv",
                "RpcNatProtoFdoDrv",
                "RpcTcpFidoDrv"
            ],  
            "Description": "This match provides a binding between the pdo and the uppper fdo and fido",
            "comment-1": "this is a new fork of the TcpStreamPdo, with better flexibility and extensibility",
            "UsingGmSSL": "false"
        },
```
. Finally when you are building a project generated by `ridlc`, the setting of the program will be updated by the makefile. In case you want to tweak the config options, you can manualy edit the proxy's description file. For example, in the [`hwdesc.json`](../../test/helloworld/hwdesc.json) for [`helloworld`](../../test/helloworld) test, note the attribute `"EnableSSL":"true"`
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

