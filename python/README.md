### Introduction to Python Support for RPC-Frmwrk
The Python support for RPC-Frmwrk is an API wrapper over the C++ RPC-Frmwrk. It exports all the major features from the C++ library to the Python. The wrapper consists of two part, the Python module and the C/C++ interface library. 
  * The Python Module includes file `proxy.py`, which interfaces with the C/C++ interface library , containing a proxy class, a server class, and some utility classes as the base support for proxy and server respectively.
  * The C/C++ interface library consists of all the `sip` files, `rpcf.sip`, `proxy.sip`, `server.sip`, which will generate the skelton of the C/C++ interface library with SIP. And you need to use SIP to generate the final interface library. [`SIP`](https://www.riverbankcomputing.com/software/sip) is a tool for automatically generating Python bindings for C and C++ libraries, and you can `apt install sip-tools` to install it.
  * The `pyproject.toml.tmpl` and `gentoml.sh` can be seen as the `sip`'s makefile.
  * Make sure the version of the Python be no less than 3.5.

The features the python wrapper can deliver are almost the same as C/C++ RPC-Frmwrk, except the two test cases `inproctst` and the `btinrtst`, which involves in-process IPC. You can refer to the tests directory to find more details about the supported features.

### Building your Python RPC applications
I strongly recommend you to use the `ridlc` to generate the Python skelton code, which can be the start point of your Python RPC application. The information about `ridlc` can be found [here](../ridl/README.md#introduction).

### Technical Information
At present, Python wrapper suport 3 types of serializations for data communication.
* The first is the ridl serialization, the recommended serialization delivered by ridlc generated Python skelton code. It can communicate with the peer written in different languages. And it is transparent to the application developer.
* The second is the native serialization from the C++ RPC-Frwmrk, which support the primary data types, bytes, bytearray plus the serializable `ObjPtr`. It can talk with both C++ server or the Python server. But the disadvantage it that you may need to explicitly serialized complex data structure to bytearray in your business code.
* the last serialization approach supported is the Python's `pickle`, which transparently seralize all the parameters passed down to the RPC-Frmwrk, and deserialized by the peer before passing to the user defined interface. The disadvantage is that, it limit both proxy and server to be written in Python. The lang-neutural serialization approach is still in the making. 

