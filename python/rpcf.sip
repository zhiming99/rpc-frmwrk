/*
 * =====================================================================================
 *
 *       Filename:  rpcf.sip
 *
 *    Description:  a sip file as the wrapper of important C++ helper classes
 *                  for Python
 *
 *        Version:  1.0
 *        Created:  11/04/2020 01:10:51 AM
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

%Module(name=rpcbase, language="C++")

%ModuleHeaderCode
#pragma once
#include <string>
#include "rpc.h"
#include "defines.h"
#include <stdint.h>
#include <errno.h>
using namespace cpp;
#include "proxy.h"
#include "seribase.h"
#include <unistd.h>
#include "dlfcn.h"

extern std::string convertPythonUnicodeObjectToStdString(PyObject *pyUnicodeObj);
extern "C" gint32 DllLoadFactory( FactoryPtr& pFactory );
extern char g_szKeyPass[ SSL_PASS_MAX + 1 ];
extern ObjPtr g_pRouter;
extern ObjPtr g_pIoMgr;
extern std::set< guint32 > g_setMsgIds;
gint32 CheckKeyPass();
gint32 OpenThisLib();
extern void* g_pLibHandle;
%End

%ModuleCode

char g_szKeyPass[ SSL_PASS_MAX + 1 ] = {0};
ObjPtr g_pRouter;
ObjPtr g_pIoMgr;
std::set< guint32 > g_setMsgIds;

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

void* g_pLibHandle = nullptr;
gint32 OpenThisLib()
{
    gint32 ret = 0;
    do{
        std::string strResult;
        const char* szLib = "rpcbase.cpython";
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

%End

typedef char gint8;
typedef unsigned char guint8;

typedef short gint16;
typedef int gint32;
typedef unsigned short guint16;
typedef unsigned int guint32;
typedef unsigned long long guint64;
// just a hint, the ultimate definition comes from
// 'defines.h'
typedef guint64 LONGWORD;
typedef LONGWORD HANDLE;

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
    enum // EnumClsid
    {
        Clsid_Invalid,
        Clsid_CIoManager,
        Clsid_CConfigDb2,
    };

    enum // EnumEventId
    {
        eventPortStarted,
        eventPortStartFailed,
        eventRmtSvrOffline,
    };

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

    enum // EnumIfState
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

    enum // EnumTypeId
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

    enum // EnumSeriProto
    {
        seriNone = 0,
        seriRidl = 1,
        seriPython = 2,
        seriJava = 3,
        seriInvalid = 4
    };

    gint32 CoInitialize( guint32 );
    %MethodCode
        Py_Initialize();
        sipRes = cpp::CoInitialize( a0 );
    %End

    gint32 CoUninitialize();
    %MethodCode
        cpp::CoUninitialize();
        DebugPrintEx( logErr, 0,
            "#Leaked objects is %d",
            CObjBase::GetActCount() );
    %End

    class CObjBase
    {
        public:
        CObjBase();
        ~CObjBase();
        gint32 AddRef();
        gint32 Release();
        gint32 SetClassId( guint32 iClsid );
        %MethodCode
        sipRes = sipCpp->SetClassId( (cpp::EnumClsid)a0 );
        %End
        gint32 GetClsid() const;
    };

    class ObjPtr
    {
        public:

        ObjPtr();
        ObjPtr( cpp::CObjBase* pObj, bool bAddRef );
        ~ObjPtr();
        gint32 NewObj( guint32 iNewClsid,
            cpp::IConfigDb* pCfg );
        %MethodCode
        sipRes = sipCpp->NewObj( (cpp::EnumClsid)a0, a1 );
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

        PyObject* SerialToByteArray();
        %MethodCode
        do{
            gint32 ret = 0;
            ObjPtr& pObj = *sipCpp;
            BufPtr pBuf( true );
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
                ret = pObj->Serialize( *pBuf );
                if( ERROR( ret ) )
                {
                    sipRes = Py_None;
                    break;
                }
            }
            PyObject* pView =
                PyMemoryView_FromMemory(
                pBuf->ptr(), pBuf->size(),
                PyBUF_READ );

            if( pView == nullptr )
                sipRes = Py_None;
            else
            {
                sipRes =
                PyByteArray_FromObject( pView );
                Py_DECREF( pView );
            }

        }while( 0 );
        %End

        PyObject* DeserialObjPtr(
            PyObject* pByteArray,
            guint32 dwOffset );
        %MethodCode
        gint32 ret = 0;
        PyObject* pPyResp = PyList_New( 3 );
        sipRes = pPyResp;
        Py_buffer *view =
            (Py_buffer *)calloc(1,sizeof(*view));
        do{
            ret = PyObject_GetBuffer(
                a0, view, PyBUF_READ);
            if( ret < 0 )
            {
                break;
            }
            else
            {
                char* pBuf = ( char* )view->buf + a1;
                guint32 dwClsid = 0;
                memcpy( &dwClsid, pBuf, sizeof( dwClsid ) );
                dwClsid = ntohl( dwClsid );

                ObjPtr* ppObj =
                    new cpp::ObjPtr( nullptr, false );

                guint32 dwSize = 0;
                memcpy( &dwSize, 
                    pBuf + sizeof( guint32 ),
                    sizeof( guint32 ) );

                dwSize = ntohl( dwSize ) +
                    sizeof( SERI_HEADER_BASE );

                if( dwClsid != clsid( Invalid ) )
                {
                    ret = ppObj->NewObj(
                        ( cpp::EnumClsid )dwClsid );
                    if( ERROR( ret ) )
                        break;

                    if( ( guint32 )( view->len - a1 ) < dwSize )
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
                }
                PyObject* pRetObj = sipConvertFromNewType(
                    ppObj, sipType_cpp_ObjPtr, Py_None );
                if( pRetObj == nullptr )
                {
                    ret = -ENOMEM;
                    break;
                }

                PyList_SetItem( pPyResp, 1, pRetObj );
                PyObject* pRetOffset =
                    PyLong_FromLong( dwSize + a1 );
                PyList_SetItem( pPyResp, 2, pRetOffset );
            }

        }while( 0 );

        PyObject* pPyRet = PyLong_FromLong( ret );
        PyList_SetItem( pPyResp, 0, pPyRet );

        if( view != nullptr )
            PyBuffer_Release( view );
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
            cpp::Clsid_CConfigDb2, nullptr );
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

        guint32 GetExDataType();
        %MethodCode
            sipRes = ( *sipCpp )->GetExDataType();
        %End

        PyObject* TransToBytes();
        %MethodCode
            BufPtr& pBuf = *sipCpp;
            PyObject* pView = PyMemoryView_FromMemory(
                pBuf->ptr(), pBuf->size(), PyBUF_READ );
            if( pView == nullptr )
                sipRes = Py_None;
            else
            {
                sipRes = PyByteArray_FromObject( pView );
                Py_DECREF( pView );
            }
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
                memcpy( pBuf->ptr(),
                    ( char* )view->buf,
                    ( guint32 )view->len);
                sipRes = 0;
                PyBuffer_Release( view );
            }
        %End

        gint32 AppendBytes( PyObject* pPyObj );
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
                pBuf->Append(
                    ( char* )view->buf,
                    ( guint32 )view->len);
                sipRes = 0;
                PyBuffer_Release( view );
            }
        %End

        gint32 AppendObjPtr( cpp::ObjPtr* );
        %MethodCode
            CObjBase* pObj = *a0;
            if( pObj == nullptr )
                sipRes = -EFAULT;
            else
            {
                BufPtr pObjBuf( true );
                sipRes =
                    pObj->Serialize( *pObjBuf );
                if( SUCCEEDED( sipRes ) )
                {
                    BufPtr& pBuf = *sipCpp;
                    APPEND( pBuf, pObjBuf->ptr(),
                        pObjBuf->size() );
                    sipRes = 0;
                }
            }
        %End

    };

    class IEventSink : public cpp::CObjBase /Abstract/
    {
        public:

        gint32 OnEvent(
            guint32 iEvent,
            LONGWORD dwParam1, 
            LONGWORD dwParam2,
            LONGWORD* pData /In/ );
        %MethodCode
            Py_BEGIN_ALLOW_THREADS;
            sipRes = sipCpp->OnEvent(
                ( EnumEventId )a0, a1, a2, ( LONGWORD* )a3 );
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
        %MethodCode
            a1 = 0;
            sipRes = sipCpp->GetByteProp(
                a0, a1 );
        %End
        gint32 SetByteProp( gint32,
            guint8 dwVal );

        gint32 GetShortProp( gint32,
            guint16& dwVal /Out/ );
        %MethodCode
            a1 = 0;
            sipRes = sipCpp->GetShortProp(
                a0, a1 );
        %End
        gint32 SetShortProp( gint32,
            guint16 dwVal );

        gint32 GetIntProp( gint32,
            guint32& dwVal /Out/ );
        %MethodCode
            a1 = 0;
            sipRes = sipCpp->GetIntProp(
                a0, a1 );
        %End
        gint32 SetIntProp( gint32,
            guint32 dwVal );

        gint32 GetQwordProp( gint32,
            guint64& dwVal /Out/ );
        %MethodCode
            a1 = 0;
            sipRes = sipCpp->GetQwordProp(
                a0, a1 );
        %End
        gint32 SetQwordProp( gint32,
            guint64 dwVal );

        gint32 GetDoubleProp( gint32,
            double& fVal /Out/ );
        %MethodCode
            a1 = .0;
            sipRes = sipCpp->GetDoubleProp(
                a0, a1 );
        %End
        gint32 SetDoubleProp( gint32,
            double fVal );

        gint32 GetFloatProp( gint32,
            float& fVal /Out/ );
        %MethodCode
            a1 = .0;
            sipRes = sipCpp->GetFloatProp(
                a0, a1 );
        %End
        gint32 SetFloatProp( gint32,
            float fVal );

        gint32 GetPointer( gint32,
            cpp::CObjBase*& pObj /Out/ );
        %MethodCode
            a1 = nullptr;
            sipRes = sipCpp->GetPointer(
                a0, a1 );
        %End
        gint32 SetPointer( gint32,
            cpp::CObjBase* pObj );

        gint32 GetObjPtr( gint32,
            cpp::ObjPtr& pObj /Out/ );
        gint32 SetObjPtr( gint32,
            cpp::ObjPtr& pObj );

        gint32 GetBufPtr( gint32,
            cpp::BufPtr& pObj /Out/ );
        gint32 SetBufPtr( gint32,
            cpp::BufPtr& pObj );

        gint32 GetBoolProp(
            gint32, bool& bVal /Out/ );
        %MethodCode
            a1 = false;
            sipRes = sipCpp->GetBoolProp(
                a0, a1 );
        %End
        gint32 SetBoolProp(
            gint32, bool bVal );

        gint32 GetPropertyType(
            gint32 iProp,
            gint32& iType /Out/ ); 
        %MethodCode
            CfgPtr pCfg = sipCpp->GetCfg();
            sipRes = pCfg->GetPropertyType(
                a0, a1 );
        %End

        /*gint32 Push( PyObject* pObj /In/ );
        %MethodCode
            sipRes = sipCpp->Push( *a0 );
        %End */

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

        cpp::CfgPtr* GetCfg()/TransferBack/;
        %MethodCode
        cpp::CfgPtr* pCfg;
        pCfg = new cpp::CfgPtr(
            ( cpp::CfgPtr& )sipCpp->GetCfg() );
        if( pCfg == nullptr )
            sipRes = SIP_NULLPTR;
        sipRes = pCfg;
        %End

        cpp::ObjPtr* GetCfgAsObj()/TransferBack/;
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
            a0 = 0;
            sipRes = sipCpp->GetSize( a0 );
        %End
    };

    cpp::ObjPtr* StartIoMgr(
        cpp::IConfigDb* pCfg )/TransferBack/;
    %MethodCode
    gint32 ret = 0;
    cpp::ObjPtr* pObj =
        new cpp::ObjPtr( nullptr, false );
    CCfgOpener oCfg( a0 );
    sipRes = SIP_NULLPTR;
    do{
        stdstr strModName;
        ret = oCfg.GetStrProp( 0, strModName );
        if( ERROR( ret ) )
            break;

        FactoryPtr p;
        ret = DllLoadFactory( p );
        if( ERROR( ret ) )
            return SIP_NULLPTR;
        ret = CoAddClassFactory( p );
        if( ERROR( ret ) )
            break;

        if( !oCfg.exist( 101 ) )
        {
            ret = pObj->NewObj(
                clsid( CIoManager ), a0 ); 
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
                clsid( CIoManager ), a0 );
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
            stdstr strDescPath = "./router.json";
            if( bAuth )
            {
                pMgr->SetCmdLineOpt(
                    propHasAuth, bAuth );
                strDescPath = "./rtauth.json";
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
    if( sipRes == SIP_NULLPTR )
    {
        delete pObj;
        if( g_pLibHandle != nullptr )
        {
            dlclose( g_pLibHandle );
            g_pLibHandle = nullptr;
        }
    }
    %End

    gint32 StopIoMgr( ObjPtr* pObj );
    %MethodCode
    sipRes = 0;
    do{
        if( !g_pRouter.IsEmpty() )
        {
            IService* pRt = g_pRouter;
            pRt->Stop();
            g_pRouter.Clear();
        }
        g_pIoMgr.Clear();
        if( a0 == nullptr )
            break;
        cpp::IService* pMgr = *a0;
        if( pMgr == nullptr )
            break;
        sipRes = pMgr->Stop();
    }while( 0 );
    if( g_pLibHandle != nullptr )
    {
        dlclose( g_pLibHandle );
        g_pLibHandle = nullptr;
    }
    %End

    cpp::ObjPtr* CreateObject(
        guint32 iClsid,
        cpp::IConfigDb* pCfg )/TransferBack/;
    %MethodCode
    cpp::ObjPtr* pObj =
        new cpp::ObjPtr( nullptr, false );
    gint32 ret = pObj->NewObj( (cpp::EnumClsid)a0, a1 ); 
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
        cpp::ObjPtr* pObj )/TransferBack/;
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

        gint32 Pause();
        %MethodCode
            Py_BEGIN_ALLOW_THREADS;
            sipRes = sipCpp->Pause_Proxy();
            Py_END_ALLOW_THREADS;
        %End

        gint32 Resume();
        %MethodCode
            Py_BEGIN_ALLOW_THREADS;
            sipRes = sipCpp->Resume_Proxy();
            Py_END_ALLOW_THREADS;
        %End

        guint32 GetState() const;
    };

    class CInterfaceServer : public cpp::CRpcServices /Abstract/
    {
        public:
        guint32 GetState() const;
    };

};

%Include proxy.sip

