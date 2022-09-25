# rpc-frmwrk概念和技术介绍
RPC是英文Remote Procedure Calls的简写。 `rpc-frmwrk`提供了一套运行库和接口API通过抽象，简化，自动化，和集成，封装了分布式应用程序中接口，序列化，错误处理，传输，配置以及运行时监控等功能。接下来，面向基于`rpc-frmwrk`作开发的听众，介绍一些开发过程中会碰到的概念和设计思想。

## RPC参与者和传输的信息
`Server`和`Proxy`是RPC通信中的必要的参与方。通信的一般的流程是，Proxy发出请求，叫做`Request`，Server响应请求，提供调用服务，发回结果`Response`，最后Proxy消费`Response`。Server有的时候也会主动的发出信息，叫做`Event`。Proxy发出的调用请求一般来说有明确的目的地，而且一般会有Server发出的`Response`作为响应。而Server主动发出的Event则是广播性质的，只要是订阅该事件的Proxy都会收到。不过`Event`也有特例，如`Keep-Alive`的消息是一对一的，不会广播发布。

有时也会用到`Client`的说法，因为`Proxy`在`rpc-frmwrk`的上下文中，指某一个接口的代理对象，而`Client`则泛指容纳一个或多个`Proxy`实例的对象或者应用程序。

### 远程调用(RPC), 进程间调用(IPC)和进程内调用(In-Process Call)
基于RPC-frmwrk的服务器可以同时支持这三种调用。如果服务器和客户端生成的是动态库，则只需改变配置即可完成RPC/IPC/In-Process的切换。

## 配置`rpc-frmwrk`
`rpc-frmwrk`的每一对Server/Proxy共享一个对象描述配置文件，该文件给出连接服务器的各种参数，认证的用户信息，或者运行时的参数。除了次文件，还需要一个`driver.json`配置的文件。`driver.json`给出的是Server/Proxy运行时依赖的IO模块，和这些模块的加载顺序。不用紧张，配置文件一般情况下可以通过配置工具和`ridlc`生成的Makefile搞定，若非高级配置，不需要手工设定。

## 同步，异步和回调函数
`rpc-frmwrk`的Proxy端的每个方法调用都可以指定是同步或者异步调用。
* 同步调用和我们平时调用函数一样，调用的线程会一直被阻断（Blocked)，直到服务器返回结果，或者系统返回错误。此时需要查看返回值，确定调用是否成功。错误包括超时，或者队列满，或者被取消，或者其他的从服务器端返回的错误。

* 异步调用当请求被发出后，调用的线程会立刻返回，不等待服务器返回结果。此时调用者需要查看返回值，确定调用的状态是正在执行（STATUS_PENDING)，或者出错。如果是正在执行中，服务器的`Response`或者系统的错误，都会从回调函数中返回。异步调用在服务器端是有流量控制的，过多的未完成的异步调用会导致新的`Request`被拒绝（ERROR_QUEUE_FULL)，直到服务端有空闲的资源时，才能恢复。

* 虽然回调函数是令人头痛的一件事，不过通过`ridlc`生成的框架代码，将自动生成回调函数，使得这一过程轻松许多。

## 任务(Tasklet)，任务组(TaskGroup)
* 任务（tasklet)源自于Linux kernel中的tasklet_struct，起初用于封装函数和函数的参数，随着开发的不断深入，任务（tasklet)也不断的添加新的功能，早已超过了tasklet的原始范畴，逐渐变成遍布`rpc-frmwrk`一个building-block。实际上，上面提到的回调函数就是由内部tasklet封装和调用的。`rpc-frmwrk`的任务(tasklet)主要包括以下的功能：
    * 封装一个函数或者一个方法和其参数
    * 作为一个异步操作的发起者
    * 作为一个异步操作的Callback
    * 为一个异步操作设置定时器
    * 作为一个取消操作（Canceling)的最小单位
    * 在任务结束时，调用预设的外部Callback，和通知所隶属的TaskGroup.
* 任务组（TaskGroup），顾名思义，就是一组任务的集合。相当于在任务（tasklet)空间上的运算。任务组继承自任务，用作协调一组子任务的执行。这些子任务可以是并行，也可以是串行。串行时，任务组的执行逻辑可以是逻辑与，逻辑或，或者没有前后依赖关系，或者只是顺序的执行。串行执行的任务组还有另一个用处，就是同步。比起使用锁来进行同步，这个方法可以免去对关键数据上锁。由于任务组本身也是一个任务，所以它也具有任务（tasklet)所有的属性。
* 可以说，一个调用`Request`在`rpc-frmwrk`中的存在形式，就是一个前后串联的任务链。

## 服务器对象和对象的地址
`rpc-frmwrk`的服务器对象是一个继承了CInterfaceServer的对象，同时这个对象在DBus上进行了注册。由于利用DBus作为`rpc-frmwrk`进程间通信的主要途径. 因此服务器对象继承了`DBus`的寻址方式，不过在远程调用时，有了`rpc-frmwrk`的扩展。这样一个服务对象的地址是一个五元组，{IP地址，TCP端口，路由器路径，对象路径，接口名称}。其中路由器路径请参考下面的条目. 对象路径和接口名称是DBus的地址。这几个参数都存在对象描述文件中， `ridlc`根据定义接口的ridl文件和编译的过程中，自动设置。当进行进程间调用时，就只需要对象路径和接口名称了。

## 流(Streaming)

`rpc-frmwrk`有相当的代码用来在服务器和客户端建立全双工的字节流通道。流的意义在于可以传输远超过`Request`和`Response`可以接受的上限的数据。理论上一个流通道可以传输2^64个字节。流的传输被保证是顺序的，先进先出的。同时流也是有流量控制的，主要为了防止发送端发送的数据超出接收端的处理速度。在`简易模式`编程时，每一个流通道将体现为分别在服务器端和客户端的两个文件，数据的收发体现为对文件的读写操作。而建立一个流通道也简单的转换成从客户端建立一个文件，比如一个shell命令`touch stream_1`。单一连接的流通道个数缺省为200个。不过在大量连接的情况下可以进一步限制。

## Multihop功能和路由器路径
当`rpc-frmwrk`以树形的级联方式部署时，可以让客户端程序通过树根节点，访问树上的其他节点。这时对某个节点的访问，就需要`路由器路径`来标识目的地。Multihop的配置可以使用图形配置工具完成。有关Multihop的更多信息请参考这篇[wiki](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support)。

## 安全和认证

* `rpc-frmwrk`通过OpenSSL支持[SSL连接](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport/Readme.md)，或者[WebSocket](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/wsport/Readme.md)的SSL连接。
* `rpc-frmwrk`支持[Kerberos 5认证](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/security/README.md)。Kerberos 5提供单点登陆
* `rpc-frmwrk`的安全和认证功能封装在守护进程中，并通过图形配置工具完成设置。

## 负载均衡
`rpc-frmwrk`在Multihop的基础上支持Round-Robin负载均衡。由于`rpc-frmwrk`的对话（Session)是有状态的长链接，负载均衡是以在连接建立时进行的，一旦连接建立起来，就不再进行负载均衡，也就不存在逐个Request负载均衡。这是和RESTful架构的RPC不同之处。关于负载均衡的详细介绍可以参考这篇[文章](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support#node-redundancyload-balance)。

## 性能
`rpc-frmwrk`在一个i7的4核笔记本上的测试中，1000-5000的连接测试中，2000个连接时达到最大吞吐量，单个`Request`的平均响应时间在1.1ms左右。大流量并发时服务器的吞吐量大概在每秒1000-1200Requests.

## 开发
* `rpc-frmwrk`通过`ridl`接口描述语言定义函数接口，和需要传输的数据结构。并通过`ridl`文件生成各种语言的框架代码，配置文件，以及Makefile和Readme文件. 同一`ridl`文件生成的不同框架可以互操作。有关`ridl`语言的介绍请点击此[链接](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl)。

* `rpc-frmwrk`内建了一套基于C++的API，对于希望进一步了解`rpc-frmwrk`接口工作原理，或者觉得ridlc生成的代码太慢，想榨取更高的性能的同学，可以参考[`test`](https://github.com/zhiming99/rpc-frmwrk/tree/master/test)
目录下的代码。如果想要提PR，改写`rpc-frmwrk`，那更加欢迎。

## 意见和建议
对本项目有任何的建议，或者吐槽，请务必不吝赐教。

