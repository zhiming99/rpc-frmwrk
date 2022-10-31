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
    Wa( "return STATUS_SUCCESS;" );
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
    CCOUT << "else";
    NEW_LINE;
    BLOCK_OPEN;
    Wa( "ret = oReqCtx.GetPointer( " );
    Wa( "    propEventSink, pCallback );" );
    Wa( "if( ERROR( ret ) ) break;" );
    CCOUT << "qwReqId = pCallback->GetObjId();";
    BLOCK_CLOSE;
    NEW_LINE;
    Wa( "Json::Value oJsReq( objectValue );" );
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
    Wa( "Json::StreamWriterBuilder oBuilder;" );
    Wa( "oBuilder[\"commentStyle\"] = \"None\";" );
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
        CCOUT<< ": public " << strBase;

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

        CCOUT << "_MyVirtBase( pCfg ), "
            << "super( pCfg )";
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
        Wa( "    const stdstr&, const stdstr&, gint32, bool ) = 0;" );
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

        CCOUT<< ": public " << strBase;

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

        CCOUT << "_MyVirtBase( pCfg ), "
            << "super( pCfg )";
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

        Wa( "//Generated by ridlc" );
        Wa( "//Your task is to implement the following classes" );
        Wa( "//to get your rpc server work" );
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
            Wa( "return STATUS_SUCCESS;" );
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
            Wa( "return STATUS_SUCCESS;" );
            BLOCK_CLOSE;
            NEW_LINE;

            NEW_LINE;
            Wa( "gint32 CustomizeRequest(" );
            Wa( "    IConfigDb* pReqCfg," );
            Wa( "    IEventSink* pCallback ) override" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "do" );
            BLOCK_OPEN;
            Wa( "ret = super::CustomizeRequest(");
            Wa( "    pReqCfg, pCallback );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
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
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            NEW_LINE;
            Wa( "return ret;" );
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
            Wa( "return STATUS_SUCCESS;" );
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
            Wa( "return STATUS_SUCCESS;" );
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
            Wa( "    const stdstr& strIfName," );
            Wa( "    const stdstr& strMethod," );
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

            BLOCK_CLOSE;
            CCOUT << ";";
            NEW_LINES( 2 );
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

        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

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

        Wa( "Json::Value val_( objectValue );" );
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
            Wa( "if( !oMsg.isMember( JSON_ATTR_MSGTYPE ) ||");
            Wa( "    !oMsg[ JSON_ATTR_MSGTYPE ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strType =" );
            Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
            Wa( "if( strType != \"resp\" &&" );
            Wa( "    strType != \"evt\" )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "if( !oMsg.isMember( JSON_ATTR_IFNAME1 ) ||" );
            Wa( "    !oMsg[ JSON_ATTR_IFNAME1 ].isString() )" );
            Wa( "{ ret = -EINVAL; break; }" );
            Wa( "stdstr strIfName =" );
            Wa( "    oMsg[ JSON_ATTR_IFNAME1 ].asString();" );
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
            Wa( "    strTaskId.c_str(), nullptr, 10);\n" );
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
            Wa( "if( strMethod != \"KeepAlive\" )" );
            Wa( "{ ret = -ENOTSUP; break;}" );
            Wa( "ret = pSkel->OnKeepAlive( qwTaskId );" );
            CCOUT << "break;";
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
            Wa( "ret = CRpcServices::LoadObjDesc(" );
            CCOUT << "    \"./" << strAppName << "desc.json\",";
            NEW_LINE;
            CCOUT << "    \"" << strSvcName << "_SvrSkel\",";
            NEW_LINE;
            CCOUT << "    true, oCfg.GetCfg() );";
            NEW_LINE;
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
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
            Wa( "CParamList oCtx_( pContext );" );
            Wa( "auto pSkel = GetSkelPtr();" );
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
                CCOUT << "    oCtx_.GetCfg(), oReq, oResp );";
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
            Wa( "{ ret = -EINVAL; break; }" );
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
            Wa( "gint32 (*func)( CRpcServices* pIf, IEventSink*, guint64) =" );
            Wa("    ([]( CRpcServices* pIf, IEventSink* pThis, guint64 qwReqId )->gint32" );
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
            Wa( "Json::Value oJsResp( objectValue );" );
            Wa( "oJsResp[ JSON_ATTR_IFNAME1 ] =" );
            Wa( "    \"IInterfaceServer\";" );
            Wa( "oJsResp[ JSON_ATTR_METHOD ] =" );
            Wa( "    \"UserCancelRequest\";" );
            Wa( "oJsResp[ JSON_ATTR_MSGTYPE ] = \"resp\";" );
            Wa( "oJsResp[ JSON_ATTR_RETCODE ] = iRet;" );
            Wa( "oJsResp[ JSON_ATTR_REQCTXID ] =" );
            Wa( "    ( Json::UInt64& )qwReqId;" );
            Wa( "Json::StreamWriterBuilder oBuilder;" );
            Wa( "oBuilder[\"commentStyle\"] = \"None\";" );
            CCOUT <<  "stdstr strResp = Json::writeString( "
                << "oBuilder, oJsResp );";
            NEW_LINE;
            CCOUT << "CFuseSvcProxy* pClient = ObjPtr( pIf );";
            NEW_LINE;
            Wa( "pClient->ReceiveMsgJson( strResp, qwReqId );" );
            CCOUT << "";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return 0;";
            BLOCK_CLOSE;
            Wa( ");" );
            Wa( "TaskletPtr pCb;" );
            Wa( "ret = NEW_FUNCCALL_TASK2( 1, pCb," );
            Wa( "    this->GetIoMgr(), func," );
            Wa( "    this, nullptr, qwThisReq );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
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
            Wa( "ptw->SetCompleteTask( pCb );" );
            NEW_LINE;
            Wa( "auto pSkel = GetSkelPtr();" );
            Wa( "ret = pSkel->CancelReqAsync(" );
            Wa( "    ptw, qwTaskId );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "   ( *ptw )( eventCancelTask );" );
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
            Wa( "    this->ReceiveMsgJson( strReq, qwReqId );" );
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
            Wa( "ret = CRpcServices::LoadObjDesc(" );
            CCOUT << "    \"./" << strAppName << "desc.json\",";
            NEW_LINE;
            CCOUT << "    \"" << strSvcName << "_SvrSkel\",";
            NEW_LINE;
            CCOUT << "    false, oCfg.GetCfg() );";
            NEW_LINE;
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "ret = pIf.NewObj(" );
            CCOUT << "    clsid( C" << strSvcName << "_CliSkel ),";
            NEW_LINE;
            CCOUT << "    oCfg.GetCfg() );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );
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
        Wa( "    const stdstr& strIfName," );
        Wa( "    const stdstr& strMethod," );
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

        Wa( "gint32 ret = 0;" );
        CCOUT <<"do";
        BLOCK_OPEN;
        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 i = 0;
        pmds->GetCount();
        guint32 dwCount = pmds->GetCount();

        Wa( "if( !oMsg.isMember( JSON_ATTR_METHOD ) ||");
        Wa( "    !oMsg[ JSON_ATTR_METHOD ].isString() )" );
        Wa( "{ ret = -EINVAL; break; }" );
        Wa( "stdstr strMethod =" );
        Wa( "    oMsg[ JSON_ATTR_METHOD ].asString();" );
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

        Wa( "stdstr strType =" );
        Wa( "    oMsg[ JSON_ATTR_MSGTYPE ].asString();" );
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            CMethodDecl* pmd = pObj;
            if( pmd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ObjPtr pInArgs = pmd->GetInArgs();
            guint32 dwInCount = GetArgCount( pInArgs );
            ObjPtr pOutArgs = pmd->GetOutArgs();
            guint32 dwOutCount = GetArgCount( pOutArgs );
            CCOUT << "if( strMethod == \""
                << pmd->GetName() << "\" )";
            NEW_LINE;
            BLOCK_OPEN;
            if( !pmd->IsEvent() && !pmd->IsNoReply() )
            {
                Wa( "if( strType != \"resp\" )" );
                Wa( "{ ret = -EINVAL; break; }" );
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
            else if( pmd->IsEvent() )
            {
                Wa( "if( strType != \"evt\" )" );
                Wa( "{ ret = -EINVAL; break; }" );
                CCOUT << "ret = " << strClass << "::"
                    << pmd->GetName() <<"(";
                if( dwInCount > 0 ) 
                    CCOUT << " oMsg );";
                else
                    CCOUT << ");";
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
            NEW_LINE;
        }
        Wa( "ret = -ENOENT;" );
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
            bool bProxy = ( strSuffix == "cli" );
            if( bProxy )
            {
                m_pWriter->SelectMainCli();
            }
            else
            {
                m_pWriter->SelectMainSvr();
            }

            Wa( "// Generated by ridlc" );
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
            Wa( "gint32 InitContext()" );
            BLOCK_OPEN;
            Wa( "gint32 ret = CoInitialize( 0 );" );
            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "return ret;";
            INDENT_DOWNL;

            CCOUT << "do";
            BLOCK_OPEN;
            CCOUT << "// load class factory for '"
                << strModName << "'";
            NEW_LINE;
            CCOUT << "FactoryPtr p = "
                << "InitClassFactory();";
            NEW_LINE;
            CCOUT << "ret = CoAddClassFactory( p );";
            NEW_LINE;
            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "break;";
            INDENT_DOWNL;
            NEW_LINE;
            Wa( "CParamList oParams;" );
            CCOUT << "oParams.Push( \""
                << strModName + strSuffix << "\" );";
            NEW_LINES( 2 );

            Wa( "// adjust the thread number if necessary" );
            if( bProxy )
            {
                Wa( "oParams[ propMaxIrpThrd ] = 0;" );
                Wa( "oParams[ propMaxTaskThrd ] = 1;" );
            }
            else
            {
                Wa( "oParams[ propMaxIrpThrd ] = 2;" );
                Wa( "oParams[ propMaxTaskThrd ] = 2;" );
            }
            NEW_LINE;

            CCOUT << "ret = g_pIoMgr.NewObj(";
            INDENT_UPL;
            CCOUT << "clsid( CIoManager ), ";
            NEW_LINE;
            CCOUT << "oParams.GetCfg() );";
            INDENT_DOWNL;
            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "break;";
            INDENT_DOWNL;
            NEW_LINE;
            CCOUT << "IService* pSvc = g_pIoMgr;";
            NEW_LINE;
            CCOUT << "ret = pSvc->Start();";
            NEW_LINE;
            BLOCK_CLOSE;
            CCOUT << "while( 0 );";
            NEW_LINES( 2 );
            Wa( "if( ERROR( ret ) )" );
            BLOCK_OPEN;
            CCOUT << "g_pIoMgr.Clear();";
            NEW_LINE;
            CCOUT << "CoUninitialize();";
            BLOCK_CLOSE;
            NEW_LINE;
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            // DestroyContext
            Wa( "gint32 DestroyContext()" );
            BLOCK_OPEN;
            Wa( "IService* pSvc = g_pIoMgr;" );
            Wa( "if( pSvc != nullptr )" );
            BLOCK_OPEN;
            Wa( "pSvc->Stop();" );
            CCOUT << "g_pIoMgr.Clear();";
            BLOCK_CLOSE;
            NEW_LINES( 2 );
            Wa( "CoUninitialize();" );

            CCOUT << "DebugPrintEx( logErr, 0,";
            INDENT_UPL;
            CCOUT << "\"#Leaked objects is %d\",";
            NEW_LINE;
            CCOUT << "CObjBase::GetActCount() );";
            INDENT_DOWNL;
            CCOUT << "return STATUS_SUCCESS;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            // main function
            Wa( "int main( int argc, char** argv)" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            Wa( "CIoManager* pMgr = nullptr;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "fuse_args args = FUSE_ARGS_INIT(argc, argv);" );
            Wa( "fuse_cmdline_opts opts;" );
            Wa( "ret = fuseif_daemonize( args, opts, argc, argv );" );
            CCOUT << "if( ERROR( ret ) )";
            NEW_LINE;
            CCOUT << "    break;";
            NEW_LINES( 2 );
            Wa( "ret = InitContext();" );
            CCOUT << "if( ERROR( ret ) )";
            NEW_LINE;
            CCOUT << "    break;";
            NEW_LINES( 2 );

            Wa( " // set a must-have option for" );
            Wa( " // DBusStreamBusPort" );
            Wa( "pMgr = ( CIoManager* )g_pIoMgr;" );
            Wa( "CCfgOpener oDrvCfg;" );
            CCOUT << "oDrvCfg[ propIsServer ] = "
                 << ( bProxy ? "false" : "true" ) << " ;";
            NEW_LINE;
            CCOUT << "stdstr strDesc= " << "\"./"
                << g_strAppName << "desc.json\";";
            NEW_LINES( 2 );
            Wa( "oDrvCfg[ propObjDescPath ] = strDesc;" );
            Wa( "oDrvCfg[ propDrvName ] = \"DBusStreamBusDrv\";" );
            Wa( "auto& oDrvMgr = pMgr->GetDrvMgr();" );
            Wa( "ret = oDrvMgr.LoadDriver(" );
            Wa( "    oDrvCfg.GetCfg() );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            NEW_LINES(2);

            CCOUT << "ret = InitRootIf( g_pIoMgr, "
                << ( bProxy ? "true" : "false" ) << " );";
            NEW_LINE;
            CCOUT << "if( ERROR( ret ) )";
            NEW_LINE;
            CCOUT << "    break;";
            NEW_LINE;
            Wa( "CRpcServices* pRoot = GetRootIf();" );
            Wa( "do" );
            BLOCK_OPEN;
            for( auto& elem : vecSvcs )
            {
                CServiceDecl* pSvc = elem;
                stdstr strSvcName = pSvc->GetName();

                std::string strClass =
                    std::string( "C" ) + strSvcName;
                if( bProxy )
                    strClass += "_CliImpl";
                else
                    strClass += "_SvrImpl";

                CCOUT << "// add the "
                    << strSvcName << " directory";
                NEW_LINE;
                CCOUT << "ret = AddSvcPoint(";
                NEW_LINE;
                CCOUT << "    \""
                    << strSvcName << "\",";
                NEW_LINE;
                CCOUT << "    \"./"
                    << g_strAppName << "desc.json\",";
                NEW_LINE;
                CCOUT << "    clsid( " << strClass << " ),";
                NEW_LINE;
                CCOUT << "    " 
                    << ( bProxy ? "true" : "false" )
                    << " );";
                NEW_LINE;
                CCOUT << "if( ERROR( ret ) )";
                NEW_LINE;
                CCOUT << "    break;";
                NEW_LINE;
            }
            NEW_LINE;
            Wa( "args = FUSE_ARGS_INIT(argc, argv);" );
            Wa( "ret = fuseif_main( args, opts );" );
            BLOCK_CLOSE;
            Wa( "while( 0 );" );

            NEW_LINE;
            Wa( "// Stop the root object" );
            Wa( "pRoot->Stop();" );
            CCOUT << "ReleaseRootIf();";
            NEW_LINE;
            BLOCK_CLOSE;
            CCOUT<< "while( 0 );";
            NEW_LINES( 2 );
            Wa( "if( pMgr != nullptr )" );
            BLOCK_OPEN;
            Wa( "auto& oDrvMgr = pMgr->GetDrvMgr();" );
            Wa( "oDrvMgr.UnloadDriver(" );
            CCOUT << "    \"DBusStreamBusDrv\" );";
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "DestroyContext();" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES(2);
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

        CCOUT << "DECLARE_AGGREGATED_PROXY(";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "C" << strSvcName << "_CliSkel,";
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
        INDENT_DOWN;
        NEW_LINES( 2 );

        CCOUT << "DECLARE_AGGREGATED_SERVER(";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "C" << strSvcName << "_SvrSkel,";
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
            Wa( "CFastRpcProxyBase," );
            Wa( "CStatCountersProxy," );
            Wa( "CStreamProxyFuse," );
            Wa( "CFuseSvcProxy," );
            for( guint32 i = 0;
                i < vecIfs.size(); i++ )
            {
                CInterfRef* pifr = vecIfs[ i ];
                std::string strName = pifr->GetName();
                CCOUT << "I" << strName << "_CliApi";
                if( i + 1 < vecIfs.size() )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }
            }
            CCOUT << " );";
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

            Wa( "CFastRpcServerBase," );
            Wa( "CStatCountersServer," );
            Wa( "CStreamServerFuse," );
            Wa( "CFuseSvcServer," );

            for( guint32 i = 0;
                i < vecIfs.size(); i++ )
            {
                CInterfRef* pifr = vecIfs[ i ];
                std::string strName = pifr->GetName();
                CCOUT << "I" << strName << "_SvrApi";
                if( i + 1 < vecIfs.size() )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }
            }
            CCOUT << " );";
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
