# rpc-frmwrk

这是一个嵌入式的RPC框架项目，关注于跨网络的互联互通。本项目欢迎有兴趣的人士加入!   
This is an asynchronous and event-driven RPC framework for embeded system with small system footprint. It is targeting at the IOT platforms, high-throughput, and high availability over hybrid network. Welcome to join!  

#### Dependency:  
This framework depends on the following packags:  
1. `dbus-1.0 (dbus-devel)`
2. `libjson-cpp (jsoncpp-devel)` 
3. `lz4 (lz4-devel)`   
4. `cppunit-1 (for the sample code, cppunit and cppunit-devel)`   
5. `glib-2.0 (for compile only,glib2-devel)`   
#### Features:   
1. `Support for multiple interfaces on a single object (COM alike).`   
2. `Support for synchronous/Asynchronous requests and handling.`   
3. `Active canceling.`   
4. `Server side event broadcasting.`   
5. `Keep-alive for time-consuming request.`   
6. `Pausable/Resumable interface.`
7. `Support RPC from remote machine, local system, and in-process.` 
8. `Transparent support for different kinds of remote communications.`
9. `File/Big data transfer.`
10. `Peer online/offline awareness.`
11. `Streaming support to provide double-direction stream transfer`
12. `Websocket/http support`(to come)

---
[`Sat 30 Nov 2019 02:33:06 PM Beijing`]   
1. Still writing openssl support, and stucked with the SSL's renegotiation. It should be done next week.   

[`Mon 25 Nov 2019 03:58:57 PM Beijing`]   
1. added LZ4 compression option to the router traffic. the `rpcrouter` now accepts command line option `-c` to enable compression on the outbound packets from the CRpcNativeProtoFdo port.
2. SSL support next...

[`Sat 23 Nov 2019 01:38:46 PM Beijing`]   
1. Finally, it took 20 days to make the new `tcp port stack` in place. Both the old `tcp port stack` and the new stack can be useful in different use cases. And therefore let's keep both alive for a period.
2. the lesson learned is that it is always better to make full research in advance than to rewrite afterward.
3. Now we can move on to add the following features, compression, SSL, websocket/http2 support.

[`Sat 09 Nov 2019 10:09:12 AM Beijing`]   
1. Refacting the tcpport. I did not realize that through the days, the tcpport has grown to a big module with about 10000 loc.
2. Continue the design of the support for websock/http.
3. Object addressing and multi-router hopping are also in the research.

[`Sun 03 Nov 2019 02:05:39 PM Beijing`]   
1. To add the support for websocket, SSL and other unknown protocols, I need to refact the current tcp port implementation, because the present design is too closely coupled without room for the new protocols. It is a bitter decision, but it should worth the effort. Sometime, you may need one step back for 2 steps forward. The good part is that the tcp port implementation is relatively isolated with little changes to other functions.

[`Fri 01 Nov 2019 04:47:07 PM Beijing`]   
1. Changes for Raspberry PI/2 PI is commited. An ARM platform is supported!   

[`Thu 31 Oct 2019 07:36:50 PM Beijing`]   
1. Just get the Raspberry PI/2 to work. the performance is about 15ms per request. Anyway the ARM support is coming soon.
2. The ideal http support is not a trivial task. It may take two months to get it run. Preparing to get hand dirty...
3. Some wonderful feature is brewing, hehe...

[`Thu 24 Oct 2019 10:31:14 AM Beijing`]   
1. IPv6 support is done.
2. It turns out http support depends on the presence of some new features. It needs some time to have a full understanding of the task.
   * Object addressing mechanism
   * multi-hop routing
   * Session management and access control
   * Site registration and discovery.

[`Mon 21 Oct 2019 05:34:24 PM Beijing`]   
1. Reduced the size of the request packets, by removing some redudant properties from the request configdb.
2. Having difficulty choosing between websocket or http/2, as well as the session manager. So let me add the support for IPv6 first as a warm-up execise.   

[`Thu 17 Oct 2019 07:36:03 PM Beijing`]   
1. Worked a `helloworld` with built-in router as the `btinrt` test, for the curiosity to know the performance of removal of dbus traffic. The outcome shows improvement of the latency from about 1.5ms to about 1ms per echo, as about 30% faster, in sacrifice of the flexibility. Anyway, well worth the effort. Also fixed a hanging bug in router and a segment fault in taskgroup which cannot reproduce in the stand-alone router setup.

[`Wed 09 Oct 2019 09:11:53 AM Beijing`]   
1. Upgraded the synchronous proxy code generation with the new macro, before the start of session manager design. It is now an easy task to write a proxy with just a few lines, and help the developers to focus on server side design. For detail information, please refer to test/helloworld.   

[`Mon 30 Sep 2019 02:53:58 PM Beijing`]
1. Finished rewriting the `sftest` and `stmtest` tests with the stream interface. And fixed many bugs.   
2. Next target is to get session manager to work. It is a premise task before the support for the `websocket` connection.   
3. Happy National Day! :)

[`Tue 17 Sep 2019 09:56:09 PM Beijing`]   
1. Major improvement! This project supports 64bit X86_64 now. Yeah!   

[`Wed 11 Sep 2019 07:16:37 PM Beijing`]   
1. fixed some bugs in the streaming module. It should be more stable now. There should still be a few bugs to fix.       

[`Sun 01 Sep 2019 09:11:20 AM Beijing`]   
1. Still writing some helper classes for the stream interface.

[`Wed 21 Aug 2019 10:21:16 AM Beijing`]   
1. The major streaming bugs are cleared so far. Next is to improve the stream interface for the proxy/server because current API is raw and difficult for third-party development.
2. the second task is to rewrite the file upload/download support via the streaming interface.
3. And the third one is to enable the project 64bit compatible in the next month.

[`Wed 21 Aug 2019 08:13:14 AM Beijing`]   
1. Encountered a concurrency problem in the router's stream flow-control these days. However it should be soon to be fixed.

[`Fri 16 Aug 2019 09:04:06 PM Beijing`]   
1. The stream channel can finally flow smoothly. There should still be some bugs around. Still need sometime to get it stable. Congratulations!   

[`Sun 11 Aug 2019 07:11:55 PM Beijing`]
1. FetchData works now. And moving on to the streaming data transfer over the tcp connection. I can see the light at the end of the tunnel...   

[`Wed 07 Aug 2019 09:48:22 PM Beijing`]   
1. OpenStream works now. Next to get the FetchData to work over the Tcp connection. It should be ok very soon...

[`Fri 02 Aug 2019 01:25:29 PM Beijing`]   
1. Start debugging the remote streaming now. Impressed by the streaming performance, and thinking if it is possible to route the RPC traffic via a streaming channel...   

[`Sat 27 Jul 2019 09:36:00 PM Beijing`]   
1. Made the local streaming work now. There remains some performance issue to solve. And afterwards to move on to the streaming over the router. It should last longer than local streaming.   

[`Sun 21 Jul 2019 02:25:01 PM Beijing`]   
1. Coding is fun and debugging is painful. ...   

[`Sat 20 Jul 2019 01:50:34 PM Beijing`]
1. Streaming support code complete finally. The next step is to get it run.

[`Mon 15 Jul 2019 06:02:27 PM Beijing`]
1. The last piece of code for streaming support is growing complicated than expected. Still trying to weld the two ends between tcp sock and unix sock gracefully inside the router. And struggling with many duplicated code with trivial differences.   

[`Wed 10 Jul 2019 04:02:20 PM Beijing`]
1. Almost code complete with the streaming support, remaining start/stop logics for the streaming channel in the bridge object. 

[`Sat 22 Jun 2019 02:03:53 PM Beijing`]   
1. Still busy designing the `flow control` for the streaming support. It turns out to be more complicated than expected. Just swaying between a simple solution and a complex one.   

[`Thu 13 Jun 2019 06:53:01 AM Beijing`]   
1. the CStreamServer and CStreamProxy are undergoing rewritten now. The unix socket operations will be put into a new port object, and all the I/O related stuffs will go to the port object. and the CStreamServer and CStreamProxy are no longing watching all the unix sockets, and instead, associate with a unix socket port object. The port object handles all the events related to the socket.   

[`Mon 03 Jun 2019 02:24:31 PM Beijing`]   
1. The earlier implementaion of `SEND/FETCH DATA` does not work for `TCP` connection. I need to tear it apart and rewrite it. The issues are:
*    Security problem with `SEND_DATA`. That is the server cannot filter the request before all the data has already uploaded to the server.
*    Both `SEND_DATA` and `FETCH_DATA` are difficult to cancel if the request is still on the way to the server, which is likely to happen over a slow connection.
*    `KEEP_ALIVE` and `OnNotify` becomes complicated.
2. The solution is to use the streaming interface to implement the `SEND_DATA/FETCH_DATA`, so that both the server and the proxy can control the traffic all the time.   
   

[`Sat 01 Jun 2019 05:48:48 PM Beijing`]   
1. Added a Wiki page for performance description.   

[`Mon 27 May 2019 08:13:24 PM Beijing`]   
1. Got sick last week, so long time no update.
2. The test cases, `Event broadcasting`, `KEEP-ALIVE`, `Pause-resume`, `Active-canceling`, and `Async call` are working now over both IRC and RPC, as well as `helloworld`.
3. Next move on to the support for SendData/FetchData RPC. 

[`Sun May 12 14:03:47 Beijing 2019`]   
1. The `communication channel setup` (`EnableRemoteEvent`) is almost OK for tcp connection. There should still be a few `hidden` bugs to fix in the future testing. Anyway the `helloworld` can now work more stable and faster. And both the `bridge` and `forwarder` are more torlerable on the error condtion.   
2. Next I will try to get `File/Big data transfer` work. It should be the last big section of un-debugged part so far.   
3. I will also get the other examples to run over TCP connection.   

[`Fri May 03 13:15:23 Beijing 2019`]   
1. Now the `helloworld` can work over the tcp connection. **Great!**   
2. There are some crash bugs and wrong behavor to fix yet.   
3. Happy International Labour Day 2019.   

[`Wed Apr 24 18:10:46 Beijing 2019`]   
1. The `bridge` side is now in good shape. Next I will take some time to clear some bugs on the `forwarder` side, before moving on to run the helloworld over the tcp connection.   

[`Sun Apr 21 12:36:37 Beijing 2019`]   
1. There is one last memory leak to fix yet. After done, we can wrap up the `EnableRemoteEvent` and move on to debugging client requests.   

[`Thu Apr 11 14:06:43 Beijing 2019`]   
1. Fixed some fatal memory leaks and concurrency bugs. Hopefully, the biggest memory leak will be fixed in the next check-in. But the progress is a bit lagging :(    

[`Wed Mar 27 17:56:33 Beijing 2019`]   
1. Fixed some bugs in the code. There are still some memory leak and concurrency bugs to fix in the `EnableRemoteEvent` and connection setup.

[`Thu Mar 14 20:47:45 Beijing 2019`]   
1. The `EnableRemoteEvent` works on the `Forwarder` side. and The bridge side has successfully received the request. Move on to get the `Bridge` side `EnableRemoteEvent` to work. This should be the last part work before getting through the whole request/response path.   

[`Thu Mar 07 23:35:20 Beijing 2019`]   
1. Fixed the memory leak issues. Move on the get the CRpcControlStream to work.   

[`Tue Mar 05 14:05:40 Beijing 2019`]   
1. Encountered and fixed some bugs in the `CRpcRouter`. There are still some memory leaks to fix. One known bug is that, the `OpenRemotePort` and `CloseRemotePort` need to be sequentially executed rather than in random order. Otherwise, the `CRpcTcpBridgeProxyImpl` and the `CTcpStreamPdo` may not be able to be released properly.   
2. After the leaks are fixed, I will get `EnableRemoteEvent` to work.   

[`Sun Feb 24 14:52:28 Beijing 2019`]   
1. This week, I had some emergency thing to handle. I will get back next week.   

[`Sat Feb 16 17:35:54 Beijing 2019`]   
1. `OpenRemotePort` is done now. The CDBusProxyPdo and CDBusProxyFdo are found lack of the `online/offline` handler for both remote server and remote module, and need to add some code next week. And after that, we can move on to debug `EnableEvent` related stuffs.   

[`Thu Feb 07 21:17:48 Beijing 2019`]   
1. Still debugging the CRpcRouter. The `OpenRemotePort` is almost completed now. And there still need a disconnection handler on the `reqfwdr` side to clean up the context for the unexpectedly disconnected proxy.   

[`Thu Jan 24 17:03:14 Beijing 2019`]   
1. Continued to refact part of the CRpcRouter. Now I expect there are some trivial patches need to be done in the coming days of this week before the refact task is done.   

[`Sat Jan 19 23:43:28 Beijing 2019`]   
1. The redesign and refact work are almost done. Next week we can start the debugging of RPC module hopefully.   

[`Fri Jan 11 08:32:13 Beijing 2019`]   
1. There are several places in the router to redesign for new requirements/discoveries. And the details have appended to the todo.txt.   

[`Tue Jan 01 20:53:12 Beijing 2019`]   
1. This week, I need to redesign the management part of the router for the bridge and reqfwdr's lifecycle management, and add new command to the tcp-level protocol for connection resiliance.   
2. Note that, if you want to run `helloworld`, please use the earlier version `bf852d55ae2342c5f01cc499c59885f50780c550` of `echodesc.json` instead. The latest version is modified for RPC debugging purpose.

[`Tue Dec 18 21:35:54 Beijing 2018`]   
1. Made the proxy ports loaded and work. Next step is to get the req-forwarder to run in a stand-alone process.
2. We have two approaches to deploy the proxy, the compact way, that is, the proxy and the req-forwarder works in the same process, and the normal way, that an rpc-router process runs as a router for all the proxys system-wide. I will first make the normal way work. The significance of compact way is to have better security in communication than the normal way if the device is to deploy in a less trustworthy environment.

[`Fri Dec  7 22:45:23 Beijing 2018`]
1. Finally, I have wrapped up local IPC testing. Now I will proceed to RPC debugging and testing.

[`Mon Dec 03 17:47:55 Beijing 2018`]
1. Added local stream support. It is yet to debug.   

[`Tue Nov 20 13:02:20 BeijingBeijing 2018`]
1. Added message filter support and multiple-interface support. Sorry for not starting the RPC debugging still, because I need to dig more in streaming support which will affect RPC module a bit.   

[`Wed Nov 14 11:24:42 Beijing 2018`]
1. Local `Send/Fetch` is done. And now it is time to move on to the RPC module. For security purpose, I will add the authentication interface sometime after the RPC module is done.   

[`Fri Nov  2 18:24:51 Beijing 2018`]   
1. This week, the `keep-alive` and `Pause/Resume` works.
2. Next week, I will try to get `Send/Fetch file` to work, before I can move on to RPC module.   
   
[ `Tue Oct 23 19:59:39 Beijing 2018` ]   
1. This week, I will get keep-alive to work and move on to make RPC module to work.   
   
[ `Mon Oct 15 18:51:48 Beijing 2018` ]  
1. The dependency of `g_main_loop` from `glib-2.0` is replaced with a simplified `mainloop`.   
2. the glib headers are still needed at compile time. But the glib shared libraries are not required at deploy time.   
   
[ `Thu Oct  4 12:24:00 Beijing 2018` ]
1. After some tests, I cannot find a way to put `libev` in the concurrent environment flawlessly.
So `libev` is not an option any more. [(Reason)](https://github.com/zhiming99/rpc-frmwrk/wiki/Why-libev-cannot-be-used-in-rpc-frmwrk%3F)  
2. I will write a simple mainloop with `poll` to fix it.   

