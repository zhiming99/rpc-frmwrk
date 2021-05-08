`rpcfg.py` is a GUI tool for rpcrouter configuration. It generate the configuration files including `driver.json, router.json, rtauth.json, and authprxy.json`. Please refer to this [article](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/readme.md), for `rpcrouter`'s concept.
The UI dialog consists of the following tab pages
* Connection page. Mainly to setup the ip address and port number to listen to remote connections. It is recommended not to use `0.0.0.0` whenever possible. Otherwise, you need to manually setup the destination IP addresses in `router.json`, `rtauth.json` and `authprxy.json`.   
  ![rpcfg tab1](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg.png)
  
* Security Page. The infomation on this page takes effect only when you have enabled the related options on the Connection Page. The detailed illustration of generation of key and certification files can be found at [SSL](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport). And the explaination of `Auth Information` can be found [here](https://github.com/zhiming99/rpc-frmwrk/tree/master/rpc/security#4-enable-authentication-for-rpc-frmwrk).   
  ![rpcfg tab2](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg2.png)
* Multihop Page. To add or remove the downstream nodes which also provide RPC services to the remote client. And [this link](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support) is an introduction to `multihop` technology. This is an optional configuration.   
  ![rpcfg tab3](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg3.png)
* Load Balance Page. Based on the `multihop` technology, `rpcrouter` can easily be configured to have the `load balance` support. However, currently the `load balance` support just one policy, the `round robin`. And you can refer to [this article](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support#node-redundancyload-balance) for more information.   
  ![rpcfg tab4](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg4.png)
  
