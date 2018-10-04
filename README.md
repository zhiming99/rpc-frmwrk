# rpc-frmwrk
An effort for embedded RPC framework, and hope it could find its usage in IOT or other embedded systems.

---
[ Thu Oct  4 12:24:00 CST 2018 ]   
1. After some tests, I cannot find a way to put `libev` in the concurrent environment flawlessly.
So `libev` is abandoned.   
2. I will write a simple mainloop with `poll` to fix it.   

[ Thu 20 Sep 2018 10:35:23 AM CST ]

## Dependency:  
This framework depends on the following packags:  
1. `glib-2.0`  
2. `dbus-1.0`  
3. `libjson-cpp`  
4. `libdbus-glib-1`  
5. `cppunit-1 (for the sample tests only)`
  
I will soon replace the `glib` with `libev` to reduce the lib dependency on the target system.
