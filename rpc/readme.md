Features of the RPC module:

1. The RPC module enables the server to expose its service objects to the remote clients via a TCP connection.
2. [`Streaming`](../Concept.md#streaming), [`Websocket`](./wsport/Readme.md), [`OpenSSL`](./sslport/Readme.md) and [authentication](./security/README.md) are also supported over this TCP connection.
3. Besides, multiple RPC modules from different devices can coordinate to make objects across the intranet available via the single connection from one client program, which is called the [`multihop`](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support). And Based on the `Multihop` technique, we have implemented the `Node redundancy/Load balance`.

To give an overview of the `rpc-frmwrk` network architecture, here is a block diagram. The light yellow boxes are the parts this module implements.   
![Image](../pics/rpc-block-diagram.png)
