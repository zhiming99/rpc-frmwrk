### Introduction   
* The `ridl`, stands for `rpc idl`, as the RPC interface description language. And this project tries to deliver a rapid development tool for `rpc-frmwrk` by generating the set of skelton source files for proxy/server , as well as the configuration files, and Makefile with the input `ridl` files. It is expected be able to generate the skelton project for C++, Python and Java.   
* This tool helps the developer to ease the learning curve and focus efforts on business implementations. And one can still refer to the `../test` directory to manually craft an `rpc-frmwrk` applications, with the maximum flexibility and performance advantage.
* The following is a sample file. 
```
// must have statement
appname "example";
typedef array< array< string > > STRMATRIX2;
const timeout_sec = 120;

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

service SimpFileSvc [
    timeout=timeout_sec, rtpath="/",
    ipaddr="192.168.3.13",
    compress, auth ]
{
    interface IEchoThings;
};

```
### Supported Data Types
ridl support 10 basic data types and 3 composite types.
The basic data types are:
* `byte`
* `int16/uint16`
* `int32/uint32`
* `int64/uint64`
* `float(32)/double(64)`
* `bool`
* `string`
* `bytearray`( binary blob )
* `ObjPtr` : RPC-framework built-in serializable data type.
* `HSTREAM` : a special data type as stream handle.

The 3 composite types are
* `array` : an array of data of basic type or composite type except `HSTREAM`
* `map` : a map consisting of key-value paires. `key` should be a comparable data type, and value could be any supported type except `HSTREAM`.
* `struct`: as `FILE_INFO` shows in the above example, is a package of informations of different data types. It is used as a build block for other data types or as the parameter to transfer between proxy and server.

### Statements
The above example shows most of the statements ridl supports. ridl now has 6 types of statements.
* `appname` : specify the application name, it is a must have statement.
* `typedef` : define an alias for a pre-defined data type, or alias.
* `const declaration`: to assign a name to a constant value.
* `struct declaration` : to define a struct.
* `interface declaration` : an interface is a set of methods logically bounded closely to deliver some kind of service to the client. It is made up of interface id and a set of methods, as similiar to a class in an OO language.
    * `interface id`: a string to uniquely identify the interface, it is used by RPC-frmwrk to locate the serice.
    * `method` : a method is the smallest unit of service an RPC server provides. The proxy send the request, and the server execute at the proxy's request and returns the response. As you can see, a `method` is made up of `method name`, `input parameter list`, `output parameter list`. Beside the three major parts a method must have, it can also carry some attributes, within the bracket as shown in the above example. the attributes include
        * `async, async_s, async_p` : to instruct the ridl compiler to generate asynchronous or synchronous code for server or proxy.
        * `timeout` : specify a method specific timeout value in second the proxy can wait before becoming impatient.
        * `noreply` : specify a method not to wait for server response. The proxy side, the method will return as soon as the request is sent and on the server side, the request is handled without sending out the response.
        * `event` : to specify a method as an event handler, as not applied to normal method, picks up a event broadcasted by the server and request the proxy to do somethings on the event.
    * the `input parameter` or `output parameter` of a method can be empty, but the method still get an status code from the server to tell if the method is handled successfully or not, unless there is an `event` attribute.


