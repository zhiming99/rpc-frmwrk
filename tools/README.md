`rpcfg.py` is a GUI tool for rpcrouter configuration. It generate the configuration files including `driver.json, router.json, rtauth.json, and authprxy.json`. Please refer to this [article](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/readme.md), for `rpcrouter`'s concept.
The UI dialog consists of the following tab pages
* Connection page. (Note: the red items are mandatory)   
  ![rpcfg tab1](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg.png)
  
* Security Page. The infomation on this page takes effect only when you have enabled the related options on the Connection Page. The detailed illustration of generation of key and certification files can be found at [SSL](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport). And the explaination of `Auth Information` can be found [here](https://github.com/zhiming99/rpc-frmwrk/tree/master/rpc/security#4-enable-authentication-for-rpc-frmwrk).   
  ![rpcfg tab2](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg2.png)
