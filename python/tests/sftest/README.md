This is an advanced test case to demonstrate the streaming mechanism. It is different from the   
CPP version of `sftest`, in the effort to reduce the complexity of the file transfer logic.   

Unlike the normal RPC request, streaming mechanism requires a set of system defined API to work.   
The workflow is as follows,   
  * To establish one or more stream channels from the proxy side with `StartStream`
  * To `Read/WriteStream` to communicate between the proxy and the server. The system provides   
  three Read( or Write ) approaches, as synchronous read/write, asynchronous read/write. And   
  `NoWait` read to peek the incoming queue, `NoWait` write to submit a write request, without   
  waiting for the result.
* To close the stream channel, both proxy and server can launch a CloseStream request, or stop   
the proxy or server directly.
