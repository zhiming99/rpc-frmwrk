// Define the SIP wrapper to the (theoretical) fib library.

%Module(name=rpcfrmwrk, language="C++")

%ModuleHeaderCode
#include <string>
#include "rpc.h"
#include "defines.h"
#include <glib.h>
#include <stdint.h>
#include <errno.h>
using namespace cpp;
#define _USE_LIBEV
#include "proxy.h"
#include "stmsvr.h"

extern std::string convertPythonUnicodeObjectToStdString(PyObject *pyUnicodeObj);
%End

%ModuleCode

void DumpObj(PyObject *o)
{
    PyObject_Print(o, stderr, 0);
    fprintf(stderr, "\n");
}

std::string convertPythonUnicodeObjectToStdString(PyObject *pyUnicodeObj)
{
  PyObject *utf8Str = PyUnicode_AsUTF8String(pyUnicodeObj);
  std::string ret(PyBytes_AsString(utf8Str));
  Py_DECREF(utf8Str);
  return ret;
}

using namespace cpp;

%End

typedef char gint8;
typedef unsigned char guint8;

typedef short gint64;
typedef int gint32;
typedef unsigned short guint16;
typedef unsigned int guint32;
typedef unsigned long long guint64;
// just a hint, the ultimate definition comes from
// 'defines.h'
typedef guint64 LONGWORD;

%MappedType std::string /TypeHint="str"/
{
%TypeHeaderCode
// Include the library interface to the type being mapped.
#include <string>
%End

%ConvertToTypeCode
  if (sipIsErr == NULL) {
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_Check(sipPy);
#else
    return PyString_Check(sipPy) || PyUnicode_Check(sipPy);
#endif
  }

  if (sipPy == Py_None) {
    *sipCppPtr = new std::string;
    return 1;
  }
#if PY_MAJOR_VERSION >= 3
  *sipCppPtr = new std::string(convertPythonUnicodeObjectToStdString(sipPy));
#else
  if (PyString_Check(sipPy)) {
    *sipCppPtr = new std::string(PyString_AS_STRING(sipPy));
  } else {
    *sipCppPtr = new std::string(convertPythonUnicodeObjectToStdString(sipPy));
  }
#endif
  return 1;
%End

%ConvertFromTypeCode
#if PY_MAJOR_VERSION >= 3
  PyObject *obj = PyUnicode_FromString(sipCpp->c_str());
#else
  PyObject *obj = PyString_FromString(sipCpp->c_str());
#endif
  return obj;
%End
};

namespace cpp
{
    enum EnumClsid
    {
        Clsid_Invalid,
        Clsid_CIoManager,
        Clsid_CConfigDb,
    };

    enum EnumEventId
    {
        eventPortStarted,
        eventPortStartFailed,
        eventRmtSvrOffline,
    };

    enum EnumPropId
    {
        propIfPtr,
        propParamCount,
        propIoMgr,
        propPyObj,
    };

    enum EnumIfState
    {
        stateStopped,
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

    enum EnumTypeId
    {
        typeNone,
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
    };

    gint32 CoInitialize( guint32 );
    %MethodCode
        Py_Initialize();
        sipRes = cpp::CoInitialize( a0 );
    %End
    gint32 CoUninitialize();

    class CObjBase
    {
        public:
        CObjBase();
        ~CObjBase();
        gint32 AddRef();
        gint32 Release();
        gint32 SetClassId( EnumClsid iClsid );
        gint32 GetClsid() const;
    };

    class CBuffer : public cpp::CObjBase
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

        BufPtr( cpp::CBuffer* pObj, bool );
        ~BufPtr();
        char* ptr();
        %MethodCode
            sipRes = ( *sipCpp )->ptr();
        %End
        guint32 size() const;
        %MethodCode
            sipRes = ( *sipCpp )->size();
        %End

        bool IsEmpty() const;
        %MethodCode
            sipRes = ( sipCpp->IsEmpty() ||
                ( *sipCpp )->empty() );
        %End

        EnumTypeId GetExDataType();
        %MethodCode
            sipRes = ( *sipCpp )->GetExDataType();
        %End

        gint32 CopyToPython( PyObject* pPyObj );
        %MethodCode
            gint32 ret = 0;
            Py_buffer *view = (Py_buffer *)
                malloc(sizeof(*view));
            ret = PyObject_GetBuffer(
                a0, view, PyBUF_WRITE);
            if( ret < 0 )
                sipRes = -ENOMEM;
            else
            {
                BufPtr& pBuf = *sipCpp;
                guint32 dwSize = std::min(
                    ( guint32 )view->len,
                    pBuf->size() );
                memcpy( view->buf,
                    pBuf->ptr(), dwSize );
                PyBuffer_Release( view );
                sipRes = 0;
            }
        %End

        gint32 CopyFromPython( PyObject* pPyObj );
        %MethodCode
            gint32 ret = 0;
            Py_buffer *view = (Py_buffer *)
                malloc(sizeof(*view));
            ret = PyObject_GetBuffer(
                a0, view, PyBUF_READ);
            if( ret < 0 )
                sipRes = -ENOMEM;
            else
            {
                BufPtr& pBuf = *sipCpp;
                pBuf->Resize( view->len );
                pBuf->Append( ( char* )view->buf,
                    ( guint32 )view->len);
                sipRes = 0;
                PyBuffer_Release( view );
            }
        %End
    };

    class IEventSink : public cpp::CObjBase /Abstract/
    {
        public:

        gint32 OnEvent(
            EnumEventId iEvent,
            LONGWORD dwParam1, 
            LONGWORD dwParam2,
            LONGWORD* pData /In/ );
        %MethodCode
            Py_BEGIN_ALLOW_THREADS;
            sipRes = sipCpp->OnEvent(
                a0, a1, a2, ( LONGWORD* )a3 );
            Py_END_ALLOW_THREADS;
        %End
    };

    class IService : public cpp::IEventSink /Abstract/
    {
        public:
        gint32 Start();
        %MethodCode
            Py_BEGIN_ALLOW_THREADS;
            sipRes = sipCpp->Start();
            Py_END_ALLOW_THREADS;
        %End
        gint32 Stop();
        %MethodCode
            Py_BEGIN_ALLOW_THREADS;
            sipRes = sipCpp->Stop();
            Py_END_ALLOW_THREADS;
        %End
    };

    class IConfigDb : public cpp::CObjBase /Abstract/
    {
        public:

        gint32 SetProperty( gint32 iProp,
            const BufPtr& pBuf );
        gint32 GetProperty( gint32 iProp,
            BufPtr& pBuf /Out/ ) const;
    };

    class ObjPtr
    {
        public:

        ObjPtr( cpp::CObjBase* pObj, bool bAddRef );
        ~ObjPtr();
        gint32 NewObj( EnumClsid iNewClsid,
            cpp::IConfigDb* pCfg );
        %MethodCode
        sipRes = sipCpp->NewObj( a0, a1 );
        %End

        bool IsEmpty() const;

        cpp::CObjBase* GetPointer();
        %MethodCode
        cpp::CObjBase* pObj =
            (cpp::CObjBase*)*sipCpp;
        if( pObj == SIP_NULLPTR )
            return SIP_NULLPTR;
        sipRes = pObj;
        %End
    };

    class CfgPtr
    {
        public:

        CfgPtr( cpp::IConfigDb* pCfg,
            bool bAddRef );
        ~CfgPtr();
        gint32 NewObj();
        %MethodCode
        sipRes = sipCpp->NewObj(
            cpp::Clsid_CConfigDb, nullptr );
        %End

        bool IsEmpty() const;
        cpp::IConfigDb* GetPointer();
        %MethodCode
        cpp::IConfigDb* pObj =
            (cpp::IConfigDb*)*sipCpp;
        if( pObj == SIP_NULLPTR )
            return SIP_NULLPTR;
        sipRes = pObj;
        %End
    };

    class CParamList
    {
        public:
        CParamList();
        CParamList( const cpp::CParamList& pCfg );
        CParamList( cpp::IConfigDb* pCfg );

        gint32 GetStrProp( gint32,
            std::string& strVal /Out/ );
        gint32 SetStrProp( gint32,
            const std::string& strVal );

        gint32 GetByteProp( gint32,
            guint8& dwVal );
        gint32 SetByteProp( gint32,
            guint8 dwVal );

        gint32 GetShortProp( gint32,
            guint16& dwVal /Out/ );
        gint32 SetShortProp( gint32,
            guint16 dwVal );

        gint32 GetIntProp( gint32,
            guint32& dwVal /Out/ );
        gint32 SetIntProp( gint32,
            guint32 dwVal );

        gint32 GetQwordProp( gint32,
            guint64& dwVal /Out/ );
        gint32 SetQwordProp( gint32,
            guint64 dwVal );

        gint32 GetPointer( gint32,
            cpp::CObjBase*& pObj /Out/ );
        gint32 SetPointer( gint32,
            cpp::CObjBase* pObj );

        gint32 GetObjPtr( gint32,
            cpp::ObjPtr& pObj /Out/ );
        gint32 SetObjPtr( gint32,
            cpp::ObjPtr& pObj );

        gint32 GetBoolProp(
            gint32, bool& bVal /Out/ );
        gint32 SetBoolProp(
            gint32, bool bVal );

        gint32 GetPropertyType(
            EnumPropId iProp,
            gint32& iType /Out/ ); 
        %MethodCode
            CfgPtr pCfg = sipCpp->GetCfg();
            sipRes = pCfg->GetPropertyType(
                a0, a1 );
        %End

        gint32 Push( PyObject* pObj /In/ );
        %MethodCode
            sipRes = sipCpp->Push( *a0 );
        %End

        gint32 PushObj( cpp::ObjPtr& val /In/ );
        %MethodCode
            sipRes = sipCpp->Push( *a0 );
        %End

        gint32 PushStr( std::string& val /In/ );
        %MethodCode
            sipRes = sipCpp->Push( *a0 );
        %End

        gint32 PushInt( guint32& val /In/ );
        %MethodCode
            sipRes = sipCpp->Push( a0 );
        %End

        gint32 PushBool( bool& val /In/ );
        %MethodCode
            sipRes = sipCpp->Push( a0 );
        %End

        gint32 PushQword( guint64& val /In/ );
        %MethodCode
            sipRes = sipCpp->Push( a0 );
        %End

        gint32 PushBufPtr( cpp::BufPtr& val /In/ );
        %MethodCode
            sipRes = sipCpp->Push( *a0 );
        %End

        cpp::CfgPtr* GetCfg();
        %MethodCode
        cpp::CfgPtr* pCfg;
        pCfg = new cpp::CfgPtr(
            ( cpp::CfgPtr& )sipCpp->GetCfg() );
        if( pCfg == nullptr )
            sipRes = SIP_NULLPTR;
        sipRes = pCfg;
        %End

        cpp::ObjPtr* GetCfgAsObj();
        %MethodCode
        cpp::IConfigDb* pCfg = 
            sipCpp->GetCfg();
        if( pCfg == nullptr )
            sipRes = SIP_NULLPTR;
        cpp::ObjPtr* pObj =
            new cpp::ObjPtr( pCfg, true );
        sipRes = pObj;
        %End

        gint32 GetSize( guint32& dwSize /Out/ );
        %MethodCode
            sipRes = sipCpp->GetSize( a0 );
        %End
    };

    cpp::ObjPtr* CreateObject(
        cpp::EnumClsid iClsid,
        cpp::IConfigDb* pCfg );
    %MethodCode
    cpp::ObjPtr* pObj =
        new cpp::ObjPtr( nullptr, false );
    gint32 ret = pObj->NewObj( a0, a1 ); 
    if( ret < 0 )
        return SIP_NULLPTR;
    sipRes = pObj;
    %End

    cpp::IConfigDb* CastToCfg(
        cpp::ObjPtr* pObj );
    %MethodCode
    if( a0 == nullptr )
        sipRes = SIP_NULLPTR;
    else
    {
        cpp::IConfigDb* pCfg = *a0;
        sipRes = pCfg;
    }
    %End

    cpp::IService* CastToSvc(
        cpp::ObjPtr* pObj );
    %MethodCode
    if( a0 == nullptr )
        sipRes = SIP_NULLPTR;
    else
    {
        cpp::IService* pSvc = *a0;
        sipRes = pSvc;
    }
    %End

    cpp::BufPtr* CastToBuf(
        cpp::ObjPtr* pObj );
    %MethodCode
    cpp::BufPtr *pBuf = new cpp::BufPtr(
        nullptr, false );
    if( a0 == nullptr )
        sipRes = SIP_NULLPTR;
    else
    {
        *pBuf = *a0;
        if( pBuf->IsEmpty() )
            sipRes = SIP_NULLPTR;
        else
            sipRes = pBuf;
    }
    %End

    class CRpcServices : public cpp::IService /Abstract/
    {
        public:
        static gint32 LoadObjDesc(
            const std::string& strFile,
            const std::string& strObjName,
            bool bServer,
            cpp::CfgPtr& pCfg );
        %MethodCode
        sipRes = CRpcServices::LoadObjDesc(
            *a0, *a1, a2, *a3);
        %End

    };

    class CInterfaceProxy : public cpp::CRpcServices /Abstract/
    {
        public:
        gint32 CancelRequest( guint64 qwTaskId );
        %MethodCode
            Py_BEGIN_ALLOW_THREADS;
            sipRes = sipCpp->CancelRequest( a0 );
            Py_END_ALLOW_THREADS;
        %End
        EnumIfState GetState() const;
    };

};

%Include proxy.sip

