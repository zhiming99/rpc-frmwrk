#### Indroduction
* This directory contains the typical examples that would be useful in distributed application development.

* The implementations of the examples are grouped in different languages, Java, Python and C++.

* The files under the directory of each example are only files manually changed from its auto-generated version. Run the command at the top lines of each ridl file to generate the complete set of files. The examples include,

    * `hellowld.ridl`: To demonstrate the helloworld example as the start-point for a beginner. 

    * `iftest.ridl`: To demonstrate how to define and pass complex data structures between the proxy and server. 

    * `asynctst.ridl`: To demonstrate how to call remote procedure asynchronously and how to service a remote call asynchronously

    * `evtest.ridl`: To demonstrate how to send a event/signal from server side and how to handle a signal or event from client side

    * `actcancel.ridl`: To demonstrate how to cancel an ongoing request from client side actively.

    * `katest.ridl`: To demonstrate how to control the keep-alive heartbeat between the proxy and server.

    * `stmtest.ridl`: To demonstrate the basic chatting session with the streaming support.

    * `sftest.ridl`: To demonstrate how to upload/download files with the streaming support.

#### Generating the example program of `hellowld`
   * Make sure `rpc-frmwrk` has been properly setup on both server and client host.
   * Let's use C++ `hellowld` for example.
   * Change to the subdirectory `cpp`
   * Run `ridlc -O ./hellowld ../hellowld.ridl`
   * Change to the subdirectory `cpp/hellowld`
   * Run `make` to build the hellowld project on both server and client host.
   * Under the `release` directory, you will get `HelloWorldsvr` as server program, and HelloWorldcli as client program.
   * Check the `README.md` for detain information about the files.
   * The generated files under `hellowld` directory are   
   ![tree](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/hellowld-tree.png)
   * The output of the client program is as follows  
   ![output](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/hellowld.png)
