### The Introduction to FUSE Integration and the `rpcfs` filesystem
This module enables `rpc-frmwrk` to operate via a set of files as known as the `rpcfs`, supported by FUSE, the userspace filesystem. And it has the following major features:
  * Generating the RPC filesystem `rpcfs` on both server/client side with an `ridl` file fully automatically.
  * All the requests/responses/events are sent and received by reading/writing specific files.
  * Programming with `rpcfs` requires only file operations and JSON support, and very little training effort.
  * The request/response/events are in JSON format, and programming language neutral.
    For example, you can even send a request via shell commands like `cat requests_file > jreq_0`. 
  * Creating stream channels by `touch stream_0` under `stream` directory, as leads to a system call `open`.
    The stream channel delivers full-duplex bytes stream transfer through double encrypted channel of SSL & kerberos.
  * Easy management of the concurrency and resource consumption by adding/removing files or adding/removing directories.
  * Monitoring the working status of `service point` with the read-only file `svcstat`.
  * Restarting/reloading individual `service point` online.
  * Supported filesystem operations: `open` `close` `read` `write` `unlink` `stat` `rmdir` `mkdir` `readdir` `releasedir` `poll` `ioctl` 
  * Dynamically loading pluggable `service points` under a single mountpoint.

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
* A formal request consists two parts, a network-order 32bit `string length` followed by a JSON string, without `\0`. The following picture shows the request/response and relationships with the [`ridl file`](https://github.com/zhiming99/rpc-frmwrk/blob/master/examples/hellowld.ridl). And for detail information about `ridl language`, please refer to this [article](https://github.com/zhiming99/rpc-frmwrk/blob/master/ridl/README.md)   
![req/resp format](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/ridl-req-mapping.png)    

### Using Streams
* Stream is a binary channel between the server and the proxy. Stream files are those files created under `streams` directory, as shown in the above picture. There is a pair of stream files sharing the same name the `steams` directory on the both end of the RPC connection. The proxy side stream file is always first created, and then the server side is created. The server side cannot create the stream file on its own.
* Unlike the req file, the resp file or the event file, which can do single direction transfer, the stream channel is a full duplex byte stream channel, and you can read/write the file concurrently. 
* The stream channel has a flow control mechamism, and if the peer has many data packets accumulated in the receiving queue, the sending party will be blocked till the peer has consumed some of them, that is, the last `send` becomes synchronous till the flow control is lifted by the peer.
* The advantage of the stream channel is that it is much faster than the request/reponse transfer. And it's limitation in size is 2^64 bytes at most , compared to 1MB limit along with the normal request.
* Tip: To access a remote shell, you can do this `bash -i < /path-to-stream_0 &>/path-to-stream_1` on the server side and on the client side, type `echo ls -l > /path-to-stream_0` and `cat /path-to-stream_1` to run a `ls` command remotely and show the output locally. Note that is very dangeouse to do so over internet unless SSL and authentication are enabled.

### Managing the Concurrency with `rpcfs`
According to the above `rpcfs` struture, You can increase the concurrency and throughput for request handling by
 * Sharing the request/response files between threads, 
 * Creating new request files like `jreq_XXX` 
 * Making directories like `TestTypesSvcXXX_XXX` under the same parent directory as `TestTypesSvc`.

And vice versa, by removing `jreq_XXX` or `TestTypesSvcXXX_XXX`, you can reduce the concurrency or the resource load. 
### Terminology
  * `rpcfs` is  an RPC system with the file system as its interface, but not a filesystem accessed via RPC.
  * `service point` is either the server object or proxy object at the either end of an RPC connection.
  * `jreq_0` is a name of a file to input requests on client side or ouput requests on the server side. The name has a fixed prefix `jreq_` followed by a   customized string. There are `jrsp_0` and `jevt_0` too if there is a `jreq_0`. And the `jrsp_0` is for the respones of the requests sent via `jreq_0` on the client side. While `jevt_0` receives a copy of event message from the server, the same as that goes to `jevt_1`, `jevt_hello`, or whatever if exists.

  
