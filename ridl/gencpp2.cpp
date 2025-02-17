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
#include "genfuse2.h"

extern std::string g_strAppName;
extern stdstr g_strCmdLine;
extern bool g_bMklib;
extern stdstr g_strLang;
extern guint32 g_dwFlags;
extern stdstr g_strTarget;
extern bool g_bBuiltinRt;

gint32 EmitOnPreStart( 
    CWriterBase* m_pWriter,
    CServiceDecl* pSvcNode,
    bool bProxy )
{
    gint32 ret = 0;
    if( m_pWriter == nullptr ||
        pSvcNode == nullptr )
        return -EINVAL;

    do{
        CServiceDecl* pSvc = pSvcNode;
        if( pSvc == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        stdstr strObjName = pSvc->GetName();
        strObjName += "_ChannelSvr";
        Wa( "gint32 ret = 0;" );
        CCOUT << "do";
        BLOCK_OPEN;
        Wa( "CCfgOpener oCtx;" );
        Wa( "CCfgOpenerObj oIfCfg( this );" );
        Wa( "oCtx[ propClsid ] = clsid( " );
        if( bProxy )
            CCOUT << "    C" << pSvc->GetName()
                << "_ChannelCli );";
        else
            CCOUT << "    C" << strObjName << " );";
        NEW_LINE;
        Wa( "oCtx.CopyProp( propObjDescPath, this );" );
        Wa( "oCtx.CopyProp( propSvrInstName, this );" );
        Wa( "stdstr strInstName;" );
        Wa( "ret = oIfCfg.GetStrProp(" );
        Wa( "    propObjName, strInstName );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        Wa( "oCtx[ 1 ] = strInstName;" );
        Wa( "guint32 dwHash = 0;" );
        Wa( "ret = GenStrHash( strInstName, dwHash );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        Wa( "char szBuf[ 16 ];" );
        Wa( "sprintf( szBuf, \"_\%08X\", dwHash );" );
        CCOUT << "strInstName = \"" << strObjName << "\";";
        NEW_LINE;
        CCOUT << "oCtx[ 0 ] = strInstName;";
        NEW_LINE;
        Wa( "strInstName += szBuf;" );
        Wa( "oCtx[ propObjInstName ] = strInstName;" );
        CCOUT << "oCtx[ propIsServer ] = "
            << ( bProxy ? "false" : "true" ) << ";";
        NEW_LINE;
        Wa( "oIfCfg.SetPointer( propSkelCtx," );
        Wa( "    ( IConfigDb* )oCtx.GetCfg() );" );
        CCOUT << "ret = super::OnPreStart( pCallback );";
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        CCOUT << "return ret;";

    }while( 0 );

    return ret;
}

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
        std::string strName;
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

        if( pmd->IsSerialize() && dwCount > 0 )
        { 
            NEW_LINE;
            // extra step the deserialize before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName <<"Wrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback, BufPtr& pBuf_ );";
            INDENT_DOWN;
        }
        else if( dwCount == 0 )
        {
            NEW_LINE;
            // extra step the deserialize before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName
                <<"Wrapper( IEventSink* pCallback );";
        }
        else if( !pmd->IsSerialize() )
        {
            NEW_LINE;
            // extra step the deserialize before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName
                <<"Wrapper(";
            INDENT_UPL;
            CCOUT << "IEventSink* pCallback, ";
            NEW_LINE;
            GenFormInArgs( pArgs );
            CCOUT << " );";
            INDENT_DOWN;
        }
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy2::OutputAsyncROSSkel(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName;
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
        if( dwInCount + dwOutCount == 0 )
            CCOUT << "";
        if( dwInCount > 0 )
        {
            CCOUT << ", ";
            NEW_LINE;
            GenFormInArgs( pInArgs );
        }
        if( dwOutCount > 0 )
        {
            CCOUT << ", ";
            NEW_LINE;
            GenFormOutArgs( pOutArgs );
        }

        CCOUT << " );";
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
            CCOUT << " );";
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

        NEW_LINE;
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
            << "<" << strSkel << "*>("
            << "( CRpcServices* )pIf );";
        NEW_LINE;
        CCOUT << "return pSkel;";
        BLOCK_CLOSE;

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
        std::string strName;
        strName = pmd->GetName();
        ObjPtr pArgs = pmd->GetInArgs();
        CCOUT<< "//RPC event handler '" << strName <<"'";
        NEW_LINE;
        std::string strDecl =
            std::string( "virtual gint32 " ) +
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

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy2::OutputSyncROS(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName;
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

        Wa( "auto pSkel = GetSkelPtr(); " );
        Wa( "if( pSkel == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "return pSkel->" << strName << "(";
        if( dwCount == 0 )
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

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy2::OutputAsyncROS(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount = GetArgCount( pInArgs );
        guint32 dwOutCount = GetArgCount( pOutArgs );
        guint32 dwCount = dwInCount + dwOutCount;

        Wa( "//RPC Async Req Sender" );
        CCOUT << "gint32 " << strName << "( ";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "IConfigDb* context";
        if( dwInCount + dwOutCount == 0 )
            CCOUT << "";
        if( dwInCount > 0 )
        {
            CCOUT << ", ";
            NEW_LINE;
            GenFormInArgs( pInArgs );
        }
        if( dwOutCount > 0 )
        {
            CCOUT << ", ";
            NEW_LINE;
            GenFormOutArgs( pOutArgs );
        }

        CCOUT << " )";
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "auto pIf = GetSkelPtr();" );
        Wa( "if( pIf == nullptr )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        CCOUT << "auto pSkel = dynamic_cast"
            << "< I" << m_pNode->GetName() << "_PImpl* >"
            << "( pSvc );";
        NEW_LINE;
        Wa( "if( pSkel == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "return pSkel->" << strName << "(";
        if( dwCount == 0 )
        {
            INDENT_UPL;
            CCOUT << "context );";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "context,";
            NEW_LINE;
            GenActParams( pInArgs, pOutArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }
        BLOCK_CLOSE;
        NEW_LINE;


        Wa( "// RPC Async Req Callback" );

        std::string strDecl;
        strDecl += "virtual gint32 " + strName
            + "Callback" + "(";
        CCOUT << strDecl;
        strDecl += "IConfigDb* context, gint32 iRet";

        if( dwOutCount > 0 )
        {
            INDENT_UPL;
            CCOUT << "IConfigDb* context, ";
            NEW_LINE;
            CCOUT << "gint32 iRet,";
            NEW_LINE;
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
            CCOUT << "gint32 iRet ) = 0;";
            INDENT_DOWN;
        }
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy2::OutputROSImpl()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

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
                ret = OutputEventROSImpl( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else if( !pmd->IsAsyncp() )
            {
                ret = OutputSyncROSImpl( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else 
            {
                ret = OutputAsyncROSImpl( pmd );
            }
        }

    }while( 0 );

    return ret;
}

gint32 CDeclInterfProxy2::OutputEventROSImpl(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;

    do{
        std::string strName;
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
            GenFormInArgs( pArgs, true );
            CCOUT << " ) override;";
        }
        else
        {
            CCOUT << ") override;";
        }
        INDENT_DOWNL;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy2::OutputSyncROSImpl(
    CMethodDecl* pmd )
{ return STATUS_SUCCESS; }

gint32 CDeclInterfProxy2::OutputAsyncROSImpl(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount = GetArgCount( pInArgs );
        guint32 dwOutCount = GetArgCount( pOutArgs );
        guint32 dwCount = dwInCount + dwOutCount;

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
            NEW_LINE;
            strDecl += ",";
            GenFormInArgs( pOutArgs, true );
            CCOUT << " ) override;";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "IConfigDb* context, ";
            NEW_LINE;
            CCOUT << "gint32 iRet ) override;";
            INDENT_DOWN;
        }
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
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

        NEW_LINE;
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
            << "<" << strSkel << "*>("
            << "( CRpcServices* )pIf );";
        NEW_LINE;
        CCOUT << "return pSkel;";
        BLOCK_CLOSE;

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
        std::string strName;
        strName = pmd->GetName();
        ObjPtr pArgs = pmd->GetInArgs();
        Wa( "//RPC event sender" );
        CCOUT << "virtual gint32 " << strName << "(";
        guint32 dwInCount = GetArgCount( pArgs );
        if( dwInCount > 0 )
        {
            INDENT_UPL;
            GenFormInArgs( pArgs );
            CCOUT << " ) = 0;";
            INDENT_DOWN;
        }
        else
        {
            CCOUT << " ) = 0;";
        }
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
        std::string strName;
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
            std::string( "virtual gint32 " )
            + strName + "(";
        CCOUT << strDecl;
        if( dwCount == 0 )
        {
            CCOUT << ") = 0;";
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
            CCOUT << " ) = 0;";
            INDENT_DOWN;
        }
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr2::OutputAsyncROS(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount = dwInCount + dwOutCount;

        Wa( "//RPC Async Req Cancel Handler" );
        CCOUT << "virtual gint32 "
            << "On" << strName << "Canceled(";
        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_, gint32 iRet";
        if( dwInCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormInArgs( pInArgs );
        }
        CCOUT << " ) = 0;";
        INDENT_DOWNL;

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
        CCOUT << "do";
        BLOCK_OPEN;
        Wa( "CCfgOpener oReqCtx( pReqCtx_ );" );
        Wa( "IEventSink* pEvt = nullptr;" );
        Wa( "ret = oReqCtx.GetPointer(" );
        Wa( "    propEventSink, pEvt );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );
        CCOUT <<  "auto pSvr_ = dynamic_cast" 
            << "<CFastRpcServerBase*>( this );";
        NEW_LINE;
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
            CCOUT << "    pReqCtx_, iRet,";
            INDENT_UPL;
            GenActParams( pOutArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

        Wa( "//RPC Async Req Handler" );
        std::string strDecl =
            std::string( "virtual gint32 " ) +
            strName + "(";

        CCOUT << strDecl;
        strDecl += "IConfigDb* pReqCtx_";

        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_";
        if( dwCount > 0 )
        {
            strDecl += ",";
        }

        if( dwInCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormInArgs( pInArgs );
        }
        if( dwOutCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormOutArgs( pOutArgs );
        }
        CCOUT << " ) = 0;";
        INDENT_DOWNL;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr2::OutputROSImpl()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

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
                ret = OutputEventROSImpl( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else if( pmd->IsAsyncs() )
            {
                ret = OutputAsyncROSImpl( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else 
            {
                ret = OutputSyncROSImpl( pmd );
                if( ERROR( ret ) )
                    break;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CDeclInterfSvr2::OutputEventROSImpl(
    CMethodDecl* pmd )
{ 
    gint32 ret = 0;
    do{
        stdstr strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();

        CCOUT << "gint32 " << strName << "(";
        guint32 dwInCount = GetArgCount( pInArgs );
        if( dwInCount > 0 )
        {
            INDENT_UPL;
            GenFormInArgs( pInArgs, true );
            CCOUT << " ) override;";
            INDENT_DOWN;
        }
        else
        {
            CCOUT << " ) override;";
        }
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CDeclInterfSvr2::OutputSyncROSImpl(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName;
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
            CCOUT << ") override;";
        }
        else
        {
            INDENT_UPL;
            bool bComma = false;
            if( dwInCount > 0 )
            {
                bComma = true;
                GenFormInArgs( pInArgs, true );
            }
            if( dwOutCount > 0 )
            {
                if( bComma )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }
                GenFormOutArgs( pOutArgs, true );
            }
            CCOUT << " ) override;";
            INDENT_DOWN;
        }
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr2::OutputAsyncROSImpl(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName;
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
            GenFormInArgs( pInArgs, true );
        }
        CCOUT << " ) override";
        INDENT_DOWNL;
        BLOCK_OPEN;
        CCOUT << "DebugPrintEx( logErr, iRet,";
        NEW_LINE;
        CCOUT << "    \"request '" << strName
            << "' is canceled.\" );";
        NEW_LINE;
        CCOUT << "return STATUS_SUCCESS;";
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
        }

        if( dwInCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormInArgs( pInArgs, true );
        }
        if( dwOutCount > 0 )
        {
            CCOUT << ",";
            NEW_LINE;
            GenFormOutArgs( pOutArgs, true );
        }
        CCOUT << " ) override;";
        INDENT_DOWNL;

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

gint32 CDeclInterfSvr2::OutputSyncROSSkel(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName;
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
        std::string strName;
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
        CCOUT << " );";
        INDENT_DOWNL;

        Wa( "//RPC Async Req Callback" );
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
        CCOUT << " );";
        INDENT_DOWNL;
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
        }

        if( dwInCount > 0 )
        {
            CCOUT << ",";
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
{
    gint32 ret = 0;
    do{
        std::string strClass = "C";
        strClass += GetSvcName() + "_SvrImpl";
        std::string strMethod = m_pNode->GetName();

        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount = GetArgCount( pInArgs );

        CAstNodeBase* pParent = m_pNode->GetParent();
        auto pifd = dynamic_cast< CInterfaceDecl* >
            ( pParent->GetParent() );
        if( pifd == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        stdstr strIfName = pifd->GetName();

        Wa( "/* RPC event sender */" );
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "(";
        if( dwInCount > 0 )
        {
            INDENT_UPL;
            GenFormInArgs( pInArgs, true );
            CCOUT << " )";
            INDENT_DOWN;
        }
        else
        {
            CCOUT << " )";
        }
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "std::vector< InterfPtr > vecSkels;" );
        Wa( "gint32 ret = this->EnumStmSkels( vecSkels );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    return ret;" );
        Wa( "for( auto& elem : vecSkels )" );
        BLOCK_OPEN;
        CCOUT << "C" << GetSvcName() << "_SvrSkel* pSkel = elem;";
        NEW_LINE;
        CCOUT << "ret = pSkel->" << "I" << strIfName
            << "_SImpl::" << strMethod << "(";
        if( dwInCount == 0 )
            CCOUT << ");";
        else
        {
            INDENT_UPL;
            GenActParams( pInArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }
        BLOCK_CLOSE;
        NEW_LINE;
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;
    }while( 0 );

    return ret;
}

gint32 CImplIfMethodSvr2::OutputSyncROS()
{
    gint32 ret = 0;
    std::string strClass = "C";
    strClass += GetSvcName() + "_SvrImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );
    guint32 dwCount = dwInCount + dwOutCount;

    bool bSerial = m_pNode->IsSerialize();
    bool bNoReply = m_pNode->IsNoReply();

    do{
        Wa( "/* Sync Req Handler*/" );
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "(";
        if( dwCount == 0 )
        {
            CCOUT << ")";
            NEW_LINE;
        }
        else
        {
            INDENT_UPL;
            bool bComma = false;
            if( dwInCount > 0 )
            {
                bComma = true;
                GenFormInArgs( pInArgs, true );
            }
            if( dwOutCount > 0 )
            {
                if( bComma )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }
                GenFormOutArgs( pOutArgs, true );
            }
            CCOUT << " )";
            INDENT_DOWNL;
        }
        BLOCK_OPEN;
        Wa( "// TODO: Process the sync request here" );
        Wa( "// return code can be an Error or" );
        Wa( "// STATUS_SUCCESS" );
        CCOUT << "return ERROR_NOT_IMPL;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvr2::OutputAsyncROS()
{
    gint32 ret = 0;
    std::string strClass = "C";
    strClass += GetSvcName() + "_SvrImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bNoReply = m_pNode->IsNoReply();
    do{
        Wa( "/* Async Req Handler*/" );
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( ";
        INDENT_UPL;
        if( dwInCount + dwOutCount > 0 )
        {
            CCOUT << "IConfigDb* pContext";
            if( dwInCount > 0 )
            {
                CCOUT << ", ";
                NEW_LINE;
                GenFormInArgs( pInArgs, true );
            }
            if( dwOutCount > 0 )
            {
                CCOUT << ", ";
                NEW_LINE;
                GenFormOutArgs( pOutArgs, true );
            }
            CCOUT << " )";
        }
        else
            CCOUT << "IConfigDb* pContext )";

        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "// TODO: Emit an async operation here." );
        CCOUT << "// And make sure to call '"
            << strMethod << "Complete'";
        NEW_LINE;
        Wa( "// when the service is done" );
        CCOUT << "return ERROR_NOT_IMPL;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvr2::OutputROSSkel()
{
    gint32 ret = 0;

    do{
        NEW_LINE;

        if( m_pNode->IsEvent() )
        {
            ret = OutputEvent();
            if( ERROR( ret ) )
                break;
            ret = OutputEventROSSkel();
            break;
        }
        else if( m_pNode->IsAsyncs() )
        {
            ret = OutputAsync();
            if( ERROR( ret ) )
                break;
            ret = OutputAsyncROSSkel();
            if( ERROR( ret ) )
                break;
            ret = OutputAsyncCallback();
        }
        else
        {
            ret = OutputSync();
            if( ERROR( ret ) )
                break;
            ret = OutputSyncROSSkel();
        }

    }while( 0 );

    return ret;
}

gint32 CImplIfMethodSvr2::OutputEventROSSkel()
{ return 0; }

gint32 CImplIfMethodSvr2::OutputSyncROSSkel()
{
    gint32 ret = 0;
    do{
        std::string strClass = "I";
        strClass += m_pIf->GetName() + "_SImpl";

        std::string strName;
        strName = m_pNode->GetName();

        ObjPtr pInArgs = m_pNode->GetInArgs();
        ObjPtr pOutArgs = m_pNode->GetOutArgs();

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount = dwInCount + dwOutCount;

        Wa( "//RPC Sync Req Handler" );
        std::string strDecl =
            std::string( "gint32 " )
            + strClass + "::" + strName + "(";
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
        Wa( "auto& pIf = GetParentIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        CCOUT << "auto pApi = dynamic_cast"
            << "< I" << m_pIf->GetName() << "_SvrApi* >"
            << "( pSvc );";
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
            GenActParams( pInArgs, pOutArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CImplIfMethodSvr2::OutputAsyncROSSkel()
{
    do{
        std::string strClass = "I";
        strClass += m_pIf->GetName() + "_SImpl";

        std::string strName;
        strName = m_pNode->GetName();
        ObjPtr pInArgs = m_pNode->GetInArgs();
        ObjPtr pOutArgs = m_pNode->GetOutArgs();

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        guint32 dwCount = dwInCount + dwOutCount;

        Wa( "//RPC Async Req Cancel Handler" );
        CCOUT << "gint32 " << strClass
            << "::On" << strName << "Canceled(";
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
        Wa( "auto& pIf = GetParentIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        CCOUT << "auto pApi = dynamic_cast"
            << "< I" << m_pIf->GetName() << "_SvrApi* >"
            << "( pSvc );";
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
            GenActParams( pInArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;
        NEW_LINES( 2 );

        Wa( "//RPC Async Req Handler" );
        std::string strDecl =
            std::string( "gint32 " ) +
            strClass + "::" + strName + "(";

        CCOUT << strDecl;
        strDecl += "IConfigDb* pReqCtx_";

        INDENT_UPL;
        CCOUT << "IConfigDb* pReqCtx_";
        if( dwCount > 0 )
        {
            strDecl += ",";
        }

        if( dwInCount > 0 )
        {
            CCOUT << ",";
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
        Wa( "auto& pIf = GetParentIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        CCOUT << "auto pApi = dynamic_cast"
            << "< I" << m_pIf->GetName() << "_SvrApi* >"
            << "( pSvc );";
        NEW_LINE;
        Wa( "if( pApi == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "return pApi->" << strName << "(";
        if( dwCount == 0 )
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_ );";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "pReqCtx_, ";
            GenActParams( pInArgs, pOutArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CImplIfMethodProxy2::OutputROSSkel()
{
    gint32 ret = 0;

    do{
        NEW_LINE;

        if( m_pNode->IsEvent() )
        {
            ret = OutputEvent();
            if( ERROR( ret ) )
                break;
            ret = OutputEventROSSkel();
        }
        else if( m_pNode->IsAsyncp() )
        {
            ret = OutputAsync();
            if( ERROR( ret ) )
                break;
            ret = OutputAsyncCbWrapper();
            if( ERROR( ret ) )
                break;
            ret = OutputAsyncROSSkel();
        }
        else
        {
            ret = OutputSync();
        }

    }while( 0 );

    return ret;
}

gint32 CImplIfMethodProxy2::OutputEventROSSkel()
{
    gint32 ret = 0;
    do{
        std::string strClass = "I";
        strClass += m_pIf->GetName() + "_PImpl";
        std::string strName;
        strName = m_pNode->GetName();
        ObjPtr pArgs = m_pNode->GetInArgs();
        CCOUT<< "//RPC event handler '" << strName <<"'";
        NEW_LINE;
        std::string strDecl =
            std::string( "gint32 " ) +
            strClass + "::" + strName + "(";
        CCOUT << strDecl;
        guint32 dwCount = GetArgCount( pArgs );

        INDENT_UPL;
        if( dwCount > 0 )
        {
            GenFormInArgs( pArgs );
            CCOUT << " )";
        }
        else
        {
            CCOUT << ")";
        }
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );
        Wa( "auto& pIf = GetParentIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        CCOUT << "auto pApi = dynamic_cast"
            << "< I" << m_pIf->GetName() << "_CliApi* >"
            << "( pSvc );";
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
        CCOUT << "return ret;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CImplIfMethodProxy2::OutputAsyncROSSkel()
{
    do{
        std::string strClass = "I";
        strClass += m_pIf->GetName() + "_PImpl";
        std::string strName;

        strName = m_pNode->GetName();
        ObjPtr pInArgs = m_pNode->GetInArgs();
        ObjPtr pOutArgs = m_pNode->GetOutArgs();

        guint32 dwInCount = GetArgCount( pInArgs );
        guint32 dwOutCount = GetArgCount( pOutArgs );


        std::string strDecl;
        strDecl += "gint32 " + strClass + "::" +
            strName + "Callback" + "(";
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
        Wa( "auto& pIf = GetParentIf();" );
        Wa( "if( pIf.IsEmpty() )" );
        Wa( "    return -EFAULT;" );
        Wa( "CRpcServices* pSvc = pIf;" );
        CCOUT << "auto pApi = dynamic_cast"
            << "< I" << m_pIf->GetName() << "_CliApi* >"
            << "( pSvc );";
        NEW_LINE;
        Wa( "if( pApi == nullptr )" );
        Wa( "    return -EFAULT;" );
        CCOUT << "return pApi->" << strName << "Callback(";
        if( dwOutCount == 0 )
        {
            INDENT_UPL;
            CCOUT << "context, iRet );";
            INDENT_DOWN;
        }
        else
        {
            INDENT_UPL;
            CCOUT << "context, iRet,";
            NEW_LINE;
            GenActParams( pOutArgs, false );
            CCOUT << " );";
            INDENT_DOWN;
        }

        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CImplIfMethodProxy2::OutputROS()
{
    gint32 ret = 0;

    do{
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
    std::string strClass = "C";
    strClass += GetSvcName() + "_CliImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwCount = dwInCount;

    bool bSerial = m_pNode->IsSerialize();

    do{
        Wa( "/* Event handler */" );
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( ";
        if( dwCount > 0 )
        {
            INDENT_UPL;
            GenFormInArgs( pInArgs, true );
            CCOUT << " )";
            INDENT_DOWN;
        }
        else
        {
            CCOUT << " )";
        }
        NEW_LINE;

        BLOCK_OPEN;

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
    std::string strClass = "C";
    strClass += GetSvcName() + "_CliImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bSerial = m_pNode->IsSerialize();
    bool bNoReply = m_pNode->IsNoReply();

    do{
        Wa( "/* Async callback handler */" );
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Callback" << "( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IConfigDb* context, gint32 iRet";
        if( dwOutCount > 0 )
        {
            CCOUT<< ",";
            NEW_LINE;
            GenFormInArgs( pOutArgs, true );
        }
        CCOUT << " )";
        INDENT_DOWNL;
        BLOCK_OPEN;
        Wa( "// TODO: Process the server response here" );
        Wa( "// return code ignored" );
        CCOUT << "return 0;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    
    return ret;
}

gint32 CDeclService2::OutputROSSkel( bool bProxy )
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

        if( bProxy )
        {
            CCOUT << "DECLARE_AGGREGATED_SKEL_PROXY(";
            INDENT_UP;
            NEW_LINE;
            CCOUT << "C" << strSvcName << "_CliSkel,";
            NEW_LINE;
            Wa( "CStatCountersProxySkel," );
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
            break;
        }

        CCOUT << "#define Clsid_C" << strSvcName << "_SvrSkel_Base Clsid_Invalid";
        NEW_LINE;
        NEW_LINE;
        CCOUT << "DECLARE_AGGREGATED_SKEL_SERVER(";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "C" << strSvcName << "_SvrSkel_Base,";
        NEW_LINE;
        Wa( "CStatCountersServerSkel," );
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
        NEW_LINES( 2 );

        Wa( "gint32 InvokeUserMethod(" );
        Wa( "    IConfigDb* pParams," );
        Wa( "    IEventSink* pCallback ) override" );
        BLOCK_OPEN;
        Wa( "CRpcServices* pSvc = nullptr;" );
        Wa( "if( !IsRfcEnabled() )" );
        BLOCK_OPEN;
        Wa( "if( m_pQpsTask.IsEmpty() )" );
        Wa( "    pSvc = this->GetStreamIf();" );
        Wa( "else" );
        Wa( "    pSvc = this;" );
        Wa( "gint32 ret = pSvc->AllocReqToken();" );
        Wa( "if( ERROR( ret ) )" );
        CCOUT << "    return ret;";
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "return super::InvokeUserMethod(" );
        CCOUT << "    pParams, pCallback );";
        BLOCK_CLOSE;
        BLOCK_CLOSE;
        Wa( ";" );
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CDeclService2::OutputROS( bool bProxy )
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

        if( !bProxy )
        {
            CCOUT << "#define Clsid_C" << strSvcName
                << "_SvrBase    Clsid_Invalid";
            NEW_LINE;
            NEW_LINE;
            CCOUT << "DECLARE_AGGREGATED_SERVER(";
            INDENT_UP;
            NEW_LINE;
            CCOUT << "C" << strSvcName << "_SvrBase,";
            NEW_LINE;

            Wa( "CStatCounters_SvrBase," );
            if( m_pNode->IsStream() )
                Wa( "CStreamServerAsync," );
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
            NEW_LINES( 2 );
        }
        else
        {
            CCOUT << "#define Clsid_C" << strSvcName
                << "_CliBase    Clsid_Invalid";
            NEW_LINE;
            NEW_LINE;
            CCOUT << "DECLARE_AGGREGATED_PROXY(";
            INDENT_UP;
            NEW_LINE;

            CCOUT << "C" << strSvcName << "_CliBase,";
            NEW_LINE;
            Wa( "CStatCounters_CliBase," );
            if( m_pNode->IsStream() )
                Wa( "CStreamProxyAsync," );
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

    }while( 0 );

    return ret;
}

bool ScanMethodsOfSvc( CServiceDecl* pSvc,
    bool ( *func )( CMethodDecl* ) )
{
    gint32 ret = 0;
    bool bRet = false;
    do{
        std::vector< ObjPtr > vecIfs;
        pSvc->GetIfRefs( vecIfs );
        for( guint32 i = 0; i < vecIfs.size(); ++i )
        {
            auto& elem = vecIfs[ i ];
            CInterfRef* pIfRef = elem;
            ObjPtr pObj;
            ret = pIfRef->GetIfDecl( pObj );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }
            CInterfaceDecl* pifd = pObj;
            if( pifd == nullptr )
                continue;

            pObj = pifd->GetMethodList();
            std::vector< ObjPtr > vecMethods;
            CMethodDecls* pmds = pObj;
            for( guint32 j = 0; j < pmds->GetCount(); j++ )
            {
                ObjPtr pObj = pmds->GetChild( j );
                CMethodDecl* pmd = pObj;
                bRet = func( pmd );
            }
            if( bRet )
                break;
        }

    }while( 0 );
    return bRet;
}

gint32 CDeclServiceImpl2::OutputROS()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();

        /*Wa( "// Generated by ridlc" );
        CCOUT << "// " << g_strCmdLine;
        NEW_LINE;
        Wa( "// Your task is to implement the following classes" );
        Wa( "// to get your rpc server work" );
        // Wa( "#pragma once" );
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
        NEW_LINES( 2 );*/

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

        ObjPtr pNode( m_pNode );
        CDeclService2 ods( pWriter, pNode );
        ods.OutputROS( !IsServer() );

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
            INDENT_UPL;
            CCOUT << "super::virtbase( pCfg ), "
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

            for( auto pifd : vecIfs )
            {
                CDeclInterfProxy2 odifp(
                    pWriter, pifd );
                odifp.OutputROSImpl();
            }

            Wa( "gint32 CreateStmSkel(" );
            Wa( "    InterfPtr& pIf ) override;" );
            NEW_LINE;
            Wa( "gint32 OnPreStart(" );
            CCOUT << "    IEventSink* pCallback ) override;";

            bool ( *func )( CMethodDecl* ) =
            ([]( CMethodDecl* pmd )->bool
            { return pmd->IsAsyncp(); });
            bool bAsyncp = ScanMethodsOfSvc( pNode, func );
            if( bAsyncp )
            {
                // adding CancelRequest method if there is
                // async call
                NEW_LINES( 2 );
                Wa( "gint32 CancelRequest(" );
                Wa( "    guint64 qwTaskToCancel )" );
                BLOCK_OPEN;
                Wa( "InterfPtr pIf = this->GetStmSkel();" );
                Wa( "if( pIf.IsEmpty() )" );
                Wa( "    return -EFAULT;" );
                Wa( "CInterfaceProxy* pProxy = pIf;" );
                Wa( "return pProxy->CancelRequest(" );
                CCOUT << "    qwTaskToCancel );";
                BLOCK_CLOSE;
            }

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
        INDENT_UPL;
        CCOUT << "super::virtbase( pCfg ), "
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

        Wa( "gint32 OnPostStart(" );
        Wa( "    IEventSink* pCallback ) override" );
        BLOCK_OPEN;
        Wa( "TaskletPtr pTask = GetUpdateTokenTask();" );
        Wa( "StartQpsTask( pTask );" );
        Wa( "if( !pTask.IsEmpty() )" );
        Wa( "    AllocReqToken();" );
        CCOUT << "return super::OnPostStart( pCallback );";
        BLOCK_CLOSE;
        NEW_LINES( 2 );

        Wa( "gint32 OnPreStop(" );
        Wa( "    IEventSink* pCallback ) override" );
        BLOCK_OPEN;
        Wa( "StopQpsTask();" );
        CCOUT << "return super::OnPreStop( pCallback );";
        BLOCK_CLOSE;
        NEW_LINES( 2 );

        for( auto pifd : vecIfs )
        {
            CDeclInterfSvr2 odifs(
                pWriter, pifd );
            odifs.OutputROSImpl();
        }

        Wa( "gint32 CreateStmSkel(" );
        Wa( "    HANDLE, guint32, InterfPtr& ) override;" );
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

gint32 CImplServiceImpl2::OutputROS()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();
        stdstr strOutPath = m_pWriter->GetOutPath();

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
        Wa( "#include \"stmport.h\"" );
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
                    iims.SetSvcName( strSvcName );
                    iims.OutputROS();
                }
                else
                {
                    CImplIfMethodProxy2 iimp(
                        pWriter, pObj );
                    iimp.SetSvcName( strSvcName );
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
            Wa( "std::string strDesc;" );
            Wa( "CCfgOpenerObj oIfCfg( this );" );
            Wa( "ret = oIfCfg.GetStrProp(" );
            Wa( "    propObjDescPath, strDesc );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "ret = CRpcServices::LoadObjDesc(" );
            CCOUT << "    strDesc," <<
                "\"" << strSvcName << "_SvrSkel\",";
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
            NEW_LINE;
            CCOUT << "gint32 " << strClass << "::OnPreStart(";
            NEW_LINE;
            Wa( "    IEventSink* pCallback )" );
            BLOCK_OPEN;
            EmitOnPreStart( m_pWriter, m_pNode, false );
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
            Wa( "auto pMgr = this->GetIoMgr();" );
            Wa( "oCfg[ propIoMgr ] = ObjPtr( pMgr );" );
            Wa( "oCfg[ propIsServer ] = false;" );
            Wa( "oCfg.SetPointer( propParentPtr, this );" );
            Wa( "oCfg.CopyProp( propSkelCtx, this );" );
            Wa( "std::string strDesc;" );
            Wa( "CCfgOpenerObj oIfCfg( this );" );
            Wa( "ret = oIfCfg.GetStrProp(" );
            Wa( "    propObjDescPath, strDesc );" );
            Wa( "if( ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "ret = CRpcServices::LoadObjDesc(" );
            CCOUT << "    strDesc," << "\""
                << strSvcName << "_SvrSkel\",";
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

            NEW_LINE;
            CCOUT << "gint32 " << strClass << "::OnPreStart(";
            NEW_LINE;
            Wa( "    IEventSink* pCallback )" );
            BLOCK_OPEN;
            EmitOnPreStart( m_pWriter, m_pNode, true );
            BLOCK_CLOSE;
        }
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplMainFunc2::OutputROS()
{ return super::Output(); }

#define EMIT_COMMON_INC_HDR( _pWriter ) \
{ \
    CCppWriter* m_pWriter = ( _pWriter ); \
    EMIT_DISCLAIMER; \
    CCOUT << "// " << g_strCmdLine; \
    NEW_LINE; \
    Wa( "#pragma once" ); \
    CCOUT << "#include \"" << \
        g_strAppName << ".h\""; \
    NEW_LINE; \
}

gint32 GenHeaderFileROS(
    CCppWriter* pWriter, ObjPtr& pRoot )
{
    gint32 ret = 0;
    do{
        CCppWriter* m_pWriter = pWriter;
        pWriter->SelectHeaderFile();

        CHeaderPrologue oHeader(
            pWriter, pRoot );

        ret = oHeader.Output();
        if( ERROR( ret ) )
            break;

        CDeclareClassIds odci( pWriter, pRoot );

        ret = odci.Output();
        if( ERROR( ret ) )
            break;

        CCOUT << "#include \""
            << g_strAppName << "struct.h\"";
        NEW_LINE;
            
        CStatements* pStmts = pRoot;
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( bFuse )
        {
            Wa( "gint32 GetSvrCtxId(" );
            Wa( "    IEventSink* pCb, stdstr& strCtxId );" );
            NEW_LINE;
            Wa( "gint32 BuildJsonReq2(" );
            Wa( "    IConfigDb* pReqCtx_," );
            Wa( "    const Json::Value& oJsParams," );
            Wa( "    const std::string& strMethod," );
            Wa( "    const std::string& strIfName, " );
            Wa( "    gint32 iRet," );
            Wa( "    std::string& strReq," );
            Wa( "    bool bProxy," );
            Wa( "    bool bResp );" );
            NEW_LINE;
        }

        pWriter->SelectStructHdr();
        EMIT_COMMON_INC_HDR( pWriter );
        NEW_LINE;
        for( guint32 i = 0;
            i < pStmts->GetCount(); i++ )
        {
            ObjPtr pObj = pStmts->GetChild( i );
            guint32 dwClsid =
                ( guint32 )pObj->GetClsid();
            if( dwClsid == clsid( CStructDecl ) )
            {
                CStructDecl* psd = pObj;
                if( psd->RefCount() == 0 )
                    continue;
                CDeclareStruct ods(
                    pWriter, pObj );
                ret = ods.Output();
                if( ERROR( ret ) )
                    break;
            }
            else if( dwClsid == clsid( CTypedefDecl ) )
            {
                CDeclareTypedef odt( pWriter, pObj );
                ret = odt.Output();
                if( ERROR( ret ) )
                    break;
            }
        }
        if( ERROR( ret ) )
            break;

        for( guint32 i = 0;
            i < pStmts->GetCount(); i++ )
        {
            ObjPtr pObj = pStmts->GetChild( i );
            guint32 dwClsid =
                ( guint32 )pObj->GetClsid();

            if( dwClsid != clsid( CInterfaceDecl ) )
                continue;

            CInterfaceDecl* pNode = pObj;
            if( pNode == nullptr )
                continue;

            if( pNode->RefCount() == 0 )
                continue;

            stdstr strIfName = pNode->GetName();
            pWriter->AddIfImpl( strIfName );
            if( bGenClient )
            {
                ret = pWriter->SelectImplFile(
                    strIfName + "cli.h" );
                if( ERROR( ret ) )
                    break;

                EMIT_COMMON_INC_HDR( pWriter );
                NEW_LINE;
                if( bFuseP )
                {
                    CDeclInterfProxyFuse2 odip(
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
                    ret = odip.OutputROS();
                }
            }
            if( bGenServer )
            {
                ret = pWriter->SelectImplFile(
                    strIfName + "svr.h" );
                if( ERROR( ret ) )
                    break;
                EMIT_COMMON_INC_HDR( pWriter );
                NEW_LINE;
                if( bFuseS )
                {
                    CDeclInterfSvrFuse2 odis(
                        pWriter, pObj );

                    ret = odis.Output();
                }
                else
                {
                    CDeclInterfSvr2 odis(
                        pWriter, pObj );

                    ret = odis.OutputROSSkel();
                    if( ERROR( ret ) )
                        break;
                    ret = odis.OutputROS();
                }
            }
        }

        if( ERROR( ret ) )
            break;

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
            ObjPtr pObj = elem.second;
            CServiceDecl* psd = pObj;

            std::vector< ObjPtr > vecIfs;
            ret = psd->GetIfRefs( vecIfs );
            if( ERROR( ret ) )
                break;
            if( bGenServer )
            {
                // server declarations
                ret = pWriter->SelectImplFile(
                    elem.first + "svr.h" ); 

                if( ERROR( ret ) )
                {
                    ret = pWriter->SelectImplFile(
                        elem.first + "svr.h.new" );
                }

                EMIT_COMMON_INC_HDR( pWriter );
                for( auto& pIf : vecIfs )
                {
                    CInterfRef* pifr = pIf;
                    if( pifr == nullptr )
                        continue;
                    NEW_LINE;
                    pifr->GetName();
                    CCOUT << "#include \"" <<
                        pifr->GetName() << "svr.h\"";
                }
                NEW_LINES( 2 );


                if( bFuseS )
                {
                    CDeclServiceFuse2 ods(
                        pWriter, pObj );
                    ret = ods.OutputROSSkel( false );
                    if( ERROR( ret ) )
                        break;

                    CDeclServiceImplFuse2 osi(
                        pWriter, elem.second, false );

                    ret = osi.Output();
                    if( ERROR( ret ) )
                        break;
                }
                else
                {
                    CDeclService2 ods(
                        pWriter, pObj );
                    ret = ods.OutputROSSkel( false );
                    if( ERROR( ret ) )
                        break;

                    CDeclServiceImpl2 osi(
                        pWriter, elem.second, false );

                    ret = osi.OutputROS();
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
                    CImplServiceImplFuse2 oisi(
                        pWriter, elem.second, false );

                    ret = oisi.Output();
                    if( ERROR( ret ) )
                        break;
                    oisi.OutputROSSkel();
                }
                else
                {
                    CImplServiceImpl2 oisi(
                        pWriter, elem.second, false );

                    ret = oisi.OutputROS();
                    if( ERROR( ret ) )
                        break;
                }
            }

            if( bGenClient )
            {
                ret = pWriter->SelectImplFile(
                    elem.first + "cli.h" ); 

                if( ERROR( ret ) )
                {
                    ret = pWriter->SelectImplFile(
                        elem.first + "cli.h.new" );
                }

                // client declarations
                EMIT_COMMON_INC_HDR( pWriter );

                for( auto& pIf : vecIfs )
                {
                    CInterfRef* pifr = pIf;
                    if( pifr == nullptr )
                        continue;
                    NEW_LINE;
                    pifr->GetName();
                    CCOUT << "#include \"" <<
                        pifr->GetName() << "cli.h\"";
                }
                NEW_LINES( 2 );

                if( bFuseP )
                {
                    CDeclServiceFuse2 ods(
                        pWriter, pObj );
                    ret = ods.OutputROSSkel( true );
                    if( ERROR( ret ) )
                        break;

                    CDeclServiceImplFuse2 osi2(
                        pWriter, elem.second, true );

                    ret = osi2.Output();
                    if( ERROR( ret ) )
                        break;
                }
                else
                {
                    CDeclService2 ods(
                        pWriter, pObj );
                    ret = ods.OutputROSSkel( true );
                    if( ERROR( ret ) )
                        break;

                    CDeclServiceImpl2 osi2(
                        pWriter, elem.second, true );

                    ret = osi2.OutputROS();
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
                    CImplServiceImplFuse2 oisi2(
                        pWriter, elem.second, true );

                    ret = oisi2.Output();
                    if( ERROR( ret ) )
                        break;

                    oisi2.OutputROSSkel();
                }
                else
                {
                    CImplServiceImpl2 oisi2(
                        pWriter, elem.second, true );

                    ret = oisi2.OutputROS();
                    if( ERROR( ret ) )
                        break;
                }
            }
        }

        if( !vecSvcNames.empty() )
        {
            // client side
            if( bGenClient )
            {
                if( bFuseP )
                {
                    CImplMainFuncFuse2 omf(
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
            }

            // server side
            if( bGenServer )
            {
                if( bFuseS )
                {
                    CImplMainFuncFuse2 omf(
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
        }

    }while( 0 );

    return ret;
}

extern gint32 FuseDeclareMsgSet(
    CCppWriter* m_pWriter, ObjPtr& pRoot );

extern gint32 EmitBuildJsonReq2( 
    CWriterBase* m_pWriter );

#define EMIT_COMMON_INC_CPP \
{ \
    EMIT_DISCLAIMER; \
    CCOUT << "// " << g_strCmdLine; \
    NEW_LINE; \
    Wa( "#include \"rpc.h\"" ); \
    Wa( "#include \"iftasks.h\"" ); \
    Wa( "using namespace rpcf;" ); \
    Wa( "#include \"stmport.h\"" );\
    Wa( "#include \"fastrpc.h\"" );\
    CCOUT << "#include \"" << strName << ".h\""; \
    NEW_LINE; \
}

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

        EMIT_COMMON_INC_CPP;
        std::vector< ObjPtr > vecSvcs;
        pStmts->GetSvcDecls( vecSvcs );
        NEW_LINE;

        if( bFuse )
        {
            ret = FuseDeclareMsgSet(
                m_pWriter, pRoot );
            if( ERROR( ret ) )
                break;

            ret = EmitBuildJsonReq2(
                m_pWriter );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }

        m_pWriter->SelectStruct();
        EMIT_COMMON_INC_CPP;
        for( guint32 i = 0;
            i < pStmts->GetCount(); i++ )
        {
            ObjPtr pObj = pStmts->GetChild( i );
            guint32 dwClsid =
                ( guint32 )pObj->GetClsid();

            if( dwClsid != clsid( CStructDecl ) )
                continue;
            CStructDecl* psd = pObj;
            if( psd->RefCount() == 0 )
                continue;
            CImplSerialStruct oiss(
                m_pWriter, pObj );
            ret = oiss.Output();
            if( ERROR( ret ) )
                break;
        }
        if( ERROR( ret ) )
            break;

        for( guint32 i = 0;
            i < pStmts->GetCount(); i++ )
        {
            ObjPtr pObj = pStmts->GetChild( i );
            guint32 dwClsid =
                ( guint32 )pObj->GetClsid();
            if( dwClsid != clsid( CInterfaceDecl ) )
                continue;

            CAstNodeBase* pNode = pObj;
            if( pNode == nullptr )
                continue;

            CInterfaceDecl* pifd = pObj;

            if( pifd->RefCount() == 0 )
                continue;

            stdstr strIfName = pifd->GetName();
            ObjPtr pmdlobj =
                pifd->GetMethodList();

            if( pmdlobj.IsEmpty() )
            {
                ret = -ENOENT;
                break;
            }

            CMethodDecls* pmdl = pmdlobj;
            if( bGenClient )
            {
                m_pWriter->SelectImplFile(
                    strIfName + "cli.cpp" );
                EMIT_COMMON_INC_CPP;
                CCOUT << "#include \"" << strIfName << "cli.h\"";
                NEW_LINE;
                if( bFuseP )
                {
                    CImplIufProxyFuse2 oiufp(
                        m_pWriter, pObj );
                    oiufp.Output();
                }
                else
                {
                    CImplIufProxy oiufp(
                        m_pWriter, pObj );
                    oiufp.Output();
                }

                for( guint32 i = 0;
                    i < pmdl->GetCount(); i++ )
                {
                    ObjPtr pmd = pmdl->GetChild( i );
                    if( bFuseP )
                    {
                        CImplIfMethodProxyFuse2 oiimp(
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
            }

            if( bGenServer )
            {
                m_pWriter->SelectImplFile(
                    strIfName + "svr.cpp" );
                EMIT_COMMON_INC_CPP;
                CCOUT << "#include \"" << strIfName << "svr.h\"";
                NEW_LINE;
                if( bFuseS )
                {
                    CImplIufSvrFuse2 oiufs(
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
                        CImplIfMethodSvrFuse2 oiims(
                            m_pWriter, pmd );
                        ret = oiims.Output();
                        if( ERROR( ret ) )
                            break;
                    }
                    else
                    {
                        CImplIfMethodSvr2 oiims(
                            m_pWriter, pmd );
                        ret = oiims.OutputROSSkel();
                        if( ERROR( ret ) )
                            break;
                    }
                }
            }
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
        ret = ReadJsonCfgFile(
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
            Json::Value oMaster1, oSkel1, oChan1;
            oMaster1 = oMaster;
            oSkel1 = oSkel;
            oChan1 = oChannel;
            oElems.append( oMaster );
            oElems.append( oSkel );
            oElems.append( oChan1 );
            ret = BuildObjDescROS( psd, oElems );
            if( ERROR( ret ) )
                break;
            oObjArray.append( oElems[ 0 ] );
            oObjArray.append( oElems[ 1 ] );
            oObjArray.append( oElems[ 2 ] );
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
            ":;";
        strCmdLine += stdstr( "s:XXXBUILTINRT:" ) +
            ( g_bBuiltinRt ? "True" : "False" ) +
            ":";

        const char* args[5];

        args[ 0 ] = "/bin/sed";
        args[ 1 ] = strCmdLine.c_str();
        args[ 2 ] = strSrcPy.c_str();
        args[ 3 ] = nullptr;
        char* env[ 1 ] = { nullptr };

        Execve( "/bin/sed",
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
        Json::Value& oChan = arrElems[ 2 ];

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
            if( g_bBuiltinRt )
            {
                oJif[ JSON_ATTR_NONSOCK_STREAM ] =
                    "true";
            }
            oIfArray.append( oJif );
        }

        oSkel[ JSON_ATTR_OBJNAME ] =
                strSvcName + "_SvrSkel";
        oSkel[ JSON_ATTR_BUSNAME ] =
            "DBusStreamBusPort";
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

        oChan[ JSON_ATTR_OBJNAME ] =
            strSvcName + "_ChannelSvr";

        Json::Value& oIfArray3 = oChan[ JSON_ATTR_IFARR ];
        gint32 iCount = 0;
        for( i = 0; i < oIfArray3.size(); i++ )
        {
            if( !oIfArray3[ i ].isMember(
                JSON_ATTR_IFNAME ) )
                continue;
            Json::Value& oIfElem = oIfArray3[ i ];
            if( oIfElem[ JSON_ATTR_IFNAME ] ==
                "RpcStreamChannelSvr" )
            {
                std::string strMainIf = "C";
                strMainIf += strSvcName + "_ChannelSvr";
                oIfElem[ JSON_ATTR_IFNAME ] =
                    strMainIf;
                oIfElem[ JSON_ATTR_BIND_CLSID ] =
                    "true";
                oIfElem[ JSON_ATTR_DUMMYIF ] =
                    "true";
                ++iCount;
            }
            else if( oIfElem[ JSON_ATTR_IFNAME ] ==
                "IStream" && g_bBuiltinRt ) 
            {
                oIfElem[ JSON_ATTR_NONSOCK_STREAM ] =
                    "true";
                oIfElem[ JSON_ATTR_SEQTGMGR ] =
                    "true";
                ++iCount;
            }
            else if( oIfElem[ JSON_ATTR_IFNAME ] ==
                "IStream" ) 
            {
                oIfElem[ JSON_ATTR_SEQTGMGR ] =
                    "true";
                ++iCount;
            }
            if( iCount == 2 )
                break;
        }
    }while( 0 );

    return ret;
}

gint32 CImplClassFactory2::OutputROS()
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

        if( g_bMklib && bFuse )
            Wa( "extern void InitMsgIds();" );

        Wa( "FactoryPtr InitClassFactory()" ); 
        BLOCK_OPEN;
        CCOUT << "BEGIN_FACTORY_MAPS;";
        NEW_LINES( 2 );

        for( auto& elem : vecSvcs )
        {
            CServiceDecl* pSvc = elem;
            if( IsServer() && bGenServer )
            {
                CCOUT << "INIT_MAP_ENTRYCFG( C"
                    << pSvc->GetName()
                    << "_SvrImpl );";
                NEW_LINE;
                CCOUT << "INIT_MAP_ENTRYCFG( C"
                    << pSvc->GetName()
                    << "_SvrSkel );";
                NEW_LINE;
                CCOUT << "INIT_MAP_ENTRYCFG( C"
                    << pSvc->GetName()
                    << "_ChannelSvr );";
                NEW_LINE;
            }
            if( ( !IsServer() || g_bMklib ) && bGenClient )
            {
                CCOUT << "INIT_MAP_ENTRYCFG( C"
                    << pSvc->GetName()
                    << "_CliImpl );";
                NEW_LINE;
                CCOUT << "INIT_MAP_ENTRYCFG( C"
                    << pSvc->GetName()
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
