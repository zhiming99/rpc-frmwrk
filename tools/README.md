### RPC Router config Tool
`rpcfg.py` is a GUI tool for rpcrouter configuration. It generate the configuration files including `driver.json, router.json, rtauth.json, and authprxy.json`. Please refer to this [article](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/readme.md), for `rpcrouter`'s concept.
The UI dialog consists of the following tab pages
* Connection page. Mainly to setup the ip address and port number to listen to remote connections. It is recommended not to use `0.0.0.0` whenever possible. Otherwise, you need to manually setup the destination IP addresses in `router.json`, `rtauth.json` and `authprxy.json`.   
  ![rpcfg tab1](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg.png)
  
* Security Page. The infomation on this page takes effect only when you have enabled the related options on the Connection Page. The detailed illustration of generation of key and certification files can be found at [SSL](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport). And the explaination of `Auth Information` can be found [here](https://github.com/zhiming99/rpc-frmwrk/tree/master/rpc/security#4-enable-authentication-for-rpc-frmwrk).   
  ![rpcfg tab2](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg2.png)
* Multihop Page. To add or remove the downstream nodes which also provide RPC services to the remote client. And [this link](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support) is an introduction to `multihop` technology. The parameters are for the current `rpcrouter` to connect to the downstream node, but not for connections from the user clients.   
  ![rpcfg tab3](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg3.png)
* Load Balance Page. Based on the `multihop` technology, `rpcrouter` can easily be configured to have the `load balance` support. However, currently the `load balance` support just one policy, the `round robin`. And you can refer to [this article](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support#node-redundancyload-balance) for more information.   
  ![rpcfg tab4](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg4.png)
  

### Quick Start with Dockerfile
If you are familiar with docker, and has no patient to setup the building environment,  you can use this tool to quickly setup the building environemt.
  * Open a terminal, and change to the directory `tools`.
  * Type `docker build -t 'rpcf-buildenv' ./`.
  * Tweak the `Dockerfile` to customize the image you want to build.
  * This is a minimum environment without X, so the `rpcfg.py` cannot run in this container. You can either install X on the container or use `rpcfg.py` on your host to export a desired set of configuration, and upload them to the container.
