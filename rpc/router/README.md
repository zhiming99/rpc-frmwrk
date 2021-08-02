#### Name
rpcrouter - the daemon program for `rpc-frmwrk`

#### Synopsis
rpcrouter [ options ]

#### Description
rpcrouter must run on both side of the client and server hosts to accomplish the request/response relay, and event broadcasting. On the server host, it must be invoked with the option `-r 2`, and on the client host, it must be invoked with option `-r 1`. If both client/server are on the same host, run both on the same host, too.

It is not required, if your client/server program just do some IPC, as known as Inter-Process-Communication.

#### Options
* -r _role number_ 1: running on the client side AKA a `reqfwdr`, 2: running on the server side AKA a `bridge`. It is a mandatory options, and must be present on the command line.

* -a To enable the authentication. So far, kerberos is the only authentication mechanism supported.

* -c To establish a seperate connection to the remote `bridge` for each proxy. It is an option for `reqfwdr` only. The seperate connections has both advantages and disadvantages. The pros is that each connection has its own quota of concurrency, without sharing with other proxy instances. the disadvangage is that it requires a full connection setup, which has the probability to timeout if the number connection requests are high.

* -f To enable the `request-based flow control`. The `bridge` with this option, will limit the outstanding requests to 2 * connections, and buffered requests to 4 * connections, and requests beyond this limit will be rejected with error `ERROR_QUEUE_FULL`. The `reqfwdr` with this option will buffer some number of rejected requests and retry them later, or pass on `ERROR_QUEUE_FULL` to the proxy if the client has overdone it without stopping. 

#### Examples
*   On the server side, `rpcrouter -r 2` for non-authentication `bridge`, and `rpcrouter -afr 2` for `bridge` with both authentication and flow-control.
*   On the client side, `rpcrouter -r 1` for non-authentication `bridge`, and `rpcrouter -afr 1` for `bridge` with both authentication and flow-control.

#### Related Files
*   router.json, rtauth.json, authprxy.json
