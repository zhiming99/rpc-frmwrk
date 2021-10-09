%module rpcbase

%{
#include <string>
#include "rpc.h"
#include "defines.h"
#include <stdint.h>
#include <errno.h>
#include <vector>
using namespace rpcf;
%}

%include "std_string.i"
%include "stdint.i"

typedef int32_t gint32;
typedef uint32_t guint32;
typedef int64_t  gint64;
typedef uint64_t guint64;
typedef gint64  LONGWORD;
typedef LONGWORD HANDLE;

%javaconst(1);

%javaconst(0) Clsid_Invalid;
%javaconst(0) Clsid_CIoManager;
%javaconst(0) Clsid_CConfigDb;

typedef int32_t EnumClsid;

enum 
{
    Clsid_Invalid,
    Clsid_CIoManager,
    Clsid_CConfigDb,
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
    propKeepAliveSec
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
    typeByteObj,
    typeUInt16Obj,
    typeUInt32Obj,
    typeUInt64Obj,
    typeFloatObj,
    typeDoubleObj,
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

gint32 CoInitialize( gint32 iCtx );
gint32 CoUninitialize();

%nodefaultctor;

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
    gint32 SetClassId( EnumClsid iClsid );
    EnumClsid GetClsid() const;
};

%feature("ref")   CObjBase "$this->AddRef();"
%feature("unref") CObjBase "$this->Release();"

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") IConfigDb {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
      }
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
      if (swigCMemOwn) {
        swigCMemOwn = false;
      }
      swigCPtr = 0;
    }
    super.delete();
  }

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") IService {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
      }
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

class ObjPtr
{
    public:

    ObjPtr();
    ~ObjPtr();
    bool IsEmpty() const;
};

%extend ObjPtr {
    gint32 NewObj(
        EnumClsid iNewClsid, CfgPtr& pCfg )
    {
        return $self->NewObj( iNewClsid,
            ( IConfigDb* )pCfg );
    }
}
%header %{

jobject NewJRet( JNIEnv* jenv )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "org/rpcf/rpcbase/JRetVal");

    jmethodID ctor  = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "()V");

    return ( *jenv )->NewObject( cls, ctor);
}

void AddElemToJRet( JNIEnv* jenv,
    jobject jRet, jobject pObj )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "org/rpcf/rpcbase/JRetVal");

    jmethodId addElem = (*jenv)->GetMethodID(
        jenv, cls, "addElemToJRet",
        "(Ljava/lang/Object;)V");

    (*jenv)->CallVoidMethod(
        jenv, jRet, addElem, pObj );

    return;
}

void SetErrorJRet( JNIEnv* jenv,
    jobject jRet, gint32 iRet )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "org/rpcf/rpcbase/JRetVal");

    jmethodId setError = (*jenv)->GetMethodID(
        jenv, cls, "setError", "(I)V");

    (*jenv)->CallVoidMethod(
        jenv, jRet, setError, iRet );

    return;
}

EnumTypeId GetTypeId(
    JNIEnv* jenv, jobject pObj )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "org/rpcf/rpcbase/Helpers");

    jmethodId gtid_method = (*jenv)->GetMethodID(
        jenv, cls, "getTypeId",
        "(Ljava/lang/Object;)I");

    jint iType = (*jenv)->CallIntMethod(
        jenv, jRet, gtid_method, pObj );

    return (EnumTypeId)iType;
}

jobject NewIService( JNIEnv* jenv,
    long lPtr, bool bOwner )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "org/rpcf/rpcbase/IService");

    jmethodID ctor  = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "(JZ)V");

    return ( *jenv )->NewObject(
        cls, ctor, lPtr, bOwner );
}

jobject NewObjPtr( JNIEnv* jenv,
    long lPtr, bool bOwner )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "org/rpcf/rpcbase/ObjPtr");

    jmethodID ctor  = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "(JZ)V");

    return ( *jenv )->NewObject(
        cls, ctor, lPtr, bOwner );
}

jobject NewBoolean( JNIEnv* jenv, bool val )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "java/lang/Boolean");
    jmethodID midInit = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "(Z)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = (*jenv)->NewObject(
        jenv, cls, midInit, (jboolean)val);
    return newObj;
}

jobject NewByte( JNIEnv* jenv, guint8 val )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "java/lang/Byte");
    jmethodID midInit = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "(B)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = (*jenv)->NewObject(
        jenv, cls, midInit, (jbyte)val);
    return newObj;
}

jobject NewShort( JNIEnv* jenv, guint16 val )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "java/lang/Short");
    jmethodID midInit = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "(S)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = (*jenv)->NewObject(
        jenv, cls, midInit, val);
    return newObj;
}

jobject NewInt( JNIEnv* jenv, int val )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "java/lang/Integer");
    jmethodID midInit = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "(I)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = (*jenv)->NewObject(
        jenv, cls, midInit, val);
    return newObj;
}

jobject NewLong( JNIEnv* jenv, gint64 val )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "java/lang/Long");
    jmethodID midInit = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "(J)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = (*jenv)->NewObject(
        jenv, cls, midInit, val);
    return newObj;
}

jobject NewFloat( JNIEnv* jenv, float val )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "java/lang/Float");
    jmethodID midInit = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "(F)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = (*jenv)->NewObject(
        jenv, cls, midInit, val);
    return newObj;
}

jobject NewDouble( JNIEnv* jenv, double val )
{
    jclass cls = (*jenv)->FindClass(
        jenv, "java/lang/Double");
    jmethodID midInit = (*jenv)->GetMethodID(
        jenv, cls, "<init>", "(D)V");
    if( nullptr == midInit )
        return nullptr;
    jobject newObj = (*jenv)->NewObject(
        jenv, cls, midInit, val);
    return newObj;
}

jobject GetJniString(JNIEnv *jenv, const stdstr& message )
{
    int byteCount = message.length();
    const jbyte* pNativeMessage =
        reinterpret_cast<const jbyte*>
            ( message.c_str() );

    jbyteArray bytes =
        (*jenv)->NewByteArray(byteCount);

    (*jenv)->SetByteArrayRegion(
        bytes, 0, byteCount, pNativeMessage);

    /*
     * find the Charset.forName method:
     *  javap -s java.nio.charset.Charset | egrep -A2 "forName"
    */
    jclass charsetClass =
        (*jenv)->FindClass("java/nio/charset/Charset");

    jmethodID forName = (*jenv)->GetStaticMethodID(
      charsetClass, "forName",
      "(Ljava/lang/String;)"
      "Ljava/nio/charset/Charset;");

    jstring utf8 = (*jenv)->NewStringUTF("UTF-8");
    jobject charset = (*jenv)->CallStaticObjectMethod(
        charsetClass, forName, utf8);

    // find a String constructor that takes a Charset:
    //   javap -s java.lang.String | egrep -A2 "String\(.*charset"
    jclass stringClass =
        (*jenv)->FindClass("java/lang/String");

    jmethodID ctor = (*jenv)->GetMethodID(
       stringClass, "<init>",
       "([BLjava/nio/charset/Charset;)V");

    return (*jenv)->NewObject(
        stringClass, ctor, bytes, charset);
}

bool IsArray( JNIEnv* jenv, jojbect pObj )
{
    jclass Class_class = 
        (*jenv)->FindClass("java/lang/Class");
    jmethodID Class_isArray_m =
        (*jenv)->GetMethodID(
            jenv, Class_class, "isArray", "()Z");
    jclass obj_class =
        (*jenv)->GetObjectClass(pObj);
    return (*jenv)->CallBooleanMethod(
            obj_class, Class_isArray_mid);
}

EnumTypeId GetObjType( JNIEnv* jenv, jobject pObj )
{
    jclass iClass =
        (*jenv)->FindClass("java/lang/String");
    if( ( *jenv )->IsInstanceOf( pObj, iClass ) )
        return typeString;

    iClass =
        (*jenv)->FindClass("java/lang/Integer");
    if( ( *jenv )->IsInstanceOf( pObj, iClass ) )
        return typeUInt32;

    iClass =
        (*jenv)->FindClass("org/rpcf/rpcbase/ObjPtr");
    if( ( *jenv )->IsInstanceOf( pObj, iClass ) )
        return typeObj;

    iClass =
        (*jenv)->FindClass("org/rpcf/rpcbase/CfgPtr");
    if( ( *jenv )->IsInstanceOf( pObj, iClass ) )
        return typeObj;

    iClass =
        (*jenv)->FindClass("java/lang/Boolean");
    if( ( *jenv )->IsInstanceOf( pObj, iClass ) )
        return typeByte;

    iClass =
        (*jenv)->FindClass("java/lang/Byte");
    if( ( *jenv )->IsInstanceOf( pObj, iClass ) )
    {
        if( IsArray( jenv, pObj ) )
            return typeByteArr;
        return typeByte;
    }
}

IService* CastToSvc(
    JNIEnv* jenv, ObjPtr* pObj )
{
    if( pObj == nullptr ||
        ( *pObj ).IsEmpty() )
        return nullptr;

    IService* pSvc = *pObj;
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
        return nullptr;

    return pObj;
}

CfgPtr* CastToCfg( ObjPtr* pObj )
{
    if( pObj == nullptr )
        return nullptr;

    CfgPtr* pCfg = new CfgPtr(
        ( IConfigDb* )pObj, true );
    return pCfg;
}


%}

%typemap(in,numinputs=0) JNIEnv *jenv "$1 = jenv;"

%extend ObjPtr
{
    jobject SerialToByteArray( JNIEnv *jenv )
    {
        BufPtr pBuf( true );
        ObjPtr& pObj = *$self;
        if( pObj.IsEmpty() )
        {
            SERI_HEADER_BASE ohb;
            ohb.dwClsid = clsid( Invalid );
            ohb.dwSize = 0;
            ohb.bVersion = 0;
            ohb.hton();
            pBuf->Resize(
                sizeof( SERI_HEADER_BASE ) );
            memcpy( pBuf->ptr(), &ohb,
                pBuf->size() );
        }
        else
        {
            pObj->Serialize( pBuf );
        }

        jbyteArray retBuf =
            ( *jenv )->NewByteArray(
                jenv, pBuf->size() );

        (*jenv)->SetByteArrayRegion(
            jenv, retBuf, 0, size, pBuf->ptr() );

        jobject jRet = NewJRet( jenv );
        AddElemToJRet( jenv, jRet, retBuf );
        return jRet;
    }

    jobject DeserialObjPtr(
        JNIEnv *jenv,
        jobject pByteArray,
        guint32 dwOffset )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        bool bRelease = false;
        jbyte* buf = nullptr;
        do{
            jsize len = ( *jenv )->GetArrayLength(
                jenv, pByteArray);

            buf=(*jenv)->GetPrimitiveArrayCritical(
                jenv, pByteArray, 0 );
            bRelease = true;

            char* pBuf = ( char* )buf + dwOffset;
            guint32 dwClsid = 0;
            memcpy( &dwClsid, pBuf, sizeof( dwClsid ) );
            dwClsid = ntohl( dwClsid );

            ObjPtr* ppObj =
                new ObjPtr( nullptr, false );

            guint32 dwSize = 0;
            memcpy( &dwSize, 
                pBuf + sizeof( guint32 ),
                sizeof( guint32 ) );

            dwSize = ntohl( dwSize ) +
                sizeof( SERI_HEADER_BASE );

            if( dwClsid != clsid( Invalid ) )
            {
                ret = ppObj->NewObj(
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

                ret = ( *ppObj )->Deserialize(
                    pBuf, dwSize );
                if( ret == -ENOTSUP )
                {
                    ret = -EINVAL;
                    break;
                }

                if( ERROR( ret ) )
                    break;

                jobject jop = NewObjPtr(
                    jenv, ppObj, true );
                if( jop == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }

                AddElemToJRet( jenv, jRet, jop );
                jobject jNewOff = NewInt(
                    jenv, dwSize + dwOffset );
                if( jNewOff == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                AddElemToJRet(
                    jenv, jRet, jNewOff );
            }

        }while( 0 );
        if( bRelease )
        {
            (*jenv)->ReleasePrimitiveArrayCritical(
                jenv, pByteArray, buf, 0 );
        }

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }
};

class CfgPtr
{
    public:

    CfgPtr( IConfigDb* pCfg,
        bool bAddRef );
    ~CfgPtr();
    bool IsEmpty() const;
};

%extend CfgPtr{
%newobject NewObj;
    gint32 NewObj()
    {
        return $self->NewObj(
            Clsid_CConfigDb, nullptr );
    }
    IConfigDb* GetPointer()
    {
        IConfigDb* pObj =
            (IConfigDb*)*$self;
        if( pObj == SIP_NULLPTR )
            return SIP_NULLPTR;
        return pObj;
    }
}

class CBuffer : public CObjBase
{
    public:
    CBuffer( guint32 dwSize = 0 );
    CBuffer( const char* pData, guint32 dwSize );
    char* ptr();
    guint32 size() const;
};

class BufPtr
{
    public:
    void Clear();

    BufPtr( CBuffer* pObj, bool );
    ~BufPtr();
    char* ptr();
    guint32 size() const;
    bool IsEmpty() const;

    EnumTypeId GetExDataType();
};

%extend BufPtr {

    void CopyToJava(
        JNIEnv *jenv, jbyteArray pArr )
    {
        BufPtr& pBuf = *$self;
        jsize len = ( *jenv )->GetArrayLength(
            jenv, pArr );
        jsize iSize = std::min(
            ( jsize )pBuf->size(), len );
        ( *jenv )->SetByteArrayRegion( jenv, pArr,
            iSize, 0, pBuf->ptr() );
    }

    gint32 CopyFromJava(
        JNIEnv *jenv, jbyteArray pArr )
    {
        gint32 ret = 0;
        jsize len = ( *jenv )->GetArrayLength(
            jenv, pArr );
        BufPtr& pBuf = *$self;
        ret = pBuf->Resize( len );
        if( ERROR( ret ) )
            return ret;
        ( *jenv )->GetByteArrayRegion( jenv, pArr,
            len, 0, pBuf->ptr() );
        return ret;
    }
}

class CParamList
{
    public:
    CParamList();
    CParamList( const CParamList& oParams );
    CParamList( CfgPtr& pCfg );
};

%extend CParamList {
    jobject GetStrProp( 
        JNIEnv* jenv, gint32 iProp )
    {
        
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            stdstr strVal;
            ret = oParams.GetStrProp(
                iProp, strVal );
            if( ERROR( ret ) )
                break;
            jobject jStr = GetJniString(
                jenv, strVal );

            AddElemToJRet( jenv, jRet, jStr );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 SetStrProp( gint32,
        const std::string& strVal );

    jobject GetByteProp(
        JNIEnv* jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            guint8 byVal;
            ret = oParams.GetByteProp(
                iProp, byVal );
            if( ERROR( ret ) )
                break;

            jobject jByte =
                NewByte( jenv, byVal );

            AddElemToJRet( jenv, jRet, jByte );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 SetByteProp(
        gint32 iProp, gint8 byVal )
    {
        return $self->SetIntProp(
            iProp, ( guint8 )byVal ); 
    }

    jobject GetShortProp(
        JNIEnv* jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            guint16 wVal;
            ret = oParams.GetShortProp(
                iProp, wVal );
            if( ERROR( ret ) )
                break;

            jobject jWord =
                NewShort( jenv, wVal );

            AddElemToJRet( jenv, jRet, jWord );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 SetShortProp(
        gint32 iProp, gint16 wVal )
    {
        return $self->SetShortProp(
            iProp, ( guint16 )wVal ); 
    }

    jobject GetIntProp(
        JNIEnv* jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            guint32 dwVal;
            ret = oParams.GetIntProp(
                iProp, dwVal );
            if( ERROR( ret ) )
                break;

            jobject jdword =
                NewInt( jenv, dwVal );

            AddElemToJRet( jenv, jRet, jdword );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 SetIntProp(
        gint32 iProp, gint32 dwVal )
    {
        return $self->SetIntProp(
            iProp, ( guint32 )dwVal ); 
    }

    gint32 GetQwordProp(
        JNIEnv* jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            guint64 qwVal;
            ret = oParams.GetQwordProp(
                iProp, qwVal );
            if( ERROR( ret ) )
                break;

            jobject jqword =
                NewInt( jenv, qwVal );

            AddElemToJRet( jenv, jRet, jqword );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }
    gint32 SetQwordProp(
        gint32 iProp, gint64 qwVal )
    {
        return $self->SetQwordProp(
            iProp, ( guint64 )qwVal ); 
    }

    jobject GetDoubleProp(
        JNIEnv* jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            double dblVal;
            ret = oParams.GetDoubleProp(
                iProp, dblVal );
            if( ERROR( ret ) )
                break;

            jobject jdbl =
                NewDouble( jenv, dblVal );

            AddElemToJRet( jenv, jRet, jdbl );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 SetDoubleProp( gint32, double fVal );

    jobject GetFloatProp(
        JNIEnv* jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            float fVal;
            ret = oParams.GetFloatProp(
                iProp, fVal );
            if( ERROR( ret ) )
                break;

            jobject jfv =
                NewFloat( jenv, fVal );

            AddElemToJRet( jenv, jRet, jfv );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 SetFloatProp( gint32, float fVal );

    jobject GetObjPtr( gint32,
        JNIEnv* jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            ObjPtr* ppObj = new ObjPtr();
            ret = oParams.GetObjPtr( iProp, &pObj );
            if( ERROR( ret ) )
                break;

            jobject jop =
                NewObjPtr( jenv, ppObj, true );
            if( jop == nullptr )
            {
                ret = -ENOMEM;
                break;
            }
            AddElemToJRet( jenv, jRet, jop );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 SetObjPtr(
        gint32 iProp, ObjPtr& pObj );

    jobject GetBoolProp(
        JNIEnv* jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            bool bVal;
            ret = oParams.GetFloatProp(
                iProp, bVal );
            if( ERROR( ret ) )
                break;

            jobject jbv =
                NewBoolean( jenv, bVal );

            AddElemToJRet( jenv, jRet, jbv );

        }while( 0 );

        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 SetBoolProp(
        gint32, bool bVal );

    jobject GetPropertyType(
        JNIEnv* jenv, gint32 iProp )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
        do{
            CParamList& oParams = *$self;
            CfgPtr& pCfg = oParams.m_pCfg;
            gint32 iType = 0;
            ret = pCfg->GetPropertyType(
                iProp, iType );
            if( ERROR( ret ) )
                break;
            jobject jval =
                NewInt( jenv, iType );
            if( jval == nullptr )
            {
                ret = -ENOMEM;
                break;
            }
            AddElemToJRet( jenv, jRet, jval );

        }while( 0 );
        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    gint32 PushByte( jbyte val )
    {
        guint8* pByte = &val;
        return $self->Push( *pByte )
    }

    gint32 PushObj( ObjPtr& val )
    {
        return $self->Push( val )
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
            jsize len = ( *jenv )->GetArrayLength(
                jenv, pba );
            if( len > 0 )
            {
                ret = pBuf->Resize();
                if( ERROR( ret ) )
                    break;
                
                ( *jenv )->GetByteArrayRegion(
                    jenv, pba, len, 0,
                    pBuf->ptr() );
            }

            ret = $self->Push( pBuf );

        }while( 0 );

        return ret;
    }

    jobject GetCfg( JNIEnv* jenv )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
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
                jenv, pCfg, true );

            if( jObj == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jRet, jObj );

        }while( 0 );
        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }

    jobject GetSize( JNIEnv* jenv )
    {
        gint32 ret = 0;
        jobject jRet = NewJRet( jenv );
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
            iSize = ( int32_t )dwSize;
            jobject jInt = NewInt( jenv, iSize );
            if( jInt == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            AddElemToJRet( jenv, jRet, jInt );

        }while( 0 );
        SetErrorJRet( jenv, jRet, ret );
        return jRet;
    }
};

%newobject CreateObject;
ObjPtr* CreateObject(
    EnumClsid iClsid, CfgPtr& pCfg );

%newobject CastToCfg;
CfgPtr* CastToCfg( ObjPtr* pObj );


%newobject CastToSvc;
IService* CastToSvc(
    JNIEnv* jenv, ObjPtr* pObj );

%nodefaultctor;
%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CRpcServices {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
      }
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
};

%typemap(javadestruct_derived, methodname="delete", methodmodifiers="public synchronized") CInterfaceProxy {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
      }
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
      if (swigCMemOwn) {
        swigCMemOwn = false;
      }
      swigCPtr = 0;
    }
    super.delete();
  }
class CInterfaceServer : public CRpcServices
{
};

%clearnodefaultctor;


