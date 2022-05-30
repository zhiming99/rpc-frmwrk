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
  * Restarting/reloading/adding individual `service point`.

### The Structures of Generated `rpcfs` Filesystems.
![this screenshot](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfs-cli.png)   
The above picture shows the directory hierarchy of a client side `rpcfs`. The blue texts are directories, and the white texts are regular files.   
![this screenshot](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfs-svr.png)   
The above picture shows the directory hierarchy of a server side `rpcfs`. Notice the differences from the client side `rpcfs`.   
### A Closer Look of the Business Code Interacting With `rpcfs`
Let's use the above client side `rpcfs` to illustrate the control flow as the example.   
* The following graph shows a simple synchronous client side flow chart, as well as a simple asynchronous one.   
![sync-async call](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/sync-async.png)   
* The following graph shows a simple synchronous server side flow chart, as well as a simple asynchronouse one.   
![sync-async call](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/sync-async-svr.png)   

### RPC Request/Response/Event format
### Using Streams
### Restart/Reload a Service Point
  
