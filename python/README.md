### Introduction to Python Support for RPC-Frmwrk
The Python support for RPC-Frmwrk is actually a wrapper over the C++ RPC-Frmwrk. It exports all the major features from the C++ library to the Python. The wrapper consists of two part, the Python module and the C/C++ interface library. 
  * The Python Module includes file `proxy.py`, which interfaces with the C/C++ interface library , containing a proxy class, a server class, and some utility classes as the base support for proxy and server respectively.
  * The C/C++ interface library consists of all the `sip` files, `rpcfrmwrk.sip`, `proxy.sip`, `server.sip`, which make up the skelton of the C/C++ interface library. And you need to use SIP to generate the final interface library. [`SIP`](https://www.riverbankcomputing.com/software/sip) is a tool for automatically generating Python bindings for C and C++ libraries, and you can conveniently `pip install sip` to install it.
  * The `pyproject.toml.tmpl` and `gentoml.sh` can be seen as the `sip`'s makefile.
  * Make sure the version of the Python should be no less than 3.5.

The features the python wrapper can deliver are almost the same as C/C++ RPC-Frmwrk, except the two test cases `inproctst` and the `btinrtst`, which involves in-process IPC. You can refer to the tests directory to find more details about the supported features.

So far the document is not complete, and if you want to start development quickly, please refer to the test cases in the `tests` directory.

### Building the Python Support for RPC-Frmwrk
So far, I have not integrated the sip building process perfectly with that of other modules of RPC-Frmwrk. So you need to take extra steps to get the interface library installed.
 * Change to the `Python` directory and run `make install` to compile and install to the Python's sytem directory `site-packages`. The installation is necessary, and the python script won't work without the installation.
 * Remove the `debug` and `tracing` flags from `pyproject.toml.tmpl` if you want a release version without debug symbol. And set `library-dirs` to the directory where release version of runtime libraries resides. 
