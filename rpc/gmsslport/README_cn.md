# 国密GmSSL支持简介:
国密GmSSL是一个国产的加密开源软件，支持TLSv1.3下的国标加密套件（SM2+SM4+GCM）。本模块封装了GmSSL的TLSv1.3的功能，用于对`rpc-frmwrk`有国密合规要求的场合。GmSSL支持是可动态加载的，在`rpc-frmwrk`下配置和使用十分简单，不需要代码修改。
使用方法如下：
 * 请参考[GmSSL](https://github.com/guanzhi/GmSSL)提供的文档安装GmSSL到本地系统.
 * 生成TLSv1.3的密钥和证书，可以参考shell脚本样例[tls13demo.sh](https://github.com/guanzhi/GmSSL/blob/master/demos/scripts/tls13demo.sh)。
 * 如果想快速上手，没有耐心搞明白各种证书和密钥的关系，也可以使用`rpcfg.py`的[安全标签页](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg2.png)上的`Gen Demo Key`生成一组用于演示的密钥。然后用`Save As`按钮生成分别用于服务器端和客户端的自解压安装包。在待部署的机器上运行安装包，部署密钥和配置系统。
    1) 安装包的名字如下图所示   
       ![图](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/installer-name.png)
    2) 安装的命令如 `bash instcli-o-2023-04-21-49-2.sh 0`。其中参数`0`是密钥的序号，本例中序号可为0或1，小于文件名末尾的2即可。
    3) 如果安装包里没有密钥，安装包的名字就形如`instsvr-2023-04-23.sh`，因此也不需要参数，`bash instsvr-2023-04-23.sh`即可。
    4) 必须妥善保管这些包含密钥的安装包，避免丢失和泄密。
 * 如果使用现成的密钥，或者正规机构颁发的密钥，则需要在`rpcfg.py`的安全标签页输入密钥和证书的路径。然后使用`Save As`按钮生成安装包。需要注意的是，此时只能生成一个服务器端或者一个客户端的安装包。如果`rpcfg.py`启动时没有参数`-c`，那么生成的是server端的安装包，如果带参数`-c`，那么生成的就是客户端的安装包。而安装包的密钥个数始终是一个。
 * 最后需要注意的是合理设置密钥的权限，避免密钥泄露造成损失。
 * 如果需要部署官方认证的国密SM2证书和CA证书，你需要和当地的CA证书提供商进行咨询。
