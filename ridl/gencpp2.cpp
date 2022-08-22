/*
 * =====================================================================================
 *
 *       Filename:  gencpp2.cpp
 *
 *    Description:  implementations of code generation for rpc-over-stream  
 *
 *        Version:  1.0
 *        Created:  08/21/2022 08:35:06 AM
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

#include <sys/stat.h>
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

extern std::string g_strAppName;
extern bool g_bMklib;
extern stdstr g_strLang;
extern guint32 g_dwFlags;
extern bool g_bRpcOverStm;
extern stdstr g_strTarget;

gint32 CDeclInterfProxy2::OutputROSSkel()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "class " ); 
        std::string strClass = "I";
        strClass += strName + "_PImpl";
        CCOUT << strClass;
        std::string strBase =
            "CFastRpcSkelProxyBase";

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
        CCOUT << "super::_MyVirtBase( pCfg ), "
            << "super( pCfg )";
        NEW_LINE;
        CCOUT << "{}";
        INDENT_DOWN;
        NEW_LINE;
        Wa( "gint32 InitUserFuncs();" );
        NEW_LINE;
        Wa( "const EnumClsid GetIid() const override" );
        CCOUT << "{ return iid( "
            << strName << " ); }";
        NEW_LINES( 2 );

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
                ret = OutputEventROSSkel( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else if( !pmd->IsAsyncp() )
            {
                ret = OutputSync( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else 
            {
                ret = OutputAsyncROSSkel( pmd );
            }
            if( i + 1 < dwCount )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclInterfProxy2::OutputEventROSSkel(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;

    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pArgs = pmd->GetInArgs();
        CCOUT<< "//RPC event handler '" << strName <<"'";
        NEW_LINE;
        std::string strDecl =
            std::string( "gint32 " ) +
            strName + "(";
        CCOUT << strDecl;
        guint32 dwCount = GetArgCount( pArgs );

        INDENT_UPL;
        if( dwCount > 0 )
        {
            GenFormInArgs( pArgs );
            CCOUT << " ) = 0;";
        }
        else
        {
            CCOUT << ") = 0;";
        }
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "auto& pIf = GetStreamIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        Wa( "auto& pApi = dynamic_cast" );
        CCOUT << "< I" << m_pNode->GetName() << "_SvrApi* >";
        NEW_LINE;
        CCOUT << "    ( pSvc );";
        NEW_LINE;
        Wa( "if( pApi == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "return pApi->" << strName << "(";
        if( dwCount == 0 )
        {
            CCOUT << ");";
        }
        else
        {
            INDENT_UPL;
            GenActParams( pArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }
        NEW_LINE;
        Wa( "return ret;" );
        BLOCK_CLOSE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy2::OutputAsyncROSSkel(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount = GetArgCount( pInArgs );
        guint32 dwOutCount = GetArgCount( pOutArgs );

        Wa( "//RPC Async Req Sender" );
        CCOUT << "gint32 " << strName << "( ";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "IConfigDb* context";
        bool bComma = false;
        if( dwInCount + dwOutCount == 0 )
            CCOUT << " ";
        if( dwInCount > 0 )
        {
            bComma = true;
            CCOUT << ", ";
            NEW_LINE;
            GenFormInArgs( pInArgs );
            if( dwOutCount == 0 )
                CCOUT << " ";
        }
        if( dwOutCount > 0 )
        {
            if( bComma )
            {
                CCOUT << ", ";
                NEW_LINE;
            }
            GenFormOutArgs( pOutArgs );
            CCOUT << " ";
        }

        CCOUT << ");";
        INDENT_DOWN;

        if( pmd->IsSerialize() && dwInCount > 0 )
        {
            NEW_LINES( 2 );
            CCOUT << "gint32 " << strName
                << "Dummy( BufPtr& pBuf_ )";
            NEW_LINE;
            CCOUT << "{ return STATUS_SUCCESS; }";
        }

        NEW_LINE;

        NEW_LINE;
        Wa( "//Async callback wrapper" );
        CCOUT << "gint32 " << strName
            << "CbWrapper" << "( ";
        INDENT_UPL;

        Wa( "IEventSink* pCallback, " );
        Wa( "IEventSink* pIoReq," );
        CCOUT <<  "IConfigDb* pReqCtx );";

        INDENT_DOWNL;
        NEW_LINE;

        Wa( "//RPC Async Req Callback" );
        Wa( "//TODO: implement me by adding" );
        Wa( "//response processing code" );

        std::string strDecl;
        strDecl += "gint32 " + strName
            + "Callback" + "(";
        CCOUT << strDecl;
        strDecl += "IConfigDb* context, gint32 iRet";

        if( dwOutCount > 0 )
        {
            INDENT_UPL;
            CCOUT << "IConfigDb* context, ";
            NEW_LINE;
            CCOUT << "gint32 iRet,";
            strDecl += ",";
            NEW_LINE;
            GenFormInArgs( pOutArgs );
            CCOUT << " )";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "IConfigDb* context, ";
            NEW_LINE;
            CCOUT << "gint32 iRet )";
            INDENT_DOWN;
        }
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "auto& pIf = GetStreamIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        Wa( "auto& pApi = dynamic_cast" );
        CCOUT << "< I" << m_pNode->GetName() << "_CliApi* >";
        NEW_LINE;
        CCOUT << "    ( pSvc );";
        NEW_LINE;
        Wa( "if( pApi == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "return pApi->" << strName << "(";
        if( dwOutCount == 0 )
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_, iRet );";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_, iRet,";
            NEW_LINE;
            GenActParams( pOutArgs );
            CCOUT << " );";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy2::OutputROS()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "class " ); 
        std::string strClass = "I";
        strClass += strName + "_CliApi";
        CCOUT << strClass;
        stdstr strSkel = "I";
        strSkel += strName + "_PImpl";
        std::string strBase;
        bool bStream = m_pNode->IsStream();
        if( bStream )
        {
            strBase = "CStreamProxyWrapper"; 
        }
        else
        {
            strBase = "CAggInterfaceProxy"; 
        }

        INDENT_UP;
        NEW_LINE;
        if( bStream )
            CCOUT<< ": public " << strBase;
        else
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
        if( m_pNode->IsStream() )
        {
            CCOUT << "super::super( pCfg ), "
                << "super( pCfg )";
            NEW_LINE;
            CCOUT << "{}";
        }
        else
        {
            CCOUT << "super( pCfg )";
            NEW_LINE;
            CCOUT << "{}";
        }
        INDENT_DOWN;
        NEW_LINE;

        CCOUT << "inline " << strSkel <<"* GetSkelPtr()";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "auto& pCli = dynamic_cast" );
        Wa( "< CFastRpcProxyBase* >( this )" );
        Wa( "if( pCli == nullptr )" );
        Wa( "    return nullptr;" );
        Wa( "InterfPtr pIf = pCli->GetStmSkel();" );
        Wa( "if( pIf->IsEmpty() )" );
        Wa( "    return nullptr;" );
        Wa( "auto& pSkel = dynamic_cast" );
        CCOUT << "<" << strSkel << "*>(";
        NEW_LINE;
        Wa( "    ( CRpcServices* )pIf );" );
        CCOUT << "return pSkel;";
        BLOCK_CLOSE;
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
                ret = OutputEventROS( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else if( !pmd->IsAsyncp() )
            {
                ret = OutputSyncROS( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else 
            {
                ret = OutputAsyncROS( pmd );
            }
            if( i + 1 < dwCount )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclInterfProxy2::OutputEventROS(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;

    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pArgs = pmd->GetInArgs();
        CCOUT<< "//RPC event handler '" << strName <<"'";
        NEW_LINE;
        std::string strDecl =
            std::string( "gint32 " ) +
            strName + "(";
        CCOUT << strDecl;
        guint32 dwCount = GetArgCount( pArgs );

        INDENT_UPL;
        if( dwCount > 0 )
        {
            GenFormInArgs( pArgs );
            CCOUT << " );";
        }
        else
        {
            CCOUT << ");";
        }
        INDENT_DOWNL;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy2::OutputSyncROS(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount =
            dwInCount + dwOutCount;

        Wa( "//RPC Sync Req Sender" );
        CCOUT << "gint32 " << strName << "(";

        bool bComma = false;
        if( dwCount == 0 )
        {
            CCOUT << ")";
            NEW_LINE;
        }
        else
        {
            INDENT_UPL;
            if( dwInCount > 0 )
            {
                GenFormInArgs( pInArgs );
                bComma = true;
            }
            if( dwOutCount > 0 )
            {
                if( bComma )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }

                GenFormOutArgs( pOutArgs );
            }

            CCOUT << " )";
            INDENT_DOWNL;
        }
        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );
        Wa( "auto& pSkel = GetSkelPtr(); " );
        Wa( "if( pSkel == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "pSkel->" << strName << "(";
        NEW_LINE;
        if( dwInCount == 0 )
        {
            CCOUT << ");";
        }
        else
        {
            INDENT_UPL;
            GenActParams( pInArgs, pOutArgs, false );
            CCOUT << " )";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy2::OutputAsyncROS(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount = GetArgCount( pInArgs );
        guint32 dwOutCount = GetArgCount( pOutArgs );

        Wa( "//RPC Async Req Sender" );
        CCOUT << "gint32 " << strName << "( ";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "IConfigDb* context";
        bool bComma = false;
        if( dwInCount + dwOutCount == 0 )
            CCOUT << " ";
        if( dwInCount > 0 )
        {
            bComma = true;
            CCOUT << ", ";
            NEW_LINE;
            GenFormInArgs( pInArgs );
            if( dwOutCount == 0 )
                CCOUT << " ";
        }
        if( dwOutCount > 0 )
        {
            CCOUT << ", ";
            NEW_LINE;
            GenFormOutArgs( pOutArgs );
            CCOUT << " ";
        }

        CCOUT << ")";
        INDENT_DOWN;
        BLOCK_OPEN;
        NEW_LINE;
        Wa( "gint32 ret = 0;" );
        Wa( "auto& pIf = GetSkelPtr()" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        Wa( "auto& pSkel = dynamic_cast" );
        CCOUT << "< I" << m_pNode->GetName() << "_pImpl* >";
        NEW_LINE;
        CCOUT << "    ( pSvc );";
        NEW_LINE;
        Wa( "if( pWrapper == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "ret = pSkel->" << strName << "(";
        if( dwOutCount == 0 )
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_, iRet );";
            INDENT_DOWNL;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_, iRet,";
            NEW_LINE;
            GenActParams( pInArgs, pOutArgs, false );
            CCOUT << " )";
            INDENT_DOWNL;
        }
        BLOCK_CLOSE;
        NEW_LINE;


        Wa( "// RPC Async Req Callback" );

        std::string strDecl;
        strDecl += "gint32 " + strName
            + "Callback" + "(";
        CCOUT << strDecl;
        strDecl += "IConfigDb* context, gint32 iRet";

        if( dwOutCount > 0 )
        {
            INDENT_UPL;
            CCOUT << "IConfigDb* context, ";
            NEW_LINE;
            CCOUT << "gint32 iRet,";
            strDecl += ",";
            GenFormInArgs( pOutArgs );
            CCOUT << " ) = 0;";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "IConfigDb* context, ";
            NEW_LINE;
            CCOUT << "gint32 iRet );";
            INDENT_DOWN;
        }
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr2::OutputROSSkel()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "class " ); 
        std::string strClass = "I";
        strClass += strName + "_SImpl";
        CCOUT << strClass;
        std::string strBase =
            "CFastRpcSkelSvrBase";
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
        CCOUT << "super::_MyVirtBase( pCfg ), "
            << "super( pCfg )";
        NEW_LINE;
        CCOUT << "{}";
        INDENT_DOWNL;
        Wa( "gint32 InitUserFuncs();" );
        NEW_LINE;
        Wa( "const EnumClsid GetIid() const override" );
        CCOUT<< "{ return iid( "
            << strName <<" ); }";
        NEW_LINES( 2 );

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
            else if( !pmd->IsAsyncs() )
            {
                ret = OutputSyncROSSkel( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else 
            {
                ret = OutputAsyncROSSkel( pmd );
            }
            if( i + 1 < dwCount )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclInterfSvr2::OutputROS()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "class " ); 
        std::string strClass = "I";
        strClass += strName + "_SvrApi";

        std::string strSkel = "I";
        strSkel += strName + "_SImpl";

        CCOUT << strClass;
        std::string strBase =
            "CAggInterfaceServer";
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
        CCOUT << "inline " << strSkel
            << "* GetSkelPtr( HANDLE hstm )";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "auto& pSvr = dynamic_cast" );
        Wa( "< CFastRpcServerBase* >( this )" );
        Wa( "if( pSvr == nullptr )" );
        Wa( "    return nullptr;" );
        Wa( "ret = pSvr->GetStmSkel(" );
        Wa( "    hstm, pIf );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    return nullptr;" );
        Wa( "auto& pSkel = dynamic_cast" );
        CCOUT << "<" << strSkel << "*>(";
        NEW_LINE;
        Wa( "    ( CRpcServices* )pIf );" );
        CCOUT << "return pSkel;";
        BLOCK_CLOSE;
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
                ret = OutputEventROS( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else if( !pmd->IsAsyncs() )
            {
                ret = OutputSyncROS( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else 
            {
                ret = OutputAsyncROS( pmd );
            }
            if( i + 1 < dwCount )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclInterfSvr2::OutputEventROS(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;

    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pArgs = pmd->GetInArgs();
        Wa( "//RPC event sender" );
        CCOUT << "gint32 " << strName << "(";
        guint32 dwInCount = GetArgCount( pArgs );
        if( dwInCount > 0 )
        {
            INDENT_UPL;
            GenFormInArgs( pArgs );
            CCOUT << " )";
            INDENT_DOWN;
        }
        else
        {
            CCOUT << " )";
        }

        stdstr strIfName = m_pNode->GetName();

        NEW_LINE;
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "auto& pSkel = GetSkelPtr(" );
        Wa( "    INVALID_HANDLE ); " );
        Wa( "if( pSkel == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "pSkel->" << strName << "(";
        NEW_LINE;
        if( dwInCount == 0 )
        {
            CCOUT << ");";
        }
        else
        {
            INDENT_UPL;
            GenActParams( pArgs, false );
            CCOUT << " )";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr2::OutputSyncROSSkel(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();

        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount = dwInCount + dwOutCount;

        Wa( "//RPC Sync Req Handler Wrapper" );
        CCOUT << "gint32 " << strName << "Wrapper(";
        INDENT_UPL;
        CCOUT << "IEventSink* pCallback";

        if( pmd->IsSerialize() &&
            dwInCount > 0 )
        {
            CCOUT << ", BufPtr& pBuf_ );";
        }
        else if( dwInCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormInArgs( pInArgs );
            CCOUT << " );"; 
        }
        else
        {
            CCOUT << " );"; 
        }
        INDENT_DOWNL;
        NEW_LINE;

        Wa( "//RPC Sync Req Handler" );
        std::string strDecl =
            std::string( "gint32 " )
            + strName + "(";
        CCOUT << strDecl;
        if( dwCount == 0 )
        {
            CCOUT << ")";
        }
        else
        {
            INDENT_UPL;
            bool bComma = false;
            if( dwInCount > 0 )
            {
                bComma = true;
                GenFormInArgs( pInArgs );
            }
            if( dwOutCount > 0 )
            {
                if( bComma )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }
                GenFormOutArgs( pOutArgs );
            }
            CCOUT << " )";
            INDENT_DOWN;
        }
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "auto& pIf = GetStreamIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        Wa( "auto& pApi = dynamic_cast" );
        CCOUT << "    < I" << m_pNode->GetName() << "_SvrApi* >";
        NEW_LINE;
        CCOUT << "    ( pSvc );";
        NEW_LINE;
        Wa( "if( pApi == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "return pApi->" << strName << "(";
        if( dwInCount == 0 )
        {
            CCOUT << ");";
        }
        else
        {
            INDENT_UPL;
            GenActParams( pInArgs, pOutArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr2::OutputSyncROS(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();

        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount = dwInCount + dwOutCount;

        Wa( "//RPC Sync Req Handler" );
        std::string strDecl =
            std::string( "gint32 " )
            + strName + "(";
        CCOUT << strDecl;
        if( dwCount == 0 )
        {
            CCOUT << ");";
        }
        else
        {
            INDENT_UPL;
            bool bComma = false;
            if( dwInCount > 0 )
            {
                bComma = true;
                GenFormInArgs( pInArgs );
            }
            if( dwOutCount > 0 )
            {
                if( bComma )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }
                GenFormOutArgs( pOutArgs );
            }
            CCOUT << " );";
            INDENT_DOWN;
        }
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr2::OutputAsyncROSSkel(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

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
        else if( pmd->IsSerialize() )
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
        else
        {
            Wa( "//RPC Async Req Handler wrapper" );
            CCOUT << "gint32 "
                << strName << "Wrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback,";
            NEW_LINE;
            GenFormInArgs( pInArgs );
            CCOUT << " );";
            INDENT_DOWNL;
            NEW_LINE;

            CCOUT << "gint32 "
                << strName << "CancelWrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback,";
            NEW_LINE;
            Wa( "gint32 iRet," );
            CCOUT << "IConfigDb* pReqCtx_,";
            NEW_LINE;
            GenFormInArgs( pInArgs );
            CCOUT << " );";
            INDENT_DOWNL;
            NEW_LINE;
        }

        Wa( "//RPC Async Req Cancel Handler" );
        CCOUT << "gint32 "
            << "On" << strName << "Canceled(";
        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_, gint32 iRet";
        if( dwInCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormInArgs( pInArgs );
        }
        CCOUT << " )";
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "auto& pIf = GetStreamIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        Wa( "auto& pApi = dynamic_cast" );
        CCOUT << "    < I" << m_pNode->GetName() << "_SvrApi* >";
        NEW_LINE;
        CCOUT << "    ( pSvc );";
        NEW_LINE;
        Wa( "if( pApi == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "return pApi->On" << strName << "Canceled(";
        if( dwInCount == 0 )
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_, iRet );";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_, iRet,";
            GenActParams( pInArgs );
            CCOUT << " );";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;
        NEW_LINES( 2 );

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
            GenFormInArgs( pOutArgs );
        }
        CCOUT << " )";
        INDENT_DOWNL;
        NEW_LINE;

        Wa( "//RPC Async Req Handler" );
        Wa( "//TODO: adding code to emit your async" );
        Wa( "//operation, keep a copy of pCallback and" );
        Wa( "//return STATUS_PENDING" );
        std::string strDecl =
            std::string( "gint32 " ) +
            strName + "(";

        CCOUT << strDecl;
        strDecl += "IConfigDb* pReqCtx_";

        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_";
        if( dwCount > 0 )
        {
            strDecl += ",";
            CCOUT << ",";
        }

        if( dwInCount > 0 )
        {
            NEW_LINE;
            GenFormInArgs( pInArgs );
        }
        if( dwOutCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormOutArgs( pOutArgs );
        }
        CCOUT << " )";
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "auto& pIf = GetStreamIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        Wa( "auto& pApi = dynamic_cast" );
        CCOUT << "< I" << m_pNode->GetName() << "_SvrApi* >";
        NEW_LINE;
        CCOUT << "    ( pSvc );";
        NEW_LINE;
        Wa( "if( pApi == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "return pApi->" << strName << "(";
        if( dwInCount == 0 )
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_ );";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_,";
            GenActParams( pInArgs, pOutArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr2::OutputAsyncROS(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount = dwInCount + dwOutCount;

        Wa( "//RPC Async Req Cancel Handler" );
        CCOUT << "gint32 "
            << "On" << strName << "Canceled(";
        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_, gint32 iRet";
        if( dwInCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormInArgs( pInArgs );
        }
        CCOUT << " )";
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "return STATUS_SUCCESS" );
        BLOCK_CLOSE;
        NEW_LINE;

        Wa( "//RPC Async Req Complete helper" );
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
            GenFormInArgs( pOutArgs );
        }
        CCOUT << " )";
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "do" );
        BLOCK_OPEN;
        Wa( "CCfgOpener oReqCtx( pReqCtx_ );" );
        Wa( "IEventSink* pEvt = nullptr;" );
        Wa( "ret = oReqCtx.GetPointer(" );
        Wa( "    propEventSink, pEvt );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        Wa( "auto pSvr_ = dynamic_cast" );
        Wa( "<CFastRpcServerBase*>( this )" );
        Wa( "HANDLE hstm = INVALID_HANDLE;" );
        Wa( "ret = pSvr_->GetStream( pEvt, hstm );");
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        Wa( "auto pSkel = GetSkelPtr( hstm );" );
        CCOUT << "pSkel->" << strName << "Complete(";
        NEW_LINE;
        if( dwOutCount == 0 )
            Wa( "    pReqCtx_, iRet );" );
        else
        {
            Wa( "    pReqCtx_, iRet, );" );
            INDENT_UPL;
            GenActParams( pOutArgs );
            CCOUT << " );";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        BLOCK_CLOSE;
        NEW_LINE;

        Wa( "//RPC Async Req Handler" );
        std::string strDecl =
            std::string( "gint32 " ) +
            strName + "(";

        CCOUT << strDecl;
        strDecl += "IConfigDb* pReqCtx_";

        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_";
        if( dwCount > 0 )
        {
            strDecl += ",";
            CCOUT << ",";
        }

        if( dwInCount > 0 )
        {
            NEW_LINE;
            GenFormInArgs( pInArgs );
        }
        if( dwOutCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormOutArgs( pOutArgs );
        }
        CCOUT << " );";
        INDENT_DOWNL;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CImplIfMethodSvr2::OutputROS()
{
    gint32 ret = 0;

    do{
        NEW_LINE;

        if( m_pNode->IsEvent() )
        {
            ret = OutputEventROS();
            break;
        }
        else if( m_pNode->IsAsyncs() )
        {
            ret = OutputAsyncROS();
            if( ERROR( ret ) )
                break;
        }
        else
        {
            ret = OutputSyncROS();
        }

    }while( 0 );

    return ret;
}

gint32 CImplIfMethodSvr2::OutputEventROS()
{ return 0; }

gint32 CImplIfMethodSvr2::OutputSyncROS()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SvrApi";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );
    guint32 dwCount = dwInCount + dwOutCount;

    bool bSerial = m_pNode->IsSerialize();
    bool bNoReply = m_pNode->IsNoReply();

    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "(";
        if( dwCount == 0 )
        {
            CCOUT << ")";
        }
        else
        {
            INDENT_UPL;
            bool bComma = false;
            if( dwInCount > 0 )
            {
                bComma = true;
                GenFormInArgs( pInArgs );
            }
            if( dwOutCount > 0 )
            {
                if( bComma )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }
                GenFormOutArgs( pOutArgs );
            }
            CCOUT << " )";
            INDENT_DOWNL;
        }
        BLOCK_OPEN;
        Wa( "// Sync Req Handler " );
        Wa( "// TODO: Process the sync request here" );
        Wa( "// return code can be an Error or" );
        Wa( "// STATUS_SUCCESS" );
        Wa( "return ERROR_NOT_IMPL;" );
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvr2::OutputAsyncROS()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SvrApi";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bNoReply = m_pNode->IsNoReply();
    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( ";
        INDENT_UPL;
        if( dwInCount > 0 )
        {
            bool bComma = false;
            CCOUT << "IConfigDb* pContext";
            if( dwInCount > 0 )
            {
                bComma = true;
                CCOUT << ", ";
                NEW_LINE;
                GenFormInArgs( pInArgs );
                if( dwOutCount == 0 )
                    CCOUT << " ";
            }
            if( dwOutCount > 0 )
            {
                if( bComma )
                {
                    CCOUT << ", ";
                    NEW_LINE;
                }
                GenFormOutArgs( pOutArgs );
                CCOUT << " ";
            }
            CCOUT << ");";
        }
        else
            CCOUT << "IConfigDb* pContext )";

        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "// Async Req handler" );
        Wa( "// TODO: Emit an async operation here." );
        CCOUT << "// And make sure to call '" << strMethod << "Complete'";
        NEW_LINE;
        Wa( "// when the service is done" );
        CCOUT << "return ERROR_NOT_IMPL;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodProxy2::OutputROSSkel()
{ return 0; }

gint32 CImplIfMethodProxy2::OutputROS()
{
    gint32 ret = 0;

    do{
        NEW_LINE;

        if( m_pNode->IsEvent() )
        {
            ret = OutputEventROS();
            break;
        }
        else if( m_pNode->IsAsyncp() )
        {
            ret = OutputAsyncROS();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

gint32 CImplIfMethodProxy2::OutputEventROS()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_CliApi";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwCount = dwInCount;

    bool bSerial = m_pNode->IsSerialize();

    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( ";
        if( dwCount > 0 )
        {
            INDENT_UPL;
            GenFormInArgs( pInArgs );
            CCOUT << " )";
            INDENT_DOWN;
        }
        else
        {
            CCOUT << " )";
        }
        NEW_LINE;

        BLOCK_OPEN;

        Wa( "// Event handler" );
        Wa( "// TODO: Process the event here" );
        CCOUT << "return STATUS_SUCCESS;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodProxy2::OutputAsyncROS()
{
    gint32 ret = 0;
    std::string strClass = "I";
    std::string strIfName = m_pIf->GetName();
    strClass += strIfName + "_CliApi";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bSerial = m_pNode->IsSerialize();
    bool bNoReply = m_pNode->IsNoReply();

    do{
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Callback" << "( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IConfigDb* context, gint32 iRet";
        if( dwOutCount > 0 )
        {
            CCOUT<< ",";
            NEW_LINE;
            GenFormInArgs( pOutArgs );
        }
        CCOUT << " )";
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "// Async callback handler " );
        Wa( "// TODO: Process the server response here" );
        Wa( "// return code ignored" );
        CCOUT << "return 0;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CDeclService2::OutputROSSkel()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();
        CCOUT << "DECLARE_AGGREGATED_PROXY(";
        INDENT_UP;
        NEW_LINE;

        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

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

gint32 CDeclService2::OutputROS( bool bServer )
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

        if( bServer )
        {
            CCOUT << "DECLARE_AGGREGATED_SERVER(";
            INDENT_UP;
            NEW_LINE;
            CCOUT << "C" << strSvcName << "_SvrBase,";
            NEW_LINE;

            if( m_pNode->IsStream() )
                Wa( "CStreamServerAsync," );
            Wa( "CFastRpcServerBase," );
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
        else
        {
            CCOUT << "DECLARE_AGGREGATED_PROXY(";
            INDENT_UP;
            NEW_LINE;

            CCOUT << "C" << strSvcName << "_CliBase,";
            NEW_LINE;
            if( m_pNode->IsStream() )
                Wa( "CStreamProxyAsync," );
            Wa( "CFastRpcProxyBase," );
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

    }while( 0 );

    return ret;
}

gint32 CDeclServiceImpl2::OutputROS()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();

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

        std::vector< ObjPtr > vecIfrs;
        m_pNode->GetIfRefs( vecIfrs );
        std::vector< ObjPtr > vecIfs;
        for( auto& elem : vecIfrs )
        {
            ObjPtr pObj;
            CInterfRef* pifr = elem;
            gint32 iRet = pifr->GetIfDecl( pObj );
            if( SUCCEEDED( iRet ) )
                vecIfs.push_back( pObj );
        }

        auto pWriter = static_cast
            < CCppWriter* >( m_pWriter );
        for( auto pifd : vecIfs )
        {
            if( IsServer() )
            {
                CDeclInterfSvr2 odifs(
                    pWriter, pifd );
                odifs.OutputROS();
            }
            else
            {
                CDeclInterfProxy2 odifp(
                    pWriter, pifd );
                odifp.OutputROS();
            }
        }

        ObjPtr pNode( m_pNode );
        CDeclService2 ods( pWriter, pNode );
        ods.OutputROS( IsServer() );

        std::string strClass, strBase;
        if( !IsServer() )
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
            CCOUT << "super::_MyVirtBase( pCfg ), "
                << "super( pCfg )";
            INDENT_DOWN;
            NEW_LINE;
            CCOUT << "{ SetClassId( clsid(" << strClass << " ) ); }";
            NEW_LINES( 2 );

            if( m_pNode->IsStream() )
            {
                Wa( "/* The following 2 methods are important for */" );
                Wa( "/* streaming transfer. rewrite them if necessary */" );
                Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
                Wa( "{ return super::OnStreamReady( hChannel ); } " );
                NEW_LINE;
                Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
                Wa( "{ return super::OnStmClosing( hChannel ); }" );
                NEW_LINE;
            }

            Wa( "gint32 CreateStmSkel(" );
            Wa( "    HANDLE hStream ) override;" );

            BLOCK_CLOSE;
            CCOUT << ";";
            NEW_LINES( 2 );
        }

        if( !IsServer() )
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
        CCOUT << "super::_MyVirtBase( pCfg ), "
            << "super( pCfg )";
        INDENT_DOWN;
        NEW_LINE;
        CCOUT << "{ SetClassId( clsid(" << strClass << " ) ); }";
        NEW_LINES( 2 );

        if( m_pNode->IsStream() )
        {
            Wa( "/* The following 3 methods are important for */" );
            Wa( "/* streaming transfer. rewrite them if necessary */" );
            Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
            Wa( "{ return super::OnStreamReady( hChannel ); } " );
            NEW_LINE;
            Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
            Wa( "{ return super::OnStmClosing( hChannel ); }" );
            NEW_LINE;
            Wa( "gint32 AcceptNewStream(" );
            Wa( "    IEventSink* pCb, IConfigDb* pDataDesc ) override" );
            Wa( "{ return STATUS_SUCCESS; }" );
            NEW_LINE;
        }

        Wa( "gint32 CreateStmSkel(" );
        Wa( "    HANDLE, InterfPtr& ) override;" );

        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CImplServiceImpl2::OutputROS()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();

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

        std::vector< ObjPtr > vecIfrs;
        m_pNode->GetIfRefs( vecIfrs );
        std::vector< ObjPtr > vecIfs;
        for( auto& elem : vecIfrs )
        {
            ObjPtr pObj;
            CInterfRef* pifr = elem;
            gint32 iRet = pifr->GetIfDecl( pObj );
            if( SUCCEEDED( iRet ) )
                vecIfs.push_back( pObj );
        }

        for( auto& elem : vecIfs )
        {
            ObjPtr pmdl;
            CInterfaceDecl* pifd = elem;
            pmdl = pifd->GetMethodList();
            if( pmdl.IsEmpty() )
                continue;
            CMethodDecls* pmds = pmdl;
            guint32 i = 0;
            pmds->GetCount();
            guint32 dwCount = pmds->GetCount();
            auto pWriter = static_cast
                < CCppWriter* >( m_pWriter );
            for( ; i < dwCount; i++ )
            {
                ObjPtr pObj = pmds->GetChild( i );
                if( IsServer() )
                {
                    CImplIfMethodSvr2 iims(
                        pWriter, pObj );
                    iims.OutputROS();
                }
                else
                {
                    CImplIfMethodProxy2 iimp(
                        pWriter, pObj );
                    iimp.OutputROS();
                }
            }
        }

        stdstr strClass = "C";
        if( IsServer() )
        {
            strClass += strSvcName + "_SvrImpl";
            CCOUT << "gint32 " << strClass << "::CreateStmSkel(";
            NEW_LINE;
            Wa( "    HANDLE hStream, InterfPtr& pIf )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "CCfgOpener oCfg;" );
            Wa( "oCfg[ propIsServer ] = true;" );
            Wa( "oCfg.SetIntPtr( propStmHandle," );
            Wa( "    ( guint32*& )hStream );" );
            Wa( "ret = CRpcServices::LoadObjDesc(" );
            CCOUT << "    \"./" << strAppName << "desc.json\",";
            NEW_LINE;
            CCOUT << "    \"" << "C" << strSvcName << "_SvrSkel\",";
            NEW_LINE;
            CCOUT << "    true, oCfg.GetCfg() )";
            NEW_LINE;
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "ret = pIf.NewObj(" );
            CCOUT << "    clsid( C" << strSvcName << "_SvrSkel,";
            NEW_LINE;
            CCOUT << "    oCfg.GetCfg() );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
        }
        else
        {
            strClass += strSvcName + "_CliImpl";
            CCOUT << "gint32 " << strClass << "::CreateStmSkel(";
            NEW_LINE;
            Wa( "    InterfPtr& pIf )" );
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "CCfgOpener oCfg;" );
            Wa( "oCfg[ propIsServer ] = false;" );
            Wa( "ret = CRpcServices::LoadObjDesc(" );
            CCOUT << "    \"./" << strAppName << "desc.json\",";
            NEW_LINE;
            CCOUT << "    \"" << "C" << strSvcName << "_SvrSkel\",";
            NEW_LINE;
            CCOUT << "    false, oCfg.GetCfg() )";
            NEW_LINE;
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "ret = pIf.NewObj(" );
            CCOUT << "    clsid( C" << strSvcName << "_CliSkel,";
            NEW_LINE;
            CCOUT << "    oCfg.GetCfg() );";
            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
        }

    }while( 0 );

    return ret;
}

gint32 CImplMainFunc2::OutputROS()
{
    gint32 ret = 0;

    do{
        CStatements* pStmts = m_pNode;
        std::vector< ObjPtr > vecSvcs;
        pStmts->GetSvcDecls( vecSvcs );
        CServiceDecl* pSvc = vecSvcs[ 0 ];
        std::string strSvcName = pSvc->GetName();
        std::string strModName = g_strTarget;
        stdstr strSuffix;
        if( m_bProxy )
            strSuffix = "cli";
        else
            strSuffix = "svr";
        {
            bool bProxy = ( strSuffix == "cli" );
            std::string strClass =
                std::string( "C" ) + strSvcName;
            if( bProxy )
            {
                strClass += "_CliImpl";
                m_pWriter->SelectMainCli();
            }
            else
            {
                strClass += "_SvrImpl";
                m_pWriter->SelectMainSvr();
            }

            Wa( "// Generated by ridlc" );
            Wa( "#include \"rpc.h\"" );
            Wa( "#include \"proxy.h\"" );
            Wa( "using namespace rpcf;" );
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
            CImplClassFactory oicf(
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

            //  business logic
            if( bProxy )
            {
                CCOUT << "gint32 maincli(";
                INDENT_UPL;
                CCOUT << strClass << "* pIf,";
                NEW_LINE;
                CCOUT << "int argc, char** argv );";
                INDENT_DOWNL;
            }
            else
            {
                CCOUT << "gint32 mainsvr( ";
                INDENT_UPL;
                CCOUT << strClass << "* pIf,";
                NEW_LINE;
                CCOUT << "int argc, char** argv );";
                INDENT_DOWNL;
            }
            NEW_LINES( 1 );

            // main function
            if( g_bMklib )
            {
                stdstr strSuffix;
                if( bProxy )
                    strSuffix = "cli";
                else
                    strSuffix = "svr";
                CCOUT << "int Start" << strSuffix
                    << "( int argc, char** argv)";
                NEW_LINE;
            }
            else
            {
                Wa( "int main( int argc, char** argv)" );
            }
            BLOCK_OPEN;
            Wa( "gint32 ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "ret = InitContext();" );
            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "break;";
            INDENT_DOWNL;
            NEW_LINE;
            Wa( " // set a must-have option for" );
            Wa( " // CDBusStreamBusPort" );
            Wa( "g_pIoMgr->SetCmdLineOpt(" );
            Wa( "    propIsServer, !bProxy );" );
            Wa( "auto& oDrvMgr = g_pIoMgr->GetDrvMgr();" );
            Wa( "ret = oDrvMgr.LoadDriver(" );
            Wa( "    \"CDBusStreamBusDrv\" );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "InterfPtr pIf;" );
            Wa( "CParamList oParams;" );
            Wa( "oParams[ propIoMgr ] = g_pIoMgr;" );
            CCOUT << "ret = CRpcServices::LoadObjDesc(";
            INDENT_UPL;
            CCOUT << "\"./"
                << g_strAppName << "desc.json\",";
            NEW_LINE;
            CCOUT << "\"" << strSvcName << "\",";
            NEW_LINE;
            if( bProxy )
                CCOUT << "false, oParams.GetCfg() );";
            else
                CCOUT << "true, oParams.GetCfg() );";

            INDENT_DOWNL;

            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "break;";
            INDENT_DOWNL;
            NEW_LINE;

            CCOUT << "ret = pIf.NewObj(";
            INDENT_UPL;
            CCOUT << "clsid( " << strClass << " ),";
            NEW_LINE;
            CCOUT << "oParams.GetCfg() );";
            INDENT_DOWNL;

            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "break;";
            INDENT_DOWNL;
            NEW_LINE;
            CCOUT << strClass << "* pSvc = pIf;";
            NEW_LINE;
            CCOUT << "ret = pSvc->Start();";
            NEW_LINE;

            CCOUT << "if( ERROR( ret ) )";
            INDENT_UPL;
            CCOUT << "break;";
            INDENT_DOWNL;

            if( !bProxy )
            {
                NEW_LINE;
                Wa( "mainsvr( pSvc, argc, argv );" );
            }
            else
            {
                NEW_LINE;
                CCOUT << "while( pSvc->GetState()"
                    << "== stateRecovery )";
                INDENT_UPL;
                CCOUT << "sleep( 1 );";
                INDENT_DOWNL;
                NEW_LINE;
                CCOUT << "if( pSvc->GetState() != "
                    << "stateConnected )";
                NEW_LINE;
                BLOCK_OPEN;
                CCOUT << "ret = ERROR_STATE;";
                NEW_LINE;
                CCOUT << "break;";
                BLOCK_CLOSE;
                NEW_LINES( 2 );
                Wa( "maincli( pSvc, argc, argv );" );
            }

            NEW_LINE;
            Wa( "// Stopping the object" );
            CCOUT << "ret = pSvc->Stop();";
            NEW_LINE;
            BLOCK_CLOSE;
            CCOUT<< "while( 0 );";
            NEW_LINES( 2 );
            Wa( "DestroyContext();" );
            CCOUT << "return ret;";
            BLOCK_CLOSE;
            NEW_LINES(2);

            Wa( "//-----Your code begins here---" );
            //  business logic
            if( bProxy )
            {
                CCOUT << "gint32 maincli(";
                INDENT_UPL;
                CCOUT << strClass << "* pIf,";
                NEW_LINE;
                CCOUT << "int argc, char** argv )";
                INDENT_DOWNL;
            }
            else
            {
                CCOUT << "gint32 mainsvr( ";
                INDENT_UPL;
                CCOUT << strClass << "* pIf,";
                NEW_LINE;
                CCOUT << "int argc, char** argv )";
                INDENT_DOWNL;
            }
            BLOCK_OPEN;
            if( bProxy )
            {
                Wa( "//-----Adding your code here---" );
            }
            else
            {
                CCOUT << "// replace 'sleep' with your code for";
                NEW_LINE;
                CCOUT << "// advanced control";
                NEW_LINE;
                CCOUT << "while( pIf->IsConnected() )";
                INDENT_UPL;
                CCOUT << "sleep( 1 );";
                INDENT_DOWNL;
            }
            CCOUT << "return STATUS_SUCCESS;";
            BLOCK_CLOSE;
            NEW_LINES( 2 );
        }

    }while( 0 );

    return ret;
}
gint32 GenHeaderFileROS(
    CCppWriter* pWriter, ObjPtr& pRoot )
{
    gint32 ret = 0;
    do{
        pWriter->SelectHeaderFile();

        CHeaderPrologue oHeader(
            pWriter, pRoot );

        ret = oHeader.Output();
        if( ERROR( ret ) )
            break;

        if( bFuseP )
        {
            CCppWriter* m_pWriter = pWriter;
        }

        CDeclareClassIds odci(
            pWriter, pRoot );

        ret = odci.Output();
        if( ERROR( ret ) )
            break;

        CStatements* pStmts = pRoot;
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        for( guint32 i = 0;
            i < pStmts->GetCount(); i++ )
        {
            ObjPtr pObj = pStmts->GetChild( i );
            guint32 dwClsid =
                ( guint32 )pObj->GetClsid();
            switch( dwClsid )
            {
            case clsid( CTypedefDecl ):
                {
                    CDeclareTypedef odt(
                        pWriter, pObj );
                    ret = odt.Output();
                    break;
                }
            case clsid( CStructDecl ) :
                {
                    CStructDecl* psd = pObj;
                    if( psd->RefCount() == 0 )
                        break;
                    CDeclareStruct ods(
                        pWriter, pObj );
                    ret = ods.Output();
                    break;
                }
            case clsid( CInterfaceDecl ):
                {
                    CInterfaceDecl* pNode = pObj;
                    if( pNode == nullptr )
                    {
                        ret = -EFAULT;
                        break;
                    }

                    if( pNode->RefCount() == 0 )
                        break;

                    if( bFuseP )
                    {
                        CDeclInterfProxyFuse odip(
                            pWriter, pObj );
                        ret = odip.Output();
                        if( ERROR( ret ) )
                            break;
                    }
                    else
                    {
                        CDeclInterfProxy2 odip(
                            pWriter, pObj );
                        ret = odip.OutputROSSkel();
                        if( ERROR( ret ) )
                            break;
                    }

                    if( bFuseS )
                    {
                        CDeclInterfSvrFuse odis(
                            pWriter, pObj );

                        ret = odis.Output();
                    }
                    else
                    {
                        CDeclInterfSvr2 odis(
                            pWriter, pObj );

                        ret = odis.OutputROSSkel();
                    }
                    break;
                }
            case clsid( CServiceDecl ):
                {
                    CDeclService2 ods(
                        pWriter, pObj );
                    ret = ods.OutputROSSkel();
                    if( ERROR( ret ) )
                        break;
                    break;
                }
            case clsid( CAppName ) :
            case clsid( CConstDecl ) :
                {
                    break;
                }
            default:
                {
                    ret = -ENOTSUP;
                    break;
                }
            }
            if( ERROR( ret ) )
                break;
        }

        std::vector< ObjPtr > vecSvcs;
        ret = pStmts->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
            break;

        std::vector< std::pair< std::string, ObjPtr > > vecSvcNames;
        for( auto& elem : vecSvcs )
        {
            CServiceDecl* psd = elem;
            if( psd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            vecSvcNames.push_back(
                { psd->GetName(), elem } );
            pWriter->AddSvcImpl( psd->GetName() );
        }

        for( auto& elem : vecSvcNames )
        {
            // server declarations
            ret = pWriter->SelectImplFile(
                elem.first + "svr.h" ); 

            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    elem.first + "svr.h.new" );
            }

            if( bFuseS )
            {
                CDeclServiceImplFuse osi(
                    pWriter, elem.second, true );

                ret = osi.Output();
                if( ERROR( ret ) )
                    break;
            }
            else
            {
                CDeclServiceImpl2 osi(
                    pWriter, elem.second, true );

                ret = osi.OutputROS();
                if( ERROR( ret ) )
                    break;
            }

            // client declarations
            ret = pWriter->SelectImplFile(
                elem.first + "cli.h" ); 

            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    elem.first + "cli.h.new" );
            }

            if( bFuseP )
            {
                CDeclServiceImplFuse osi2(
                    pWriter, elem.second, false );

                ret = osi2.Output();
                if( ERROR( ret ) )
                    break;
            }
            else
            {
                CDeclServiceImpl2 osi2(
                    pWriter, elem.second, false );

                ret = osi2.OutputROS();
                if( ERROR( ret ) )
                    break;
            }

            /* svr implementions*/
            ret = pWriter->SelectImplFile(
                elem.first + "svr.cpp" ); 

            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    elem.first + "svr.cpp.new" );
            }

            if( ERROR( ret ) )
                break;

            if( bFuseS )
            {
                CImplServiceImplFuse oisi(
                    pWriter, elem.second, true );

                ret = oisi.Output();
                if( ERROR( ret ) )
                    break;
            }
            else
            {
                CImplServiceImpl2 oisi(
                    pWriter, elem.second, true );

                ret = oisi.OutputROS();
                if( ERROR( ret ) )
                    break;
            }

            /* client implementions*/
            ret = pWriter->SelectImplFile(
                elem.first + "cli.cpp" ); 

            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    elem.first + "cli.cpp.new" );
            }

            if( ERROR( ret ) )
                break;

            if( bFuseP )
            {
                CImplServiceImplFuse oisi2(
                    pWriter, elem.second, false );

                ret = oisi2.Output();
                if( ERROR( ret ) )
                    break;
            }
            else
            {
                CImplServiceImpl2 oisi2(
                    pWriter, elem.second, false );

                ret = oisi2.OutputROS();
                if( ERROR( ret ) )
                    break;
            }
        }

        if( !vecSvcNames.empty() )
        {
            // client side
            if( bFuseP )
            {
                CImplMainFuncFuse omf(
                    pWriter, pRoot, true );
                ret = omf.Output();
                if( ERROR( ret ) )
                    break;
            }
            else
            {
                CImplMainFunc2 omf(
                    pWriter, pRoot, true );
                ret = omf.OutputROS();
                if( ERROR( ret ) )
                    break;
            }

            // server side
            if( bFuseS )
            {
                CImplMainFuncFuse omf(
                    pWriter, pRoot, false );
                ret = omf.Output();
                if( ERROR( ret ) )
                    break;
            }
            else
            {
                CImplMainFunc2 omf(
                    pWriter, pRoot, false );
                ret = omf.OutputROS();
                if( ERROR( ret ) )
                    break;
            }
        }

    }while( 0 );

    return ret;
}

extern gint32 FuseDeclareMsgSet(
    CCppWriter* m_pWriter, ObjPtr& pRoot );

gint32 EmitBuildJsonReq( 
    CWriterBase* m_pWriter );

gint32 GenCppFileROS(
    CCppWriter* m_pWriter, ObjPtr& pRoot )
{
    gint32 ret = 0;
    do{
        m_pWriter->SelectCppFile();

        CStatements* pStmts = pRoot;
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        std::string strName = pStmts->GetName();

        Wa( "// Generated by ridlc" );
        Wa( "// This file content could be rewritten by ridlc" );
        Wa( "#include \"rpc.h\"" );
        Wa( "#include \"iftasks.h\"" );

        Wa( "using namespace rpcf;" );
        CCOUT << "#include \"" << strName << ".h\"";
        NEW_LINE;
        std::vector< ObjPtr > vecSvcs;
        pStmts->GetSvcDecls( vecSvcs );
        NEW_LINE;

        if( bFuse )
        {
            ret = FuseDeclareMsgSet(
                m_pWriter, pRoot );
            if( ERROR( ret ) )
                break;

            ret = EmitBuildJsonReq(
                m_pWriter );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }

        for( guint32 i = 0;
            i < pStmts->GetCount(); i++ )
        {
            ObjPtr pObj = pStmts->GetChild( i );
            guint32 dwClsid =
                ( guint32 )pObj->GetClsid();
            switch( dwClsid )
            {
            case clsid( CStructDecl ) :
                {
                    CStructDecl* psd = pObj;
                    if( psd->RefCount() == 0 )
                        break;
                    CImplSerialStruct oiss(
                        m_pWriter, pObj );
                    ret = oiss.Output();
                    break;
                }
            case clsid( CInterfaceDecl ):
                {
                    CAstNodeBase* pNode = pObj;
                    if( pNode == nullptr )
                    {
                        ret = -EFAULT;
                        break;
                    }

                    CInterfaceDecl* pifd = pObj;

                    if( pifd->RefCount() == 0 )
                        break;

                    if( bFuseP )
                    {
                        CImplIufProxyFuse oiufp(
                            m_pWriter, pObj );
                        oiufp.Output();
                    }
                    else
                    {
                        CImplIufProxy oiufp(
                            m_pWriter, pObj );
                        oiufp.Output();
                    }

                    ObjPtr pmdlobj =
                        pifd->GetMethodList();

                    if( pmdlobj.IsEmpty() )
                    {
                        ret = -ENOENT;
                        break;
                    }

                    CMethodDecls* pmdl = pmdlobj;
                    for( guint32 i = 0;
                        i < pmdl->GetCount(); i++ )
                    {
                        ObjPtr pmd = pmdl->GetChild( i );
                        if( bFuseP )
                        {
                            CImplIfMethodProxyFuse oiimp(
                                m_pWriter, pmd );
                            ret = oiimp.Output();
                            if( ERROR( ret ) )
                                break;
                        }
                        else
                        {
                            CImplIfMethodProxy2 oiimp(
                                m_pWriter, pmd );
                            ret = oiimp.OutputROSSkel();
                            if( ERROR( ret ) )
                                break;
                        }
                    }

                    if( bFuseS )
                    {
                        CImplIufSvrFuse oiufs(
                            m_pWriter, pObj );
                        oiufs.Output();
                    }
                    else
                    {
                        CImplIufSvr oiufs(
                            m_pWriter, pObj );
                        oiufs.Output();
                    }

                    for( guint32 i = 0;
                        i < pmdl->GetCount(); i++ )
                    {
                        ObjPtr pmd = pmdl->GetChild( i );
                        if( bFuseS )
                        {
                            CImplIfMethodSvrFuse oiims(
                                m_pWriter, pmd );
                            ret = oiims.Output();
                            if( ERROR( ret ) )
                                break;
                        }
                        else
                        {
                            CImplIfMethodSvr oiims(
                                m_pWriter, pmd );
                            ret = oiims.Output();
                            if( ERROR( ret ) )
                                break;
                        }
                    }

                    break;
                }
            case clsid( CServiceDecl ):
            case clsid( CTypedefDecl ):
            case clsid( CAppName ) :
            case clsid( CConstDecl ) :
                {
                    break;
                }
            default:
                {
                    ret = -ENOTSUP;
                    break;
                }
            }
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

CExportObjDesc2::CExportObjDesc2(
    CWriterBase* pWriter,
    ObjPtr& pNode )
    : super( pWriter, pNode )
{ m_strFile = "./odesctpl2.json"; }

gint32 CExportObjDesc2::OutputROS()
{
    gint32 ret = 0;
    do{
        ret = super::super::Output();
        if( ERROR( ret ) )
            break;

        m_pWriter->m_curFp->flush();
        m_pWriter->m_curFp->close();

        Json::Value oVal;
        ret = ReadJsonCfg(
            m_pWriter->m_strCurFile,
            oVal );
        if( ERROR( ret ) )
            break;

        m_pWriter->m_curFp->open(
            m_pWriter->m_strCurFile,
            std::ofstream::out |
            std::ofstream::trunc);
        if( !m_pWriter->m_curFp->good() )
        {
            ret = -errno;
            break;
        }

        std::string strAppName =
            m_pNode->GetName();

        oVal[ JSON_ATTR_SVRNAME ] = strAppName;

        Json::Value oElemTmpl;

        // get object array
        Json::Value& oObjArray =
            oVal[ JSON_ATTR_OBJARR ];

        if( oObjArray == Json::Value::null ||
            !oObjArray.isArray() ||
            oObjArray.size() < 3 )
        {
            ret = -ENOENT;
            break;
        }

        Json::Value oChannel = oObjArray[ 0 ];
        Json::Value oMaster = oObjArray[ 1 ];
        Json::Value oSkel = oObjArray[ 2 ];
        oObjArray.clear();
        oObjArray.append( oChannel );

        std::vector< ObjPtr > vecSvcs;
        ret = m_pNode->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
            break;

        for( auto& elem : vecSvcs )
        {
            CServiceDecl* psd = elem;
            if( psd == nullptr )
                continue;

            Json::Value oElems( Json::arrayValue );
            Json::Value oMaster1, oSkel1;
            oMaster1 = oMaster;
            oSkel1 = oSkel;
            oElems.append( oMaster );
            oElems.append( oSkel );
            ret = BuildObjDescROS( psd, oElems );
            if( ERROR( ret ) )
                break;
            oObjArray.append( oElems[ 0 ] );
            oObjArray.append( oElems[ 1 ] );
        }

        if( ERROR( ret ) )
            break;

        Json::StreamWriterBuilder oBuilder;
        oBuilder["commentStyle"] = "None";
        oBuilder["indentation"] = "    ";
        std::unique_ptr<Json::StreamWriter> pWriter(
            oBuilder.newStreamWriter() );
        pWriter->write( oVal, m_pWriter->m_curFp );
        m_pWriter->m_curFp->flush();

        // output a python tool for syncing the
        // config paramters with the system
        // settings
        stdstr strDstPy = m_pWriter->GetOutPath();
        strDstPy += "/synccfg.py";
        stdstr strDesc = m_pWriter->GetCurFile();
        stdstr strSrcPy;

        strDesc = std::string( "./" ) +
            basename( strDesc.c_str() );

        ret = FindInstCfg( "synccfg.py", strSrcPy );
        if( ERROR( ret ) )
        {
            ret = 0;
            break;
        }

        stdstr strObjList;
        for( int i = 0; i < vecSvcs.size(); i++ )
        {
            auto elem = vecSvcs[ i ];
            CServiceDecl* psd = elem;
            if( psd == nullptr )
                continue;

            stdstr strObjName = psd->GetName();
            strObjList += "'" + strObjName + "'";
            if( i < vecSvcs.size() - 1 )
                strObjList += ",";
        }

        stdstr strCmdLine = "s:XXXDESTDIR:";
        strCmdLine += strDesc + ":;" +
            "s:XXXOBJLIST:" + strObjList +
            ":";

        const char* args[5];

        args[ 0 ] = "/usr/bin/sed";
        args[ 1 ] = strCmdLine.c_str();
        args[ 2 ] = strSrcPy.c_str();
        args[ 3 ] = nullptr;
        char* env[ 1 ] = { nullptr };

        Execve( "/usr/bin/sed",
            const_cast< char* const*>( args ),
            env, strDstPy.c_str() );

    }while( 0 );

    return ret;
}
gint32 CExportObjDesc2::BuildObjDescROS(
    CServiceDecl* psd, Json::Value& arrElems )
{
    if( psd == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        Json::Value& oMaster = arrElems[ 0 ];
        Json::Value& oSkel = arrElems[ 1 ];

        std::string strSvcName = psd->GetName();
        oMaster[ JSON_ATTR_OBJNAME ] = strSvcName;

        guint32 dwVal = 0;
        guint32 dwkasec = 0;

        ret = psd->GetTimeoutSec( dwVal );
        if( SUCCEEDED( ret ) && dwVal > 1 )
        {
            ret = psd->GetKeepAliveSec( dwkasec );
            if( ERROR( ret ) || dwkasec > dwVal )
                dwkasec = ( dwVal >> 1 );
            else if( dwkasec >
                IFSTATE_DEFAULT_IOREQ_TIMEOUT )
            {
                dwkasec =
                    IFSTATE_DEFAULT_IOREQ_TIMEOUT - 10;
            }
        }
        else
        {
            dwVal = IFSTATE_DEFAULT_IOREQ_TIMEOUT;
            dwkasec = ( dwVal >> 1 );
        }

        oMaster[ JSON_ATTR_REQ_TIMEOUT ] =
            std::to_string( dwVal );
        oMaster[ JSON_ATTR_KA_TIMEOUT ] =
            std::to_string( dwkasec );

        ret = 0;

        // get interface array
        Json::Value& oIfArray =
            oMaster[ JSON_ATTR_IFARR ];

        if( oIfArray == Json::Value::null ||
            !oIfArray.isArray() ||
            oIfArray.empty() )
        {
            ret = -ENOENT;
            break;
        }

        guint32 i;
        for( i = 0; i < oIfArray.size(); i++ )
        {
            if( !oIfArray[ i ].isMember(
                JSON_ATTR_IFNAME ) )
                continue;
            Json::Value& oIfElem = oIfArray[ i ];
            if( oIfElem[ JSON_ATTR_IFNAME ]
                == "XXXX" )
            {
                std::string strMainIf = "C";
                strMainIf += strSvcName + "_SvrImpl";
                oIfElem[ JSON_ATTR_IFNAME ] =
                    strMainIf;
                oIfElem[ JSON_ATTR_BIND_CLSID ] =
                    "true";
                oIfElem[ JSON_ATTR_DUMMYIF ] =
                    "true";
                break;
            }
        }

        if( psd->IsStream() || bFuse )
        {
            Json::Value oJif;
            oJif[ JSON_ATTR_IFNAME ] =
                "IStream";
            oIfArray.append( oJif );
        }

        oSkel[ JSON_ATTR_OBJNAME ] =
                strSvcName + "_SvrSkel";
        Json::Value& oIfArray2 = oSkel[ JSON_ATTR_IFARR ];

        for( i = 0; i < oIfArray2.size(); i++ )
        {
            if( !oIfArray2[ i ].isMember(
                JSON_ATTR_IFNAME ) )
                continue;
            Json::Value& oIfElem = oIfArray2[ i ];
            if( oIfElem[ JSON_ATTR_IFNAME ]
                == "XXXX_SvrSkel" )
            {
                std::string strMainIf = "C";
                strMainIf += strSvcName + "_SvrSkel";
                oIfElem[ JSON_ATTR_IFNAME ] =
                    strMainIf;
                oIfElem[ JSON_ATTR_BIND_CLSID ] =
                    "true";
                oIfElem[ JSON_ATTR_DUMMYIF ] =
                    "true";
                break;
            }
        }

        std::vector< ObjPtr > vecIfs;
        ret = psd->GetIfRefs( vecIfs );
        if( ERROR( ret ) )
            break;

        for( auto& pIf : vecIfs )
        {
            CInterfRef* pifr = pIf;
            if( pifr == nullptr )
                continue;

            Json::Value oJif;
            oJif[ JSON_ATTR_IFNAME ] =
                pifr->GetName();
            oIfArray2.append( oJif );
        }

    }while( 0 );

    return ret;
}
