[English](https://github.com/zhiming99/rpc-frmwrk/wiki/How-to-manually-build-RPC-frmwrk)

# rpc-frmwrk开发教程
## 第九节 如何编译rpc-frmwrk
首先，熟悉docker的用户，或者急性子的用户可以参考[这篇文章](../tools/README_cn.md#docker容器)用docker快速搭建一个`rpc-frmwrk`的运行环境。
## 另一种快速编译`rpc-frmwrk`的途径
1. 下载如下三个文件 [buildall-deb.sh](../tools/buildall-deb.sh), [buildall-fed.sh](../tools/buildall-fed.sh), 和[makerpcf.sh](../tools/makerpcf.sh)到同一个目录下.
2. 在debian或者ubuntu类的平台上执行命令`bash buildall-deb.sh`, 或者在fedora上执行命令`bash buildall-fed.sh`。
3. 我们还提供一个树莓派的的编译脚本[buildall-rasp.sh](../tools/buildall-rasp.sh)，和[makerpcf-rasp.sh](../tools/makerpcf-rasp.sh)。它主要是裁减了几个比较占资源的功能模块，以降低下载失败的概率，和加快编译速度。下面的`交叉编译`一节还提供了使用Docker生成树莓派代码的方法。

## 硬核编译`rpc-frmwrk`的方法
### 搭建编译环境
1. 安装`automake`工具。具体到ubuntu系统, 工具软件包有`autoconf, shtool, libtool, automake`等， fedora系统上，软件包的名字相同。
2. 安装所需的编译器，如`gcc, g++, bison, flex`
3. 生成Python框架用到的软件包, ubuntu上是`python3-dev`和`sip-dev`，fedora上，名字略有不同，`python3-devel`和`sip`。还包括`pip`, `numpy` `wheel`这几个重要的python包.
4. 生成Java框架用到的软件包, 有`openjdk-8`，`swig`和命令行工具`libcommons-cli-java`
5. 生成JS框架用到的软件包有`npm`. 
6. 安装其他开发工具包. 在上边提到的`buildall-deb.sh`或者`buildall-fed.sh`文件中有完整的安装包列表供参考。
7. 从`https://github.com/zhiming99/GmSSL.git`下载GMSSL并按照[说明书](https://github.com/zhiming99/GmSSL/blob/master/README.md)，编译安装GMSSL. 如果不打算使用GmSSL, 可以不用下载，在后面运行`cfgsel`命令时，追加`--disable-gmssl`。
8. 从`https://github.com/zhiming99/rpc-frmwrk.git`下载`rpc-frmwrk`的代码树。
9. 在代码的根目录下，运行如下命令初始化编译环境。
    * `libtoolize` 
    * 运行`autoreconf -vfi`生成`config.h.in`.
### 根据需求裁剪功能模块
`rpc-frmwrk`目前支持四种语言，两种安全认证方式，两种SSL连接。所以对于不在需求范围的语言和功能，可以进行一些裁减。可裁减的模块有一定的依赖关系，因此在裁减时需要注意。  
| 序号 | 功能模块 | 描述 | 示例|依赖关系|能否禁用|
| -------- | --------- | --------- |------------------|---|---|
|1|Python|Python模块提供对Python的运行时支持|`bash cfgsel -r --disable-python`将禁用Python的支持||**不能**禁用|
|2|Java|Java模块提供对Java的运行时支持|`bash cfgsel -r --disable-java`将裁掉Java的支持。||可以禁用|
|3|JS|JS模块提供对JS的运行时支持|`bash cfgsel -r --disable-js`将裁掉JS的支持。JS模块和上面两个语言不一样，是纯客户端的语言，服务器端的语言还需要在C++，Python，和Java里选一个|依赖于openssl和auth模块|可以禁用|
|4|testcases|testcases模块有一些测试用例可以用来测试rpc-frmwrk是否正常工作，通常用于测试|`bash cfgsel -r --disable-testcases`将关闭此功能。||可以禁用|
|5|fuse3|fuse3是用来安装`rpcfs`提供系统状态的模块，该功能主要用于服务器端。|`bash cfgsel -r --disable-fuse3`将关闭此功能||**不能**禁用|
|6|krb5|安全认证模块|krb5是用来支持Kerberos认证的框架，因此关闭此模块，所有的认证功能都被关闭.`bash cfgsel -r --disable-krb5`将关闭此功能.|依赖于openssl或gmssl|可以禁用|
|7|openssl和gmssl|安全连接模块|这两个模块都是提供安全连接的功能，根据需要，关闭其中的一个，不影响另一个的工作, GmSSL是国产的安全连接软件，使用SM2+SM4的安全连接。如`bash cfgsel -r disable-openssl`将关闭openssl的支持|可以禁用其中一个|
### 在X86平台上编译`rpc-frmwrk`:
1. 运行`bash cfgsel -r`或者`bash cfgsel -d`生成release版或debug版的Makefile树. release版是编译器优化过的版本，而debug版是没有优化并带有debug symbol的代码，适合调试用。这里[`cfgsel`](https://github.com/zhiming99/rpc-frmwrk/blob/master/cfgsel)是脚本`configure`的wrapper, 并增加了debug/release的命令选项，同时它会转发其他命令选项给`configure`. 比如`rpc-frmwrk`有许多可配置特性, 包括 `gmssl`, `openssl`, `fuse3`, `auth`, `python`, `java`, `js` `testcases`等. 缺省时, 这些特性将全部编译. 但是如果目标系统有资源限制或者额外考虑，可以用`bash cfgsel -[rd] --disable-xxx`关闭一些特性. 下面是一些配置的例子 
    * `bash cfgsel -r`
    * `bash cfgsel -r --disable-java --disable-krb5 --disable-js --disable-testcases`.    
       禁用Java和JS的框架生成功能，禁用kerberos认证，禁用测试用例。安装的开发工具较少，编译时间短（副作用是没有网页版监视器）。
    * `bash cfgsel -d`    
       构建调试版，会产生更多的调试信息，并适合gdb调试。
3. 运行 `make && make install`编译并安装.   
   如果你的机器强大，可以考虑`make -j4`(4是可并行编译的进程数，可以更多)，以加快编译速度. 
4. 运行 `make deb` 可以生成一个deb包.
5. 运行 `make rpm` 可以生成一个rpm包.
 

### 在树梅派上编译`rpc-frmwrk`:
* 过程同X86上的编译。树莓派上，编译时使用`make`即可，使用`make -j4`，有时会死机。

### 交叉编译
* 建议使用docker，方法如下
    * 国内的用户需要先配置docker使用国内的docker加速镜像，如腾讯云。
    * 先拉取QEMU虚拟机`sudo docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`
    * 然后在[`tools`](../tools)目录下运行`sudo docker buildx build --platform linux/armhf -t armhf/rpc-frmwrk:latest -f Dockerfile.arm .`
    * `Dokerfile`会自动构建armhf的rpc-frmwrk镜像。
    * 这个方法可以在x86的机器上完美生成arm架构的代码。缺点是速度稍微慢一些。

### 已知问题
* 由于各个linux的发行版繁杂，不能一一测试，可能会由于软件包的名字错误或者版本过期，导致编译或者连接错误。如有发生，请及时反馈，我们会尽快修复。

[上一讲](./Tut-Debug_cn-8.md)   
