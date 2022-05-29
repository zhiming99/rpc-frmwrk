### The Introduction of FUSE Integration
This module enables `rpc-frmwrk` to operate via a set of files as known as the FUSE, the userspace filesystem, which makes `rpc-frmwrk` unique among all the RPC systems on the planet. And it has the following major features:
  * All the requests/responses/events are sent and received by reading/writing specific files.
  * The difference of synch/async programming depends only on how you polling the respective file descriptiors. 
  * The request/response/events are in JSON format, and programming language neutral.
    For example, you can send a request via shell commands like `cat requests_file > req_input`. 
  * You can create stream channel by `touch stream_0` or system call `open` under a well-known directory.
    The stream channel is similiar to a Unix Domain   Socket over the internet connection, and protected
    transparently by encrypted channel of SSL plus kerberos.
