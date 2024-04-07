### 简介
`rpc-frmwrk`的JavaScript模块是一个运行在浏览器中的功能全面的客户端。用户可以通过`ridlc`生成的JS框架，添加适当的业务逻辑，即可访问已有的`rpc-frmwrk`服务器。JavaScript模块的优势是部署快速，且跨平台。

### 技术特点
`rpc-frmwrk`的JavaScript依赖于html5，通过`websocket`和`rpc-frmwrk`的服务器通信。JS的运行库一部分运行在浏览器的`web-worker`上，另一部分运行于`main-thread`上，确保后台RPC传输和前台页面展示不会互相影响。JavaScript模块和Python，Java的客户端一样，可以连接所有类型的`rpc-frmwrk`的服务器，不同之处则是JavaScript的客户端是完全独立的，不依赖C++运行库，而Python和Java的客户端跑在C++的运行库之上，以获得更好的性能和扩展性。JavaScript客户端不支持Kerberos认证，不过下一步，`rpc-frmwrk`服务器添加oAuth2的支持，以使JS客户端有更好的安全特性。

### 注意事项
所有的JS例子程序的部署的链接均被设置为假想的`http://example.com/rpcf`。所以用户在开发时，务必使用`ridlc`的`--odesc_path`选项，替换成真实的网络地址。

