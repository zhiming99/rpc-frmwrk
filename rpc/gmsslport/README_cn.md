# 国密GmSSL支持简介:
国密GmSSL是一个国产的加密开源软件，支持TLSv1.3下的国标加密套件（SM2+SM4+GCM）。本模块封装了GmSSL的TLSv1.3的功能，用于对`rpc-frmwrk`有国密合规要求的场合。GmSSL支持是可动态加载的，在`rpc-frmwrk`下配置和使用十分简单，不需要代码修改。
使用方法如下：
 * 请参考[GmSSL](https://github.com/guanzhi/GmSSL)提供的文档安装GmSSL到本地系统.
 * 生成TLSv1.3的密钥和证书，可以参考shell脚本样例[tls13demo.sh](https://github.com/guanzhi/GmSSL/blob/master/demos/scripts/tls13demo.sh)。
 * 如果想快速上手，没有耐心搞明白各种证书和密钥的关系，也可以使用`rpcfg.py`的`生成demo密钥`生成一组用于演示的密钥。需要注意的是生成的是分别用于服务器端和客户端的自解压安装包。在待部署的机器上运行安装包，部署密钥和配置系统。

    1) 安装包的名字如下图所示
    2) 安装的命令可为 `bash instcli-o-2023-04-21-49-2.sh 0`。其中参数`0`是密钥的序号，本例中序号可为0或1，不超过文件名末尾的2即可。
    3) [图](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/installer-name.png)

 * 部署密钥, 是在`rpcfg.py`的[安全标签页](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg2.png)输入密钥和证书的路径。
 * 最后需要注意的是合理设置密钥的权限，避免密钥泄露造成损失。
 * 如果需要部署官方认证的国密SM2证书和CA证书，你需要和当地的CA证书提供商进行咨询。