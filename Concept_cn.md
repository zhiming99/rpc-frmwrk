# rpc-frmwrk概念，术语和名词
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

* 异步调用当请求被发出后，调用的线程会立刻返回，不等待服务器返回结果。此时调用者需要查看返回值，确定调用的状态是正在执行（STATUS_PENDING)，或者出错。如果是正在执行中，服务器的`Response`或者系统的错误，都会从回调函数中返回。

* 写回调函数一般是令人头痛的一件事，不过通过`ridlc`生成的框架代码，将自动生成回调函数，使得这一过程轻松许多。


## 任务(Tasklet)，任务组(TaskGroup)
* 任务（tasklet)源自于Linux kernel中的tasklet_struct，起初用于封装函数和函数的参数，随着开发的不断深入，任务（tasklet)也不断的添加新的功能，早已超过了tasklet的原始范畴，逐渐变成遍布`rpc-frmwrk`一个building-block。实际上，上面提到的回调函数就是由内部tasklet封装和调用的。`rpc-frmwrk`的任务(tasklet)主要包括以下的功能：
    * 封装一个函数或者一个方法和其参数
    * 作为一个异步操作的发起者
    * 作为一个异步操作的Callback
    * 为一个异步操作设置定时器
    * 作为取消一个I/O操作一个节点
    * 在操作结束时，调用预设的外部Callback。
* 任务组（TaskGroup），顾名思义，就是一组任务的集合，相当于任务（tasklet)空间上的运算。任务组继承自任务，用作协调一组任务的执行，可以是并行，也可以是串行，串行时，任务组的执行逻辑可以是逻辑与，逻辑或，或者没有前后依赖关系，只是顺序的执行。串行执行的任务组还有另一个用处，就是同步。比起使用锁来进行同步，这个方法可以免去对关键数据上锁。由于任务组本身也是一个任务，所以它也具有任务（tasklet)所有的属性。
* 可以说，一个异步的调用的存在形式，就是一个前后串联的任务链。而一批并行的异步操作，就是由一个并发任务组下的任务链。

## 服务器对象和对象的地址
`rpc-frmwrk`的服务器对象是一个继承了CInterfaceServer的对象，同时这个对象在DBus上进行了注册。由于利用DBus作为`rpc-frmwrk`进程间通信的主要途径. 因此服务器对象继承了`DBus`的寻址方式，不过在远程调用时，有了`rpc-frmwrk`的扩展。这样一个服务对象的地址是一个五元组，{IP地址，TCP端口，路由器路径，对象路径，接口名称}。其中路由器路径在没有级联的情况下为"/". 对象路径和接口名称是DBus的地址。这几个参数都存在对象描述文件中， `ridlc`根据定义接口的ridl文件和编译的过程中，自动设置。

## 流(Streaming)

`rpc-frmwrk` 有相当的代码用来在服务器和客户端建立全双工的字节流通道。流的意义在于可以传输远超过`Request`和`Response`可以接受的上限的数据。理论上一个流通道可以传输2^64个字节。流的传输被保证时顺序的，先进先出的。同时流也是有流量控制的，主要为了防止发送端发送的数据超出接收端的处理速度。在`简易模式`编程时，每一个流通道将体现为分别在服务器端和客户端的两个文件，数据的收发体现为对文件的读写操作。而建立一个流通道也简单的转换成从客户端建立一个文件，类似 `touch abc.stm`。单一连接的流通道个数缺省为200个。不过在大量连接的情况下可以进一步限制。

## 有状态的连接
Before the RPC calls can be delivered to the Server, the RPC-frmwrk will
setup a connection between the proxy/server. The connection is dedicated to
the proxy/server pair. And only the requests can be accepted with the valid
`Object Path`, `Interface name` plus the specific `TCP connection` and
`RouterPath`. The connection also delivers the event from the server to all
the subscriber proxies. Besides the RPC calls and events, the connection
also carries the streaming messages between proxy/server in either
direction.



### Load Balance & Node Redudancy
RPC-frmwrk currently support a simple load-balance/Node redundancy strategy,
the round-robin load distribution among a list of available redudant nodes.
You can view a `redudant node` as a device running RPC-frmwrk which provide 
access to the same set of RPC services. The two connection requests to the
same service from different clients will be effectively distributed to
different node. And [here](https://github.com/zhiming99/rpc-frmwrk/wiki/Introduction-of-Multihop-support#node-redundancyload-balance) is a technical introduction.

### Security over the Network

RPC-frmwrk supports SSL connection or WSS connection between the peer over
the internet. And RPC-frmwrk also supports the Kerberos authentication over
SSL. Besides authentication, The auth module adds even more to security by
signing or encrypting all the outbound messages. Also, the server can
filter the individual messages with the auth infomation for more precise
access control.

## Development
* The RPC-frmwrk provides `ridlc`, that is, `RPC IDL compiler`, to quickly
generate the skelton project with a user defined `ridl` file. And the skelton 
can serve as the start point for your distributed application development.
For detailed information, please refer to [`ridl`](https://github.com/zhiming99/rpc-frmwrk/tree/master/ridl)

* The RPC-frmwrk also provides a set of API to facilitate the development
with the powerful support from `C++11`. A typical distributed RPC
application makes up of a proxy and a server. The proxy can be developed
with some system provided macros in most cases. And your efforts can focus
on the implementation of the features the server delivers. It is
recommended to refer to the sample codes in the [`test`](https://github.com/zhiming99/rpc-frmwrk/tree/master/test)
directory as the start-point of your RPC development.


