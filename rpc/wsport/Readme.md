### Technical Spec:   

  1. The data is transferred in binary format in a `BINARY_FRAME`.
  2. The max bytes per frame is 1MB.
  3. Support to echo `PING_FRAME`, but will not actively pinging from the client.   
  4. Support to respond `CLOSE_FRAME`, but will not actively `CLOSE_FRAME` at present.   
  5. The `protocol` header option is set to `rpc-frmwrk` in the handshake so far.

  
  
