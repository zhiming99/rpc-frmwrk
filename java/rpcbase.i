/*
 * =====================================================================================
 *
 *       Filename:  rpcbase.i
 *
 *    Description:  a swig file as the wrapper of CInterfaceProxy class and
 *                  helper classes for Java.
 *
 *        Version:  1.0
 *        Created:  10/16/2021 02:43:51 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */ 
%module rpcbase

%{
#include <string>
#include "rpc.h"
#include "defines.h"
#include "proxy.h"
#include <stdint.h>
#include <errno.h>
#include <vector>
#include <algorithm>
using namespace rpcf;
#include "stmport.h"
#include "fastrpc.h"

extern "C" gint32 DllLoadFactory( FactoryPtr& pFactory );
extern char g_szKeyPass[ SSL_PASS_MAX + 1 ];
ObjPtr g_pIoMgr;
ObjPtr g_pRouter;
extern std::set< guint32 > g_setMsgIds;
gint32 CheckKeyPass();

%}

%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"

typedef int32_t gint32;
typedef uint32_t guint32;
typedef int64_t  gint64;
typedef uint64_t guint64;
typedef intptr_t LONGWORD;
typedef LONGWORD HANDLE;

%javaconst(1);

%javaconst(0) Clsid_Invalid;
%javaconst(0) Clsid_CIoManager;
%javaconst(0) Clsid_CConfigDb2;

typedef int32_t EnumClsid;

enum 
{
    Clsid_Invalid,
    Clsid_CIoManager,
    Clsid_CConfigDb2,
};

%javaconst(0) eventPortStarted;
%javaconst(0) eventPortStartFailed;
%javaconst(0) eventRmtSvrOffline;
typedef int32_t EnumEventId;
enum // EnumEventId
{
    eventPortStarted,
    eventPortStartFailed,
    eventRmtSvrOffline,
};

%javaconst(0)    propIfPtr;
%javaconst(0)    propParamCount;
%javaconst(0)    propIoMgr;
%javaconst(0)    propPyObj;
%javaconst(0)    propIfName;
%javaconst(0)    propMethodName;
%javaconst(0)    propSysMethod;
%javaconst(0)    propSeriProto;
%javaconst(0)    propNoReply;
%javaconst(0)    propTimeoutSec;
%javaconst(0)    propKeepAliveSec;
%javaconst(0)    propConfigPath;

typedef int32_t EnumPropId;
enum // EnumPropId
{
    propIfPtr,
    propParamCount,
    propIoMgr,
    propPyObj,
    propIfName,
    propMethodName,
    propSysMethod,
    propSeriProto,
    propNoReply,
    propTimeoutSec,
    propKeepAliveSec,
    propConfigPath,
};

typedef int32_t EnumIfState;
enum // EnumIfState
{
    stateStopped = 0,
    stateStarting,
    stateStarted,
    stateConnected,
    stateRecovery,
    statePaused,
    stateUnknown,
    stateStopping,
    statePausing,
    stateResuming,
    stateInvalid,
    stateIoDone,    // for the task state
    stateStartFailed,
};

typedef int32_t EnumTypeId;
enum // EnumTypeId
{
    typeNone = 0,
    typeByte, 
    typeUInt16,
    typeUInt32,
    typeUInt64,
    typeFloat,
    typeDouble,
    typeString,
    typeDMsg,
    typeObj,
    typeByteArr,
    typeByteObj = 100,
    typeUInt16Obj,
    typeUInt32Obj,
    typeUInt64Obj,
    typeFloatObj,
    typeDoubleObj,
    typeJRet = 200,
    typeObjArr = 201,
    typeBufPtr = 202
};

typedef int32_t EnumSeriProto;
enum // EnumSeriProto
{
    seriNone = 0,
    seriRidl = 1,
    seriPython = 2,
    seriJava = 3,
    seriInvalid = 4
};

enum
{
    EPERM =		 1	/* Operation not permitted */,
    ENOENT =		 2	/* No such file or directory */,
    ESRCH =		 3	/* No such process */,
    EINTR =		 4	/* Interrupted system call */,
    EIO =		 5	/* I/O error */,
    ENXIO =		 6	/* No such device or address */,
    E2BIG =		 7	/* Argument list too long */,
    ENOEXEC =		 8	/* Exec format error */,
    EBADF =		 9	/* Bad file number */,
    ECHILD =		10	/* No child processes */,
    EAGAIN =		11	/* Try again */,
    ENOMEM =		12	/* Out of memory */,
    EACCES =		13	/* Permission denied */,
    EFAULT =		14	/* Bad address */,
    ENOTBLK =		15	/* Block device required */,
    EBUSY =		16	/* Device or resource busy */,
    EEXIST =		17	/* File exists */,
    EXDEV =		18	/* Cross-device link */,
    ENODEV =		19	/* No such device */,
    ENOTDIR =		20	/* Not a directory */,
    EISDIR =		21	/* Is a directory */,
    EINVAL =		22	/* Invalid argument */,
    ENFILE =		23	/* File table overflow */,
    EMFILE =		24	/* Too many open files */,
    ENOTTY =		25	/* Not a typewriter */,
    ETXTBSY =		26	/* Text file busy */,
    EFBIG =		27	/* File too large */,
    ENOSPC =		28	/* No space left on device */,
    ESPIPE =		29	/* Illegal seek */,
    EROFS =		30	/* Read-only file system */,
    EMLINK =		31	/* Too many links */,
    EPIPE =		32	/* Broken pipe */,
    EDOM =		33	/* Math argument out of domain of func */,
    ERANGE =		34	/* Math result not representable */,

    EDEADLK =		35	/* Resource deadlock would occur */,
    ENAMETOOLONG =	36	/* File name too long */,
    ENOLCK =		37	/* No record locks available */,

    ENOSYS =		38	/* Invalid system call number */,

    ENOTEMPTY =	39	/* Directory not empty */,
    ELOOP =		40	/* Too many symbolic links encountered */,
    EWOULDBLOCK =	EAGAIN	/* Operation would block */,
    ENOMSG =		42	/* No message of desired type */,
    EIDRM =		43	/* Identifier removed */,
    ECHRNG =		44	/* Channel number out of range */,
    EL2NSYNC =	45	/* Level 2 not synchronized */,
    EL3HLT =		46	/* Level 3 halted */,
    EL3RST =		47	/* Level 3 reset */,
    ELNRNG =		48	/* Link number out of range */,
    EUNATCH =		49	/* Protocol driver not attached */,
    ENOCSI =		50	/* No CSI structure available */,
    EL2HLT =		51	/* Level 2 halted */,
    EBADE =		52	/* Invalid exchange */,
    EBADR =		53	/* Invalid request descriptor */,
    EXFULL =		54	/* Exchange full */,
    ENOANO =		55	/* No anode */,
    EBADRQC =		56	/* Invalid request code */,
    EBADSLT =		57	/* Invalid slot */,

    EDEADLOCK =	EDEADLK,

    EBFONT =		59	/* Bad font file format */,
    ENOSTR =		60	/* Device not a stream */,
    ENODATA =		61	/* No data available */,
    ETIME =		62	/* Timer expired */,
    ENOSR =		63	/* Out of streams resources */,
    ENONET =		64	/* Machine is not on the network */,
    ENOPKG =		65	/* Package not installed */,
    EREMOTE =		66	/* Object is remote */,
    ENOLINK =		67	/* Link has been severed */,
    EADV =		68	/* Advertise error */,
    ESRMNT =		69	/* Srmount error */,
    ECOMM =		70	/* Communication error on send */,
    EPROTO =		71	/* Protocol error */,
    EMULTIHOP =	72	/* Multihop attempted */,
    EDOTDOT =		73	/* RFS specific error */,
    EBADMSG =		74	/* Not a data message */,
    EOVERFLOW =	75	/* Value too large for defined data type */,
    ENOTUNIQ =	76	/* Name not unique on network */,
    EBADFD =		77	/* File descriptor in bad state */,
    EREMCHG =		78	/* Remote address changed */,
    ELIBACC =		79	/* Can not access a needed shared library */,
    ELIBBAD =		80	/* Accessing a corrupted shared library */,
    ELIBSCN =		81	/* .lib section in a.out corrupted */,
    ELIBMAX =		82	/* Attempting to link in too many shared libraries */,
    ELIBEXEC =	83	/* Cannot exec a shared library directly */,
    EILSEQ =		84	/* Illegal byte sequence */,
    ERESTART =	85	/* Interrupted system call should be restarted */,
    ESTRPIPE =	86	/* Streams pipe error */,
    EUSERS =		87	/* Too many users */,
    ENOTSOCK =	88	/* Socket operation on non-socket */,
    EDESTADDRREQ =	89	/* Destination address required */,
    EMSGSIZE =	90	/* Message too long */,
    EPROTOTYPE =	91	/* Protocol wrong type for socket */,
    ENOPROTOOPT =	92	/* Protocol not available */,
    EPROTONOSUPPORT =	93	/* Protocol not supported */,
    ESOCKTNOSUPPORT =	94	/* Socket type not supported */,
    EOPNOTSUPP =	95	/* Operation not supported on transport endpoint */,
    EPFNOSUPPORT =	96	/* Protocol family not supported */,
    EAFNOSUPPORT =	97	/* Address family not supported by protocol */,
    EADDRINUSE =	98	/* Address already in use */,
    EADDRNOTAVAIL =	99	/* Cannot assign requested address */,
    ENETDOWN =	100	/* Network is down */,
    ENETUNREACH =	101	/* Network is unreachable */,
    ENETRESET =	102	/* Network dropped connection because of reset */,
    ECONNABORTED =	103	/* Software caused connection abort */,
    ECONNRESET =	104	/* Connection reset by peer */,
    ENOBUFS =		105	/* No buffer space available */,
    EISCONN =		106	/* Transport endpoint is already connected */,
    ENOTCONN =	107	/* Transport endpoint is not connected */,
    ESHUTDOWN =	108	/* Cannot send after transport endpoint shutdown */,
    ETOOMANYREFS =	109	/* Too many references: cannot splice */,
    ETIMEDOUT =	110	/* Connection timed out */,
    ECONNREFUSED =	111	/* Connection refused */,
    EHOSTDOWN =	112	/* Host is down */,
    EHOSTUNREACH =	113	/* No route to host */,
    EALREADY =	114	/* Operation already in progress */,
    EINPROGRESS =	115	/* Operation now in progress */,
    ESTALE =		116	/* Stale file handle */,
    EUCLEAN =		117	/* Structure needs cleaning */,
    ENOTNAM =		118	/* Not a XENIX named type file */,
    ENAVAIL =		119	/* No XENIX semaphores available */,
    EISNAM =		120	/* Is a named type file */,
    EREMOTEIO =	121	/* Remote I/O error */,
    EDQUOT =		122	/* Quota exceeded */,

    ENOMEDIUM =	123	/* No medium found */,
    EMEDIUMTYPE =	124	/* Wrong medium type */,
    ECANCELED =	125	/* Operation Canceled */,
    ENOKEY =		126	/* Required key not available */,
    EKEYEXPIRED =	127	/* Key has expired */,
    EKEYREVOKED =	128	/* Key has been revoked */,
    EKEYREJECTED =	129	/* Key was rejected by service */,

    /* for robust mutexes */
    EOWNERDEAD =	130	/* Owner died */,
    ENOTRECOVERABLE =	131	/* State not recoverable */,

    ERFKILL =		132	/* Operation not possible due to RF-kill */,

    EHWPOISON =	133	/* Memory page has hardware error */,

    ENOTSUP = EOPNOTSUPP
};

enum // rpcf specific errors
{
    STATUS_PENDING         =             ( ( int ) 0x10001 ),
    STATUS_MORE_PROCESS_NEEDED =         ( ( int ) 0x10002 ),
    STATUS_CHECK_RESP      =             ( ( int ) 0x10003 ),

    // STATUS_SUCCESS, the irp is completed successfully
    STATUS_SUCCESS         = 0,
    // STATUS_FAIL, the irp is completed with unknown error
    ERROR_FAIL             = ( ( int )0x80000001 ),
    // STATUS_TIMEOUT, the irp is timeout, no response or
    // no incoming notification
    ERROR_TIMEOUT          = ( -ETIMEDOUT ),

    // STATUS_CANCEL, the irp is cancelled at the caller
    // request
    ERROR_CANCEL           = ( -ECANCELED ),

    // bad address
    ERROR_ADDRESS          = ( ( int )0x80010002 ) ,
    ERROR_STATE            = ( ( int )0x80010003 ),
    ERROR_WRONG_THREAD     = ( ( int )0x80010004 ),
    ERROR_CANNOT_CANCEL    = ( ( int )0x80010005 ),
    ERROR_PORT_STOPPED     = ( ( int )0x80010006 ),
    ERROR_FALSE            = ( ( int )0x80010007 ),

    ERROR_REPEAT           = ( ( int )0x80010008 ),
    ERROR_PREMATURE        = ( ( int )0x80010009 ),
    ERROR_NOT_HANDLED      = ( ( int )0x8001000a ),
    ERROR_CANNOT_COMP      = ( ( int )0x8001000b ),
    ERROR_USER_CANCEL      = ( ( int )0x8001000c ),
    ERROR_PAUSED           = ( ( int )0x8001000d ),
    ERROR_CANCEL_INSTEAD   = ( ( int )0x8001000f ),
    ERROR_NOT_IMPL         = ( ( int )0x80010010 ),
    ERROR_DUPLICATED       = ( ( int )0x80010011 ),
    ERROR_KILLED_BYSCHED   = ( ( int )0x80010012 ),

    // for flow control
    ERROR_QUEUE_FULL       = ( ( int )0x8001000e )
};

enum // some constants
{
    INVALID_HANDLE = (int) 0,
    MAX_BUF_SIZE = ( int )( 16 * 1024 * 1024 )
};

%template(vectorVars) std::vector<Variant>;

gint32 CoInitialize( gint32 iCtx );
gint32 CoUninitializeEx();

%nodefaultctor;

%feature("ref")   CObjBase "$this->AddRef();"
%feature("unref") CObjBase "$this->Release();"

%typemap(javadestruct, methodname="delete", methodmodifiers="public synchronized") CObjBase {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        Release();
      }
      swigCPtr = 0;
    }
  }

%typemap(javabody) CObjBase %{
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;
  protected CObjBase(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
    if( cMemoryOwn )
        AddRef();
  }

  protected static long getCPtr(CObjBase obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }
%}

class CObjBase
{
    public:
    gint32 AddRef();
    gint32 Release();
    gint32 GetRefCount();
    gint32 SetClassId( EnumClsid iClsid );
    EnumClsid GetClsid() const;
};

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") IConfigDb {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
  }
class IConfigDb : public CObjBase
{
    public:
    gint32 SetProperty( gint32 iProp,
        const BufPtr& pBuf );
    gint32 GetProperty( gint32 iProp,
        BufPtr& pBuf ) const;
};

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") IEventSink {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
  }

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") IService {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
  }

class IEventSink : public CObjBase
{
    public:

    gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1, 
        LONGWORD dwParam2,
        LONGWORD* pData );
};

class IService : public IEventSink
{
    public:
    gint32 Start();
    gint32 Stop();
};

%clearnodefaultctor;

%header %{

#define CHECK_ENV( _jenv ) \
    gint32 ret = 0; \
    if( _jenv == nullptr ) \
        return nullptr; \
    jobject jret = NewJRet( _jenv ); \
    if( jret == nullptr ) \
        return nullptr; \
    do{ \
        if( self == nullptr ) \
        { \
            SetErrorJRet( _jenv, jret, -EFAULT ); \
            return jret; \
        } \
    }while( 0 );

typedef std::vector<Variant> vectorVars;

LONGWORD GetWrappedPtr(
    JNIEnv *jenv, jobject jObj );

jobject NewJRet( JNIEnv *jenv )
{
    jclass cls = jenv->FindClass(
        "org/rpcf/rpcbase/JRetVal");

    jmethodID ctor  = jenv->GetMethodID(
        cls, "<init>", "()V");

    return jenv->NewObject( cls, ctor);
}

void AddElemToJRet( JNIEnv *jenv,
    jobject jret, jobject pObj )
{
    jclass cls = jenv->FindClass(
        "org/rpcf/rpcbase/JRetVal");

    jmethodID addElem = jenv->GetMethodID(
        cls, "addElem",
        "(Ljava/lang/Object;)V");

    jenv->CallVoidMethod(
        jret, addElem, pObj );

    return;
}

void SetErrorJRet( JNIEnv *jenv,
    jobject jret, gint32 iRet )
{
    jclass cls = jenv->FindClass(
        "org/rpcf/rpcbase/JRetVal");

    jmethodID setError = jenv->GetMethodID(
        cls, "setError", "(I)V");

    jenv->CallVoidMethod(
        jret, setError, iRet );

    return;
}

gint32 GetErrorJRet( JNIEnv *jenv, jobject jret )
{
    jclass cls = jenv->FindClass(
        "org/rpcf/rpcbase/JRetVal");

    jmethodID getError = jenv->GetMethodID(
        cls, "getError", "()I");

    return jenv->CallIntMethod(
        jret, getError );
}

// returns an array of BufPtr
int GetParamsJRet(
    JNIEnv *jenv, jobject jret, vectorVars& vecParams )
{
    jclass cls = jenv->FindClass(
        "org/rpcf/rpcbase/JRetVal");

    jmethodID getParams = jenv->GetMethodID(
        cls, "getParams",
        "()Lorg/rpcf/rpcbase/vectorVars;");

    jobject jvecParams = jenv->CallObjectMethod(
        jret, getParams );

    if( jvecParams == nullptr )
        return -EFAULT;

    intptr_t lPtr = GetWrappedPtr(
        jenv, jvecParams );

    if( lPtr == 0 )
        return -EFAULT;

    vectorVars* pvecBufs =
        ( vectorVars* )lPtr;

    vecParams = *pvecBufs;
    return STATUS_SUCCESS;
}

int GetParamsObjArr(
    JNIEnv *jenv, jobject arrObj, vectorVars& vecParams )
{
    jclass cls = jenv->FindClass(
        "org/rpcf/rpcbase/Helpers");

    jmethodID cotb = jenv->GetStaticMethodID(
        cls, "convertObjectToVars",
        "([Ljava/lang/Object;)"
        "Lorg/rpcf/rpcbase/vectorVars;");

    if( cotb == nullptr )
        return -EFAULT;

    jobject jvecParams =
        jenv->CallStaticObjectMethod(
        cls, cotb, arrObj );

    if( jvecParams == nullptr )
        return -EFAULT;

    intptr_t lPtr = GetWrappedPtr(
        jenv, jvecParams );

    if( lPtr == 0 )
        return -EFAULT;

    vectorVars* pvecBufs =
        ( vectorVars* )lPtr;

    vecParams = *pvecBufs;
    ( *pvecBufs ).clear();
    return STATUS_SUCCESS;
}
    

// returns an array of BufPtr
int GetParamCountJRet(
    JNIEnv *jenv, jobject jret )
{
    jclass cls = jenv->FindClass(
        "org/rpcf/rpcbase/JRetVal");

    jmethodID getParams = jenv->GetMethodID(
        cls, "getParamCount", "()I");

    return jenv->CallIntMethod( jret, getParams );
}

EnumTypeId GetTypeId(
    JNIEnv *jenv, jobject pObj )
{
    jclass cls = jenv->FindClass(
        "org/rpcf/rpcbase/Helpers");

    jmethodID gtid_method =
        jenv->GetStaticMethodID(
        cls, "getTypeId",
        "(Ljava/lang/Object;)I");

    jint iType = jenv->CallStaticIntMethod(
        cls, gtid_method, pObj );

    return (EnumTypeId)iType;
}

jobject NewWrapperObj( JNIEnv *jenv,
    const stdstr& strClass,
    long lPtr, bool bOwner = true )
{
    jclass cls = jenv->FindClass(
        strClass.c_str() );

    jmethodID ctor  = jenv->GetMethodID(
        cls, "<init>", "(JZ)V");

    return jenv->NewObject(
        cls, ctor, lPtr, bOwner );

}
jobject NewIService( JNIEnv *jenv,
    long lPtr, bool bOwner )
{
    stdstr strClass =
        "org/rpcf/rpcbase/IService";
    return NewWrapperObj(
        jenv, strClass, lPtr, bOwner );
}

jobject NewObjPtr( JNIEnv *jenv,
    long lPtr, bool bOwner = true )
{
    stdstr strClass =
        "org/rpcf/rpcbase/ObjPtr";

    return NewWrapperObj(
        jenv, strClass, lPtr, bOwner );
}

jobject NewBufPtr( JNIEnv *jenv,
    long lPtr, bool bOwner = true )
{
    stdstr strClass =
        "org/rpcf/rpcbase/BufPtr";

    return NewWrapperObj(
        jenv, strClass, lPtr, bOwner );
}

jobject NewCfgPtr( JNIEnv *jenv,
    long lPtr, bool bOwner = true )
{
    stdstr strClass =
        "org/rpcf/rpcbase/CfgPtr";

    return NewWrapperObj(
        jenv, strClass, lPtr, bOwner );
}

jobject NewJavaProxy( JNIEnv *jenv,
    long lPtr, bool bOwner = true )
{
    stdstr strClass =
        "org/rpcf/rpcbase/CJavaProxy";

    return NewWrapperObj(
        jenv, strClass, lPtr, bOwner );
}

jobject NewJavaServer( JNIEnv *jenv,
    long lPtr, bool bOwner = true )
{
    stdstr strClass =
        "org/rpcf/rpcbase/CJavaServer";

    return NewWrapperObj(
        jenv, strClass, lPtr, bOwner );
}

jobject NewCParamList( JNIEnv *jenv,
    long lPtr, bool bOwner = true )
{
    stdstr strClass =
        "org/rpcf/rpcbase/CParamList";

    return NewWrapperObj(
        jenv, strClass, lPtr, bOwner );
}

LONGWORD GetWrappedPtr(
    JNIEnv *jenv, jobject jObj )
{
    jclass objClass =
        jenv->GetObjectClass( jObj );

    jmethodID mid = jenv->GetMethodID(
        objClass, "getClass", "()Ljava/lang/Class;");

    jobject clsObj =
        jenv->CallObjectMethod( jObj, mid);

    objClass = jenv->GetObjectClass( clsObj );

    jmethodID getName = jenv->GetMethodID(
        objClass, "getName",
        "()Ljava/lang/String;" );

    if( getName == nullptr )
        return 0;

    jstring jName = ( jstring )jenv->CallObjectMethod(
        clsObj, getName );

    const char* szName =
        jenv->GetStringUTFChars( jName, 0 );

    stdstr strSig;
    strSig = strSig + "(L" + szName + ";)J";
    std::replace( strSig.begin(),
        strSig.end(), '.', '/' );
    jenv->ReleaseStringUTFChars( jName, szName );

    objClass = jenv->GetObjectClass( jObj );

    jmethodID getCPtr =
        jenv->GetStaticMethodID(
            objClass, "getCPtr",
            strSig.c_str() );

    return jenv->CallStaticLongMethod(
            objClass, getCPtr, jObj );
}

LONGWORD GetWrappedBufPtr(
    JNIEnv *jenv, jobject jObj )
{
    jclass objClass =
        jenv->GetObjectClass( jObj );

    stdstr strSig =
        "(Lorg/rpcf/rpcbase/BufPtr;)J";

    jmethodID getCPtr =
        jenv->GetStaticMethodID(
            objClass, "getCPtr",
            strSig.c_str() );

    return jenv->CallStaticLongMethod(
            objClass, getCPtr, jObj );
}

jobject NewBoolean( JNIEnv *jenv, bool val )
{
    jclass cls = jenv->FindClass(
        "java/lang/Boolean");
    jmethodID midInit = jenv->GetMethodID(
        cls, "<init>", "(Z)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = jenv->NewObject(
        cls, midInit, (jboolean)val);
    return newObj;
}

jobject NewByte( JNIEnv *jenv, guint8 val )
{
    jclass cls = jenv->FindClass(
        "java/lang/Byte");
    jmethodID midInit = jenv->GetMethodID(
        cls, "<init>", "(B)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = jenv->NewObject(
        cls, midInit, (jbyte)val);
    return newObj;
}

jobject NewShort( JNIEnv *jenv, guint16 val )
{
    jclass cls = jenv->FindClass(
        "java/lang/Short");
    jmethodID midInit = jenv->GetMethodID(
        cls, "<init>", "(S)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = jenv->NewObject(
        cls, midInit, val);
    return newObj;
}

jobject NewInt( JNIEnv *jenv, guint32 val )
{
    jclass cls = jenv->FindClass(
        "java/lang/Integer");
    jmethodID midInit = jenv->GetMethodID(
        cls, "<init>", "(I)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = jenv->NewObject(
        cls, midInit, val);
    return newObj;
}

jobject NewLong( JNIEnv *jenv, gint64 val )
{
    jclass cls = jenv->FindClass(
        "java/lang/Long");
    jmethodID midInit = jenv->GetMethodID(
        cls, "<init>", "(J)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = jenv->NewObject(
        cls, midInit, val);
    return newObj;
}

jobject NewFloat( JNIEnv *jenv, float val )
{
    jclass cls = jenv->FindClass(
        "java/lang/Float");
    jmethodID midInit = jenv->GetMethodID(
        cls, "<init>", "(F)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = jenv->NewObject(
        cls, midInit, val);
    return newObj;
}

jobject NewDouble( JNIEnv *jenv, double val )
{
    jclass cls = jenv->FindClass(
        "java/lang/Double");
    jmethodID midInit = jenv->GetMethodID(
        cls, "<init>", "(D)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = jenv->NewObject(
        cls, midInit, val);
    return newObj;
}

jobject GetJniString(JNIEnv *jenv, const stdstr& message )
{
    int byteCount = message.length();
    const jbyte* pNativeMessage =
        reinterpret_cast<const jbyte*>
            ( message.c_str() );

    jbyteArray bytes =
        jenv->NewByteArray( byteCount);

    jenv->SetByteArrayRegion(
        bytes, 0, byteCount, pNativeMessage);

    /*
     * find the Charset.forName method:
     *  javap -s java.nio.charset.Charset | egrep -A2 "forName"
    */
    jclass charsetClass =
        jenv->FindClass("java/nio/charset/Charset");

    jmethodID forName = jenv->GetStaticMethodID(
      charsetClass, "forName",
      "(Ljava/lang/String;)"
      "Ljava/nio/charset/Charset;");

    jstring utf8 = jenv->NewStringUTF("UTF-8");
    jobject charset = jenv->CallStaticObjectMethod(
        charsetClass, forName, utf8);

    // find a String constructor that takes a Charset:
    //   javap -s java.lang.String | egrep -A2 "String\(.*charset"
    jclass stringClass =
        jenv->FindClass("java/lang/String");

    jmethodID ctor = jenv->GetMethodID(
       stringClass, "<init>",
       "([BLjava/nio/charset/Charset;)V");

    return jenv->NewObject(
        stringClass, ctor, bytes, charset);
}

bool IsArray( JNIEnv *jenv, jobject pObj )
{
    jclass Class_class = 
        jenv->FindClass("java/lang/Class");
    jmethodID Class_isArray_mid =
        jenv->GetMethodID(
            Class_class, "isArray", "()Z");
    jclass obj_class =
        jenv->GetObjectClass(pObj);
    return jenv->CallBooleanMethod(
            obj_class, Class_isArray_mid);
}

EnumTypeId GetObjType( JNIEnv *jenv, jobject pObj )
{
    jclass iClass =
        jenv->FindClass("org/rpcf/rpcbase/JRetVal");
    if( jenv->IsInstanceOf( pObj, iClass ) )
        return ( EnumTypeId )200;

    iClass =
        jenv->FindClass("java/lang/Object");
    if( jenv->IsInstanceOf( pObj, iClass ) )
    {
        if( IsArray( jenv, pObj ) )
            return ( EnumTypeId )201;
    }
    return typeNone;
}

IService* CastToSvc(
    JNIEnv *jenv, ObjPtr* pObj )
{
    if( pObj == nullptr ||
        ( *pObj ).IsEmpty() )
        return nullptr;

    IService* pSvc = *pObj;
    return pSvc;
}

CRpcServices* CastToRpcSvcs(
    JNIEnv *jenv, ObjPtr* pObj )
{
    if( pObj == nullptr ||
        ( *pObj ).IsEmpty() )
        return nullptr;

    CRpcServices* pSvc = *pObj;
    return pSvc;
}

ObjPtr* CreateObject(
    EnumClsid iClsid, CfgPtr& pCfg )
{

    ObjPtr* pObj =
        new ObjPtr( nullptr, false );
    gint32 ret = pObj->NewObj(
        iClsid, ( IConfigDb* )pCfg ); 
    if( ret < 0 )
    {
        delete pObj;
        return nullptr;
    }

    return pObj;
}

CfgPtr* CastToCfg( ObjPtr* pObj )
{
    if( pObj == nullptr )
        return nullptr;

    IConfigDb* cfgPtr = *pObj;
    if( cfgPtr == nullptr )
        return nullptr;

    CfgPtr* pCfg =
        new CfgPtr( cfgPtr, true );

    return pCfg;
}

static void* g_pLibHandle = nullptr;
gint32 OpenThisLib()
{
    gint32 ret = 0;
    do{
        std::string strResult;
        const char* szLib = "librpcbaseJNI";
        ret = GetLibPathName( strResult, szLib );
        if( ERROR( ret ) )
            break;
        // explicitly open this library so that the
        // global variables can be found by other
        // shared libraray to load.
        g_pLibHandle = dlopen( strResult.c_str(),
            RTLD_NOW | RTLD_GLOBAL );
        if( g_pLibHandle == nullptr )
        {
            DebugPrintEx( logErr,
                0, "%s", dlerror() );
        }
    }while( 0 );
    return ret;
}

char g_szKeyPass[ SSL_PASS_MAX + 1 ];
gint32 CheckKeyPass()
{
    gint32 ret = 0;
    do{
        stdstr strPath;
        ret = GetLibPath(
            strPath, "libipc.so" );
        if( ERROR( ret ) )
            break;

        strPath += "/librpc.so";
        void* handle = dlopen( strPath.c_str(),
            RTLD_NOW | RTLD_GLOBAL );
        if( handle == nullptr )
        {
            ret = -ENOENT;
            DebugPrintEx( logErr, ret,
                "Error librpc.so not found" );
            break;
        }
        auto func = ( gint32 (*)(bool& ) )
            dlsym( handle, "CheckForKeyPass" );
        if( func == nullptr )
        {
            ret = -ENOENT;
            DebugPrintEx( logErr, ret,
                "Error undefined symobol %s in " 
                "librpc.so ",
                __func__ );
            break;
        }

        bool bPrompt = false;
        bool bExit = false;
        ret = func( bPrompt );
        while( SUCCEEDED( ret ) && bPrompt )
        {
            char* pPass = getpass( "SSL Key Password:" );
            if( pPass == nullptr )
            {
                bExit = true;
                ret = -errno;
                break;
            }
            size_t len = strlen( pPass );
            len = std::min(
                len, ( size_t )SSL_PASS_MAX );
            memcpy( g_szKeyPass, pPass, len );
            break;
        }
        ret = 0;
        if( bExit )
            break;
    }while( 0 );
    return ret;
}

ObjPtr* StartIoMgr( CfgPtr& pCfg )
{
    gint32 ret = 0;
    cpp::ObjPtr* pObj =
        new cpp::ObjPtr( nullptr, false );
    CCfgOpener oCfg( ( IConfigDb* )pCfg );
    ObjPtr* sipRes = nullptr;
    do{
        stdstr strModName;
        ret = oCfg.GetStrProp( 0, strModName );
        if( ERROR( ret ) )
            break;

        FactoryPtr p;
        ret = DllLoadFactory( p );
        if( ERROR( ret ) )
            return nullptr;
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;

        if( !oCfg.exist( 101 ) )
        {
            ret = pObj->NewObj(
                clsid( CIoManager ), pCfg ); 
            if( ret < 0 )
                break;

            CIoManager* pMgr = *pObj;
            ret = pMgr->Start();
            if( ERROR( ret ) )
                break;

            sipRes = pObj;
            g_pIoMgr = *pObj;
        }
        else
        {
            // built-in router enabled
            OpenThisLib();
            guint32 dwNumThrds =
                ( guint32 )std::max( 1U,
                std::thread::hardware_concurrency() );
            if( dwNumThrds > 1 )
                dwNumThrds = ( dwNumThrds >> 1 );
            oCfg[ propMaxTaskThrd ] = dwNumThrds;
            oCfg[ propMaxIrpThrd ] = 2;

            guint32 dwRole = 1;
            oCfg.GetIntProp( 101, dwRole );
            oCfg.RemoveProperty( 101 );
            
            bool bAuth = false;
            if( oCfg.exist( 102 ) )
            {
                oCfg.GetBoolProp( 102, bAuth );
                oCfg.RemoveProperty( 102 );
            }

            bool bDaemon = false;
            if( oCfg.exist( 103 ) )
            {
                oCfg.GetBoolProp( 103, bDaemon );
                oCfg.RemoveProperty( 103 );
            }
            stdstr strAppName;
            if( oCfg.exist( 104 ) )
            {
                oCfg.GetStrProp( 104, strAppName );
                oCfg.RemoveProperty( 104 );
            }
            else
            {
                ret = -ENOENT;
                break;
            }

            stdstr strRtFile;
            if( oCfg.exist( 106 ) )
            {
                oCfg.GetStrProp( 106, strRtFile );
                oCfg.RemoveProperty( 106 );
            }

            // it will prompt for keypass if necessary
            ret = CheckKeyPass();
            if( ERROR( ret ) )
                break;

            if( bDaemon )
            {
                ret = daemon( 1, 0 );
                if( ret < 0 )
                {
                    ret = -errno;
                    break;
                }
            }

            ret = pObj->NewObj(
                clsid( CIoManager ), pCfg );
            if( ERROR( ret ) )
                break;

            CIoManager* pMgr = *pObj;
            pMgr->SetCmdLineOpt(
                propRouterRole, dwRole );
            ret = pMgr->Start();
            if( ERROR( ret ) )
                break;

            // create and start the router
            stdstr strRtName =
                strAppName + "_Router";
            pMgr->SetRouterName(
                strRtName + "_" +
                std::to_string( getpid() ) );
            stdstr strDescPath;
            if( strRtFile.size() )
                strDescPath = strRtFile;
            else if( bAuth )
                strDescPath = "./rtauth.json";
            else
                strDescPath = "./router.json";

            if( bAuth )
            {
                pMgr->SetCmdLineOpt(
                    propHasAuth, bAuth );
            }

            CCfgOpener oRtCfg;
            oRtCfg.SetStrProp(
                propSvrInstName, strRtName );
            oRtCfg.SetPointer( propIoMgr, pMgr );
            pMgr->SetCmdLineOpt(
                propSvrInstName, strRtName );
            ret = CRpcServices::LoadObjDesc(
                strDescPath,
                OBJNAME_ROUTER,
                true, oRtCfg.GetCfg() );
            if( ERROR( ret ) )
                break;

            oRtCfg[ propIfStateClass ] =
                clsid( CIfRouterMgrState );
            oRtCfg[ propRouterRole ] = dwRole;
            ret =  g_pRouter.NewObj(
                clsid( CRpcRouterManagerImpl ),
                oRtCfg.GetCfg() );
            if( ERROR( ret ) )
                break;

            CInterfaceServer* pRouter = g_pRouter;
            if( unlikely( pRouter == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }
            ret = pRouter->Start();
            if( ERROR( ret ) )
            {
                pRouter->Stop();
                g_pRouter.Clear();
                break;
            }            
            sipRes = pObj;
            g_pIoMgr = *pObj;
        }

    }while( 0 );
    if( sipRes == nullptr )
    {
        delete pObj;
        if( g_pLibHandle != nullptr )
        {
            dlclose( g_pLibHandle );
            g_pLibHandle = nullptr;
        }
    }
    return sipRes;
}

gint32 StopIoMgr( ObjPtr* pObj )
{
    gint32 ret = 0;
    do{
        if( !g_pRouter.IsEmpty() )
        {
            IService* pRt = g_pRouter;
            pRt->Stop();
            g_pRouter.Clear();
        }
        g_pIoMgr.Clear();
        if( pObj == nullptr )
            break;
        cpp::IService* pMgr = *pObj;
        if( pMgr == nullptr )
            break;
        ret = pMgr->Stop();

    }while( 0 );
    if( g_pLibHandle != nullptr )
    {
        dlclose( g_pLibHandle );
        g_pLibHandle = nullptr;
    }
    return ret;
}

inline gint32 CheckAndResize( BufPtr& pBuf,
    gint32 iPos, gint32 iSize )
{
    gint32 ret = 0;
    do{
        if( pBuf.IsEmpty() || pBuf->empty() )
        {
            ret = ERROR_STATE;
            break;
        }
        if( iPos < 0 ||
            iPos + iSize > MAX_BYTES_PER_BUFFER )
        {
            ret = -ERANGE;
            break;
        }

        if( iPos + iSize > pBuf->size() )
        {
            if( iPos + iSize < 2048 )
            {
                pBuf->Resize( 2048 );
                break;
            }

            gint32 iNewSize = std::min(
                ( iPos + iSize ) * 2,
                MAX_BYTES_PER_BUFFER );
            pBuf->Resize( iNewSize );
        }

    }while( 0 );

    return ret;
}

void SerialIntNoCheck(
    BufPtr& pBuf, int iPos, gint32 val )
{
    guint32 nval = htonl( val );
    memcpy( pBuf->ptr() + iPos,
        &nval, sizeof( guint32 ) );
}

gint32 CoUninitializeEx()
{
    cpp::CoUninitialize();
    DebugPrintEx( logErr, 0,
        "#Leaked objects is %d",
        CObjBase::GetActCount() );
    return 0;
}
%}

%typemap(in,numinputs=0) JNIEnv *jenv "$1 = jenv;"

class ObjPtr
{
    public:

    ObjPtr();
    ~ObjPtr();
    void Clear();
    bool IsEmpty() const;
};

%extend ObjPtr {
    gint32 NewObj(
        EnumClsid iNewClsid, CfgPtr& pCfg )
    {
        return $self->NewObj( iNewClsid,
            ( IConfigDb* )pCfg );
    }

    jobject SerialToByteArray( JNIEnv *jenv )
    {
        BufPtr pBuf( true );
        ObjPtr& pObj = *$self;
        jobject jret = NewJRet( jenv );
        gint32 ret = 0;
        do{
            if( pObj.IsEmpty() )
            {
                SERI_HEADER_BASE ohb;
                ohb.dwClsid = clsid( Invalid );
                ohb.dwSize = 0;
                ohb.bVersion = 0;
                ohb.hton();
                ret = pBuf->Resize(
                    sizeof( SERI_HEADER_BASE ) );
                if( ERROR( ret ) )
                    break;

                memcpy( pBuf->ptr(), &ohb,
                    pBuf->size() );
            }
            else
            {
                ret = pObj->Serialize( *pBuf );
                if( ERROR( ret ) )
                    break;
            }

            jbyteArray retBuf =
                jenv->NewByteArray( pBuf->size() );

            jenv->SetByteArrayRegion(
                retBuf, 0, pBuf->size(),
                ( jbyte* )pBuf->ptr() );

            AddElemToJRet( jenv, jret, retBuf );
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject DeserialObjPtr(
        JNIEnv *jenv,
        jobject pByteArray,
        guint32 dwOffset )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        bool bRelease = false;
        char* buf = nullptr;
        jarray pArr = static_cast
            < jarray >( pByteArray );
        do{
            jsize len =
                jenv->GetArrayLength( pArr);

            buf = ( char* )jenv->GetPrimitiveArrayCritical(
                pArr, 0 );
            bRelease = true;

            char* pBuf = ( char* )buf + dwOffset;
            guint32 dwClsid = 0;
            memcpy( &dwClsid, pBuf, sizeof( dwClsid ) );
            dwClsid = ntohl( dwClsid );

            ObjPtr& pObj = *$self;

            guint32 dwSize = 0;
            memcpy( &dwSize, 
                pBuf + sizeof( guint32 ),
                sizeof( guint32 ) );

            dwSize = ntohl( dwSize ) +
                sizeof( SERI_HEADER_BASE );

            if( dwClsid != clsid( Invalid ) )
            {
                ret = pObj.NewObj(
                    ( EnumClsid )dwClsid );
                if( ERROR( ret ) )
                    break;

                if( len < dwOffset )
                {
                    ret = -ERANGE;
                    break;
                }
                if( ( guint32 )( len - dwOffset ) < dwSize )
                {
                    ret = -ERANGE;
                    break;
                }

                ret = pObj->Deserialize( pBuf, dwSize );
                if( ret == -ENOTSUP )
                    ret = -EINVAL;

                if( ERROR( ret ) )
                    break;

                jobject jNewOff = NewInt(
                    jenv, dwSize + dwOffset );
                if( jNewOff == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                AddElemToJRet(
                    jenv, jret, jNewOff );
            }

        }while( 0 );
        if( bRelease )
        {
            jenv->ReleasePrimitiveArrayCritical(
                pArr, buf, 0 );
        }
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
};

class CfgPtr
{
    public:

    CfgPtr( IConfigDb* pCfg, bool bAddRef );
    ~CfgPtr();
    bool IsEmpty() const;
    void Clear();
};

%extend CfgPtr{
%newobject NewObj;
    gint32 NewObj()
    {
        if( $self == nullptr )
            return -EFAULT;
        return $self->NewObj();
    }
    IConfigDb* GetPointer()
    {
        IConfigDb* pObj =
            (IConfigDb*)*$self;
        if( pObj == nullptr )
            return nullptr;
        return pObj;
    }
}

%nodefaultctor;
%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CBuffer {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
  }
class CBuffer : public CObjBase
{
    public:
    CBuffer( gint32 dwSize = 0 );
    CBuffer( const char* pData, gint32 dwSize );
    char* ptr();
    gint32 size() const;
};
%clearnodefaultctor;

class BufPtr
{
    public:
    void Clear();

    BufPtr( bool );
    BufPtr( const BufPtr );
    ~BufPtr();

};

%extend BufPtr {

    char* ptr()
    {
        if( $self == nullptr ||
            $self->IsEmpty() )
            return nullptr;
        if( ( *$self )->empty() )
            return nullptr;

        return ( *$self )->ptr();
    }

    gint32 size() const
    {
        if( $self == nullptr ||
            $self->IsEmpty() )
            return 0;
        return ( *$self )->size();
    }

    gint32 Resize( gint32 iSize )
    {
        if( $self == nullptr ||
            $self->IsEmpty() )
            return 0;
        return ( *$self )->Resize(
            ( guint32 ) iSize );
    }

    bool IsEmpty() const
    {
        if( $self == nullptr )
            return true;
        return $self->IsEmpty();
    }
 
    EnumTypeId GetExDataType()
    {
        if( $self == nullptr ||
            $self->IsEmpty() )
            return typeNone;
        if( ( *$self )->empty() )
            return typeNone;

        return ( *$self )->GetExDataType(); 
    }

    gint32 NewObj()
    {
        if( $self == nullptr )
            return -EFAULT;
        return $self->NewObj(
            Clsid_CBuffer, nullptr );
    }
    jobject GetByteArray( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            BufPtr& pBuf = *$self;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EFAULT;
                break;
            }
            jsize len = ( jsize )pBuf->size();
            jbyteArray bytes =
                jenv->NewByteArray( len );
            jenv->SetByteArrayRegion( bytes,
                0, len, ( jbyte* )pBuf->ptr() );
            AddElemToJRet( jenv, jret, bytes );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetByte( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            BufPtr& pBuf = *$self;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            guint8 val = ( guint8& )*pBuf;
            jobject jval = NewByte( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetShort( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            BufPtr& pBuf = *$self;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            guint16 val = ( guint16& )*pBuf;
            jobject jval = NewShort( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetInt( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            BufPtr& pBuf = *$self;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            guint32 val = ( guint32& )*pBuf;
            jobject jval = NewInt( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetLong( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            BufPtr& pBuf = *$self;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            guint64 val = ( guint64& )*pBuf;
            jobject jval = NewLong( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetFloat( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            BufPtr& pBuf = *$self;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            float val = ( float& )*pBuf;
            jobject jval = NewFloat( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetDouble( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            BufPtr& pBuf = *$self;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            double val = ( double& )*pBuf;
            jobject jval = NewDouble( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetObjPtr( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            BufPtr& pBuf = *$self;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            ObjPtr& val = ( ObjPtr& )*pBuf;
            ObjPtr* ppObj = new ObjPtr( val );
            jobject jval =
                NewObjPtr( jenv, ( jlong )ppObj );
            if( jval == nullptr )
            {
                delete ppObj;
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetString( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            BufPtr& pBuf = *$self;
            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -EINVAL;
                break;
            }
            stdstr strVal = ( char* )*pBuf;
            jobject jval =
                GetJniString( jenv, strVal );

            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    gint32 SetByteArray(
        JNIEnv *jenv, jbyteArray pArr )
    {
        gint32 ret = 0;
        jsize len =
            jenv->GetArrayLength( pArr );

        BufPtr& pBuf = *$self;
        ret = pBuf->Resize( len );
        if( ERROR( ret ) )
            return ret;
        jenv->GetByteArrayRegion( pArr,
            0, len, ( jbyte* )pBuf->ptr() );
        return ret;
    }

    gint32 SetString(
        JNIEnv *jenv, jstring jstr )
    {
        if( $self == nullptr )
            return -EINVAL;

        if( jstr == nullptr )
            return -EFAULT;

        BufPtr& pBuf = *$self;
        if( pBuf.IsEmpty() )
            pBuf.NewObj();

        const char* val = 
            jenv->GetStringUTFChars( jstr, 0 );

        *pBuf = val;
        jenv->ReleaseStringUTFChars( jstr, val); 
        return STATUS_SUCCESS;
    }

    gint32 SetByte( jbyte val )
    {
        if( $self == nullptr )
            return -EINVAL;
        BufPtr& pBuf = *$self;
        if( pBuf.IsEmpty() )
            pBuf.NewObj();
        *pBuf = ( guint8 )val;
        return STATUS_SUCCESS;
    }
    gint32 SetShort( jshort val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        BufPtr& pBuf = *$self;
        if( pBuf.IsEmpty() )
            pBuf.NewObj();
        *pBuf = ( guint16 )val;
        return STATUS_SUCCESS;
    }
    gint32 SetInt( jint val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        BufPtr& pBuf = *$self;
        if( pBuf.IsEmpty() )
            pBuf.NewObj();
        *pBuf = ( guint32 )val;
        return STATUS_SUCCESS;
    }
    gint32 SetLong( jlong val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        BufPtr& pBuf = *$self;
        if( pBuf.IsEmpty() )
            pBuf.NewObj();
        *pBuf = ( guint64 )val;
        return STATUS_SUCCESS;
    }
    gint32 SetFloat( jfloat val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        BufPtr& pBuf = *$self;
        if( pBuf.IsEmpty() )
            pBuf.NewObj();
        *pBuf = ( float )val;
        return STATUS_SUCCESS;
    }
    gint32 SetDouble( jdouble val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        BufPtr& pBuf = *$self;
        if( pBuf.IsEmpty() )
            pBuf.NewObj();
        *pBuf = ( double )val;
        return STATUS_SUCCESS;
    }
    gint32 SetObjPtr( ObjPtr* ppObj )
    { 
        if( $self == nullptr )
            return -EINVAL;
        BufPtr& pBuf = *$self;
        if( pBuf.IsEmpty() )
            pBuf.NewObj();
        *pBuf = *ppObj;
        return STATUS_SUCCESS;
    }

    gint32 SerialByteArray(
        JNIEnv *jenv, int iPos, jbyteArray pArr )
    {
        gint32 ret = 0;
        jsize len = jenv->GetArrayLength( pArr );

        BufPtr& pBuf = *$self;
        ret = CheckAndResize( pBuf, iPos, len + 4 );
        if( ERROR( ret ) )
            return ret;

        SerialIntNoCheck( pBuf, iPos, len );
        if( len == 0 )
            return iPos + 4;
        jenv->GetByteArrayRegion( pArr, 0, len,
            ( jbyte* )( pBuf->ptr() + iPos + 4 ) );
        
        return iPos + len + 4;
    }

    gint32 SerialString(
        JNIEnv *jenv, int iPos, jstring jstr )
    {
        if( $self == nullptr )
            return -EINVAL;

        if( jstr == nullptr )
            return -EFAULT;

        gint32 ret = 0;
        do{
            const char* val = 
                jenv->GetStringUTFChars( jstr, 0 );

            gint32 len = strlen( val );
            BufPtr& pBuf = *$self;
            ret = CheckAndResize( pBuf, iPos, len + 4 );
            if( ERROR( ret ) )
                break;

            SerialIntNoCheck( pBuf, iPos, len );

            iPos += 4;
            if( len == 0 )
                break;
            memcpy( pBuf->ptr() + iPos, val, len );
            iPos += len;
            jenv->ReleaseStringUTFChars( jstr, val); 

        }while( 0 );

        if( ERROR( ret ) )
            return ret;

        return iPos;
    }

    gint32 SerialByte( int iPos, jbyte val )
    {
        if( $self == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            BufPtr& pBuf = *$self;
            ret = CheckAndResize( pBuf, iPos, 1 );
            if( ERROR( ret ) )
                break;
            *( pBuf->ptr() + iPos ) = ( guint8 )val;

        }while( 0 );

        if( ERROR( ret ) )
            return ret;

        return iPos + 1;
    }

    gint32 SerialShort( int iPos, jshort val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        constexpr gint32 iSize = sizeof( gint16 );
        BufPtr& pBuf = *$self;
        gint32 ret = CheckAndResize(
            pBuf, iPos, iSize );
        if( ERROR( ret ) )
            return ret;

        guint16 nval = htons( val );
        guint8* psrc = ( guint8* )&nval;
        guint8* pdest = ( guint8*)
            ( pBuf->ptr() + iPos );
        *pdest++=*psrc++;
        *pdest=*psrc;
        return iPos + iSize;
    }

    gint32 SerialInt( int iPos, jint val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        constexpr gint32 iSize = sizeof( gint32 );
        BufPtr& pBuf = *$self;
        gint32 ret = CheckAndResize(
            pBuf, iPos, iSize );
        if( ERROR( ret ) )
            return ret;

        guint32 nval = htonl( val );
        guint8* psrc = ( guint8* )&nval;
        guint8* pdest = ( guint8*)
            ( pBuf->ptr() + iPos );
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest=*psrc;
        return iPos + iSize;
    }
    gint32 SerialLong( int iPos, jlong val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        constexpr gint32 iSize = sizeof( gint64 );
        BufPtr& pBuf = *$self;
        gint32 ret = CheckAndResize(
            pBuf, iPos, iSize );
        if( ERROR( ret ) )
            return ret;

        guint64 nval = htonll( val );
        guint8* psrc = ( guint8* )&nval;
        guint8* pdest =
            ( guint8*) ( pBuf->ptr() + iPos );
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest=*psrc;
        return iPos + iSize;
    }
    gint32 SerialFloat( int iPos, jfloat val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        gint32 iSize = sizeof( float );
        BufPtr& pBuf = *$self;
        gint32 ret = CheckAndResize(
            pBuf, iPos, iSize );
        if( ERROR( ret ) )
            return ret;

        guint32 nval = htonl( *( guint32* )&val );
        guint8* psrc = ( guint8* )&nval;
        guint8* pdest = ( guint8*)
            ( pBuf->ptr() + iPos );
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest=*psrc;
        return iPos + iSize;
    }

    gint32 SerialDouble( int iPos, jdouble val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        gint32 iSize = sizeof( double );
        BufPtr& pBuf = *$self;
        gint32 ret = CheckAndResize(
            pBuf, iPos, iSize );
        if( ERROR( ret ) )
            return ret;

        guint64 nval = htonll( *( guint64* )&val );
        guint8* psrc = ( guint8* )&nval;
        guint8* pdest =
            ( guint8*) ( pBuf->ptr() + iPos );
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest++=*psrc++;
        *pdest=*psrc;
        return iPos + iSize;
    }

    gint32 SerialObjPtr( int iPos, ObjPtr* ppObj )
    {
        if( $self == nullptr )
            return -EINVAL;
        if( ppObj == nullptr ||
            ( *ppObj ).IsEmpty() )
            return -EINVAL;

        BufPtr pObjBuf( true );
        gint32 ret = 0;
        do{
            BufPtr& pBuf = *$self;
            ret = ( *ppObj )->Serialize( *pObjBuf );
            if( ERROR( ret ) )
                break;

            ret = CheckAndResize(
                pBuf, iPos, pObjBuf->size() );
            if( ERROR( ret ) )
                break;

            memcpy( pBuf->ptr() + iPos,
                pObjBuf->ptr(), pObjBuf->size() );

        }while( 0 );
        if( ERROR( ret ) )
            return ret;

        return iPos + pObjBuf->size();
    }

    gint32 SerialInt8Arr(
        JNIEnv *jenv, int iPos, jbyteArray val )
    { 
        if( jenv == nullptr || val == nullptr )
            return -EINVAL;
        gint32 ret = 0;
        gint32 iNewPos = 0;
        do{
            constexpr gint32 iHeader = 8;
            jsize iCount =
                jenv->GetArrayLength( val );

            BufPtr& pBuf = *$self;
            ret = CheckAndResize( pBuf, iPos,
                iCount * sizeof( gint8 ) + iHeader );
            if( ERROR( ret ) )
                break;

            SerialIntNoCheck( pBuf,
                iPos, iCount * sizeof( gint8 ) );

            SerialIntNoCheck( pBuf,
                iPos + sizeof( guint32 ), iCount );
            if( iCount == 0 )
            {
                iNewPos = iPos + iHeader;
                break;
            }

            jbyte* vals =
                jenv->GetByteArrayElements( val, 0 );

            gint8* pInt =
                ( gint8* )( pBuf->ptr() + iPos + iHeader );
            for( gint32 i = 0; i < iCount; i++ )
                *pInt++ = vals[ i ];

            iNewPos = iPos +
                iHeader + iCount * sizeof( gint8 );

            jenv->ReleaseByteArrayElements(
                val, vals, 0);
        }while( 0 );
        if( ERROR( ret ) )
            return ret;
        return iNewPos;
    }

    gint32 SerialShortArr(
        JNIEnv *jenv, int iPos, jshortArray val )
    { 
        if( jenv == nullptr || val == nullptr )
            return -EINVAL;
        gint32 ret = 0;
        gint32 iNewPos = 0;
        do{
            constexpr gint32 iHeader = 8;
            jsize iCount =
                jenv->GetArrayLength( val );

            BufPtr& pBuf = *$self;
            ret = CheckAndResize( pBuf, iPos,
                iCount * sizeof( gint16 ) + iHeader );
            if( ERROR( ret ) )
                break;

            SerialIntNoCheck( pBuf,
                iPos, iCount * sizeof( gint16 ) );

            SerialIntNoCheck( pBuf,
                iPos + sizeof( guint32 ), iCount );
            if( iCount == 0 )
            {
                iNewPos = iPos + iHeader;
                break;
            }


            jshort* vals =
                jenv->GetShortArrayElements( val, 0 );

            gint8* pInt =
                ( gint8* )( pBuf->ptr() + iPos + iHeader );
            for( gint32 i = 0; i < iCount; i++ )
            {
                guint16 sval = htons( vals[ i ] );
                gint8* psrc = ( gint8* )&sval;
                *pInt++ = *psrc++;
                *pInt++ = *psrc;
            }

            iNewPos = iPos +
                iHeader + iCount * sizeof( gint16 );

            jenv->ReleaseShortArrayElements(
                val, vals, 0);
        }while( 0 );
        if( ERROR( ret ) )
            return ret;
        return iNewPos;
    }

    gint32 SerialIntArr(
        JNIEnv *jenv, int iPos, jintArray val )
    {
        if( jenv == nullptr || val == nullptr )
            return -EINVAL;
        gint32 ret = 0;
        gint32 iNewPos = 0;
        do{
            constexpr gint32 iHeader = 8;
            jsize iCount =
                jenv->GetArrayLength( val );

            BufPtr& pBuf = *$self;
            ret = CheckAndResize( pBuf, iPos,
                iCount * sizeof( gint32 ) + iHeader );
            if( ERROR( ret ) )
                break;

            SerialIntNoCheck( pBuf,
                iPos, iCount * sizeof( guint32 ) );
            SerialIntNoCheck( pBuf,
                iPos + sizeof( guint32 ), iCount );
            if( iCount == 0 )
            {
                iNewPos = iPos + iHeader;
                break;
            }

            jint* vals =
                jenv->GetIntArrayElements( val, 0 );

            gint8* pInt =
                ( gint8* )( pBuf->ptr() + iPos + iHeader );
            for( gint32 i = 0; i < iCount; i++ )
            {
                // to avoid not-align-on-boundary
                // exception on some low-end arch
                guint32 sval = htonl( vals[ i ] );
                gint8* psrc = ( gint8* )&sval;
                *pInt++ = *psrc++;
                *pInt++ = *psrc++;
                *pInt++ = *psrc++;
                *pInt++ = *psrc;
            }
            iNewPos = iPos +
                iHeader + iCount * sizeof( gint32 );
            jenv->ReleaseIntArrayElements(
                val, vals, 0);
        }while( 0 );
        if( ERROR( ret ) )
            return ret;
        return iNewPos;
    }

    gint32 SerialLongArr(
        JNIEnv *jenv, int iPos, jlongArray val )
    {
        if( jenv == nullptr || val == nullptr )
            return -EINVAL;
        gint32 ret = 0;
        gint32 iNewPos = 0;
        do{
            constexpr gint32 iHeader = 8;
            jsize iCount =
                jenv->GetArrayLength( val );

            BufPtr& pBuf = *$self;
            ret = CheckAndResize( pBuf, iPos,
                iCount * sizeof( guint64 ) + iHeader );
            if( ERROR( ret ) )
                break;

            SerialIntNoCheck( pBuf,
                iPos, iCount * sizeof( guint64 ) );

            SerialIntNoCheck( pBuf,
                iPos + sizeof( guint32 ), iCount );
            if( iCount == 0 )
            {
                iNewPos = iPos + iHeader;
                break;
            }

            jlong* vals =
                jenv->GetLongArrayElements( val, 0 );

            gint8* pLong =
                ( gint8* )( pBuf->ptr() + iPos + iHeader );
            for( gint32 i = 0; i < iCount; i++ )
            {
                guint64 sval = htonll( vals[ i ] );
                gint8* psrc = ( gint8*)&sval;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc;
            }
            iNewPos = iPos + iHeader +
                iCount * sizeof( guint64 );
            jenv->ReleaseLongArrayElements(
                val, vals, 0);
        }while( 0 );
        if( ERROR( ret ) )
            return ret;
        return iNewPos;
    }
    gint32 SerialFloatArr(
        JNIEnv *jenv, int iPos, jfloatArray val )
    {
        if( jenv == nullptr || val == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        gint32 iNewPos = 0;
        do{
            constexpr gint32 iHeader = 8;
            jsize iCount =
                jenv->GetArrayLength( val );

            BufPtr& pBuf = *$self;
            ret = CheckAndResize( pBuf, iPos,
                iCount * sizeof( float ) + iHeader );
            if( ERROR( ret ) )
                break;

            SerialIntNoCheck( pBuf,
                iPos, iCount * sizeof( float ) );

            SerialIntNoCheck( pBuf,
                iPos + sizeof( guint32 ), iCount );

            if( iCount == 0 )
            {
                iNewPos = iPos + iHeader;
                break;
            }

            jfloat* vals =
                jenv->GetFloatArrayElements( val, 0 );

            gint8* pInt =
                ( gint8* )( pBuf->ptr() + iPos + iHeader );

            for( gint32 i = 0; i < iCount; i++ )
            {
                guint32 sval = htonl( vals[ i ] );
                gint8* psrc = ( gint8* )&sval;
                *pInt++ = *psrc++;
                *pInt++ = *psrc++;
                *pInt++ = *psrc++;
                *pInt++ = *psrc;
            }
            iNewPos = iPos + iHeader +
                iCount * sizeof( float );

            jenv->ReleaseFloatArrayElements(
                val, vals, 0);
        }while( 0 );
        if( ERROR( ret ) )
            return ret;
        return iNewPos;
    }

    gint32 SerialDoubleArr(
        JNIEnv *jenv, int iPos, jdoubleArray val )
    {
        if( jenv == nullptr || val == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        gint32 iNewPos = 0;
        do{
            constexpr gint32 iHeader = 8;
            jsize iCount =
                jenv->GetArrayLength( val );

            BufPtr& pBuf = *$self;
            ret = CheckAndResize( pBuf, iPos,
                iCount * sizeof( double ) + iHeader );
            if( ERROR( ret ) )
                break;

            SerialIntNoCheck( pBuf,
                iPos, iCount * sizeof( double ) );
            SerialIntNoCheck( pBuf,
                iPos + sizeof( guint32 ), iCount );
            if( iCount == 0 )
            {
                iNewPos = iPos + iHeader;
                break;
            }

            jdouble* vals =
                jenv->GetDoubleArrayElements( val, 0 );

            gint8* pLong =
                ( gint8* )( pBuf->ptr() + iPos + iHeader );

            for( gint32 i = 0; i < iCount; i++ )
            {
                guint64 sval = htonll( vals[ i ] );
                gint8* psrc = ( gint8*)&sval;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc++;
                *pLong++ = *psrc;
            }
            iNewPos = iPos + iHeader +
                iCount * sizeof( double );

            jenv->ReleaseDoubleArrayElements(
                val, vals, 0);

        }while( 0 );

        if( ERROR( ret ) )
            return ret;
        return iNewPos;
    }

};

class CParamList
{
    public:
    CParamList();
    CParamList( const CParamList& oParams );
    CParamList( CfgPtr& pCfg );

    gint32 Reset();

    gint32 SetStrProp( gint32,
        const std::string& strVal );

    gint32 SetDoubleProp(
        gint32, double fVal );

    gint32 SetFloatProp(
        gint32, float fVal );

    gint32 SetObjPtr(
        gint32 iProp, ObjPtr& pObj );

    gint32 SetBoolProp(
        gint32, bool bVal );

    gint32 SetBufPtr(
        gint32 iProp, BufPtr& pObj );

};

%extend CParamList {
    jobject GetStrProp( 
        JNIEnv *jenv, gint32 iProp )
    {
        
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            stdstr strVal;
            ret = oParams.GetStrProp(
                iProp, strVal );
            if( ERROR( ret ) )
                break;
            jobject jStr = GetJniString(
                jenv, strVal );

            AddElemToJRet( jenv, jret, jStr );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetByteProp(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            guint8 byVal;
            ret = oParams.GetByteProp(
                iProp, byVal );
            if( ERROR( ret ) )
                break;

            jobject jByte =
                NewByte( jenv, byVal );

            AddElemToJRet( jenv, jret, jByte );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    gint32 SetByteProp(
        gint32 iProp, gint8 byVal )
    {
        return $self->SetIntProp(
            iProp, ( guint8 )byVal ); 
    }

    jobject GetShortProp(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            guint16 wVal;
            ret = oParams.GetShortProp(
                iProp, wVal );
            if( ERROR( ret ) )
                break;

            jobject jWord =
                NewShort( jenv, wVal );

            AddElemToJRet( jenv, jret, jWord );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    gint32 SetShortProp(
        gint32 iProp, gint16 wVal )
    {
        return $self->SetShortProp(
            iProp, ( guint16 )wVal ); 
    }

    jobject GetIntProp(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            guint32 dwVal;
            ret = oParams.GetIntProp(
                iProp, dwVal );
            if( ERROR( ret ) )
                break;

            jobject jdword =
                NewInt( jenv, dwVal );

            AddElemToJRet( jenv, jret, jdword );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    gint32 SetIntProp(
        gint32 iProp, gint32 dwVal )
    {
        return $self->SetIntProp(
            iProp, ( guint32 )dwVal ); 
    }

    jobject GetQwordProp(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            guint64 qwVal;
            ret = oParams.GetQwordProp(
                iProp, qwVal );
            if( ERROR( ret ) )
                break;

            jobject jqword =
                NewInt( jenv, qwVal );

            AddElemToJRet( jenv, jret, jqword );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    gint32 SetQwordProp(
        gint32 iProp, gint64 qwVal )
    {
        return $self->SetQwordProp(
            iProp, ( guint64 )qwVal ); 
    }

    jobject GetDoubleProp(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            double dblVal;
            ret = oParams.GetDoubleProp(
                iProp, dblVal );
            if( ERROR( ret ) )
                break;

            jobject jdbl =
                NewDouble( jenv, dblVal );

            AddElemToJRet( jenv, jret, jdbl );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetFloatProp(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            float fVal;
            ret = oParams.GetFloatProp(
                iProp, fVal );
            if( ERROR( ret ) )
                break;

            jobject jfv =
                NewFloat( jenv, fVal );

            AddElemToJRet( jenv, jret, jfv );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetObjPtr(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            ObjPtr* ppObj = new ObjPtr();
            ret = oParams.GetObjPtr( iProp, *ppObj );
            if( ERROR( ret ) )
            {
                delete ppObj;
                break;
            }

            jobject jop = NewObjPtr(
                jenv, ( jlong )ppObj, true );
            if( jop == nullptr )
            {
                delete ppObj;
                ret = -ENOMEM;
                break;
            }
            AddElemToJRet( jenv, jret, jop );

        }while( 0 );


        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetBoolProp(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            bool bVal;
            ret = oParams.GetBoolProp(
                iProp, bVal );
            if( ERROR( ret ) )
                break;

            jobject jbv =
                NewBoolean( jenv, bVal );

            AddElemToJRet( jenv, jret, jbv );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetByteArray(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            BufPtr pBuf;
            ret = oParams.GetProperty(
                iProp, pBuf );
            if( ERROR( ret ) )
                break;

            jsize len = ( jsize )pBuf->size();
            jbyteArray bytes =
                jenv->NewByteArray( len );
            jenv->SetByteArrayRegion( bytes,
                0, len, ( jbyte* )pBuf->ptr() );

            AddElemToJRet( jenv, jret, bytes );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetPropertyType(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            CfgPtr& pCfg = oParams.GetCfg();
            gint32 iType = 0;
            ret = pCfg->GetPropertyType(
                iProp, iType );
            if( ERROR( ret ) )
                break;
            jobject jval = NewInt( jenv, iType );
            if( jval == nullptr )
            {
                ret = -ENOMEM;
                break;
            }
            AddElemToJRet( jenv, jret, jval );

        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetBufPtr(
        JNIEnv *jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            BufPtr* ppBuf = new BufPtr();

            ret = oParams.GetBufPtr(
                iProp, *ppBuf );

            if( ERROR( ret ) )
            {
                delete ppBuf;
                break;
            }

            jobject jop = NewBufPtr(
                jenv, ( jlong )ppBuf, true );

            if( jop == nullptr )
            {
                delete ppBuf;
                ret = -ENOMEM;
                break;
            }
            AddElemToJRet( jenv, jret, jop );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    gint32 PushByte( jbyte val )
    {
        return $self->Push( ( guint8 )val );
    }

    gint32 PushObj( ObjPtr& val )
    {
        return $self->Push( val );
    }

    gint32 PushStr( const std::string& val )
    {
        return $self->Push( val );
    }

    gint32 PushInt( guint32 val )
    {
        return $self->Push( val );
    }

    gint32 PushBool( bool& val )
    {
        return $self->Push( val );
    }

    gint32 PushQword( gint64& val )
    {
        return $self->Push( val );
    }

    gint32 PushByteArr(
        JNIEnv *jenv, jbyteArray pba )
    {
        gint32 ret = 0;
        if( jenv == nullptr || pba == nullptr )
            return -EINVAL;
        do{
            BufPtr pBuf( true );
            jsize len =
                jenv->GetArrayLength( pba );

            if( len > 0 )
            {
                ret = pBuf->Resize( len );
                if( ERROR( ret ) )
                    break;
                
                jenv->GetByteArrayRegion(
                    pba, 0, len,
                    ( jbyte* )pBuf->ptr() );
            }

            ret = $self->Push( pBuf );

        }while( 0 );

        return ret;
    }

    jobject GetCfg( JNIEnv *jenv )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            if( $self == nullptr )
            {
                ret = -EINVAL;
                break;
            };

            ObjPtr* pCfg;
            pCfg = new ObjPtr( ( CObjBase* )
                $self->GetCfg(), true );

            jobject jObj = NewObjPtr(
                jenv, ( jlong )pCfg, true );

            if( jObj == nullptr )
            {
                delete pCfg;
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jObj );

        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetSize( JNIEnv *jenv )
    {
        gint32 ret = 0;
        jobject jret = NewJRet( jenv );
        do{
            guint32 dwSize = 0;
            gint32 ret = $self->GetSize( dwSize );
            if( ERROR( ret ) )
                break;

            if( dwSize & 0x80000000 )
            {
                ret = -ERANGE;
                break;
            }
            jobject jInt = NewInt( jenv, dwSize );
            if( jInt == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jInt );

        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
};

%newobject CreateObject;
ObjPtr* CreateObject(
    EnumClsid iClsid, CfgPtr& pCfg );

%newobject CastToCfg;
CfgPtr* CastToCfg( ObjPtr* pObj );

%newobject StartIoMgr;
ObjPtr* StartIoMgr( CfgPtr& pCfg );

gint32 StopIoMgr( ObjPtr* pObj );

%newobject CastToSvc;
IService* CastToSvc(
    JNIEnv *jenv, ObjPtr* pObj );

%nodefaultctor;
%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CRpcServices {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
  }
class CRpcServices : public IService
{
    public:
    static gint32 LoadObjDesc(
        const std::string& strFile,
        const std::string& strObjName,
        bool bServer,
        CfgPtr& pCfg );
    EnumIfState GetState() const;
    bool IsServer();
};

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CInterfaceProxy {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
  }
class CInterfaceProxy : public CRpcServices
{
    public:
    gint32 CancelRequest( gint64 qwTaskId );
    gint32 Pause();
    gint32 Resume();
};

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CInterfaceServer {
    if (swigCPtr != 0) {
      swigCPtr = 0;
    }
    super.delete();
  }
class CInterfaceServer : public CRpcServices
{
};

%clearnodefaultctor;

%newobject CastToRpcSvcs;
CRpcServices* CastToRpcSvcs(
    JNIEnv *jenv, ObjPtr* pObj );

class Variant
{
    public:
    void Clear();

    Variant();
    Variant( const Variant& );
    ~Variant();
    bool empty() const;
};

%extend Variant {

    EnumTypeId GetTypeId()
    {
        if( $self == nullptr || $self->empty() )
            return typeNone;
        return ( $self )->GetTypeId(); 
    }

    jobject GetByteArray( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            if( $self->GetTypeId() != typeByteArr )
            {
                ret = -EINVAL;
                break;
            }
            BufPtr& pBuf = *$self;
            jsize len = 0;
            if( !pBuf.IsEmpty() )
                len = ( jsize )pBuf->size();
            jbyteArray bytes =
                jenv->NewByteArray( len );
            jenv->SetByteArrayRegion( bytes,
                0, len, ( jbyte* )pBuf->ptr() );
            AddElemToJRet( jenv, jret, bytes );

        }while( 0 );

        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetByte( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            if( $self->GetTypeId() != typeByte )
            {
                ret = -EINVAL;
                break;
            }
            guint8 val = ( guint8& )*$self;
            jobject jval = NewByte( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetShort( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            if( $self->GetTypeId() != typeUInt16 )
            {
                ret = -EINVAL;
                break;
            }
            guint16 val = ( guint16& )*$self;
            jobject jval = NewShort( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetInt( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            if( $self->GetTypeId() != typeUInt32 )
            {
                ret = -EINVAL;
                break;
            }
            guint32 val = ( guint32& )*$self;
            jobject jval = NewInt( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetLong( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            if( $self->GetTypeId() != typeUInt64 )
            {
                ret = -EINVAL;
                break;
            }
            guint64 val = ( guint64& )*$self;
            jobject jval = NewLong( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetFloat( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            if( $self->GetTypeId() != typeFloat )
            {
                ret = -EINVAL;
                break;
            }
            float val = ( float& )*$self;
            jobject jval = NewFloat( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }
    jobject GetDouble( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            if( $self->GetTypeId() != typeDouble )
            {
                ret = -EINVAL;
                break;
            }
            double val = ( double& )*$self;
            jobject jval = NewDouble( jenv, val );
            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetObjPtr( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            if( $self->GetTypeId() != typeObj )
            {
                ret = -EINVAL;
                break;
            }
            ObjPtr& val = ( ObjPtr& )*$self;
            ObjPtr* ppObj = new ObjPtr( val );
            jobject jval =
                NewObjPtr( jenv, ( jlong )ppObj );
            if( jval == nullptr )
            {
                delete ppObj;
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    jobject GetString( JNIEnv *jenv )
    {
        CHECK_ENV( jenv );
        do{
            if( $self->GetTypeId() != typeString )
            {
                ret = -EINVAL;
                break;
            }
            stdstr strVal = *$self;
            jobject jval =
                GetJniString( jenv, strVal );

            if( jval == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jret, jval );
            
        }while( 0 );
        SetErrorJRet( jenv, jret, ret );
        return jret;
    }

    gint32 SetByteArray(
        JNIEnv *jenv, jbyteArray pArr )
    {
        gint32 ret = 0;
        jsize len =
            jenv->GetArrayLength( pArr );

        BufPtr pBuf( true );
        ret = pBuf->Resize( len );
        if( ERROR( ret ) )
            return ret;
        jenv->GetByteArrayRegion( pArr,
            0, len, ( jbyte* )pBuf->ptr() );
        *$self = pBuf;
        return ret;
    }

    gint32 SetString(
        JNIEnv *jenv, jstring jstr )
    {
        if( $self == nullptr )
            return -EINVAL;

        if( jstr == nullptr )
            return -EFAULT;

        const char* val = 
            jenv->GetStringUTFChars( jstr, 0 );

        *$self = val;
        jenv->ReleaseStringUTFChars( jstr, val); 
        return STATUS_SUCCESS;
    }

    gint32 SetByte( jbyte val )
    {
        if( $self == nullptr )
            return -EINVAL;
        *$self = ( guint8 )val;
        return STATUS_SUCCESS;
    }
    gint32 SetShort( jshort val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        *$self = ( guint16 )val;
        return STATUS_SUCCESS;
    }
    gint32 SetInt( jint val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        *$self = ( guint32 )val;
        return STATUS_SUCCESS;
    }
    gint32 SetLong( jlong val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        *$self = ( guint64 )val;
        return STATUS_SUCCESS;
    }
    gint32 SetFloat( jfloat val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        *$self = ( float )val;
        return STATUS_SUCCESS;
    }
    gint32 SetDouble( jdouble val )
    { 
        if( $self == nullptr )
            return -EINVAL;
        *$self = ( double )val;
        return STATUS_SUCCESS;
    }
    gint32 SetObjPtr( ObjPtr* ppObj )
    { 
        if( $self == nullptr )
            return -EINVAL;
        *$self = *ppObj;
        return STATUS_SUCCESS;
    }
    gint32 SetBufPtr( BufPtr* ppObj )
    {
        if( $self == nullptr )
            return -EINVAL;
        *$self = *ppObj;
        return STATUS_SUCCESS;
    }
};
%include "proxy.i"
