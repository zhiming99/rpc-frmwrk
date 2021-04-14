### Introduction   
* The `ridl`, stands for `rpc idl`, as the RPC interface description language. And this project tries to deliver a rapid development tool for `rpc-frmwrk` by generating the set of skelton source files for proxy/server , as well as the configuration files, and Makefile with the input `ridl` files. It is expected be able to generate the skelton project for C++, Python and Java.   
* This tool helps the developer to ease the learning curve and focus efforts on business implementations. And one can still refer to the `../test` directory to manually craft an `rpc-frmwrk` applications, with the maximum flexibility and performance advantage.
* The following is a sample file. 
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
    timeout=120, rtpath="/",
    ipaddr="192.168.3.13",
    compress, auth ]
{
    interface IEchoThings;
};

```

