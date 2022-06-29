### Introduction to Java Support for RPC-Frmwrk
The Java support for RPC-Frmwrk is an API wrapper over the C++ RPC-Frmwrk. It exports all the major features from the C++ library to the Java. The wrapper consists of two part, the Java module and the C/C++ interface library. 
  * The Java Module is under the directory `src/org/rpcf/rpcbase`, which interfaces with the C/C++ interface library , and after fully built, contains a proxy class, a server class, and some utility classes as the base support for proxy and server respectively.
  * The source code for C/C++ interface library consists of all the `.i` files, `rpcbase.i`, `proxy.i`, `server.i`, which will generate the skelton of the C/C++ interface library with SWIG. [`SWIG`](http://www.swig.org/Doc1.3/Sections.html#Sections) is a tool for automatically generating Java bindings for C and C++ libraries, and the installation command is `apt install swig`.

### Building your Java RPC applications
The Java support package must be used as the support library for the `ridlc` generated the Java skelton project. The information about `ridlc` can be found [here](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl#invoking-ridlc).

### Technical Information
At present, Java wrapper suport 1 type of serializations for data transfer.
* The ridl serialization, the recommended serialization delivered by ridlc generated Java skelton code. It can communicate with the peer written in different languages. And it is transparent to the application developer.
