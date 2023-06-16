/*
 * =====================================================================================
 *
 *       Filename:  genfuse2.cpp
 *
 *    Description:  implementation of classes for code generator of fuse over ROS
 *
 *        Version:  1.0
 *        Created:  09/28/2022 07:00:40 PM
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


#include "rpc.h"
using namespace rpcf;
#include "astnode.h"
#include "gencpp.h"
#include "gencpp2.h"
#include "sha1.h"
#include "proxy.h"
#include "genfuse.h"
#include "frmwrk.h"
#include "jsondef.h"
#include <json/json.h>
#include "genfuse2.h"

extern bool g_bMklib;
extern stdstr g_strCmdLine;
extern bool g_bBuiltinRt;

extern gint32 EmitOnPreStart( 
    CWriterBase* m_pWriter,
    CServiceDecl* pSvcNode,
    bool bProxy );

gint32 EmitGetSvrCtxId( 
    CWriterBase* m_pWriter )
{
    Wa( "gint32 GetSvrCtxId( " );
    Wa( "    IEventSink* pCb, stdstr& strCtxId )" );
    BLOCK_OPEN;
    Wa( "if( pCb == nullptr )" );
    Wa( "    return -EINVAL;" );
    Wa( "gint32 ret = 0;" );
    Wa( "guint64 qwTaskId = pCb->GetObjId();" );
    Wa( "HANDLE hstm = INVALID_HANDLE;" );
    Wa( "CCfgOpenerObj oCfg( pCb );" );
    Wa( "ret = oCfg.GetIntPtr( " );
    Wa( "    propStmHandle, ( guint32*& )hstm );" );
    Wa( "if( ERROR( ret ) )" );
    Wa( "    return ret;" );
    Wa( "strCtxId = std::to_string( qwTaskId );" );
    Wa( "strCtxId += \":\";" );
    Wa( "strCtxId += std::to_string(" );
    Wa( "    ( intptr_t )hstm );" );
    CCOUT << "return STATUS_SUCCESS;";
    BLOCK_CLOSE;
    NEW_LINE;
    return STATUS_SUCCESS;
}

gint32 EmitBuildJsonReq2( 
    CWriterBase* m_pWriter )
{
    EmitGetSvrCtxId( m_pWriter );
    Wa( "gint32 BuildJsonReq2(" );
    Wa( "    IConfigDb* pReqCtx_," );
    Wa( "    const Json::Value& oJsParams," );
    Wa( "    const std::string& strMethod," );
    Wa( "    const std::string& strIfName, " );
    Wa( "    gint32 iRet," );
    Wa( "    std::string& strReq," );
    Wa( "    bool bProxy," );
    Wa( "    bool bResp )" );
    BLOCK_OPEN;
    Wa( "gint32 ret = 0;" );
    Wa( "if( pReqCtx_ == nullptr )" );
    Wa( "    return -EINVAL;" );
    CCOUT << "do";
    BLOCK_OPEN;
    Wa( "CCfgOpener oReqCtx( pReqCtx_ );" );
    Wa( "IEventSink* pCallback = nullptr;" );
    Wa( "guint64 qwReqId = 0;" );
    Wa( "if( bProxy )" );
    BLOCK_OPEN;
    Wa( "ret = oReqCtx.GetQwordProp(" );
    Wa( "    1, qwReqId );" );
    CCOUT << "if( ERROR( ret ) ) break;";
    BLOCK_CLOSE;
    NEW_LINE;
    CCOUT << "else";
    NEW_LINE;
    BLOCK_OPEN;
    Wa( "ret = oReqCtx.GetPointer( " );
    Wa( "    propEventSink, pCallback );" );
    Wa( "if( ERROR( ret ) ) break;" );
    CCOUT << "qwReqId = pCallback->GetObjId();";
    BLOCK_CLOSE;
    NEW_LINE;
    Wa( "Json::Value oJsReq( Json::objectValue );" );
    Wa( "if( SUCCEEDED( iRet ) &&" );
    Wa( "    !oJsParams.empty() &&" );
    Wa( "    oJsParams.isObject() )" );
    Wa( "    oJsReq[ JSON_ATTR_PARAMS ] = oJsParams;" );
    Wa( "oJsReq[ JSON_ATTR_METHOD ] = strMethod;" );
    Wa( "oJsReq[ JSON_ATTR_IFNAME1 ] = strIfName;" );
    Wa( "if( bProxy )" );
    BLOCK_OPEN;
    Wa( "if( bResp )" );
    Wa( "{" );
    Wa( "    oJsReq[ JSON_ATTR_MSGTYPE ] = \"resp\";" );
    Wa( "    oJsReq[ JSON_ATTR_RETCODE ] = iRet;");
    Wa( "}" );
    Wa( "else" );
    Wa( "    oJsReq[ JSON_ATTR_MSGTYPE ] = \"evt\";" );
    BLOCK_CLOSE;
    NEW_LINE;
    Wa( "else" );
    Wa( "    oJsReq[ JSON_ATTR_MSGTYPE ] = \"req\";" );

    Wa( "if( bProxy )" );
    BLOCK_OPEN;
    Wa( "oJsReq[ JSON_ATTR_REQCTXID ] =" );
    CCOUT << "    ( Json::UInt64& )qwReqId;";
    BLOCK_CLOSE;
    NEW_LINE;
    Wa( "else" );
    BLOCK_OPEN;
    Wa( "stdstr strCtxId;" );
    Wa( "ret = GetSvrCtxId( pCallback, strCtxId );" );
    Wa( "if( ERROR( ret ) ) break;" );
    Wa( "oJsReq[ JSON_ATTR_REQCTXID ] = strCtxId;" );
    BLOCK_CLOSE;
    NEW_LINE;
    Wa( "Json::StreamWriterBuilder oBuilder;" );
    Wa( "oBuilder[\"commentStyle\"] = \"None\";" );
    Wa( "oBuilder[\"indentation\"] = \"\";" );
    CCOUT <<  "strReq = Json::writeString( "
        << "oBuilder, oJsReq );";
    BLOCK_CLOSE;
    Wa( "while( 0 );" );
    CCOUT << "return ret;";
    BLOCK_CLOSE;
    NEW_LINE;
    return 0;
}

CDeclInterfProxyFuse2::CDeclInterfProxyFuse2(
    CCppWriter* pWriter, ObjPtr& pNode ) :
        super( pWriter, pNode )
{
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "interface declaration" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CDeclInterfProxyFuse2::Output()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "class " ); 
        std::string strClass = "I";
        strClass += strName + "_PImpl";
        CCOUT << strClass;
        std::string strBase;
        strBase = "CFastRpcSkelProxyBase";
        INDENT_UP;
        NEW_LINE;
        CCOUT<< ": public virtual " << strBase;

        INDENT_DOWN;
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "public:" );
        CCOUT << "typedef "
            << strBase << " super;";
        NEW_LINE;

        CCOUT << strClass
            << "( const IConfigDb* pCfg ) :";

        INDENT_UP;
        NEW_LINE;

        CCOUT << "super( pCfg )";
        NEW_LINE;

        CCOUT << "{}";
        INDENT_DOWN;
        NEW_LINE;
        Wa( "gint32 InitUserFuncs() override;" );
        NEW_LINE;
        Wa( "gint32 OnReqComplete( IEventSink*," );
        Wa( "    IConfigDb* pReqCtx, Json::Value& val_," );
        Wa( "    const std::string&, const std::string&," );
        Wa( "    gint32, bool );" );
        NEW_LINES( 2 );
        Wa( "// Dispatch the Json request messages" );
        Wa( "gint32 DispatchIfReq(" );
        Wa( "    IConfigDb* pContext," );
        Wa( "    const Json::Value& oReq," );
        Wa( "    Json::Value& oResp );" );
        NEW_LINE;

        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 i = 0;
        pmds->GetCount();
        guint32 dwCount = pmds->GetCount();
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            CMethodDecl* pmd = pObj;
            if( pmd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            if( pmd->IsEvent() )
            {
                ret = OutputEvent( pmd );
                if( ERROR( ret ) )
                    break;
            }
            // FUSE support does not have synchronous
            // method calls
            ret = OutputAsync( pmd );
            if( i + 1 < dwCount )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINE;

        AP( "class " );
        strClass = "I";
        strClass += strName + "_CliApi";
        strBase = "virtual CAggInterfaceProxy";

        CCOUT << strClass << " :";
        INDENT_UP;
        NEW_LINE;
        CCOUT<< "public " << strBase;
        INDENT_DOWNL;
        BLOCK_OPEN;
        strBase = "CAggInterfaceProxy";
        Wa( "public:" );
        CCOUT << "typedef "
            << strBase << " super;";
        NEW_LINE;
        CCOUT << strClass
            << "( const IConfigDb* pCfg ) :";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "super( pCfg )";
        NEW_LINE;
        CCOUT << "{}";
        NEW_LINE;

        INDENT_DOWNL;
        Wa( "virtual gint32 OnReqComplete( IEventSink*," );
        Wa( "    IConfigDb* pReqCtx, Json::Value& val_," );
        Wa( "    const std::string&, const std::string&, gint32, bool ) = 0;" );
        NEW_LINE;
        Wa( "const EnumClsid GetIid() const override" );
        CCOUT << "{ return iid( " << strName << " ); }";
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}


gint32 CDeclInterfSvrFuse2::Output()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "class " ); 
        std::string strClass = "I";
        strClass += strName + "_SImpl";
        CCOUT << strClass;
        std::string strBase = "CFastRpcSkelSvrBase";
        INDENT_UP;
        NEW_LINE;

        CCOUT<< ": public virtual " << strBase;

        INDENT_DOWN;
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "public:" );
        CCOUT << "typedef "
            << strBase << " super;";
        NEW_LINE;
        CCOUT << strClass
            << "( const IConfigDb* pCfg ) :";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "super( pCfg )";
        NEW_LINE;
        CCOUT << "{}";

        INDENT_DOWN;
        NEW_LINE;
        Wa( "gint32 InitUserFuncs();" );
        NEW_LINES( 2 );
        Wa( "// Dispatch the Json response/event messages" );
        Wa( "gint32 DispatchIfMsg(" );
        Wa( "    const Json::Value& oMsg );" );
        NEW_LINE;

        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 i = 0;
        guint32 dwCount = pmds->GetCount();
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            CMethodDecl* pmd = pObj;
            if( pmd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            if( pmd->IsEvent() )
            {
                ret = OutputEvent( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else
            {
                ret = OutputAsync( pmd );
                if( ERROR( ret ) )
                    break;
            }
            if( i + 1 < dwCount )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINE;
        AP( "class " );
        strClass = "I";
        strClass += strName + "_SvrApi";
        strBase = "virtual CAggInterfaceServer";

        CCOUT << strClass << " :";
        INDENT_UP;
        NEW_LINE;
        CCOUT<< "public " << strBase;
        INDENT_DOWNL;
        BLOCK_OPEN;
        strBase = "CAggInterfaceServer";
        Wa( "public:" );
        CCOUT << "typedef "
            << strBase << " super;";
        NEW_LINE;
        CCOUT << strClass
            << "( const IConfigDb* pCfg ) :";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "super( pCfg )";
        NEW_LINE;
        CCOUT << "{}";
        INDENT_DOWNL;

        NEW_LINE;
        Wa( "const EnumClsid GetIid() const override" );
        CCOUT << "{ return iid( " << strName << " ); }";
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclInterfSvrFuse2::OutputAsync(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    gint32 ret = 0;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        if( !pmd->IsSerialize() )
        {
            ret = ERROR_NOT_IMPL;
            break;
        }

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount = dwInCount + dwOutCount;
        if( dwInCount == 0 )
        {
            Wa( "//RPC Async Req Handler wrapper" );
            CCOUT << "gint32 "
                << strName << "Wrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback );";
            INDENT_DOWNL;
            NEW_LINE;

            CCOUT << "gint32 "
                << strName << "CancelWrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback,";
            NEW_LINE;
            Wa( "gint32 iRet," );
            CCOUT << "IConfigDb* pReqCtx_ );";
            INDENT_DOWNL;
            NEW_LINE;
        }
        else
        {
            Wa( "//RPC Async Req Handler wrapper" );
            CCOUT << "gint32 "
                << strName << "Wrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback"
                << ", BufPtr& pBuf_ );";
            INDENT_DOWNL;
            NEW_LINE;

            CCOUT << "gint32 "
                << strName << "CancelWrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback,";
            NEW_LINE;
            Wa( "gint32 iRet," );
            CCOUT << "IConfigDb* pReqCtx_, BufPtr& pBuf_ );";
            INDENT_DOWNL;
            NEW_LINE;
        }

        Wa( "//RPC Async Req Callback" );
        Wa( "//Call this method when you have" );
        Wa( "//finished the async operation" );
        Wa( "//with all the return value set" );
        Wa( "//or an error code" );

        CCOUT << "gint32 " << strName
            << "Complete" << "( ";

        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_, "
            << "gint32 iRet";

        if( dwOutCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            CCOUT << "const Json::Value& oResp";
        }
        CCOUT << " );";
        INDENT_DOWNL;

        std::string strDecl =
            std::string( "gint32 " ) +
            strName + "(";

        strDecl += "IConfigDb* pReqCtx_";
        if( dwInCount > 0 )
            strDecl += ",";

        pmd->SetAbstDecl( strDecl, false );

        guint32 dwFlags = 0;
        if( dwInCount > 0 )
            dwFlags = MF_IN_AS_IN | MF_IN_ARG;

        pmd->SetAbstFlag( dwFlags, false );

    }while( 0 );

    return ret;
}

gint32 CDeclServiceImplFuse2::Output()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();

        std::vector< ABSTE > vecPMethods;
        ret = FindAbstMethod( vecPMethods, true );
        if( ERROR( ret ) )
            break;

        std::vector< ABSTE > vecSMethods;
        ret = FindAbstMethod( vecSMethods, false );
        if( ERROR( ret ) )
            break;
        
        if( vecSMethods.empty() &&
            vecPMethods.empty() )
            break;

        EMIT_DISCLAIMER;
        CCOUT << "// " << g_strCmdLine;
        NEW_LINE;
        Wa( "// Your task is to implement the following classes" );
        Wa( "// to get your rpc server work" );
        Wa( "#pragma once" );
        CAstNodeBase* pParent =
            m_pNode->GetParent();
        if( pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CStatements* pStmts = ObjPtr( pParent );
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        std::string strAppName = pStmts->GetName();
        CCOUT << "#include \"" << strAppName << ".h\"" ;
        NEW_LINES( 2 );

        auto pWriter = static_cast
            < CCppWriter* >( m_pWriter );

        ObjPtr pNode = m_pNode;
        CDeclServiceFuse2 ods( pWriter, pNode );
        ods.OutputROS( IsServer() );

        std::string strClass, strBase, strSkel;
        if( !vecPMethods.empty() && !IsServer() )
        {
            strClass = "C";
            strClass += strSvcName + "_CliImpl";
            strBase = "C";
            strBase += strSvcName + "_CliBase";
            CCOUT << "class " << strClass;

            INDENT_UP;
            NEW_LINE;

            CCOUT << ": public " << strBase;

            INDENT_DOWN;
            NEW_LINE;
            BLOCK_OPEN;

            Wa( "public:" );
            CCOUT << "typedef "
                << strBase << " super;";
            NEW_LINE;

            CCOUT << strClass
                << "( const IConfigDb* pCfg ) :";
            INDENT_UP;
            NEW_LINE;
            CCOUT << "super::virtbase( pCfg ), "
                << "super( pCfg )";
            INDENT_DOWN;
            NEW_LINE;
            CCOUT << "{ SetClassId( clsid(" << strClass << " ) ); }";
            NEW_LINES( 2 );

            Wa( "std::hashmap< guint64, guint64 >" );
            Wa( "    m_mapReq2Task, m_mapTask2Req;" );
            Wa( "gint32 inline AddReq(" );
            Wa( "    guint64 qwReq, guint64 qwTask )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrReq = m_mapReq2Task.find( qwReq );" );
            Wa( "auto itrTask = m_mapTask2Req.find( qwTask );" );
            Wa( "if( itrReq != m_mapReq2Task.end() ||" );
            Wa( "    itrTask != m_mapTask2Req.end() )" );
            Wa( "    return -EEXIST;" );
            Wa( "m_mapReq2Task[ qwReq ] = qwTask;" );
            Wa( "m_mapTask2Req[ qwTask ] = qwReq;" );
            Wa( "return STATUS_SUCCESS;" );
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "inline gint32 GetReqId(" );
            Wa( "    guint64 qwTask, guint64& qwReq )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrTask = m_mapTask2Req.find( qwTask );" );
            Wa( "if( itrTask == m_mapTask2Req.end() )" );
            Wa( "    return -ENOENT;" );
            Wa( "qwReq = itrTask->second;" );
            CCOUT << "return STATUS_SUCCESS;";
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "inline gint32 GetTaskId(" );
            Wa( "    guint64 qwReq, guint64& qwTask )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrReq = m_mapReq2Task.find( qwReq );" );
            Wa( "if( itrReq == m_mapReq2Task.end() )" );
            Wa( "    return -ENOENT;" );
            Wa( "qwTask = itrReq->second;" );
            CCOUT << "return STATUS_SUCCESS;";
            BLOCK_CLOSE;
            NEW_LINE;

            NEW_LINE;
            Wa( "gint32 CustomizeRequest2(" );
            Wa( "    IConfigDb* pReqCfg," );
            Wa( "    IEventSink* pCallback )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "do" );
            BLOCK_OPEN;
            Wa( "guint64 qwTaskId = 0, qwReqId = 0;" );
            Wa( "CCfgOpener oReq( ( IConfigDb*)pReqCfg );" );
            Wa( "ret = oReq.GetQwordProp(" );
            Wa( "    propTaskId, qwTaskId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "CCfgOpenerObj oCfg( ( IConfigDb*)pCallback );" );
            Wa( "ret = oCfg.GetQwordProp(" );
            Wa( "    propTaskId, qwReqId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "ret = AddReq( qwReqId, qwTaskId );" );
            CCOUT << "oCfg.RemoveProperty( propTaskId );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            NEW_LINE;
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "inline gint32 RemoveReq( guint64 qwReq )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrReq = m_mapReq2Task.find( qwReq );" );
            Wa( "if( itrReq == m_mapReq2Task.end() )" );
            Wa( "    return -ENOENT;" );
            Wa( "guint64 qwTask = itrReq->second;" );
            Wa( "m_mapReq2Task.erase( itrReq );" );
            Wa( "m_mapTask2Req.erase( qwTask );" );
            CCOUT << "return STATUS_SUCCESS;";
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "inline gint32 RemoveTask(" );
            Wa( "    guint64 qwTask )" );
            BLOCK_OPEN;
            Wa( "CStdRMutex oLock( GetLock() );" );
            Wa( "auto itrTask = m_mapTask2Req.find( qwTask );" );
            Wa( "if( itrTask == m_mapTask2Req.end() )" );
            Wa( "    return -ENOENT;" );
            Wa( "guint64 qwReq = itrTask->second;" );
            Wa( "m_mapReq2Task.erase( qwReq );" );
            Wa( "m_mapTask2Req.erase( itrTask );" );
            CCOUT << "return STATUS_SUCCESS;";
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "//Request Dispatcher" );
            Wa( "gint32 DispatchReq(" );
            Wa( "    IConfigDb* pContext," );
            Wa( "    const Json::Value& valReq," );
            Wa( "    Json::Value& valResp ) override;" );
            NEW_LINE;
            Wa( "gint32 CancelRequestByReqId(" );
            Wa( "    guint64 qwReqId, guint64 qwThisReq );" );

            NEW_LINE;
            Wa( "gint32 OnReqComplete(" );
            Wa( "    IEventSink* pCallback," );
            Wa( "    IConfigDb* pReqCtx," );
            Wa( "    Json::Value& valResp," );
            Wa( "    const std::string& strIfName," );
            Wa( "    const std::string& strMethod," );
            Wa( "    gint32 iRet, bool ) override;" );
            NEW_LINE;

            Wa( "/* The following 2 methods are important for */" );
            Wa( "/* streaming transfer. rewrite them if necessary */" );
            Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
            BLOCK_OPEN;
            Wa( "gint32 ret = super::OnStreamReady( hChannel );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    return ret;" );
            CCOUT << "return OnStreamReadyFuse( hChannel );";
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
            BLOCK_OPEN;
            Wa( "OnStmClosingFuse( hChannel );" );
            CCOUT << "return super::OnStmClosing( hChannel );";
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "gint32 OnReadStreamComplete( HANDLE hChannel," );
            Wa( "    gint32 iRet, BufPtr& pBuf, IConfigDb* pCtx ) override" );
            Wa( "{ return OnReadStreamCompleteFuse( hChannel, iRet, pBuf, pCtx ); } ");
            NEW_LINE;

            Wa( "gint32 OnWriteStreamComplete( HANDLE hChannel," );
            Wa( "    gint32 iRet, BufPtr& pBuf, IConfigDb* pCtx ) override" );
            Wa( "{ return OnWriteStreamCompleteFuse( hChannel, iRet, pBuf, pCtx ); } ");
            NEW_LINE;

            Wa( "gint32 OnWriteResumed( HANDLE hChannel ) override" );
            CCOUT << "{ return OnWriteResumedFuse( hChannel ); } ";
            NEW_LINES(2);
            Wa( "gint32 DoRmtModEvent(" );
            Wa( "    EnumEventId iEvent," );
            Wa( "    const std::string& strModule," );
            CCOUT << "    IConfigDb* pEvtCtx ) override";
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "gint32 ret = super::DoRmtModEvent(" );
            Wa( "     iEvent, strModule, pEvtCtx );");
            Wa( "if( SUCCEEDED( ret ) )" );
            Wa( "    ret = DoRmtModEventFuse(" );
            Wa( "        iEvent, strModule, pEvtCtx );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            strSkel = "C";
            strSkel += strSvcName + "_CliSkel";

            CCOUT << "inline " << strSkel <<"* GetSkelPtr()";
            NEW_LINE;
            BLOCK_OPEN;
            CCOUT << "auto pCli = dynamic_cast"
                << "< CFastRpcProxyBase* >( this );";
            NEW_LINE;
            Wa( "if( pCli == nullptr )" );
            Wa( "    return nullptr;" );
            Wa( "InterfPtr pIf = pCli->GetStmSkel();" );
            Wa( "if( pIf.IsEmpty() )" );
            Wa( "    return nullptr;" );
            CCOUT << "auto pSkel = dynamic_cast"
                << "<" << strSkel << "* >";
            NEW_LINE;
            CCOUT  << "    ( ( CRpcServices* )pIf );";
            NEW_LINE;
            CCOUT << "return pSkel;";
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "gint32 CreateStmSkel(" );
            Wa( "    InterfPtr& pIf ) override;" );
            NEW_LINE;
            Wa( "gint32 OnPreStart(" );
            CCOUT << "    IEventSink* pCallback ) override;";
            BLOCK_CLOSE;
            CCOUT << ";";
            NEW_LINES( 2 );

            stdstr strChanClass = "C";
            strChanClass += strSvcName + "_ChannelCli";
            strBase = "CRpcStreamChannelCli";

            CCOUT << "class " << strChanClass;
            INDENT_UP;
            NEW_LINE;

            CCOUT << ": public " << strBase;

            INDENT_DOWN;
            NEW_LINE;
            BLOCK_OPEN;

            Wa( "public:" );
            CCOUT << "typedef "
                << strBase << " super;";
            NEW_LINE;

            CCOUT << strChanClass << "(";
            NEW_LINE;
            CCOUT << "    const IConfigDb* pCfg ) :";
            INDENT_UPL;
            CCOUT << "super::virtbase( pCfg ), super( pCfg )";
            INDENT_DOWN;
            NEW_LINE;
            Wa( "{ SetClassId( clsid(" );
            CCOUT << "    " << strChanClass << " ) ); }";
            BLOCK_CLOSE;
            Wa( ";" );
            NEW_LINE;
        }

        if( vecSMethods.empty() || !IsServer() )
            break;

        strClass = "C";
        strClass += strSvcName + "_SvrImpl";
        strBase = "C";
        strBase += strSvcName + "_SvrBase";

        CCOUT << "class " << strClass;

        INDENT_UP;
        NEW_LINE;

        CCOUT << ": public " << strBase;

        INDENT_DOWN;
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "public:" );
        CCOUT << "typedef "
            << strBase << " super;";
        NEW_LINE;

        CCOUT << strClass
            << "( const IConfigDb* pCfg ) :";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "super::virtbase( pCfg ), "
            << "super( pCfg )";
        INDENT_DOWN;
        NEW_LINE;
        CCOUT << "{ SetClassId( clsid(" << strClass << " ) ); }";
        NEW_LINES( 2 );

        Wa( "//Response&Event Dispatcher" );
        Wa( "gint32 DispatchMsg(" );
        CCOUT << "    const Json::Value& valMsg ) override;";

        NEW_LINE;
        Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
        BLOCK_OPEN;
        Wa( "gint32 ret = super::OnStreamReady( hChannel );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    return ret;" );
        CCOUT << "return OnStreamReadyFuse( hChannel );";
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
        BLOCK_OPEN;
        Wa( "OnStmClosingFuse( hChannel );" );
        CCOUT << "return super::OnStmClosing( hChannel );";
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "gint32 AcceptNewStream(" );
        Wa( "    IEventSink* pCb, IConfigDb* pDataDesc ) override" );
        Wa( "{ return CFuseSvcServer::AcceptNewStreamFuse( pCb, pDataDesc ); }");
        NEW_LINE;

        Wa( "gint32 OnReadStreamComplete( HANDLE hChannel," );
        Wa( "    gint32 iRet, BufPtr& pBuf, IConfigDb* pCtx ) override" );
        Wa( "{ return OnReadStreamCompleteFuse( hChannel, iRet, pBuf, pCtx ); } ");
        NEW_LINE;

        Wa( "gint32 OnWriteStreamComplete( HANDLE hChannel," );
        Wa( "    gint32 iRet, BufPtr& pBuf, IConfigDb* pCtx ) override" );
        Wa( "{ return OnWriteStreamCompleteFuse( hChannel, iRet, pBuf, pCtx ); } ");
        NEW_LINE;

        Wa( "gint32 OnWriteResumed( HANDLE hChannel ) override" );
        CCOUT << "{ return OnWriteResumedFuse( hChannel ); } ";
        NEW_LINES( 2 );

        strSkel = "C";
        strSkel += strSvcName + "_SvrSkel";
        CCOUT << "inline " << strSkel
            << "* GetSkelPtr( HANDLE hstm )";
        NEW_LINE;
        BLOCK_OPEN;
        CCOUT <<  "auto pSvr = dynamic_cast"
          << "< CFastRpcServerBase* >( this );";
        NEW_LINE;
        Wa( "if( pSvr == nullptr )" );
        Wa( "    return nullptr;" );
        Wa( "InterfPtr pIf;" );
        Wa( "gint32 ret = pSvr->GetStmSkel(" );
        Wa( "    hstm, pIf );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    return nullptr;" );
        CCOUT << "auto pSkel = dynamic_cast" 
            << "<" << strSkel << "*>";
        NEW_LINE;
        CCOUT << "    ( ( CRpcServices* )pIf );";
        NEW_LINE;
        CCOUT << "return pSkel;";
        BLOCK_CLOSE;
        NEW_LINES( 2 );

        Wa( "gint32 CreateStmSkel(" );
        Wa( "    HANDLE, guint32, " );
        CCOUT << "    InterfPtr& ) override;";
        NEW_LINE;
        Wa( "gint32 OnPreStart(" );
        CCOUT << "    IEventSink* pCallback ) override;";

        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

        stdstr strChanClass = "C";
        strChanClass += strSvcName + "_ChannelSvr";
        strBase = "CRpcStreamChannelSvr";

        CCOUT << "class " << strChanClass;
        INDENT_UP;
        NEW_LINE;

        CCOUT << ": public " << strBase;

        INDENT_DOWN;
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "public:" );
        CCOUT << "typedef "
            << strBase << " super;";
        NEW_LINE;

        CCOUT << strChanClass << "(";
        NEW_LINE;
        CCOUT << "    const IConfigDb* pCfg ) :";
        INDENT_UPL;
        CCOUT << "super::virtbase( pCfg ), super( pCfg )";
        INDENT_DOWN;
        NEW_LINE;
        Wa( "{ SetClassId( clsid(" );
        CCOUT << "    "<< strChanClass << " ) ); }";
        BLOCK_CLOSE;
        Wa( ";" );
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplIfMethodProxyFuse2::OutputEvent()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_PImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwCount = dwInCount;

    bool bSerial = m_pNode->IsSerialize();
    if( !bSerial )
        return  ERROR_NOT_IMPL;

    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Wrapper( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IEventSink* pCallback";
        if( dwCount > 0 )
            CCOUT << ", BufPtr& pBuf_";

        CCOUT << " )";
        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );
        CCOUT << "do";
        BLOCK_OPEN;
        Wa( "Json::Value val_( Json::objectValue );" );
        if( dwCount > 0 )
        {
            ret = GenDeserialArgs(
                pInArgs, "pBuf_", false, true );
            if( ERROR( ret ) )
                break;
        }

        CMethodDecls* pmds =
            ObjPtr( m_pNode->GetParent() );
        CInterfaceDecl* pifd =
            ObjPtr( pmds->GetParent() );

        Wa( "stdstr strEvent;" );
        Wa( "CCfgOpener oReqCtx;" );
        Wa( "guint32 qwReqId = pCallback->GetObjId();" );
        Wa( "oReqCtx.SetQwordProp( 1, qwReqId );" );
        Wa( "ret = BuildJsonReq2( oReqCtx.GetCfg(), val_," );
        CCOUT << "    \"" << m_pNode->GetName() << "\",";
        NEW_LINE;
        CCOUT << "    \"" << pifd->GetName() << "\",";
        NEW_LINE;
        Wa( "    0, strEvent, true, false );" );
        Wa( "//TODO: pass strEvent to FUSE" );
        Wa( "InterfPtr pIf = this->GetParentIf();" );
        Wa( "CFuseSvcProxy* pProxy = ObjPtr( pIf );" );
        Wa( "pProxy->ReceiveEvtJson( strEvent );" );
        Wa( "if( ERROR( ret ) ) break;" );

        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        NEW_LINE;
        CCOUT << "return ret;";

        BLOCK_CLOSE;
        NEW_LINES( 1 );

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvrFuse2::OutputAsyncSerial()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bNoReply = m_pNode->IsNoReply();
    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Wrapper( ";
        INDENT_UPL;
        if( dwInCount > 0 )
            CCOUT << "IEventSink* pCallback"
                << ", BufPtr& pBuf_ )";
        else
            CCOUT << "IEventSink* pCallback )";

        INDENT_DOWNL;
        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );
        if( !bNoReply )
            Wa( "TaskletPtr pNewCb;" );
        CCOUT << "do";
        BLOCK_OPEN;

        Wa( "CParamList oReqCtx_;" );
        Wa( "oReqCtx_.SetPointer(" );
        Wa( "    propEventSink, pCallback );" );
        Wa( "IConfigDb* pReqCtx_ = oReqCtx_.GetCfg();" );

        if( !bNoReply )
        {
            guint32 dwTimeoutSec =
                m_pNode->GetTimeoutSec();

            guint32 dwKeepAliveSec =
                m_pNode->GetKeepAliveSec();
            if( dwKeepAliveSec == 0 )
            {
                dwKeepAliveSec =
                    ( dwTimeoutSec >> 1 );
            }

            if( dwTimeoutSec > 0 &&
                dwKeepAliveSec > 0 &&
                dwTimeoutSec > dwKeepAliveSec )
            {
                CCOUT << "ret = SetInvTimeout( pCallback, "
                    << dwTimeoutSec << ", "
                    << dwKeepAliveSec << " );";
                NEW_LINE;
            }
            else
            {
                Wa( "ret = SetInvTimeout( pCallback, 0 );" );
            }
            Wa( "if( ERROR( ret ) ) break;" );
            Wa( "DisableKeepAlive( pCallback );" );
            NEW_LINE;

            CCOUT << "ret = DEFER_CANCEL_HANDLER2(";
            INDENT_UPL;
            CCOUT << "-1, pNewCb, this,";
            NEW_LINE;
            CCOUT << "&" << strClass << "::"
                << strMethod << "CancelWrapper,";
            NEW_LINE;
            if( dwInCount > 0 )
                CCOUT << "pCallback, 0, pReqCtx_, pBuf_ );";
            else
                CCOUT << "pCallback, 0, pReqCtx_ );";
            INDENT_DOWNL;
            NEW_LINE;
            Wa( "if( ERROR( ret ) ) break;" );
            NEW_LINE;
        }

        Wa( "Json::Value val_( Json::objectValue );" );
        if( dwInCount > 0 )
        {
            ret = GenDeserialArgs(
                pInArgs, "pBuf_",
                false, false, true );
            if( ERROR( ret ) )
                break;
        }

        Wa( "stdstr strReq;" );
        Wa( "ret = BuildJsonReq2( pReqCtx_, val_," );
        CCOUT << "    \"" << strMethod << "\",";
        NEW_LINE;
        CCOUT << "    \"" << m_pIf->GetName() << "\",";
        NEW_LINE;
        Wa( "    0, strReq, false, false );" );
        Wa( "if( ERROR( ret ) ) break;" );
        CCOUT << "//NOTE: pass strReq to FUSE";
        NEW_LINE;
        Wa( "InterfPtr pIf = this->GetParentIf();" );
        Wa( "CFuseSvcServer* pSvr = ObjPtr( pIf );" );
        Wa( "ret = pSvr->ReceiveMsgJson( strReq," );
        CCOUT << "    pCallback->GetObjId() );";
        NEW_LINE;
        if( !bNoReply )
        {
            Wa( "if( SUCCEEDED( ret ) )" );
            CCOUT << "    ret = STATUS_PENDING;";
        }
        else
        {
            Wa( "if( ret == STATUS_PENDING )" );
            CCOUT << "    ret = STATUS_SUCCESS;";
        }
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        if( !bNoReply )
        {
            NEW_LINE;
            Wa( "if( ret == STATUS_PENDING )" );
            Wa( "    return ret;" );
            NEW_LINE;
            Wa( "CParamList oResp_;" );
            Wa( "oResp_[ propSeriProto ] = seriRidl;" );
            Wa( "oResp_[ propReturnValue ] = ret;" );
            Wa( "this->SetResponse( pCallback," );
            Wa(  "    oResp_.GetCfg() );" );
            NEW_LINE;
            Wa( "if( !pNewCb.IsEmpty() )" );
            BLOCK_OPEN;
            Wa( "CIfRetryTask* pTask = pNewCb;" );
            Wa( "pTask->ClearClientNotify();" );
            Wa( "if( pCallback != nullptr )" );
            Wa( "    rpcf::RemoveInterceptCallback(" );
            Wa( "        pTask, pCallback );" );
            CCOUT << "( *pNewCb )( eventCancelTask );";
            BLOCK_CLOSE;
            NEW_LINE;
        }
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvrFuse2::OutputAsyncCancelWrapper()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    guint32 dwInCount = GetArgCount( pInArgs );

    bool bSerial = m_pNode->IsSerialize();
    if( !bSerial )
        return ERROR_NOT_IMPL;

    do{
        Wa( "// this method is called when" );
        Wa( "// timeout happens or user cancels" );
        Wa( "// this pending request" );

        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "CancelWrapper(";
        NEW_LINE;
        CCOUT << "    IEventSink* pCallback, gint32 iRet,";
        NEW_LINE;
        CCOUT << "    IConfigDb* pReqCtx_";
        if( dwInCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            CCOUT << "    BufPtr& pBuf_";
        }
        CCOUT << " )";
        NEW_LINE;

        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );

        CCOUT << "do";
        BLOCK_OPEN;

        Wa( "// clean up the FUSE resources" );
        Wa( "ObjPtr pIf = GetParentIf();" );
        Wa( "guint64 qwTaskId = pCallback->GetObjId();" );
        Wa( "CFuseSvcServer* pSvr = pIf;" );
        CCOUT << "pSvr->OnUserCancelRequest( qwTaskId );";
        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINES( 2 );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplServiceImplFuse2::Output()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();

        stdstr strOutPath = m_pWriter->GetOutPath();

        std::vector< ABSTE > vecPMethods;
        ret = FindAbstMethod( vecPMethods, true );
        if( ERROR( ret ) )
            break;

        std::vector< ABSTE > vecSMethods;
        ret = FindAbstMethod( vecSMethods, false );
        if( ERROR( ret ) )
            break;
        
        if( vecSMethods.empty() &&
            vecPMethods.empty() )
            break;

        Wa( "/****BACKUP YOUR CODE BEFORE RUNNING RIDLC***/" );
        CCOUT << "// " << g_strCmdLine;
        NEW_LINE;
        Wa( "// Implement the following methods" );
        Wa( "// to get the RPC proxy/server work" );
        CAstNodeBase* pParent =
            m_pNode->GetParent();
        if( pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CStatements* pStmts = ObjPtr( pParent );
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        std::string strAppName = pStmts->GetName();
        Wa( "#include \"rpc.h\"" );
        Wa( "using namespace rpcf;" );
        Wa( "#include \"fastrpc.h\"" );
        CCOUT << "#include \""
            << strAppName << ".h\"" ;
        NEW_LINE;
        if( IsServer() )
            CCOUT << "#include \""
                << strSvcName << "svr.h\"";
        else
            CCOUT << "#include \""
                << strSvcName << "cli.h\"";

        NEW_LINES( 2 );

        std::string strClass, strBase;

        if( IsServer() )
        {
            strClass = "C";
            strClass += strSvcName + "_SvrImpl";

            // implement the DispatchMsg
            CCOUT << "gint32 " << strClass
                << "::DispatchMsg(";
            NEW_LINE;
            Wa( "    const Json::Value& oMsg )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "if( oMsg.empty() || !oMsg.isObject() )" );
            Wa( "    return -EINVAL;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "if( this->GetState() != stateConnected )" );
            Wa( "{ ret = -ENOTCONN; break; }" );
            Wa( "if( !oMsg.isMember( JSON_ATTR_IFNAME1 ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_IFNAME1 ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strIfName =" );
            Wa( "    oMsg[ JSON_ATTR_IFNAME1 ].asString();" );
            Wa( "if( !oMsg.isMember( JSON_ATTR_MSGTYPE ) ||");
            Wa( "    !oMsg[ JSON_ATTR_MSGTYPE ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strType =" );
            Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
            CCOUT << "if( strType == \"resp\" )";
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "if( !oMsg.isMember( JSON_ATTR_REQCTXID ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_REQCTXID ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strCtxId = " );
            Wa( "    oMsg[ JSON_ATTR_REQCTXID ].asString();" );
            Wa( "size_t pos = strCtxId.find( ':' );" );
            Wa( "if( pos == stdstr::npos )" );
            Wa( "{ ret = -EINVAL; break; }" );
            CCOUT << "stdstr strHstm = strCtxId.substr( pos + 1 );\n";
            CCOUT << "#if BITWIDTH==64";
            NEW_LINE;
            Wa( "HANDLE hStream = ( HANDLE )strtoull( " );
            CCOUT <<  "    strHstm.c_str(), nullptr, 10);\n";
            CCOUT << "#else";
            NEW_LINE;
            Wa( "HANDLE hStream = ( HANDLE )strtoul( " );
            CCOUT << "    strHstm.c_str(), nullptr, 10);\n";
            CCOUT << "#endif";
            NEW_LINE;
            Wa( "auto pSkel = GetSkelPtr( hStream );" );
            Wa( "if( pSkel == nullptr )" );
            Wa( "{ ret = -ENOENT; break; }" );
            std::vector< ObjPtr > vecIfRefs;
            m_pNode->GetIfRefs( vecIfRefs );
            for( auto& elem : vecIfRefs )
            {
                CInterfRef* pifr = elem;
                ObjPtr pObj;
                pifr->GetIfDecl( pObj );
                CInterfaceDecl* pifd = pObj;
                stdstr strIfName = pifd->GetName();
                CCOUT << "if( strIfName == \""
                    << strIfName << "\" )";
                NEW_LINE;
                BLOCK_OPEN;
                CCOUT << "ret = pSkel->" << "I" << strIfName
                    << "_SImpl::DispatchIfMsg( oMsg );";
                NEW_LINE;
                CCOUT << "break;";
                BLOCK_CLOSE;
                NEW_LINE;
            }
            Wa( "if( strIfName == \"IInterfaceServer\" )" );
            BLOCK_OPEN;
            Wa( "if( !oMsg.isMember( JSON_ATTR_METHOD ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_METHOD ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strMethod =" ); 
            Wa( "    oMsg[ JSON_ATTR_METHOD ].asString();" );
            Wa( "if( strMethod != \"UserCancelRequest\" )" );
            Wa( "{ ret = -ENOSYS; break; }" );
            Wa( "ret = pSkel->UserCancelRequestCbWrapper( oMsg );" );
            CCOUT << "break;";
            BLOCK_CLOSE;// IInterfaceServer
            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "else if( strType == \"evt\" )";
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "std::vector< InterfPtr > vecIfs;" );
            Wa( "ret = this->EnumStmSkels( vecIfs );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            for( auto& elem : vecIfRefs )
            {
                CInterfRef* pifr = elem;
                ObjPtr pObj;
                pifr->GetIfDecl( pObj );
                CInterfaceDecl* pifd = pObj;
                stdstr strIfName = pifd->GetName();
                CCOUT << "if( strIfName == \""
                    << strIfName << "\" )";
                NEW_LINE;
                BLOCK_OPEN;

                Wa( "for( auto& elem : vecIfs )" );
                BLOCK_OPEN;
                CCOUT << "C" << strSvcName << "_SvrSkel* pSkel = elem;";
                NEW_LINE;
                CCOUT << "ret = pSkel->" << "I" << strIfName
                    << "_SImpl::DispatchIfMsg( oMsg );";
                BLOCK_CLOSE;// for vecIfs
                NEW_LINE;
                CCOUT << "break;";
                BLOCK_CLOSE;
                NEW_LINE;
            }
            {
                Wa( "if( strIfName == \"IInterfaceServer\" )" );
                BLOCK_OPEN;
                Wa( "if( !oMsg.isMember( JSON_ATTR_METHOD ) ||" );
                Wa( "    !oMsg[ JSON_ATTR_METHOD ].isString() )" );
                Wa( "{ ret = -EINVAL; break; }" );
                Wa( "stdstr strMethod =" );
                Wa( "    oMsg[ JSON_ATTR_METHOD ].asString();" );
                Wa( "if( strMethod != \"OnKeepAlive\" )" );
                Wa( "{ ret = -ENOTSUP; break;}" );

                Wa( "if( !oMsg.isMember( JSON_ATTR_REQCTXID ) ||" );
                Wa( "    !oMsg[ JSON_ATTR_REQCTXID ].isString() )" );
                Wa( "{ ret = -EINVAL; break; }" );
                Wa( "stdstr strCtxId = " );
                Wa( "    oMsg[ JSON_ATTR_REQCTXID ].asString();" );
                Wa( "size_t pos = strCtxId.find( ':' );" );
                Wa( "if( pos == stdstr::npos )" );
                Wa( "{ ret = -EINVAL; break; }" );
                Wa( "stdstr strTaskId = strCtxId.substr( 0, pos );" );
                Wa( "stdstr strHstm = strCtxId.substr( pos + 1 );" );
                Wa( "guint64 qwTaskId = strtoull( " );
                CCOUT << "    strTaskId.c_str(), nullptr, 10);\n";
                CCOUT << "#if BITWIDTH==64";
                NEW_LINE;
                Wa( "HANDLE hStream = ( HANDLE )strtoull( " );
                CCOUT <<  "    strHstm.c_str(), nullptr, 10);\n";
                CCOUT << "#else";
                NEW_LINE;
                Wa( "HANDLE hStream = ( HANDLE )strtoul( " );
                CCOUT << "    strHstm.c_str(), nullptr, 10);\n";
                CCOUT << "#endif";
                NEW_LINE;
                Wa( "auto pSkel = GetSkelPtr( hStream );" );
                Wa( "if( pSkel == nullptr )" );
                Wa( "{ ret = -ENOENT; break; }" );
                Wa( "ret = pSkel->OnKeepAlive( qwTaskId );" );
                CCOUT << "break;";
                BLOCK_CLOSE;
            }
            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "ret = -ENOTSUP;";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            CCOUT << "gint32 " << strClass << "::CreateStmSkel(";
            NEW_LINE;
            Wa( "    HANDLE hStream, guint32 dwPortId, InterfPtr& pIf )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "CCfgOpener oCfg;" );
            Wa( "auto pMgr = this->GetIoMgr();" );
            Wa( "oCfg[ propIoMgr ] = ObjPtr( pMgr );" );
            Wa( "oCfg[ propIsServer ] = true;" );
            Wa( "oCfg.SetIntPtr( propStmHandle," );
            Wa( "    ( guint32*& )hStream );" );
            Wa( "oCfg.SetPointer( propParentPtr, this );" );
            CCOUT << "const char* szDesc = " << "\""
                << strOutPath << "/"
                << g_strAppName << "desc.json\";";
            NEW_LINE;
            Wa( "ret = CRpcServices::LoadObjDesc(" );
            CCOUT << "    szDesc," << "\""
                << strSvcName << "_SvrSkel\",";
            NEW_LINE;
            CCOUT << "    true, oCfg.GetCfg() );";
            NEW_LINE;
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "oCfg.CopyProp( propSkelCtx, this );" );
            Wa( "oCfg[ propPortId ] = dwPortId;" );
            Wa( "ret = pIf.NewObj(" );
            CCOUT << "    clsid( C" << strSvcName << "_SvrSkel ),";
            NEW_LINE;
            CCOUT << "    oCfg.GetCfg() );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );
            CCOUT << "gint32 " << strClass << "::OnPreStart(";
            NEW_LINE;
            Wa( "    IEventSink* pCallback )" );
            BLOCK_OPEN;
            EmitOnPreStart( m_pWriter, m_pNode, false );
            BLOCK_CLOSE;
            NEW_LINES( 2 );
        }
        else
        {
            strClass = "C";
            strClass += strSvcName + "_CliImpl";

            // implement the DispatchReq
            CCOUT << "gint32 " << strClass
                << "::DispatchReq(";
            NEW_LINE;
            Wa( "    IConfigDb* pContext," );
            Wa( "    const Json::Value& oReq," );
            Wa( "    Json::Value& oResp )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "if( oReq.empty() || !oReq.isObject() )" );
            Wa( "    return -EINVAL;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "if( !oReq.isMember( JSON_ATTR_METHOD ) ||");
            Wa( "    !oReq[ JSON_ATTR_METHOD ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strMethod =" );
            Wa( "    oReq[ JSON_ATTR_METHOD ].asString();" );
            Wa( "if( !oReq.isMember( JSON_ATTR_IFNAME1 ) ||");
            Wa( "    !oReq[ JSON_ATTR_IFNAME1 ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strIfName =" );
            Wa( "    oReq[ JSON_ATTR_IFNAME1 ].asString();" );
            Wa( "if( !oReq.isMember( JSON_ATTR_MSGTYPE ) ||");
            Wa( "    !oReq[ JSON_ATTR_MSGTYPE ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strType =" );
            Wa( "    oReq[ JSON_ATTR_MSGTYPE ].asString();" );
            Wa( "if( strType != \"req\" )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "if( !oReq.isMember( JSON_ATTR_REQCTXID ) ||");
            Wa( "    !oReq[ JSON_ATTR_REQCTXID ].isUInt64() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "guint64 qwReqId =" );
            Wa( "    oReq[ JSON_ATTR_REQCTXID ].asUInt64();" );
            Wa( "// init the response value" );
            Wa( "oResp[ JSON_ATTR_MSGTYPE ] = \"resp\";" );
            Wa( "oResp[ JSON_ATTR_METHOD ] = strMethod;" );
            Wa( "oResp[ JSON_ATTR_IFNAME1 ] = strIfName;" );
            Wa( "oResp[ JSON_ATTR_REQCTXID ] =" );
            Wa( "    ( Json::UInt64& )qwReqId;" );
            Wa( "if( this->GetState() != stateConnected )" );
            Wa( "{ ret = -ENOTCONN; break; }" );
            Wa( "auto pSkel = GetSkelPtr();" );
            Wa( "if( pSkel == nullptr )" );
            Wa( "{ ret = -EFAULT; break; }" );
            std::vector< ObjPtr > vecIfRefs;
            m_pNode->GetIfRefs( vecIfRefs );
            for( auto& elem : vecIfRefs )
            {
                CInterfRef* pifr = elem;
                ObjPtr pObj;
                pifr->GetIfDecl( pObj );
                CInterfaceDecl* pifd = pObj;
                stdstr strIfName = pifd->GetName();
                CCOUT << "if( strIfName == \""
                    << strIfName << "\" )";
                NEW_LINE;
                BLOCK_OPEN;
                CCOUT << "ret = pSkel->" << "I" << strIfName
                    << "_PImpl::DispatchIfReq( ";
                NEW_LINE;
                CCOUT << "    pContext, oReq, oResp );";
                NEW_LINE;
                Wa( "if( ret != STATUS_PENDING )" );
                Wa( "    this->RemoveReq( qwReqId );" );
                NEW_LINE;
                CCOUT << "break;";
                BLOCK_CLOSE;
                NEW_LINE;
            }
            Wa( "if( strIfName == \"IInterfaceServer\" )" );
            BLOCK_OPEN;
            Wa( "if( strMethod != \"UserCancelRequest\" )" );
            Wa( "{ ret = -ENOSYS; break; }" );
            Wa( "if( !oReq.isMember( JSON_ATTR_REQCTXID ) ||");
            Wa( "    !oReq[ JSON_ATTR_REQCTXID ].isUInt64() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "guint64 qwThisReq =" );
            Wa( "    oReq[ JSON_ATTR_REQCTXID ].asUInt64();" );
            Wa( "if( !oReq.isMember( JSON_ATTR_PARAMS ) ||");
            Wa( "    !oReq[ JSON_ATTR_PARAMS ].isObject() ||" );
            Wa( "    oReq[ JSON_ATTR_PARAMS ].empty() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "Json::Value val_ = oReq[ JSON_ATTR_PARAMS ];" );
            Wa( "if( !val_.isMember( JSON_ATTR_REQCTXID ) ||");
            Wa( "    !val_[ JSON_ATTR_REQCTXID ].isUInt64() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "guint64 qwReqId =" );
            Wa( "    val_[ JSON_ATTR_REQCTXID ].asUInt64();" );
            Wa( "ret = CancelRequestByReqId( qwReqId, qwThisReq );" );
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "ret = -ENOSYS;";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            Wa( "if( ret == -STATUS_PENDING )" );
            Wa( "    oResp[ JSON_ATTR_RETCODE ] = STATUS_PENDING;" );
            Wa( "else" );
            Wa( "    oResp[ JSON_ATTR_RETCODE ] = ret;" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            // implement the CancelRequestByReqId
            CCOUT << "gint32 " << strClass
                << "::CancelRequestByReqId( ";
            NEW_LINE;
            CCOUT << "    guint64 qwReqId, guint64 qwThisReq )";
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "guint64 qwTaskId = 0;" );
            Wa( "ret = this->GetTaskId( qwReqId, qwTaskId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "gint32 (*func)( CRpcServices* pIf," );
            Wa( "    IEventSink*, guint64, guint64) =" );
            Wa( "    ([]( CRpcServices* pIf," );
            Wa( "        IEventSink* pThis," );
            Wa( "        guint64 qwReqId, " );
            Wa( "        guint64 qwThisReq )->gint32" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "IConfigDb* pResp = nullptr;" );
            Wa( "CCfgOpenerObj oCfg( pThis );" );
            Wa( "ret = oCfg.GetPointer(" );
            Wa( "    propRespPtr, pResp );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "CCfgOpener oResp( pResp );" );
            Wa( "gint32 iRet = 0;" );
            Wa( "ret = oResp.GetIntProp(" );
            Wa( "    propReturnValue, ( guint32& )iRet );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "Json::Value oJsResp( Json::objectValue );" );
            Wa( "oJsResp[ JSON_ATTR_IFNAME1 ] =" );
            Wa( "    \"IInterfaceServer\";" );
            Wa( "oJsResp[ JSON_ATTR_METHOD ] =" );
            Wa( "    \"UserCancelRequest\";" );
            Wa( "oJsResp[ JSON_ATTR_MSGTYPE ] = \"resp\";" );
            Wa( "oJsResp[ JSON_ATTR_RETCODE ] = iRet;" );
            Wa( "oJsResp[ JSON_ATTR_REQCTXID ] =" );
            Wa( "    ( Json::UInt64& )qwThisReq;" );
            Wa( "Json::Value oParams( Json::objectValue );" );
            Wa( "oParams[ JSON_ATTR_REQCTXID ] =" );
            Wa( "    ( Json::UInt64& )qwReqId;" );
            Wa( "oJsResp[ JSON_ATTR_PARAMS ] = oParams;" );
            Wa( "Json::StreamWriterBuilder oBuilder;" );
            Wa( "oBuilder[\"commentStyle\"] = \"None\";" );
            Wa( "oBuilder[\"indentation\"] = \"\";" );
            CCOUT <<  "stdstr strResp = Json::writeString( "
                << "oBuilder, oJsResp );";
            NEW_LINE;
            CCOUT << strClass << "* pClient = ObjPtr( pIf );";
            NEW_LINE;
            Wa( "pClient->ReceiveMsgJson( strResp, qwThisReq );" );
            CCOUT << "pClient->RemoveReq( qwThisReq );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return 0;";
            BLOCK_CLOSE;
            Wa( ");" );
            Wa( "TaskletPtr pCb;" );
            Wa( "ret = NEW_FUNCCALL_TASK( pCb," );
            Wa( "    this->GetIoMgr(), func, this," );
            Wa( "    nullptr, qwReqId, qwThisReq );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            NEW_LINE;
            Wa( "CDeferredFuncCallBase< CIfRetryTask >*" );
            Wa( "    pCall = pCb;" );
            Wa( "ObjPtr pObj( pCall );" );
            Wa( "Variant oArg0( pObj );" );
            Wa( "pCall->UpdateParamAt( 1, oArg0 );" );
            NEW_LINE;
            Wa( "TaskletPtr pWrapper;" );
            Wa( "CCfgOpener oCfg;" );
            Wa( "oCfg.SetPointer( propIfPtr, this );" );
            Wa( "ret = pWrapper.NewObj(" );
            Wa( "   clsid( CTaskWrapper )," );
            Wa( "   ( IConfigDb* )oCfg.GetCfg() );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "CTaskWrapper* ptw = pWrapper;" );
            Wa( "ptw->SetCompleteTask( pCall );" );
            Wa( "CCfgOpenerObj otwCfg( ptw );" );
            Wa( "otwCfg.SetQwordProp( propTaskId, qwThisReq );" );
            NEW_LINE;
            Wa( "auto pSkel = GetSkelPtr();" );
            Wa( "ret = pSkel->CancelReqAsync(" );
            Wa( "    ptw, qwTaskId );" );
            Wa( "if( ERROR( ret ) )" );
            BLOCK_OPEN;
            Wa( "( *ptw )( eventCancelTask );" );
            CCOUT << "this->RemoveReq( qwReqId );";
            BLOCK_CLOSE;
            Wa( "break;" );
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;

            NEW_LINE;
            Wa( "extern gint32 BuildJsonReq2(" );
            Wa( "    IConfigDb* pReqCtx_," );
            Wa( "    const Json::Value& oJsParams," );
            Wa( "    const std::string& strMethod," );
            Wa( "    const std::string& strIfName, " );
            Wa( "    gint32 iRet," );
            Wa( "    std::string& strReq," );
            Wa( "    bool bProxy," );
            Wa( "    bool bResp );" );
            NEW_LINE;

            // implement OnReqComplete
            CCOUT << "gint32 " << strClass
                << "::OnReqComplete(";
            NEW_LINE;
            Wa( "    IEventSink* pCallback," );
            Wa( "    IConfigDb* pReqCtx," );
            Wa( "    Json::Value& valResp," );
            Wa( "    const std::string& strIfName," );
            Wa( "    const std::string& strMethod," );
            Wa( "    gint32 iRet," );
            Wa( "    bool bNoReply )" );
            BLOCK_OPEN;
            Wa( "UNREFERENCED( pCallback );" );
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "stdstr strReq;" );
            Wa( "ret = BuildJsonReq2( pReqCtx, valResp," );
            Wa( "    strMethod, strIfName, iRet," );
            Wa( "    strReq, true, true );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "CCfgOpener oReqCtx_( pReqCtx );" );
            Wa( "guint64 qwReqId = 0;" );
            Wa( "ret = oReqCtx_.GetQwordProp( 1, qwReqId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "if( !bNoReply )" );
            Wa( "    ret = this->ReceiveMsgJson( strReq, qwReqId );" );
            CCOUT << "this->RemoveReq( qwReqId );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            CCOUT << "gint32 " << strClass << "::CreateStmSkel(";
            NEW_LINE;
            Wa( "    InterfPtr& pIf )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "CCfgOpener oCfg;" );
            Wa( "auto pMgr = this->GetIoMgr();" );
            Wa( "oCfg[ propIoMgr ] = ObjPtr( pMgr );" );
            Wa( "oCfg[ propIsServer ] = false;" );
            Wa( "oCfg.SetPointer( propParentPtr, this );" );
            CCOUT << "const char* szDesc = " << "\""
                << strOutPath << "/"
                << g_strAppName << "desc.json\";";
            NEW_LINE;
            Wa( "ret = CRpcServices::LoadObjDesc(" );
            CCOUT << "    szDesc," << "\""
                << strSvcName << "_SvrSkel\",";
            NEW_LINE;
            CCOUT << "    false, oCfg.GetCfg() );";
            NEW_LINE;
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "oCfg.CopyProp( propSkelCtx, this );" );
            Wa( "ret = pIf.NewObj(" );
            CCOUT << "    clsid( C" << strSvcName << "_CliSkel ),";
            NEW_LINE;
            CCOUT << "    oCfg.GetCfg() );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );
            CCOUT << "gint32 " << strClass << "::OnPreStart(";
            NEW_LINE;
            Wa( "    IEventSink* pCallback )" );
            BLOCK_OPEN;
            EmitOnPreStart( m_pWriter, m_pNode, true );
            BLOCK_CLOSE;
            NEW_LINES( 2 );
        }

    }while( 0 );

    return ret;
}

gint32 CImplServiceImplFuse2::OutputROSSkel()
{
    gint32 ret = 0; 
    do{
        stdstr strSvcName = m_pNode->GetName();
        if( IsServer() )
        {
            stdstr strClassName = "C";
            strClassName += strSvcName + "_SvrSkel";
            CCOUT << "gint32 " << strClassName
                << "::UCRCancelHandler(";
            NEW_LINE;
            Wa( "    IEventSink* pCallback," );
            Wa( "    gint32 iRet," );
            Wa( "    guint64 qwTaskId )" );
            BLOCK_OPEN;
            Wa( "return super::UserCancelRequest(" );
            CCOUT << "    pCallback, qwTaskId );";
            BLOCK_CLOSE;
            NEW_LINE;

            Wa( "extern gint32 GetSvrCtxId(" );
            Wa( "    IEventSink* pCb, stdstr& strCtxId );" );
            NEW_LINE;

            Wa( "extern gint32 BuildJsonReq2(" );
            Wa( "    IConfigDb* pReqCtx_," );
            Wa( "    const Json::Value& oJsParams," );
            Wa( "    const std::string& strMethod," );
            Wa( "    const std::string& strIfName, " );
            Wa( "    gint32 iRet," );
            Wa( "    std::string& strReq," );
            Wa( "    bool bProxy," );
            Wa( "    bool bResp );" );
            NEW_LINE;

            CCOUT << "gint32 " << strClassName
                << "::UserCancelRequest(";
            NEW_LINE;
            Wa( "    IEventSink* pCallback," );
            Wa( "    guint64 qwInvTaskId )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "TaskletPtr pNewCb;" );
            CCOUT << "do";
            BLOCK_OPEN;

            Wa( "DisableKeepAlive( pCallback );" );
            Wa( "ret = SetInvTimeout( pCallback, 30 );" );
            Wa( "if( ERROR( ret ) ) break;" );
            NEW_LINE;

            Wa( "TaskGrpPtr pTaskGrp = GetTaskGroup();" );
            Wa( "if( pTaskGrp.IsEmpty() )" );
            BLOCK_OPEN;
            Wa( "ret = -EFAULT;" );
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            Wa( "TaskletPtr pTask;" );
            Wa( "ret = pTaskGrp->FindTaskByRmtId(" );
            Wa( "    qwInvTaskId, pTask );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            NEW_LINE;
            
            Wa( "CParamList oReqCtx_;" );
            Wa( "oReqCtx_.SetPointer(" );
            Wa( "    propEventSink, pCallback );" );
            NEW_LINE;
            
            
            Wa( "ret = DEFER_CANCEL_HANDLER2(" );
            Wa( "    -1, pNewCb, this," );
            CCOUT <<"    &" << strClassName
                << "::UCRCancelHandler,";
            NEW_LINE;
            Wa( "    pCallback, 0, qwInvTaskId );" );
            Wa( "if( ERROR( ret ) ) break;" );
            
            NEW_LINE;
            Wa( "Json::Value val_( Json::objectValue );" );
            Wa( "stdstr strCtxId;" );
            Wa( "ret = GetSvrCtxId( pTask, strCtxId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "val_[ JSON_ATTR_REQCTXID ] = strCtxId;" );

            NEW_LINE;
            Wa( "stdstr strReq;" );
            Wa( "IConfigDb* pReqCtx_ = oReqCtx_.GetCfg();" );
            Wa( "ret = BuildJsonReq2( pReqCtx_, val_," );
            Wa( "    \"UserCancelRequest\"," );
            Wa( "    \"IInterfaceServer\"," );
            Wa( "    0, strReq, false, false );" );
            Wa( "if( ERROR( ret ) ) break;" );

            NEW_LINE;
            Wa( "// NOTE: pass strReq to FUSE" );
            Wa( "InterfPtr pIf = this->GetParentIf();" );
            Wa( "CFuseSvcServer* pSvr = ObjPtr( pIf );" );
            Wa( "ret = pSvr->ReceiveMsgJson( strReq," );
            Wa( "    pCallback->GetObjId() );" );
            Wa( "if( SUCCEEDED( ret ) )" );
            CCOUT << "    ret = STATUS_PENDING;";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );

            NEW_LINE;
            Wa( "if( ret == STATUS_PENDING )" );
            Wa( "    return ret;" );

            NEW_LINE;
            Wa( "if( !pNewCb.IsEmpty() )" );
            BLOCK_OPEN;
            Wa( "CIfRetryTask* pTask = pNewCb;" );
            Wa( "pTask->ClearClientNotify();" );
            Wa( "if( pCallback != nullptr )" );
            Wa( "    rpcf::RemoveInterceptCallback(" );
            Wa( "        pTask, pCallback );" );
            Wa( "( *pNewCb )( eventCancelTask );" );
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "// fallback to the default request cancel handler" );
            Wa( "return super::UserCancelRequest(" );
            CCOUT << "    pCallback, qwInvTaskId );";
            BLOCK_CLOSE;
            NEW_LINES(2);

            CCOUT << "gint32 " << strClassName
                << "::UserCancelRequestComplete(";
            NEW_LINE;
            Wa( "    IEventSink* pCallback," );
            Wa( "    guint64 qwTaskToCancel )" );
            BLOCK_OPEN;
            Wa( "if( pCallback == nullptr )" );
            Wa( "    return -EINVAL;" );
            Wa( "gint32 ret = super::UserCancelRequest(" );
            Wa( "    pCallback, qwTaskToCancel );" );
            Wa( "pCallback->OnEvent(" );
            Wa( "    eventTaskComp, ret, 0, 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES(2);

            CCOUT << "gint32 " << strClassName
                <<"::UserCancelRequestCbWrapper(";
            NEW_LINE;
            Wa( "    const Json::Value& oJsResp )" );
            BLOCK_OPEN; 
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;

            Wa( "stdstr strCtxId =" );
            Wa( "    oJsResp[ JSON_ATTR_REQCTXID ].asString();" );
            Wa( "size_t pos = strCtxId.find( ':' );" );
            Wa( "if( pos == stdstr::npos )" );
            Wa( "{ ret = -EINVAL; break; }" );

            NEW_LINE;
            Wa( "stdstr strTaskId = strCtxId.substr( 0, pos );" );
            Wa( "guint64 qwTaskId = ( HANDLE )strtoull(" );
            Wa( "    strTaskId.c_str(), nullptr, 10);" );

            NEW_LINE;
            Wa( "TaskGrpPtr pTaskGrp = GetTaskGroup();" );
            Wa( "if( pTaskGrp.IsEmpty() )" );
            Wa( "    break;" );

            NEW_LINE;
            Wa( "TaskletPtr pTask;" );
            Wa( "ret = pTaskGrp->FindTask(" );
            Wa( "    qwTaskId, pTask );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            NEW_LINE;

            Wa( "IConfigDb* pReq = nullptr;" );
            Wa( "CCfgOpenerObj oInvTask(" );
            Wa( "    ( IEventSink* )pTask );" );
            Wa( "ret = oInvTask.GetPointer(" );
            Wa( "    propReqPtr, pReq );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            NEW_LINE;

            Wa( "guint64 qwTaskToCancel = 0;" );
            Wa( "CCfgOpener oReq( pReq );" );
            Wa( "ret = oReq.GetQwordProp(" );
            Wa( "    0, qwTaskToCancel );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );

            NEW_LINE;
            Wa( "ret = UserCancelRequestComplete(" );
            CCOUT << "    pTask, qwTaskToCancel );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINE;
        }
        else
        {
            stdstr strSvcName = m_pNode->GetName();
            CCOUT << "gint32 C" << strSvcName
                << "_CliSkel::CustomizeRequest(";
            NEW_LINE;
            Wa( "    IConfigDb* pReqCfg," );
            Wa( "    IEventSink* pCallback )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = super::CustomizeRequest(" );
            Wa( "    pReqCfg, pCallback );" );
            Wa( "if( ERROR( ret ) ) return ret;" );
            CCOUT << "C" << strSvcName << "_CliImpl* pParent =";
            NEW_LINE;
            Wa( "    GetParentIf();" );
            Wa( "if( pParent == nullptr )" );
            Wa( "    return -EFAULT;" );
            Wa( "return pParent->CustomizeRequest2(" );
            CCOUT << "    pReqCfg, pCallback );";
            BLOCK_CLOSE;
            NEW_LINE;
        }
    }while( 0 );

    return ret;
}

gint32 CImplIufProxyFuse2::Output()
{
        gint32 ret = super::Output();
        if( ERROR( ret ) )
            return ret;
        return OutputOnReqComplete();
}

gint32 CImplIufProxyFuse2::OutputOnReqComplete()
{
    gint32 ret = 0;
    do{
        std::string strClass = "I";
        std::string strIfName = m_pNode->GetName();
        strClass += strIfName + "_PImpl";

        CCOUT << "gint32 " << strClass<< "::OnReqComplete(";
        NEW_LINE;
        Wa( "    IEventSink* pCallback," );
        Wa( "    IConfigDb* pReqCtx," );
        Wa( "    Json::Value& val_," );
        Wa( "    const std::string& strIfName," );
        Wa( "    const std::string& strMethod," );
        Wa( "    gint32 iRet," );
        Wa( "    bool bNoReply )" );
        BLOCK_OPEN;
        Wa( "InterfPtr& pParent = GetParentIf();" );
        CCOUT <<  "auto pApi = dynamic_cast< I" <<
            strIfName << "_CliApi* >";
        NEW_LINE;
        Wa( "    ( ( CObjBase* )pParent );" );
        Wa( "return pApi->OnReqComplete( pCallback," );
        Wa( "    pReqCtx, val_, strIfName," );
        CCOUT << "    strMethod, iRet, bNoReply );";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}


gint32 CImplIufSvrFuse2::OutputDispatch()
{
    gint32 ret = 0;
    do{
        std::string strClass = "I";
        strClass += m_pNode->GetName() + "_SImpl";
        std::string strIfName = m_pNode->GetName();
        CCOUT << "gint32 " <<
            strClass << "::DispatchIfMsg(";
        NEW_LINE;
        CCOUT << "    const Json::Value& oMsg )";
        NEW_LINE;
        BLOCK_OPEN;

        ObjPtr pMethods =
            m_pNode->GetMethodList();
        CMethodDecls* pmds = pMethods;
        guint32 dwCount = pmds->GetCount();

        if( dwCount == 0 )
        {
            CCOUT << "return 0;";
            BLOCK_CLOSE;
            NEW_LINE;
            break;
        }

        bool bHasReq = false;
        bool bHasEvent = false;

        guint32 i;
        for( i = 0; i < dwCount; i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            CMethodDecl* pmd = pObj;
            if( pmd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            if( pmd->IsEvent() )
            {
                bHasEvent = true;
                continue;
            }
            bHasReq = true;
        }
        if( ERROR( ret ) )
            break;

        Wa( "gint32 ret = 0;" );
        CCOUT <<"do";
        BLOCK_OPEN;
        Wa( "if( !oMsg.isMember( JSON_ATTR_METHOD ) ||");
        Wa( "    !oMsg[ JSON_ATTR_METHOD ].isString() )" );
        Wa( "{ ret = -EINVAL; break; }" );
        Wa( "stdstr strMethod =" );
        Wa( "    oMsg[ JSON_ATTR_METHOD ].asString();" );
        Wa( "if( !oMsg.isMember( JSON_ATTR_MSGTYPE ) ||");
        Wa( "    !oMsg[ JSON_ATTR_MSGTYPE ].isString() )" );
        Wa( "{ ret = -EINVAL; break; }" );
        Wa( "stdstr strType =" );
        Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
        if( bHasReq )
        {
            Wa( "if( strType == \"resp\" )" );
            BLOCK_OPEN;
            Wa( "if( !oMsg.isMember( JSON_ATTR_RETCODE ) ||");
            Wa( "    !oMsg[ JSON_ATTR_RETCODE ].isUInt() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "gint32 iRet =" );
            Wa( "    oMsg[ JSON_ATTR_RETCODE ].asInt();" );
            Wa( "if( !oMsg.isMember( JSON_ATTR_REQCTXID ) ||");
            Wa( "    !oMsg[ JSON_ATTR_REQCTXID ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );

            Wa( "stdstr strCtxId = " );
            Wa( "    oMsg[ JSON_ATTR_REQCTXID ].asString();" );
            Wa( "size_t pos = strCtxId.find( ':' );" );
            Wa( "if( pos == stdstr::npos )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strTaskId = strCtxId.substr( 0, pos );" );
            Wa( "guint64 qwTaskId = strtoull( " );
            Wa( "    strTaskId.c_str(), nullptr, 10);\n" );

            Wa( "TaskGrpPtr pGrp;" );
            Wa( "ret = this->GetParallelGrp( pGrp );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "TaskletPtr pTask;" );
            Wa( "ret = pGrp->FindTask( qwTaskId, pTask );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "CParamList oReqCtx;" );
            Wa( "oReqCtx[ propEventSink ] = ObjPtr( pTask );" );

            for( i = 0; i < dwCount; i++ )
            {
                ObjPtr pObj = pmds->GetChild( i );
                CMethodDecl* pmd = pObj;
                if( pmd == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                if( pmd->IsEvent() )
                    continue;

                ObjPtr pInArgs = pmd->GetInArgs();
                guint32 dwInCount = GetArgCount( pInArgs );
                ObjPtr pOutArgs = pmd->GetOutArgs();
                guint32 dwOutCount = GetArgCount( pOutArgs );
                CCOUT << "if( strMethod == \""
                    << pmd->GetName() << "\" )";
                NEW_LINE;
                BLOCK_OPEN;
                if( !pmd->IsNoReply() )
                {
                    CCOUT << "ret = " << strClass << "::"
                        << pmd->GetName() <<"Complete(";
                    NEW_LINE;
                    CCOUT << "    oReqCtx.GetCfg(), iRet";
                    if( dwOutCount > 0 ) 
                        CCOUT << ", oMsg";
                    CCOUT << " );";
                    NEW_LINE;
                    Wa( "if( ret == STATUS_PENDING )" );
                    Wa( "    ret = STATUS_SUCCESS;" );
                }
                else
                {
                    Wa( "// no reply" );
                    Wa( "ret = -EINVAL;" );
                }
                CCOUT << "break;";
                BLOCK_CLOSE;
                if( i < dwCount - 1 )
                    NEW_LINE;
            }
            BLOCK_CLOSE; // strType == resp
            NEW_LINE;
        }

        if( bHasEvent )
        {
            Wa( "if( strType == \"evt\" )" );
            BLOCK_OPEN;

            for( i = 0; i < dwCount; i++ )
            {
                ObjPtr pObj = pmds->GetChild( i );
                CMethodDecl* pmd = pObj;
                if( pmd == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                if( !pmd->IsEvent() )
                    continue;

                ObjPtr pInArgs = pmd->GetInArgs();
                guint32 dwInCount = GetArgCount( pInArgs );
                ObjPtr pOutArgs = pmd->GetOutArgs();
                guint32 dwOutCount = GetArgCount( pOutArgs );
                CCOUT << "if( strMethod == \""
                    << pmd->GetName() << "\" )";
                NEW_LINE;
                BLOCK_OPEN;

                CCOUT << "ret = " << strClass << "::"
                    << pmd->GetName() <<"(";
                if( dwInCount > 0 ) 
                    CCOUT << " oMsg );";
                else
                    CCOUT << ");";
                NEW_LINE;
                Wa( "if( ret == STATUS_PENDING )" );
                Wa( "    ret = STATUS_SUCCESS;" );

                CCOUT << "break;";
                BLOCK_CLOSE;
                if( i < dwCount - 1 )
                    NEW_LINE;
            }
            BLOCK_CLOSE; // strType == evt
            NEW_LINE;
        }
        CCOUT << "ret = -ENOENT;";
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

extern std::string g_strTarget;
gint32 CImplMainFuncFuse2::Output()
{
    gint32 ret = 0;

    do{
        CStatements* pStmts = m_pNode;
        std::vector< ObjPtr > vecSvcs;
        pStmts->GetSvcDecls( vecSvcs );
        std::string strModName = g_strTarget;
        stdstr strSuffix;
        if( m_bProxy )
            strSuffix = "cli";
        else
            strSuffix = "svr";
        {
            bool bProxy = m_bProxy;
            if( bProxy )
            {
                m_pWriter->SelectMainCli();
            }
            else
            {
                m_pWriter->SelectMainSvr();
            }

            EMIT_DISCLAIMER;
            CCOUT << "// " << g_strCmdLine;
            NEW_LINE;
            Wa( "#include \"rpc.h\"" );
            Wa( "#include \"proxy.h\"" );
            Wa( "#include \"fuseif.h\"" );
            Wa( "using namespace rpcf;" );
            Wa( "#include \"stmport.h\"" );
            Wa( "#include \"fastrpc.h\"" );
            for( auto elem : vecSvcs )
            {
                CServiceDecl* pSvc = elem;
                bool bNewLine = false;
                if( bProxy || g_bMklib )
                {
                    CCOUT << "#include \""
                        << pSvc->GetName() << "cli.h\"";
                    bNewLine = true;
                }
                if( !bProxy )
                {
                    if( bNewLine )
                        NEW_LINE;
                    CCOUT << "#include \""
                        << pSvc->GetName() << "svr.h\"";
                }
                NEW_LINE;
            }
            if( g_bMklib && bProxy )
                break;

            NEW_LINES( 1 );
            if( g_bMklib )
                Wa( "extern ObjPtr g_pIoMgr;" );
            else
                Wa( "ObjPtr g_pIoMgr;" );
            NEW_LINE;

            ObjPtr pRoot( m_pNode );
            CImplClassFactoryFuse2 oicf(
                m_pWriter, pRoot, !bProxy );

            ret = oicf.Output();
            if( ERROR( ret ) )
                break;

            if( g_bMklib )
                break;

            NEW_LINE;

            // InitContext
            EmitInitContext(
                bProxy, m_pWriter );

            EmitFuseMain( vecSvcs,
                bProxy, m_pWriter );
        }

    }while( 0 );

    return ret;
}

gint32 CDeclServiceFuse2::OutputROSSkel()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();
        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        CCOUT << "#define Clsid_C" << strSvcName << "_CliSkel_Base Clsid_Invalid";
        NEW_LINE;
        CCOUT << "DECLARE_AGGREGATED_SKEL_PROXY(";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "C" << strSvcName << "_CliSkel_Base,";
        NEW_LINE;
        for( guint32 i = 0;
            i < vecIfs.size(); i++ )
        {
            CInterfRef* pifr = vecIfs[ i ];
            std::string strName = pifr->GetName();
            CCOUT << "I" << strName << "_PImpl";
            if( i + 1 < vecIfs.size() )
            {
                CCOUT << ",";
                NEW_LINE;
            }
        }
        CCOUT << " );";
        INDENT_DOWNL;
        NEW_LINE;

        CCOUT << "class C" << strSvcName << "_CliSkel :";
        NEW_LINE;
        CCOUT << "    public C" << strSvcName << "_CliSkel_Base";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "public:" );
        CCOUT << "typedef C" << strSvcName << "_CliSkel_Base super;";
        NEW_LINE;
        CCOUT << "C" << strSvcName << "_CliSkel( const IConfigDb* pCfg ):";
        NEW_LINE;
        Wa( "    super::virtbase( pCfg ), super( pCfg )" );
        CCOUT << "{ SetClassId( clsid( C" << strSvcName << "_CliSkel ) ); }";
        NEW_LINE;

        Wa( "gint32 CustomizeRequest(" );
        Wa( "    IConfigDb* pReqCfg," );
        CCOUT << "    IEventSink* pCallback ) override;";
        BLOCK_CLOSE;
        Wa( ";" );
        NEW_LINE;

        CCOUT << "#define Clsid_C" << strSvcName << "_SvrSkel_Base Clsid_Invalid";
        NEW_LINE;
        NEW_LINE;
        CCOUT << "DECLARE_AGGREGATED_SKEL_SERVER(";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "C" << strSvcName << "_SvrSkel_Base,";
        NEW_LINE;
        for( guint32 i = 0;
            i < vecIfs.size(); i++ )
        {
            CInterfRef* pifr = vecIfs[ i ];
            std::string strName = pifr->GetName();
            CCOUT << "I" << strName << "_SImpl";
            if( i + 1 < vecIfs.size() )
            {
                CCOUT << ",";
                NEW_LINE;
            }
        }
        CCOUT << " );";
        INDENT_DOWN;
        NEW_LINES( 2 );

        CCOUT << "class C" << strSvcName << "_SvrSkel :";
        NEW_LINE;
        CCOUT << "    public C" << strSvcName << "_SvrSkel_Base";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "public:" );
        CCOUT << "typedef C" << strSvcName << "_SvrSkel_Base super;";
        NEW_LINE;
        CCOUT << "C" << strSvcName << "_SvrSkel( const IConfigDb* pCfg ):";
        NEW_LINE;
        Wa( "    super::virtbase( pCfg ), super( pCfg )" );
        CCOUT << "{ SetClassId( clsid( C" << strSvcName << "_SvrSkel ) ); }";
        NEW_LINE;

        Wa( "// methods for active canceling" );
        Wa( "gint32 UserCancelRequest(" );
        Wa( "    IEventSink* pCallback," );
        Wa( "    guint64 qwTaskId ) override;" );
        NEW_LINE;

        Wa( "gint32 UserCancelRequestCbWrapper(" );
        Wa( "    const Json::Value& oJsResp );" );
        NEW_LINE;

        Wa( "private:" );
        Wa( "gint32 UCRCancelHandler(" );
        Wa( "    IEventSink* pCallback," );
        Wa( "    gint32 iRet," );
        Wa( "    guint64 qwTaskId );" );
        NEW_LINE;

        Wa( "gint32 UserCancelRequestComplete(" );
        Wa( "    IEventSink* pCallback," );
        CCOUT << "    guint64 qwTaskToCancel );";
        BLOCK_CLOSE;
        Wa( ";" );

    }while( 0 );

    return ret;
}

gint32 CDeclServiceFuse2::OutputROS( bool bServer )
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();
        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        if( !bServer )
        {
            CCOUT << "#define Clsid_C" << strSvcName
                << "_CliBase    Clsid_Invalid";
            NEW_LINES( 2 );
            CCOUT << "DECLARE_AGGREGATED_PROXY(";
            INDENT_UP;
            NEW_LINE;
            CCOUT << "C" << strSvcName << "_CliBase,";
            NEW_LINE;
            Wa( "CStatCountersProxy," );
            Wa( "CStreamProxyFuse," );
            Wa( "CFuseSvcProxy," );
            for( guint32 i = 0;
                i < vecIfs.size(); i++ )
            {
                CInterfRef* pifr = vecIfs[ i ];
                std::string strName = pifr->GetName();
                CCOUT << "I" << strName << "_CliApi,";
                NEW_LINE;
            }
            CCOUT << "CFastRpcProxyBase );";
            INDENT_DOWN;
            NEW_LINES( 2 );
        }
        else
        {
            CCOUT << "#define Clsid_C" << strSvcName
                << "_SvrBase    Clsid_Invalid";
            NEW_LINES( 2 );
            CCOUT << "DECLARE_AGGREGATED_SERVER(";
            INDENT_UP;
            NEW_LINE;
            CCOUT << "C" << strSvcName << "_SvrBase,";
            NEW_LINE;

            Wa( "CStatCountersServer," );
            Wa( "CStreamServerFuse," );
            Wa( "CFuseSvcServer," );

            for( guint32 i = 0;
                i < vecIfs.size(); i++ )
            {
                CInterfRef* pifr = vecIfs[ i ];
                std::string strName = pifr->GetName();
                CCOUT << "I" << strName << "_SvrApi,";
                NEW_LINE;
            }
            CCOUT << "CFastRpcServerBase );";
            INDENT_DOWN;
            INDENT_DOWN;
            NEW_LINES( 2 );
        }

    }while( 0 );

    return ret;
}

gint32 CImplClassFactoryFuse2::Output()
{
    if( m_pNode == nullptr )
        return -EFAULT;

    gint32 ret = 0;
    do{
        CStatements* pStmts = m_pNode;

        std::vector< ObjPtr > vecSvcs;
        pStmts->GetSvcDecls( vecSvcs );

        std::vector< ObjPtr > vecStructs;
        pStmts->GetStructDecls( vecStructs );
        std::vector< ObjPtr > vecActStructs;
        for( auto& elem : vecStructs )
        {
            CStructDecl* psd = elem;
            if( psd->RefCount() == 0 )
                continue;
            vecActStructs.push_back( elem );
        }

        NEW_LINE;

        if( g_bMklib && bFuse )
            Wa( "extern void InitMsgIds();" );

        Wa( "FactoryPtr InitClassFactory()" ); 
        BLOCK_OPEN;
        CCOUT << "BEGIN_FACTORY_MAPS;";
        NEW_LINES( 2 );

        for( auto& elem : vecSvcs )
        {
            CServiceDecl* pSvc = elem;
            if( IsServer() )
            {
                CCOUT << "INIT_MAP_ENTRYCFG( ";
                CCOUT << "C" << pSvc->GetName()
                    << "_SvrImpl );";
                NEW_LINE;

                CCOUT << "INIT_MAP_ENTRYCFG( ";
                CCOUT << "C" << pSvc->GetName()
                    << "_SvrSkel );";
                NEW_LINE;
                CCOUT << "INIT_MAP_ENTRYCFG( C"
                    << pSvc->GetName()
                    << "_ChannelSvr );";
                NEW_LINE;
            }
            if( !IsServer() || g_bMklib )
            {
                CCOUT << "INIT_MAP_ENTRYCFG( ";
                CCOUT << "C" << pSvc->GetName()
                    << "_CliImpl );";
                NEW_LINE;

                CCOUT << "INIT_MAP_ENTRYCFG( ";
                CCOUT << "C" << pSvc->GetName()
                    << "_CliSkel );";
                NEW_LINE;
                CCOUT << "INIT_MAP_ENTRYCFG( C"
                    << pSvc->GetName()
                    << "_ChannelCli );";
                NEW_LINE;
            }
        }
        if( vecActStructs.size() )
            NEW_LINE;
        for( auto& elem : vecActStructs )
        {
            CStructDecl* psd = elem;
            CCOUT << "INIT_MAP_ENTRY( ";
            CCOUT << psd->GetName() << " );";
            NEW_LINE;
        }

        NEW_LINE;
        CCOUT << "END_FACTORY_MAPS;";
        BLOCK_CLOSE;
        NEW_LINES( 2 );

        Wa( "extern \"C\"" );
        Wa( "gint32 DllLoadFactory( FactoryPtr& pFactory )" );
        BLOCK_OPEN;

        if( g_bMklib && bFuse )
            Wa( "InitMsgIds();" );

        Wa( "pFactory = InitClassFactory();" );
        CCOUT << "if( pFactory.IsEmpty() )";
        INDENT_UPL;
        CCOUT << "return -EFAULT;";
        INDENT_DOWNL;
        CCOUT << "return STATUS_SUCCESS;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

