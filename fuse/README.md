### The Introduction of FUSE Integration
This module enables `rpc-frmwrk` to operate via a set of files as known as the FUSE, the userspace filesystem, which makes `rpc-frmwrk` unique among all the RPC systems on the planet. And it has the following major features:
  * You can generate the RPC filesystem for both server/client side by the `ridl` file fully automatically.
  * All the requests/responses/events are sent and received by reading/writing specific files.
  * The difference of synch/async programming depends only on how you polling the respective file descriptiors. 
  * The request/response/events are in JSON format, and programming language neutral.
    For example, you can even send a request via shell commands like `cat requests_file > jreq_0`. 
  * You can create stream channel by `touch stream_0` or system call `open` under a well-known directory.
    The stream channel is similiar to a Unix Domain Socket over the internet connection, and protected
    transparently by double encrypted channel of SSL & kerberos.
  * You can increase the concurrency of request handling by sharing the fd between thread, 
    creating new request files `jreq_1` , or adding new instances of `service_point` on both client and server sides.
    And you can reduce the resource occupation by `rm jreq_1` or `rmdir service_point` or your favorite file removing functions. 
  * Monitoring the working status of `service point` via `svcstat` files.

### Illustration of the Directory Hierarchy of a generated filesystem.
[this screenshot](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfs-cli.png)

  
