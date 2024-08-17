[English](./Readme.md)
# SSL过滤端口(SSL Filter Port)简介
SSL端口是一个过滤型端口，可以动态插入到一个物理端口是`TcpStreamPdo2`，功能端口是`RpcNativeProtoFdo`的端口管线之间。其功能是建立安全连接，加密发送数据包，解密接收的数据包，以及执行关闭流程等。
* SSL端口依赖OpenSSL-1.1或者更高的版本。连接协议为TLS-1.2或TLS-1.3。
* 密钥和证书的获取
  * OpenSSL的密钥和证书可以通过第三方CA机构获取。
  * 可以生成自签名的密钥和证书。方法见配置工具[rpcfg.py](../../tools/README_cn.md#安全页security)。
  * 也可以手工在命令行生成, 如：   
      ```
      openssl req -x509 -newkey rsa:4096 -nodes -keyout key.pem -out cert.pem -days 365
      ```

# SSL过滤端口的配置细节：
SSL端口的配置完全可以由配置工具`rpcfg.py`和生成框架代码下的`synccfg.py`完成。不关心技术细节的用户可以完全忽略。
SSL端口的配置选项主要在[driver.json](../../ipc/driver.json)，和用户代码里的对象描述文件中。
* 在driver.json的 `RpcTcpBusPort`字段下面，添加`"EnableSSL":"true"`
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
* 在driver.json的`RpcOpenSSLFido`字段下面添加密钥和证书的路径。
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
    }
    ```
* 在`TcpStreamPdo2`字段的`加载顺序(ProbeSeQuence)`数组头部插入`RpcOpenSSLFidoDrv`。因为SSL端口必须是第一个被加载的端口。
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
    }
    ```
* 最后，当你用[`ridlc`](../../ridl/README_cn.md#启动ridlc)生成一个项目时，会有一个对象描述文件文件产生。假设你用[hellowld.ridl](../../examples/hellowld.ridl)生成的项目，对象描述文件是`HelloWorlddesc.json`。在名为Objects数组下面，每一个元素根据目标机器是否启用SSL连接，设置字段`"EnableSSL":"true"`或`"EnableSSL":"false"`。
    ```
    {
        "BusName": "DBusBusPort_0",
        "Compression": "true",
        "EnableSSL": "true",
        "EnableWS": "false",
        "Interfaces": [
            {
                "InterfaceName": "IInterfaceServer"
            },
            {
                "InterfaceName": "IHelloWorld"
            }
        ],
        "IpAddress": "172.17.0.1",
        "KeepAliveTimeoutSec": "60",
        "ObjectName": "HelloWorldSvc",
        "PortClass": "DBusLocalPdo",
        "PortId": "0",
        "ProxyPortClass": "DBusProxyPdo",
        "ProxyPortClass-1": "DBusLocalPdo",
        "QueuedRequests": "false",
        "RequestTimeoutSec": "120",
        "RouterPath": "/",
        "PortNumber": "4132",
    }
    ```