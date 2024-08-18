[中文](./README_cn.md)
#### Name
rpcrouter - the daemon program for `rpc-frmwrk`

#### Synopsis
rpcrouter [ options ]

#### Description
rpcrouter must run on both side of the client and server hosts to accomplish the request/response relay, and event broadcasting. On the server host, it must be invoked with the option `-r 2`, and on the client host, it must be invoked with option `-r 1`. If both client/server are on the same host, run both on the same host, too.

It is not required, if your client/server program just do some IPC, as known as Inter-Process-Communication.

#### Options
* -r _role number_ 1: running on the client side aka. a `reqfwdr`, abbr. of _request forwarder_, 2: running on the server side aka. a `bridge`. It is a mandatory option.

* -a To enable the authentication. So far, kerberos is the only authentication mechanism supported.

* -c To establish a seperate connection to the remote `bridge` for each proxy. It is an option for `reqfwdr` only. The seperate connections has both advantages and disadvantages. The pros is that each connection has its own quota of concurrency, without sharing with other proxy instances. the disadvangage is that it requires a full connection setup, which has the probability to timeout if the number connection requests are high.

* -f To enable the `request-based flow control`. The `bridge` with this option, will limit the outstanding requests to 2 * connections, and buffered requests to 4 * connections, and requests beyond this limit will be rejected with error `ERROR_QUEUE_FULL`. The `reqfwdr` with this option will buffer some number of rejected requests and retry them later, or pass on `ERROR_QUEUE_FULL` to the proxy if the client has overdone it without stopping. 

* -m _mount point_ : To export rpcrouter's runtime information via 'rpcfs' mounted at the directory 'mount point'. The files are exported in JSON format under 'user' directory,  including 'InConnections' and 'OutConnections' at present.

* -d To run `rpcrouter` as a daemon
* -g To enable logging to the log server.

#### Examples
*   On the server side, `rpcrouter -dr 2` for non-authentication `bridge`, and `rpcrouter -adfr 2` for `bridge` with both authentication and request flow-control.
*   On the client side, `rpcrouter -dr 1` for non-authentication `reqfwdr`, and `rpcrouter -adfr 1` for `reqfwdr` with both authentication and request flow-control.
*   To export the runtime information, add the `-m` option with a directory as a mountpoint, so that the rpcrouter will export several files which contains  statistics counters and important running status about the rpcrouter. `rpcrouter -adfr 2 -m ./foodir`, for example.

#### Related Configuration Files
*   router.json, rtauth.json, authprxy.json, driver.json
