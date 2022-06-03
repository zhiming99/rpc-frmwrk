### The Introduction to FUSE Integration and the `rpcfs` filesystem
This module enables `rpc-frmwrk` to operate via a set of files as known as the FUSE, the userspace filesystem, which makes `rpc-frmwrk` unique among all the RPC systems on the planet. And it has the following major features:
  * You can generate the RPC filesystem `rpcfs` for both server/client side with an `ridl` file fully automatically.
  * All the requests/responses/events are sent and received by reading/writing specific files.
  * The difference of synch/async programming depends only on how you polling the respective file descriptiors. 
  * The request/response/events are in JSON format, and programming language neutral.
    For example, you can even send a request via shell commands like `cat requests_file > jreq_0`. 
  * You can create stream channels by `touch stream_0`, any `open_file` function call of your programming language.
    The stream channel is similiar to a Unix Domain Socket over the internet connection, and protected
    transparently by double encrypted channel of SSL & kerberos.
  * You can increase the extent of concurrency and resource load for request handling by sharing the fd between thread, 
    or creating new request files `jreq_XXX` , or adding new instances of `service point` on both client and server sides.
    And you can reduce the resource load by `rm jreq_1` or `rmdir service_point` or your favorite file removing functions. 
  * Monitoring the working status of `service point` with the read-only file `svcstat`.
  * Restarting/reloading individual `service point`.
  * Supported filesystem operations: `open` `close` `read` `write` `unlink` `stat` `rmdir` `mkdir` `readdir` `releasedir` `poll` `ioctl` 

### The Structures of Generated `rpcfs` Filesystems.
![this screenshot](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfs-cli.png)   
The above picture shows the directory hierarchy of a client side `rpcfs`. The blue texts are directories, and the white texts are regular files.   
![this screenshot](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfs-svr.png)   
The above picture shows the directory hierarchy of a server side `rpcfs`. Notice the differences from the client side `rpcfs`.   
### A Closer Look of the Business Code Interacting With `rpcfs`
Let's use the above generated `rpcfs` to illustrate the control flow.   
* The following graph shows a simple synchronous client side flow chart, as well as a simple asynchronous one.   
![sync-async call](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/sync-async.png)   
* The following graph shows a simple synchronous server side flow chart, as well as a simple asynchronouse one.   
![sync-async call](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/sync-async-svr.png)   

### RPC Request/Response/Event format
* A formal request consists two parts, a network-order 32bit `string length` followed by a JSON string, without `\0`. The following picture shows the request/response and relationships with the [`ridl`](https://github.com/zhiming99/rpc-frmwrk/examples/hellowld.ridl) file. For detail information about `ridl`, please refer to this [article](https://github.com/zhiming99/rpc-frmwrk/blob/master/ridl/README.md)   
![req/resp format](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/ridl-req-mapping.png)    

### Using Streams
* Stream is a binary channel between the server and the proxy. Stream files are those files created under `streams` directory, as shown in the above picture. There is a pair of stream files sharing the same name the `steams` directory on the both end of the RPC connection. The proxy side stream file is always first created, and then the server side is created. The server side cannot create the stream file on its own.
* Unlike the req file, the resp file or the event file, which can do single direction transfer, the stream channel is a full duplex byte stream channel, and you can read/write the file concurrently. 
* The stream channel has flow control, and if the peer has too many data accumulated in the stream file, the sending party will fail till the peer has consumed some of them, that is, `write` operation will fail with `EAGAIN` till flow control lifted.
* The stream channel advantage is that it is much faster than the request/reponse transfer. And it's limitations is size, 2^64 the maximum, compared to 1MB limit per request.

### Managing a Service Point

### Terminology
  * `rpcfs` is  an RPC system with the file system as its interface, but not a filesystem accessed via RPC.
  * `service point` is either the server object or proxy object at the either end of an RPC connection.
  * `jreq_0` is a name of a file to input requests on client side or ouput requests on the server side. The name has a fixed prefix `jreq_` followed by a   customized string. There are `jrsp_0` and `jevt_0` too if there is a `jreq_0`. And the `jrsp_0` is for the respones of the requests sent via `jreq_0` on the client side. While `jevt_0` receives a copy of event message from the server, the same as that goes to `jevt_1`, `jevt_hello`, or whatever if exists.

  
