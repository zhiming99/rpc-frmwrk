# 国密GmSSL支持简介:
国密GmSSL是一个国产的加密开源软件，支持TLSv1.3下的国标加密套件（SM2+SM4+GCM）。本模块封装了GmSSL的TLSv1.3的功能，用于对`rpc-frmwrk`有国密合规要求的场合。GmSSL支持是可动态加载的，在`rpc-frmwrk`下配置和使用十分简单，不需要代码修改。
使用方法如下：
 * 请参考[GmSSL](https://github.com/guanzhi/GmSSL)提供的文档安装GmSSL到本地系统.
 * 生成TLSv1.3的密钥和证书，可以shell脚本样例[tls13demo.sh](https://github.com/guanzhi/GmSSL/blob/master/demos/scripts/tls13demo.sh)。
 * 如果没有耐心搞明白各种证书和密钥的关系，也可以使用`rpcfg.py`的`生成demo密钥`生成一组用于演示的密钥。需要注意的是生成的`clientkeys.tar.gz`是要部署到客户端的密钥。需要在客户端使用`rpcfg.py -c`进行部署。
 * 部署密钥, 是在`rpcfg.py`的[安全标签页](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/rpcfg2.png)输入密钥和证书的路径。
 * 最后需要注意的是合理设置密钥的权限，避免密钥泄露造成损失。
 