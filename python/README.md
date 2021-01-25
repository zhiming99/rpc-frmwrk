### Introduction to Python Support for RPC-Frmwrk
The Python support for RPC-Frmwrk is actually a wrapper over the C++ RPC-Frmwrk. It exports all the major features from the C++ library to the Python. The wrapper consists of two part, the Python module and the C/C++ interface library. 
  * The Python Module includes file `proxy.py`, which interfaces with the C/C++ interface library , containing a proxy class, a server class, and some utility classes as the base support for proxy and server respectively.
  * The C/C++ interface library consists of all the `sip` files, `rpcfrmwrk.sip`, `proxy.sip`, `server.sip`, which make up the skelton of the C/C++ interface library. And you need to use SIP to generate the final interface library, which can be installed by `pip`.
  * The `pyproject.toml.tmpl` and `gentoml.sh` can be seen as the `sip`'s makefile.

The features the python wrapper can deliver are almost the same as C/C++ RPC-Frmwrk, except the two test cases `inproctst` and the `btinrtst`, which involves in-process IPC. You can refer to the tests directory to find more details about the supported features.
