# rpc-frmwrk概念和技术介绍
RPC是英文Remote Procedure Call的简写。 `rpc-frmwrk`提供了一套运行库和接口API通过抽象，简化，自动化，和集成，封装了分布式应用程序传输层的接口，序列化，错误处理，传输，配置以及运行时监控等功能。接下来，面向基于`rpc-frmwrk`作开发的听众，介绍一些开发过程中会碰到的概念和设计思想。

## RPC参与者和传输的信息
服务器`Server`和代理`Proxy`是RPC通信中的必要的参与方。通信的一般的流程是，Proxy发出请求，叫做`Request`，Server响应请求，提供调用服务，发回结果`Response`，最后Proxy消费`Response`。Server有的时候也会主动的发出信息，叫做`Event`。Proxy发出的调用请求一般来说有明确的目的地，而且一般会有Server发出的`Response`作为响应。而Server主动发出的Event则是广播性质的，只要是订阅该事件的Proxy都会收到。不过`Event`也有特例，如`Keep-Alive`的消息是一对一的，不会广播发布。

有时也会用到`Client`的说法，因为`Proxy`在`rpc-frmwrk`的上下文中，指某一个接口的代理对象，而`Client`则泛指容纳一个或多个`Proxy`实例的对象或者应用程序。

## 远程调用(RPC), 进程间调用(IPC)和进程内调用(In-Process Call)
基于RPC-frmwrk的服务器可以同时支持这三种调用。如果服务器和客户端生成的是动态库，则只需改变配置即可完成RPC/IPC/In-Process的切换。

## 远程调用（RPC）和本地调用的区别
在分布式程序员的视角下，远程调用有以下特点
* 远程调用的发出和调用的发生是有延迟的，快的上百微秒，慢的几秒甚至几十秒。本地调用，这个延迟是几个指令周期。
* 远程调用的传输过程是不可靠的，有可能是服务器故障，网络连接中断，或者网络拥塞。本地调用则不存在上述问题。所以分布式程序员需要处理这些本地调用不存在的问题。
* 本地调用基本上是同步的调用。远程调用在内部实现上出于性能和健壮性的考虑，普遍是异步的（非阻塞的），但在接口上提供同步和异步两种接口。
* 显而易见的是开发过程中，实现远程调用的工作量比本地调用要大很多。

## 配置`rpc-frmwrk`
* `rpc-frmwrk`的每一对Server/Proxy共享一个`对象描述文件`，该文件给出连接服务器的各种参数，认证的用户信息，或者运行时的参数。除了此文件，还有一个`driver.json`配置的文件。`driver.json`给出的是Server/Proxy运行时依赖的`I/O子系统`配置信息。不用紧张，配置文件一般情况下可以通过配置工具和`ridlc`生成的Makefile搞定，若非高级配置，不需要手工设定。
* 关于配置工具的详细介绍，请参考这篇[文章](https://github.com/zhiming99/rpc-frmwrk/tree/master/tools#rpc-router-config-tool)。

## 同步，异步和回调函数
`rpc-frmwrk`的Proxy和Server的每个方法调用都可以指定是同步或者异步调用。
* 同步调用和我们平时调用函数一样，调用的线程会一直被阻断（Blocked)，直到服务器返回结果，或者系统返回错误。此时需要查看返回值，确定调用是否成功。错误包括超时，或者队列满，或者被取消，或者其他的从服务器端返回的错误。

* 异步调用当请求被发出后，调用的线程会立刻返回，不等待服务器返回结果。此时调用者需要查看返回值，确定调用的状态是正在执行（STATUS_PENDING)，或者出错。如果是正在执行中，服务器的`Response`或者系统的错误，都会从回调函数中返回。异步调用在服务器端是有流量控制的，过多的未完成的异步调用会导致新的`Request`被拒绝（ERROR_QUEUE_FULL)，直到服务端有空闲的资源时，才能恢复。

* `rpc-frmwrk`仅提供基于回调函数的异步调用。如果一个rpc请求在`ridl`文件中被定义成异步的请求，`ridlc`可以自动生成必要的回调函数，简化开发过程，节省工作量。

## 任务(Tasklet)和任务组(TaskGroup)
* 任务（tasklet)源自于Linux kernel中的tasklet_struct，起初用于封装函数和函数的参数，随着开发的不断深入，任务（tasklet)也不断的添加新的功能，早已超过了tasklet的原始范畴，逐渐变成遍布`rpc-frmwrk`一个building-block。实际上，上面提到的回调函数就是由内部tasklet封装和调用的。`rpc-frmwrk`的任务(tasklet)主要包括以下的功能：
    * 封装一个函数或者一个方法和其参数
    * 跳出当前运行栈，预防死锁，或者避免调用栈嵌套过多。
    * 作为一个异步操作的发起者
    * 作为一个异步操作的Callback
    * 为一个异步操作设置定时器
    * 作为一个取消操作（Canceling)的最小单位
    * 在任务结束时，调用预设的外部Callback，和通知所隶属的TaskGroup.
* 任务组（TaskGroup），顾名思义，就是一组任务的集合。相当于在任务（tasklet)空间上的运算。任务组继承自任务，用作协调一组子任务的执行。这些子任务可以是并行，也可以是串行。串行时，任务组的执行逻辑可以是逻辑与，逻辑或，或者没有前后依赖关系，或者只是顺序的执行。串行执行的任务组还有另一个用处，就是同步。比起使用锁来进行同步，这个方法可以免去对关键数据上锁。由于任务组本身也是一个任务，所以它也具有任务（tasklet)所有的属性。
* 可以说，一个调用`Request`在`rpc-frmwrk`中的存在形式，就是一个前后串联的任务链。

## 取消一个RPC调用请求
* `rpc-frwmrk`允许用户取消一个还为完成的RPC调用请求，这个请求必须已被服务器端收到，并且在等待处理结果。
* 取消操作必须在异步模式下进行，也就是服务器端的`Request`处理和代理端的`Request`发送都是异步的，有一方处于同步操作的话，都不能进行取消操作，或者会失败。


## 服务器对象和对象的地址
`rpc-frmwrk`的服务器对象是一个继承自CInterfaceServer的对象，同时这个对象在DBus上进行了注册。由于利用DBus作为`rpc-frmwrk`进程间通信的主要途径. 因此服务器对象继承了`DBus`的寻址方式，不过在远程调用时，增加了`rpc-frmwrk`的扩展。所以一个RPC服务对象的地址是一个五元组，{IP地址，TCP端口，路由器路径，对象路径，接口名称}。其中路由器路径请参考下面的条目. 对象路径和接口名称是DBus的地址。这几个参数都存在对象描述文件中， `ridlc`根据接口定义文件（`ridl`文件）在编译的过程中自动设置。如果服务器和代理都在本地，那么进程间调用就只需对象路径和接口名称了。

## 流(Streaming)

`rpc-frmwrk`有相当的代码用来在服务器和客户端建立流通道。流的意义在于可以传输远超过`Request`和`Response`可以接受的上限的数据。流有如下的特点：
* 流通道是全双工的
* 单个流通道可以传输2^64个字节。
* 流的传输被保证是顺序的，先进先出的。
* 流是有流量控制的，主要为了防止发送端发送的数据超出接收端的处理速度。

在`简易模式`编程时，每一个流通道将体现为分别在服务器端和客户端的两个文件，数据的收发体现为对文件的读写操作。用户可以通过在客户端建立一个stream文件而建立一个流通道，比如命令`touch stream_1`。单一连接的流通道个数缺省为32个。不过在大量连接的情况下可以进一步限制。

## Multihop功能和路由器路径
当`rpc-frmwrk`以树形的级联方式部署时，可以让客户端程序通过树根节点（注：可以把一个节点理解成一个主机），访问树上的所有节点。这时对某个节点的访问，就需要`路由器路径`来标识目的地。Multihop的配置可以使用图形配置工具完成。有关Multihop的更多信息请参考这篇[wiki](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support)。

## 安全和认证

* `rpc-frmwrk`通过OpenSSL支持[SSL连接](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport/Readme.md)，或者[WebSocket](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/wsport/Readme.md)的SSL连接。
* `rpc-frmwrk`支持[Kerberos 5认证](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/security/README.md)。Krb5提供单点登陆支持，也提供AES的加密或者签名功能。就是说数据可以获得SSL之外的二重加密。出于性能方面的考虑，数据签名+SSL是更加合适的组合。
* `rpc-frmwrk`的安全和认证功能封装在守护进程中，并通过图形配置工具进行设置。

## 负载均衡
`rpc-frmwrk`在Multihop的基础上支持Round-Robin负载均衡。由于`rpc-frmwrk`的对话（Session)是有状态的长链接，负载均衡是以在连接建立时进行的，一旦连接建立起来，就不再进行负载均衡，也就不存在逐个Request负载均衡。这是和RESTful架构的RPC不同之处。关于负载均衡的详细介绍可以参考这篇[文章](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support#node-redundancyload-balance)。

## `rpcfs`文件系统
`rpcfs`文件系统是一个由Linux FUSE支持的文件系统，是`rpc-frmwrk`提供的有别于传统编程接口的应用接口和管理接口，它主要有以下功能   
* 加载和启停用户的服务器或者代理模块。
* 导出业务模块的运行时状态。
* 增加或者减少发送`Request`的并发量，发送和接收代理的`Request`。
* 增加或者减少`Response`的处理进程，发送和接收服务器的`Response`和`Event`。
* 添加，删除流通道，发送和接收字节流。
* 导出守护进程的运行状态，和参数和计数器统计。
* 通过mount/umount优雅的关闭服务器或者客户端。
* 关于`rpcfs`的内容和结构可以参考这篇[文章](https://github.com/zhiming99/rpc-frmwrk/blob/master/fuse/README.md)。
* `rpc-frmwrk`将提供支持`rpcfs`编程的框架生成工具。

## I/O子系统
`rpc-frmwrk`的I/O子系统类似于操作系统的I/O系统，有驱动程序和端口对象组成。不管是服务器还是代理程序都需要I/O子系统。I/O子系统的配置放在`driver.json`的文件中，它给出服务器和代理程序需要加载的驱动程序和类库。在系统目录下的`driver.json`给出了守护进程，系统自带的工具，和测试用例所需要加载的驱动程序和类库。I/O子系统的主要特点是可以分层的，就是由多层的端口对象来组成一个管线(Pipeline)，用以处理复杂的I/O需求。另一个特点是它是异步的，每个I/O请求对应一个IoRequestPacket对象，它保存了该I/O请求的所有信息和状态，以保证在I/O就绪时，可以发起新的I/O操作，继续未完成的操作，或者出错时，取消当前的操作。

## 内存管理
`rpc-frmwrk`采用引用计数来管理C++对象的生命周期。所有的对象都继承自基类CObjBase。在此基础上，`rpc-frmwrk`提供了CAutoPtr进一步封装了引用计数的管理。使用CAutoPtr可以有效降低内存泄漏的风险。需要注意的是尽量避免`相互引用`的问题，也就是两个对象分别保有对方的引用计数，或者CAutoPtr。如果不能避免，要在执行析构函数前，一方要主动释放掉自己持有的对方对象的引用计数，或者指向对方的CAutoPtr变量。

## 性能
`rpc-frmwrk`在一个i7的4核笔记本上的测试中，1000-5000个连接测试中，2000个连接时达到吞吐量峰值，单个`Request`的平均响应时间在1.1ms左右。大流量并发时服务器的吞吐量大概在每秒1000-1200Requests.

## 开发
* `rpc-frmwrk`通过`ridl`接口描述语言定义函数接口，和需要传输的数据结构。并通过`ridlc`生成各种语言的框架代码，配置文件，以及Makefile和Readme文件. 同一`ridl`文件生成的不同框架的客户端和服务器可以互操作。有关`ridl`语言的介绍请点击此[链接](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl)。

* `rpc-frmwrk`内建了一套基于C++的API，对于希望进一步了解`rpc-frmwrk`接口工作原理，或者觉得ridlc生成的代码太慢，想榨取更高的性能的同学，可以参考[`test`](https://github.com/zhiming99/rpc-frmwrk/tree/master/test)
目录下的代码。欢迎改进和优化`rpc-frmwrk`的各种建议。
* `ridlc`是`ridl compiler`的意思。有关`ridlc`的详细信息请参考上面提到的`ridl`的详细介绍一文。

## 意见和建议
对本项目有任何的建议，或者吐槽，请务必不吝赐教。

