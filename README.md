# rpc-frmwrk
An effort for embedded RPC framework, and hope it could find its usage in IOT or other embedded systems.   
#### Dependency:  
This framework depends on the following packags:  
1. `dbus-1.0`  
2. `libjson-cpp`  
3. `cppunit-1 (for the sample tests only)`   
4. `glib-2.0 ( for compile only )`   
#### Features:   
1. `Multiple interfaces on a single object (COM alike).`   
2. `Synchronous/Asynchronous request and handling.`   
3. `Active canceling.`   
4. `Server side event broadcasting.`   
5. `Keep-alive for time-consuming request.`   
6. `Pausable/Resumable interface.`
7. `Support RPC from remote machine, local system, and in-process.` 
8. `Transparent remote transportation layer.`
9. `File/Big data transfer.`


---
[ `Tue Oct 23 19:59:39 CST 2018` ]   
1. This week, I will get keep-alive to work and move on to make RPC module to work.   
   
[ `Mon Oct 15 18:51:48 CST 2018` ]  
1. The dependency of `g_main_loop` from `glib-2.0` is replaced with a simplified `mainloop`.   
2. the glib headers are still needed at compile time. But the glib shared libraries are not required at deploy time.   
   
[ `Thu Oct  4 12:24:00 CST 2018` ]
1. After some tests, I cannot find a way to put `libev` in the concurrent environment flawlessly.
So `libev` is not an option any more. [(Reason)](https://github.com/zhiming99/rpc-frmwrk/wiki/Why-libev-cannot-be-used-in-rpc-frmwrk%3F)  
2. I will write a simple mainloop with `poll` to fix it.   

