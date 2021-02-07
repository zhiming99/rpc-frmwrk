Features of the RPC module:

1. The RPC module enables the server to expose its service objects to the remote clients via a TCP connection.
2. [`Streaming`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md#streaming), [`Websocket`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/wsport/Readme.md), [`OpenSSL`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport/Readme.md) and [authentication](https://github.com/zhiming99/rpc-frmwrk/tree/master/rpc/security/README.md) are also supported over this TCP connection.
3. Besides, multiple RPC modules from different devices can coordinate to make objects across the network available via the single connection to the same proxy, which is called the [`multihop`](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support). And Based on the `Multihop` technique, we have implemented the `Node redundancy/Load balance`.

To give an overview of the `rpc-frmwrk` network architecture, here is a block diagram. The light yellow boxes are the parts this module implements.   
![Image](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpc-block-diagram.png)
