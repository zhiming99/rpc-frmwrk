### The Introduction to FUSE Integration and the `rpcfs` filesystem
This module enables `rpc-frmwrk` to operate via a set of files as known as the FUSE, the userspace filesystem, which makes `rpc-frmwrk` unique among all the RPC systems on the planet. And it has the following major features:
  * You can generate the RPC filesystem for both server/client side with an `ridl` file fully automatically.
  * All the requests/responses/events are sent and received by reading/writing specific files.
  * The difference of synch/async programming depends only on how you polling the respective file descriptiors. 
  * The request/response/events are in JSON format, and programming language neutral.
    For example, you can even send a request via shell commands like `cat requests_file > jreq_0`. 
  * You can create stream channels by `touch stream_0`, any `open_file` function call of your programming language.
    The stream channel is similiar to a Unix Domain Socket over the internet connection, and protected
    transparently by double encrypted channel of SSL & kerberos.
  * You can increase the concurrency of request handling by sharing the fd between thread, 
    creating new request files `jreq_1` , or adding new instances of `service_point` on both client and server sides.
    And you can reduce the resource occupation by `rm jreq_1` or `rmdir service_point` or your favorite file removing functions. 
  * Monitoring the working status of `service point` via `svcstat` files.

### The Structure of a Generated `rpcfs` Filesystem.
![this screenshot](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfs-cli.png)   
The above picture is the directory hierarchy of a client side `rpcfs`. The blue texts are directories, and the white texts are regular files.

### How to Make an RPC call via `rpcfs`
Let's use the above example client side `rpcfs` to illustrate the control flow, as shows in the following graph.   
* The following graph shows a client side processing sequence   
![sync-async call](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/sync-async.png)   
* The following graph shows a server side processing sequence   
![sync-async call](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/sync-async-svr.png)   
  
