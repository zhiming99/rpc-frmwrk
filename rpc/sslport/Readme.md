. sslfido.cpp, sslfido.h and sslutils.cpp implemented a filter port, as inserted between the upper `Native Protocol Fdo port` and the bottom `Tcp Stream Pdo port`. It encrypts the outbound packets and decrypts the inbound packets as well as performs the `SSL handshake` and `SSL shutdown`, plus the `SSL renegotiation` occationally.   

. The `sslfido` requires OpenSSL-1.1 to work properly. And I have not tested whether it works with lower version so far. And the `SSL renegotioation` is probably not working since there is no test case for it yet.

. OpenSSL requires to have the key file and certificate file ready before it can work.   

. Here is a command line example. Note it generates a self-signed key with no password protection.   
```
# command line
openssl req -x509 -newkey rsa:4096 -nodes -keyout key.pem -out cert.pem -days 365
```

. The `sslfido` related settings are all added to the [driver.json](https://github.com/zhiming99/rpc-frmwrk/blob/master/ipc/driver.json). You can find the detail in this file.
