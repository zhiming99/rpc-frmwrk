### Introduction

* The `ridl`, stands for `rpc idl`, as the RPC interface description language. And this project tries to deliver a rapid development tool for `rpc-frmwrk` by generating a set of skelton source files for proxy/server , as well as the configuration files, and Makefile with the input `ridl` files. It is expected to generates the skelton project for C++, Python and Java.   
* This tool helps the developer to ease the learning curve and focus efforts on business implementations. And one can still refer to the `../test` directory to manually craft an `rpc-frmwrk` applications, with the maximum flexibility and performance advantage.
* The following code snippet is what the ridl looks like:

```
// must have statement
appname "example";
typedef array< array< string > > STRMATRIX2;

struct FILE_INFO
{
    /* define the fileds here, with default value optionally*/
    string szFileName = "test.dat";
    uint64 fileSize = 0;
    bool bRead = true;
    bytearray fileHeader;
    STRMATRIX2 vecLines;
    map<int32, bytearray> vecBlocks;
};

// echo different type of information
interface IEchoThings
{
    // synchronous call on both server/proxy side by default.
    Echo( string strText )
        returns ( string strResp ) ;

    // server/proxy both sides asynchronous
    [ async ]EchoMany ( int32 i1, int16 i2,
        int64 i3, float i4, double i5, string szText )
        returns ( int32 i1r, int16 i2r,
            int64 i3r, float i4r,
            double i5r, string szTextr );

    // server side asynchronous, and proxy side synchronous
    [ async_s ]EchoStruct( FILE_INFO fi ) returns ( FILE_INFO fir );

    // An event Handler
    [ event ]OnHelloWorld( string strMsg ) returns ();
};

service SimpFileSvc [ stream ]
{
    interface IEchoThings;
};
```

### Supported Data Types

`ridl` support 10 basic types and 3 composite types.
The basic data types are:

* **byte** : 1-byte unsigned integer.
* **bool** : 1-byte boolean value.
* **int16/uint16** : 2-byte signed integer or unsigned integer.
* **int32/uint32** : 4-byte signed integer or unsigned integer.
* **int64/uint64** : 8-byte signed integer or unsigned integer.
* **float(32)/double(64)** : float point number.
* **string** 
* **bytearray** : binary blob.
* **ObjPtr** : `rpc-frmwrk` built-in serializable data type.
* **HSTREAM** : a handle representing an existing stream channel, which can be transferred between the proxy/server.

The 3 composite types are

* **array** : an array of data of basic type or composite type except `HSTREAM`. 
* **map** : a map consisting of key-value paires. `key` should be a comparable data type, and value can be any supported type except `HSTREAM`.
* **struct**: as `FILE_INFO` shows in the above example, is a package of informations of different data types. It is used as a build block for other data types or as the parameter to transfer between proxy and server.

The data types are mapped to the concrete data types of each supported languages, as shown in the following.
![image](https://github.com/zhiming99/rpc-frmwrk/blob/master/pics/ridldatatype.png).

### Statements

The above example shows most of the statements ridl supports. ridl now has 7 types of statements.

* **appname** : specify the application name, it is a must have statement, if there are multiple `appname` statements, the last one will be the winner.
* **include** : specify another ridl file, whose content are referred in this file. for example, `include "abc.ridl";` 
* **typedef** : define an alias for a pre-defined data type.  For example,`typedef myint int32;`
* **const**: to assign a name to a constant value. For example, `const i = 2;`
* **struct** : to define a struct.
* **interface** : an interface is a set of methods logically bounded closely to deliver some kind of service to the client. It is made up of interface id and a set of methods, as similiar to a class in an OO language.
  * **interface id**: a string to uniquely identify the interface, it is used by `rpc-frmwrk` to locate the service.
  * **method** : a method is the smallest unit of interaction between server and proxy. The proxy send the request, and the server executes at the proxy's request and returns the response. As you can see, an `ridl method` is made up of `method name`, `input parameter list`, `output parameter list`. Beside the three major components a method must have, it can have some attributes, too, within the bracket ahead of the `method name` as shown in the above example. The attributes supported by a `method` include
    * **async, async_s, async_p** : to instruct the ridl compiler to generate asynchronous or synchronous code for server or proxy. `async_s` indicates server side will handle the request asynchronously, `async_p` indicates proxy side to send request asynchronously, and `async` means both sides behave asynchronously. About `asynchronous`, Let me explain a bit more. 
      * *Asynchronous* means the method call can return immediately with the operation still in process. Especially in the context of `rpc-frmwrk`, the method may return a status code STATUS_PENDING, to inform the caller the operation is still in-process, besides success or error.
      *  *Synchronous* is in the opposite, that a method must return after the operation is finished, either successfully or failed, and never returns STATUS_PENDING.
    * **timeout** : an integer in second to specify a request-specific timeout value, which is the life-time of a request that can wait before getting processed by the server. If the request is being processed asynchronously, the request can extend its life-time by the keep-alive event.
    * **noreply** : specify a method not require a response from server. On thhe proxy side, the method will return as soon as the request is sent and on the server side, the request is handled without sending out the response.
    * **event** : to specify a method as an event handler, as not applied to normal method. The event flag means the proxy side has an event handler to pick up the event broadcasted by the server and do some processing. The event delivery is a reverse process compared to a normal rpc request, as initiated by server side and ends up at the proxy side. 
  * **the input parameter list** or **output parameter list** of a method can be empty, but normally the method will get a status code from the server to tell if the request succeeds or not, unless the method has attributes such as the `event` or `noreply`.

  * **duplicated method names** : Note that the duplicated method names could have different problems between different language implementations. So the ridlc will generate error if it finds duplicate method names within an interface declaration. It is recommended to avoid duplicate method names within one service declaration.

* **service declaration** : to declare a `service` object. A `service` object contains a set of interfaces, to deliver a relatively independent feature or service. it contains a `service id` and a set of interfaces.
  * **service id**: will appear in the proxy request's `ObjPath` string, as part of an object address as to find the service.
  * Besides interfaces, the service can also be assigned some more attributes.
    * **stream** : a flag to enable streaming support on this `service` object.

### Invoking `ridlc`

`ridlc` stands for `ridl compiler`. Its command line looks like `ridlc [options] <ridl file>`, and there are the following major options:

```
        -I:     To specify the path to search for the included files. And this option can repeat
                manytimes
                
        -O:     To specify the path for the output files. 'output' is the default path if not
                specified
                
        -p:     To generate Python skelton files   

        -j:     To generate Java skelton files       
        
        -l:     To output a shared library instead of executables. This option is for CPP project only

```

Currently `ridlc` can output `c++` and `python` project. In the future, it will be able to generate `Java` project as well.

### Output for C++ project

On a successful compile of the above sample ridl file, `ridlc` will generate the following files:

* **maincli.cpp, mainsvr.cpp**: as the name indicate, the two files define main function of the proxy and server respectively. Each file contains a same-name function, that is a `maincli` function in maincli.cpp and `mainsvr` in mainsvr.cpp, which is the ideal place to add your custom code.

* **SimpFileSvccli.cpp, SimpFileSvccli.h, SimpFileSvcsvr.cpp, SimpFileSvcsvr.h:** the files are named with the *service id* of the *service statement* appending a `svr` or `cli`. Each individual service declaration has two pairs of `.cpp` and `.h` file, for both server/client side. The cpp file contains all the methods that user should implement to get server/proxy to work. They are mainly the same-name methods as defined in ridl file on Server side as the request handlers. 
The amount of efforts to take are different depending on whether or not the meethod is labeled `async`. Without `async` tag, the implementation is a synchronous version and time-saving at development time. And the implementation with `async` tag takes more efforts, but has better performance. The implementation consists of two parts, one is the skelton code to serialize/deserialize parameters, schedule callbacks, setup timers, and the other part are the business logics for the user to implement, including handling response, or error condition, timeout or bad response for example. And `rpc-frmwrk` provides rich utilities to help developing asynchronous implementation to reduce the developing efforts.
* *example.cpp, example.h* : the files are named with the name defined by *appname*. It contains all the skelton method declarations, and implementations.
* *Makefile*: The make file to build the project. It has two build targets, `debug` and `release`. the debug version of image will go to the `debug` directory, and the release version of image will go to the `release` directory. During making, the communication settings are synchronized with current system settings, and usually you don't need to manually config it.
* *exampledesc.json, driver.json:* The configuration files. `driver.json` is for `I/O manager` and `port objsects`, which is relative stable, and `exampledesc.json` contains all the things needed for the services defined in the ridl file. Everytime you have used rpcfg.py to update the settings of `rpc-frmwrk`, you need to run `make` to update the two files as well.
* **run:** After a successful make, there will be two executables, `examplecli` and `examplesvr`. Run the server on the `bridge` side and the client on the `reqfwdr` side. If you have specified `-l` option when generating the code, the `make` command will produce a shared library named `examplelib.so` instead, which has all the functions and classes except the `main` function.

### Output for Python project

On a successful compile of the above sample ridl file, `ridlc` will generate the following files.

* **maincli.py, mainsvr.py**: as the name indicate, the two files define main function of the proxy and server respectively. Each file contains a same-name function, that is a `maincli` function in maincli.py and `mainsvr` in mainsvr.py, which is the ideal place to add your custom code.

* **SimpFileSvccli.py, SimpFileSvcsvr.py:**: the files are named with the *service id* of the *service statement* plus `cli` or `svr`. These files list all the methods that require the user the implement. The synchronous method call has the least set of methods to implement, while the asynchronous method call has 1 or 2 more methods for each requests to implement.

* *examplestructs.py, seribase.py* : the struct file is named with the name defined by *appname*. It contains all the struct declarations, with serialization implementations respectively, and `seribase.py` contains the serialization support utilities.

* *SimpFileSvcclibase.py, SimpFileSvcsvrbase.py:* the files are named with the *service id* of the *service statement* plus `clibase` or `svrbase`. as their name show, they contain the the server or client side classes which serve as the base class of the client/server. These files should not be touched by the users, and they will be overwritten after running `ridlc` again.


* *Makefile*: The make file to build the project. Note that, it just synchronizes the configuration with the system settings.
* *exampledesc.json, driver.json:* The configuration files.
* **run:** you can run `python3 mainsvr.py` and `python3 maincli.py` to start the server and client. Before the first run after `ridlc`, make sure to run `make` to update the configuration file, that is, the `exampledesc.json` file in this context.

### Output for Java Project
* **maincli.java**, **mainsvr.java**: Containing defintion of `main()` method for client, as the main entry for client program and definition of `main()` function server program respectively. 
And you can make changes to the files to customize the program. The `ridlc` will not touch them if they exist in the project directory, when it runs again, and put the newly generated code in the file with '.new' as the name extension.

* **SimpFileSvcsvr.java**, **SimpFileSvccli.java**: Containing the declarations and definitions of all the server/client side methods that need to be implemented by you, mainly the request/event handlers, for service `SimpFileSvc`.
And you need to make changes to the files to implement the functionality for server/client. The `ridlc` will not touch them if they exist in the project directory, when it runs again, and put the newly generated code to `SimpFileSvc.java.new`.

* *SimpFileSvcsvrbase.java*, *SimpFileSvcclibase.java* : Containing the declarations and definitions of all the server/client side utilities and helpers for the interfaces of service `SimpFileSvc`.
And please don't edit them, since they will be overwritten by `ridlc` without backup.

* *exampleFactory.java*: Containing the definition of struct factory declared and referenced in the ridl file.
And please don't edit it, since they will be overwritten by `ridlc` without auto-backup.

* *exampledesc.json*: Containing the configuration parameters for all the services declared in the ridl file
And please don't edit it, since they will be overwritten by `ridlc` and synccfg.py without backup.

* *driver.json*: Containing the configuration parameters for all the ports and drivers
And please don't edit it, since they will be overwritten by `ridlc` and synccfg.py without backup.

* *Makefile*: The Makefile will just synchronize the configurations with the local system settings. And it does nothing else.
And please don't edit it, since it will be overwritten by `ridlc` and synccfg.py without backup.

* *DeserialMaps*, *JavaSerialBase.java*, *JavaSerialHelperS.java*, *JavaSerialHelperP.java*: Containing the utility classes for serializations.
And please don't edit it, since they will be overwritten by `ridlc`.

* *synccfg.py*: a small python script to synchronous settings with the system settings, just ignore it.
* * **run:** you can run `java org.rpcf.example.mainsvr` and `java org.rpcf.example.maincli` to start the server and client. Before the first run after running `ridlc` successfully, make sure to run `make` to update the configuration file, that is, the `exampledesc.json` file.


### Interchangable client and server between C++, Python, and Java.
You can connect the C++ server with a python client or a Python server with a Java client as long as both are generated with the same ridl file.
