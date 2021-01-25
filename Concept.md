# RPC-frmwrk Concepts Overview

Remote Procedure Calls (RPCs) provide a useful abstraction for building
distributed applications and services. The libraries in this repository
provide a concrete implementation of an `IPC/RPC` combined communication 
framework. These libraries enable communication between clients and servers
using c++ at present. And other language such as python will be supported
in the future.


## RPC Communication Participants and Roles


The system uses the concept `Server` and `Proxy` to represent the
communcation role of the RPC participants. `Proxy` sends out the request
and `Server` accepts, processes and return responses to the `Proxy`. Also
`Server` can broadcast events and `Proxy` can subscribe the events
interested. Usually, the role of `Proxy` and `Server` are not
interchangable. And the `Server` and the `Proxy` has a one-to-many
relationship, that is, the single `Server` instance can service mulitple
`proxy` instances at the same time.


### RPC, IPC and In-Process

RPC-frmwrk supports `RPC` as remote process call, `IPC` as inter-process
call, and `In-Process` as in-process call. That is, a `Server` can
simutaneously be accessed by `Proxies` from internet, different processes,
or different threads. A `Proxy` can switch to different target `Server` by
modifying its configuration file, as `XXXdesc.json` from the sample codes
under the `test` directory.

#### Security over the Network

RPC-frmwrk can be configured to use WSS(Secure WebSocket) for the
communication with the web server over the internet. And it can also be
configured to use SSL within the intranet. The pluggable authentation and
access control will be supported in the future.

## Object and Interface

The RPC-frmwrk utilizes the DBus for the IPC communication. It also uses
the `DBusMessage` as one of the several message formats in the RPC
communication. And thereby, RPC-frmwrk inherits from DBUS, the concept of
`Object` and `Interface` to identify the `IPC/RPC server`.  You can refer
to the [`DBus's
documentation`](https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names-bus)
for detailed information.  Basically, you can view the `Object` as a host
who provides the different services via different `interfaces`. And
therefore, `Object` has a `one-to-many` mapping to the `interfaces`
similiar as that of `Server` and `Proxy`. And the `Object Path` and
`Interface Name` are two important info to address an `Object` in the IPC
communication.  However, the RPC call goes beyond, when An `Object` can be
addressed by `TCP connection`, `URL`, `RouterPath` plus `Object Path` and
`Interface Name`.


## Synchronous vs. Asynchronous
Synchronous RPC calls, that block until a response arrives from the server,
are the closest approximation to the abstraction of a procedure call that
RPC aspires to.

On the other hand, networks are inherently asynchronous and in many
scenarios, it is desirable to have the ability to start RPCs without
blocking the current thread.

The RPC-frmwrk user API comes in both synchronous and asynchronous flavors.

## Stateful Connection

Before the RPC calls can be delivered to the Server, the RPC-frmwrk will
setup a connection between the proxy/server. The connection is dedicated to
the proxy/server pair. And only the requests can be accepted with the valid
`Object Path`, `Interface name` plus the specific `TCP connection` and
`RouterPath`. The connection also delivers the event from the server to all
the subscriber proxies. Besides the RPC calls and events, the connection
also carries the streaming messages between proxy/server in either
direction.


### RPC calls and events

An RPC call is a request from proxy to the server, and most likely a
response will return with the desired result or error code if the request
fails.

An RPC event is a message broadcasted by the server to all the interested
proxies. It is a one-way message without response.

### Streaming

RPC-frmwrk supports streaming semantics, where the `Proxy` requests to
setup a `stream channel` with the `Server` over the establised connection
between the proxy and the server. After the channel is set up sucessfully,
bi-directional byte stream can start. The streaming channel has better
flow-control and more sensitive connection status awareness.

Streaming transfer are for the scenarios where large data transfer is
necessary. Normal RPC request/response has a upper limit of 1MB. When the
request or the response exceeds the limit, You are recommend to use one
or more stream channels to exchange data between proxy/server. The stream
channels can last till the proxy/server closes them. The upper limit of
the capacity is 2^64 bytes per stream channel.

## Development

The RPC-frmwrk provides a set of API to facilitate the development with
the powerful support from `C++11`. A typical distributed RPC application
makes up of a proxy and a server. The proxy can be developed with some
system provided macros in most cases. And your efforts can focus on the
implementation of the features the server delivers. It is recommended to
refer to the sample codes in the [`test`](https://github.com/zhiming99/rpc-frmwrk/tree/master/test)
directory as the start-point of your RPC development.


