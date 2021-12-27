**Brief Description**
* This directory contains the typical examples that would be useful in distributed application development.
* The implementations of the examples are grouped in different languages, Java, Python and C++.
* The files under the directory of each example are only files manually changed from its auto-generated version. Run the command as listed in each ridl file to generate the complete set of files for each example.

**Introductions to the Individual Examples**
  * `hellowld.ridl`: To demonstrate the helloworld example as the start-point for a beginner. 

  * `iftest.ridl`: To demonstrate how to define and pass complex data structures between the proxy and server. 

  * `asynctst.ridl`: To demonstrate how to call remote procedure asynchronously and how to service a remote call asynchronously

  * `evtest.ridl`: To demonstrate how to send a event/signal from server side and how to handle a signal or event from client side

  * `actcancel.ridl`: To demonstrate how to cancel an ongoing request from client side actively.

  * `katest.ridl`: To demonstrate how to control the keep-alive heartbeat between the proxy and server.

  * `stmtest.ridl`: To demonstrate the basic chatting session with the streaming support.

  * `sftest.ridl`: To demonstrate a simple file-server with the streaming support.
