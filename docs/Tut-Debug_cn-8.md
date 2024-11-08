# C++开发教程
## 第八节 调试
**本节要点**：   
* 本文介绍一些rpc-frmwrk程序的调试手段和技巧, 尤其是查找程序崩溃，泄漏，或者死锁等问题时的应对之策。
### 使用GDB调试
* GDB是Linux平台上使用广泛的调试器，功能十分强大，缺点是命令行方式，对于习惯了IDE的用户来说有些难以适应。
* GDB下有一个简单的源代码调试界面啊，可以通过`ctrl-x a`快捷键进行切换。
* 最常用的调试命令有以下几个：   

| 序号 | 命令 | 例子| 描述 | 
| -------- | --------- | -------------- |------------------|
|1|设置断点| b main.cpp:123| 在main.cpp的123行设置断点|
|2|设置条件断点| b main.cpp:123 if ret > 0 | 在main.cpp的123行设置断点，并且该断点只在ret变量大于零时起作用|
|3|查看堆栈| i s [10] | 查看当前线程的堆栈，后面的数字表示选择的堆栈层数，如果没有，就会显示所有的堆栈帧|
|4|查看线程| i th | 查看调试进程的所有线程，查死锁时，`i th`是十分有效的工具，往往从给线程的当前调用栈就能看出端倪来。 |
|5|切换到某个线程 | t 3 | 切换到三号线程 |
|6|现场调用函数| call (int)abc() | 在当前堆栈上，调用程序里名为abc的函数, 注意GDB需要返回值的类型。call的局限性在于调用者需要专门在程序里写额外的代码准备所需的参数传递给该调用函数，而且有些堆栈的上下文是不可重入的。有些漫不经心的调用会让程序崩溃。|
|7|打印变量| p ptr | 打印指针变量ptr. 如果在ptr前面加上类型和`*`，则可以打印指定类型的值。如 p *(CObjBase)ptr，将打印ptr指向的CObjBase类的所有内容 |
|8|打印内存| x/16b this | 打印this指针开始的16个字节，this也可以换作合法的数字地址如(0x555555590bc0)|

* 详细的GDB的命令行帮助，可以在GDB的命令行下，使用`help`命令加上需要了解的命令名。如`help break`.

### rpc-frmwrk的GDB辅助工具
* rpc-frmwrk有以下几个工具提供额外的调试信息   

| 序号 | 命令 | 例子| 描述 | 
| -------- | --------- | --------- |------------------|
| 1 | 打印已创建的所有对象 | call DumpObjs(0) | DumpObjs是rpc-frmwrk内建的函数，可以打印当前所有未销毁的CObject对象和其子类，对于查找内存泄漏，检查对象状态很有用处。如果发现某些object超出预期的数量，则可断定存在内存泄漏|
| 2 | 打印指定类名的对象| call DbgFindCN( "CIoManager" ) | 打印所有CIoManager的实例的地址|
* rpc-frmwrk的线程命名有别于其他的系统线程. 标有MainLoop，CTaskThread, SockLoop, UxLoop的线程为rpc-frwmrk线程池里的线程。
  
 ```
  1    Thread 0x7ffff7c55800 (LWP 644082) "regfsmnt"    __futex_abstimed_
  2    Thread 0x7ffff6bff6c0 (LWP 644083) "MainLoop"    0x00007ffff791b15
  3    Thread 0x7ffff63fe6c0 (LWP 644084) "CTaskThread" __futex_abstimed_
  4    Thread 0x7ffff5bfd6c0 (LWP 644086) "regfsmnt"    __GI___libc_read 
 ```

### 使用Valgrind查找泄漏和崩溃点
* 对于复杂程序的崩溃和泄漏，有时GDB也显得力不从心。此时也可以尝试使用valgrind来查找泄漏和崩溃点。valgrind对读写野指针，double-free等方面还是十分准确的。缺点就是很慢，对于并发的bug，改变时序有可能bug就不出现了。不过valgrind是在不能确定bug发生地点时很有力的工具。

### 使用dbus-monitor查看dbus通信
* 当怀疑传输出现问题时，可以通过dbus-monitor查看通信状态和通信内容，定位问题发生的环节。一般会用到下面的几条命令:
  * 查看本地服务器或者客户端注册的dbus destination   
    `dbus-send --session --dest=org.freedesktop.DBus --type=method_call --print-reply /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep rpcf`
  * 监视服务器或者客户端同`rpcrouter`的通信, 注意interface即为`ridl`文件里定义的interface名称加上`org.rpcf.interf.`, path为`ridl`文件里定义的appname(TestTypes)和service名称(TestTypesSvc)的组合。   
    `dbus-monitor interface='org.rpcf.Interf.IStream',path='/org/rpcf/TestTypes/objs/TestTypesSvc'`

### 使用wireshark观察网络传输
* wireshark是一款强大的网络调试工具，可以用来查看`rpcrouter`之间的通信状况。它的使用方法不在这里赘述。唯一需要强调的是使用wireshark时，需关闭SSL支持，否则观察到的数据包均为加密的数据包。

### 代码审查是查找bug十分重要手段
* 当用其他的工具查找bug时，代码审查也应该同时进行。虽然代码审查很慢，很费时间，但是困难的bug往往是在结合所有工具的调查结果和代码审查，最终被定位的。不应过分依赖某一种手段或者方法。

### Python和Java的服务器和客户端也可以使用GDB进行调试。
Python和Java的服务器和客户端，分别会调用rpc-frmwrk的运行库，因此通过`gdb python3`或者`gdb java`也可以进行底层逻辑的调试。

[上一讲](./Tut-AsyncProgramming_cn-7.md)   