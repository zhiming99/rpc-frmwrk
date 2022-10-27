/*
 * =====================================================================================
 *
 *       Filename:  genpy2.cpp
 *
 *    Description:  implementations of classes of python skelton generator
 *
 *        Version:  1.0
 *        Created:  10/23/2022 08:47:14 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
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
#include "genpy.h"
#include "genpy2.h"
#include "../fuse/fusedefs.h"
extern stdstr g_strAppName;
extern bool g_bAsyncProxy;
extern gint32 SetStructRefs( ObjPtr& pRoot );
extern guint32 GenClsid( const std::string& strName );
extern gint32 GenInitFile(
    CPyWriter* pWriter, ObjPtr& pRoot );

extern gint32 EmitFormalArgListPy(
    CWriterBase* pWriter, CArgList* pInArgs );

static gint32 EmitSerialBySig(
    CWriterBase* m_pWriter,
    const stdstr& strDest,
    const stdstr& strField,
    const stdstr& strSig,
    bool bNoRet = false )
{
    gint32 ret = 0;
    switch( strSig[ 0 ] )
    {
    case '(' :
        {
            CCOUT << "iRet = osb.SerialArray("
                << strField << ", \"" << strSig << "\" )";
            NEW_LINE;
            CCOUT << "if iRet[ 0 ] < 0 :";
            NEW_LINE;
            if( bNoRet )
                CCOUT << "    break";
            else
                CCOUT << "    return None";
            NEW_LINE;
            CCOUT << strDest << " = iRet[ 1 ]";
            break;
        }
    case '[' :
        {
            CCOUT << "iRet = osb.SerialMap( "
                << strField << ", \"" << strSig << "\" )";
            NEW_LINE;
            CCOUT << "if iRet[ 0 ] < 0 :";
            NEW_LINE;
            if( bNoRet )
                CCOUT << "    break";
            else
                CCOUT << "    return None";
            NEW_LINE;
            CCOUT << strDest << " = iRet[ 1 ]";
            break;
        }

    case 'O' :
        {
            CCOUT << "iRet = osb.SerialStruct( " << strField << " )";
            NEW_LINE;
            CCOUT << "if iRet is None :";
            NEW_LINE;
            if( bNoRet )
                CCOUT << "    break";
            else
                CCOUT << "    return None";
            NEW_LINE;
            CCOUT << strDest << " = iRet[ 1 ]";
            break;
        }
    case 'Q':
    case 'q':
        {
            CCOUT << strDest << " = osb.SerialInt64( " << strField << " )";
            break;
        }
    case 'D':
    case 'd':
        {
            CCOUT << strDest << " = osb.SerialInt32( " << strField << " )";
            break;
        }
    case 'W':
    case 'w':
        {
            CCOUT << strDest << " = osb.SerialInt16( " << strField << " )";
            break;
        }
    case 'b':
    case 'B':
        {
            CCOUT << strDest << " = osb.SerialInt8( " << strField << " )";
            break;
        }
    case 'f':
        {
            CCOUT << strDest << " = osb.SerialFloat( " << strField << " )";
            break;
        }
    case 'F':
        {
            CCOUT << strDest << " = osb.SerialDouble( " << strField << " )";
            break;
        }
    case 's':
        {
            CCOUT << strDest << " = osb.SerialString( " << strField << " )";
            break;
        }
    case 'a':
        {
            CCOUT << strDest << " = osb.SerialBuf( " << strField << " )";
            break;
        }
    case 'o':
        {
            CCOUT << strDest << " = osb.SerialObjPtr( " << strField << " )";
            break;
        }
    case 'h':
        {
            CCOUT << strDest << " = osb.SerialHStream( " << strField << " )";
            break;
        }
    default:
        {
            ret = -EINVAL;
            break;
        }
    }

    if( SUCCEEDED( ret ) )
        NEW_LINE;

    return ret;
}

static gint32 EmitDeserialBySig(
    CWriterBase* m_pWriter,
    const stdstr& strDest,
    const stdstr& strSrc,
    const stdstr& strSig )
{
    gint32 ret = 0;
    switch( strSig[ 0 ] )
    {
    case '(' :
        {
            CCOUT << "iRet = osb.DeserialArray( " << strSrc <<", \""
                << strSig << "\" )";
            NEW_LINE;
            Wa( "if iRet[ 0 ] < 0:" );
            Wa( "    break" );
            CCOUT << strDest << " = iRet[ 1 ]";
            break;
        }
    case '[' :
        {
            CCOUT << "iRet = osb.DeserialMap( " << strSrc <<", \""
                << strSig << "\" )";
            NEW_LINE;
            Wa( "if iRet[ 0 ] < 0:" );
            Wa( "    break" );
            CCOUT << strDest << " = iRet[ 1 ]";
            break;
        }

    case 'O' :
        {
            CCOUT << "iRet = osb.DeserialStruct( " << strSrc <<" )";
            NEW_LINE;
            Wa( "if iRet[ 0 ] < 0:" );
            Wa( "    break" );
            CCOUT << strDest << " = iRet[ 1 ]";
            break;
        }
    case 'Q':
    case 'q':
        {
            CCOUT << strDest << " = osb.DeserialInt64( " << strSrc <<" )";
            break;
        }
    case 'D':
    case 'd':
        {
            CCOUT << strDest << " = osb.DeserialInt32( " << strSrc <<" )";
            break;
        }
    case 'W':
    case 'w':
        {
            CCOUT << strDest << " = osb.DeserialInt16( " << strSrc <<" )";
            break;
        }
    case 'b':
    case 'B':
        {
            CCOUT << strDest << " = osb.DeserialInt8( " << strSrc <<" )";
            break;
        }
    case 'f':
        {
            CCOUT << strDest << " = osb.DeserialFloat( " << strSrc <<" )";
            break;
        }
    case 'F':
        {
            CCOUT << strDest << " = osb.DeserialDouble( " << strSrc <<" )";
            break;
        }
    case 's':
        {
            CCOUT << strDest << " = osb.DeserialString( " << strSrc <<" )";
            break;
        }
    case 'a':
        {
            CCOUT << strDest << " = osb.DeserialBuf( " << strSrc <<" )";
            break;
        }
    case 'o':
        {
            CCOUT << strDest << " = osb.DeserialObjPtr( " << strSrc <<" )";
            break;
        }
    case 'h':
        {
            CCOUT << strDest << " = osb.DeserialHStream( " << strSrc <<" )";
            break;
        }
    default:
        {
            ret = -EINVAL;
            break;
        }
    }
    if( SUCCEEDED( ret ) )
        NEW_LINE;

    return ret;
}


gint32 EmitPyArgPack( 
    CWriterBase* m_pWriter,
    CArgList* pArgList,
    const stdstr& strArgPack )
{
    gint32 ret = 0;
    do{
        CCOUT << strArgPack << " = [";
        INDENT_UPL;
        guint32 dwCount = pArgList->GetCount();
        for( guint32 i = 0; i < dwCount; i++ )
        {
            ObjPtr pObj = pArgList->GetChild( i );
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strName = pfa->GetName();
            CCOUT << strName;
            if( i < dwCount - 1 )
            {
                CCOUT << ",";
            }
            else
            {
                CCOUT << " ]";
            }
            if( i < dwCount - 1 )
                NEW_LINE;
        }
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 EmitPyBuildReqHeader( 
    CWriterBase* m_pWriter )
{
    Wa( "def BuildReqHeader(" );
    Wa( "    repId : object," );
    Wa( "    strMethod : str," );
    Wa( "    strIfName : str,  " );
    Wa( "    retCode : int," );
    Wa( "    bProxy : bool," );
    Wa( "    bResp : bool ) -> object:" );
    INDENT_UPL;
    Wa( "oReq = dict()" );
    CCOUT << "if bProxy :";
    NEW_LINE;
    CCOUT << "    oReq[ \"" << JSON_ATTR_MSGTYPE << "\" ] = \"req\"";
    NEW_LINE;
    CCOUT << "elif bResp :";
    NEW_LINE;
    CCOUT << "    oReq[ \"" << JSON_ATTR_MSGTYPE << "\" ] = \"resp\"";
    NEW_LINE;
    CCOUT << "    oReq[ \"" << JSON_ATTR_RETCODE << "\" ] = \"retCode\"";
    NEW_LINE;
    CCOUT << "else :";
    NEW_LINE;
    CCOUT << "    oReq[ \"" << JSON_ATTR_MSGTYPE << "\" ] = \"evt\"";
    NEW_LINE;
    CCOUT << "oReq[ \"" << JSON_ATTR_IFNAME1 << "\" ] = strIfName";
    NEW_LINE;
    CCOUT << "oReq[ \"" << JSON_ATTR_REQCTXID << "\" ] = reqId";
    NEW_LINE;
    CCOUT << "oReq[ \"" << JSON_ATTR_METHOD << "\" ] = strMethod";
    NEW_LINE;
    CCOUT << "return oReq";
    INDENT_DOWNL;

    return 0;
}

gint32 EmitImports(
    CWriterBase* m_pWriter )
{
    Wa( "#Generated by RIDLC, "
        "make sure to backup before recompile" );
    Wa( "import sys" );
    Wa( "from rpcf import iolib" );
    Wa( "from rpcf import serijson" );
    Wa( "import errno" );
    Wa( "from rpcf import ErrorCode as Err" );
    return 0;
}

static gint32 EmitPySerialReq( 
    CWriterBase* m_pWriter,
    ObjPtr pArgs,
    const stdstr strDDict,
    const stdstr strSList )
{
    gint32 ret = 0;
    CArgList* pArgList = pArgs;
    guint32 dwCount = pArgList->GetCount();

    do{
        bool bCheck = false;
        for( guint32 i = 0; i < dwCount; i++ )
        {
            CFormalArg* pfa =
                pArgList->GetChild( i );

            CAstNodeBase* pType =
                pfa->GetType();

            stdstr strSig =
                pType->GetSignature();

            int ch = strSig[ 0 ];
            if( ch == '(' || ch == '[' || ch == 'O' )
            {
                bCheck = true;
                break;
            }
        }

        if( bCheck )
            Wa( "iRet = [ 0, None ]" );
        CCOUT << strDDict << " = dict()";
        NEW_LINE;
        CCOUT << "while ret == 0:";
        INDENT_UPL;
        for( guint32 i = 0; i < dwCount; i++ )
        {
            CFormalArg* pfa = pArgList->GetChild( i );
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strSrc = strSList + "[";
            strSrc += std::to_string( i ) + "]";

            stdstr strDest = strDDict + "[ \"";
            strDest += pfa->GetName() + "\" ]";

            CAstNodeBase* pNode = pfa->GetType();
            std::string strSig =
                pNode->GetSignature();

            ret = EmitSerialBySig(
                m_pWriter, strDest,
                strSrc, strSig, true );
            if( ERROR( ret ) )
                break;
            
        } 
        if( ERROR( ret ) )
            break;
        CCOUT << "break";
        INDENT_DOWNL;
        if( bCheck )
        {
            Wa( "if iRet[ 0 ] < 0 :" );
            Wa( "    error = iRet[ 0 ]" );
            Wa( "    raise Exception( " );
            CCOUT <<  "        \"Error deserialize "
                << "parameters %d\" \% error  )";
            NEW_LINE;
        }

    }while( 0 );
    return ret;
}

void EmitExceptHandler( 
    CWriterBase* m_pWriter,
    bool bList, bool bNoRet = false,
    const stdstr errcode = "-errno.EFAULT" )
{

    CCOUT << "except Exception as err:";
    INDENT_UPL;
    Wa( "print( err )" );
    if( bNoRet )
    {
        Wa( "return" );
        INDENT_DOWNL;
        return;
    }
    Wa( "if error < 0 :" );
    if( !bList )
        Wa( "    return error" );
    else
        Wa( "    return [ error, None ]" );
    CCOUT << "return " << errcode;
    INDENT_DOWNL;
    return;
}

gint32 EmitPyDeserialReq( 
    CWriterBase* m_pWriter,
    ObjPtr pArgs,
    const stdstr strDList,
    const stdstr strSDict )
{
    gint32 ret = 0;
    CArgList* pArgList = pArgs;
    guint32 dwCount = pArgList->GetCount();

    do{
        bool bCheck = false;
        for( guint32 i = 0; i < dwCount; i++ )
        {
            CFormalArg* pfa =
                pArgList->GetChild( i );

            CAstNodeBase* pType =
                pfa->GetType();

            stdstr strSig =
                pType->GetSignature();

            int ch = strSig[ 0 ];
            if( ch == '(' || ch == '[' || ch == 'O' )
            {
                bCheck = true;
                break;
            }
        }

        if( bCheck )
            Wa( "iRet = [ 0, None ]" );
        CCOUT << strDList << "= [ None ] *"
            << std::to_string( dwCount );
        NEW_LINE;
        CCOUT << "while True:";
        INDENT_UPL;
        for( guint32 i = 0; i < dwCount; i++ )
        {
            CFormalArg* pfa =
                pArgList->GetChild( i );

            CAstNodeBase* pType =
                pfa->GetType();

            stdstr strSig =
                pType->GetSignature();

            stdstr strDest = "args[";
            strDest += std::to_string( i ) + " ]";

            stdstr strSrc = "oParams[ \"";
            strSrc += pfa->GetName() + "\" ]";

            ret = EmitDeserialBySig( m_pWriter,
                strDest, strSrc, strSig );

            if( ERROR( ret ) )
                break;
        }
        if( ERROR( ret ) )
            break;
        CCOUT << "break";
        INDENT_DOWNL; // while
        if( bCheck )
        {
            Wa( "if iRet[ 0 ] < 0 :" );
            Wa( "    error = iRet[ 0 ]" );
            Wa( "    raise Exception( " );
            CCOUT <<  "        \"Error deserialize "
                << "parameters %d\" \% error  )";
            NEW_LINE;
        }

    }while( 0 );
    return ret;
}

CPyFileSet2::CPyFileSet2(
    const stdstr& strOutPath,
    const stdstr& strAppName )
    : super( strOutPath )
{
    if( strAppName.empty() )
    {
        std::string strMsg =
            "appname is empty";
        throw std::invalid_argument( strMsg );
    }

    m_strInitPy =
        strOutPath + "/" + "__init__.py";

    GEN_FILEPATH( m_strStructsPy,
        strOutPath, strAppName + "structs.py",
        false );

    GEN_FILEPATH( m_strMakefile,
        strOutPath, "Makefile",
        false );

    GEN_FILEPATH( m_strMainCli,
        strOutPath, "maincli.py",
        true );

    GEN_FILEPATH( m_strMainSvr, 
        strOutPath, "mainsvr.py",
        true );

    GEN_FILEPATH( m_strIfImpl, 
        strOutPath, "ifimpl.py",
        false );

    GEN_FILEPATH( m_strReadme, 
        strOutPath, "README.md",
        false );
    m_strPath = strOutPath;

    gint32 ret = OpenFiles();
    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "internal error open files" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CPyFileSet2::OpenFiles()
{
    STMPTR pstm( new std::ofstream(
        m_strStructsPy,
        std::ofstream::out |
        std::ofstream::trunc ) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm= STMPTR( new std::ofstream(
        m_strInitPy,
        std::ofstream::out |
        std::ofstream::trunc ) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strMakefile,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strMainCli,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strMainSvr,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strIfImpl,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strReadme,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    return STATUS_SUCCESS;
}

gint32 CPyFileSet2::AddSvcImpl(
    const stdstr& strSvcName )
{
    if( strSvcName.empty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        gint32 idx = m_vecFiles.size();
        std::string strExt = ".py";
        std::string strSvrPy = m_strPath +
            "/" + strSvcName + "svr.py";
        std::string strCliPy = m_strPath +
            "/" + strSvcName + "cli.py";

        ret = access( strSvrPy.c_str(), F_OK );
        if( ret == 0 )
        {
            strExt = "py.new";
            strSvrPy = m_strPath + "/" +
                strSvcName + "svr.py.new";
        }

        ret = access( strCliPy.c_str(), F_OK );
        if( ret == 0 )
        {
            strExt = ".py.new";
            strCliPy = m_strPath + "/" +
                strSvcName + "cli.py.new";
        }

        STMPTR pstm( new std::ofstream(
            strSvrPy,
            std::ofstream::out |
            std::ofstream::trunc) );

        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strSvrPy ] = idx;

        pstm = STMPTR( new std::ofstream(
            strCliPy,
            std::ofstream::out |
            std::ofstream::trunc) );

        idx += 1;
        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strCliPy ] = idx;

    }while( 0 );

    return ret;
}

gint32 CDeclarePyStruct2::Output()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        CCOUT << "class " << strName
            <<  "( CStructBase ) :";
        INDENT_UPL;
    
        NEW_LINE;

        ObjPtr pFields =
            m_pNode->GetFieldList();

        CFieldList* pfl = pFields;
        guint32 dwCount = pfl->GetCount();
        if( dwCount == 0 )
        {
            ret = -ENOENT;
            break;
        }

        ObjPtr pParent = m_pNode->GetParent();
        CStatements* pstmts = pParent;

        std::string strAppName =
            pstmts->GetName();

        std::string strMsgId =
            strAppName + "::" + strName;

        guint32 dwMsgId = GenClsid( strMsgId );
        stdstr strClsid = FormatClsid( dwMsgId );

        Wa( "#Message identity" );

        Wa( "@classmethod" );
        Wa( "def GetStructId( cls ) -> int:" );
        CCOUT << "    return "<< strClsid;

        NEW_LINES( 2 );

        Wa( "@classmethod" );
        Wa( "def GetStructName( cls ) -> str:" );
        CCOUT << "    return \""<< strMsgId << "\"";

        NEW_LINES( 2 );

        Variant oVar;
        oVar = dwMsgId;
        m_pNode->SetProperty(
            PROP_MESSAGE_ID, oVar );

        CCOUT << "def __init__( self ) :";
        INDENT_UPL;
        CCOUT << "super().__init__( self )";
        NEW_LINE;

        for( guint32 i = 0; i < dwCount; i++ )
        {
            ObjPtr pObj = pfl->GetChild( i );
            CFieldDecl* pfd = pObj;
            if( pfd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strName = pfd->GetName();
            CAstNodeBase* pType = pfd->GetType();
            if( pType == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strSig = pType->GetSignature();
            BufPtr& pVal = pfd->GetVal();

            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    CCOUT << "self." << strName << " = list() ";
                    break;
                }
            case '[' :
                {
                    CCOUT << "self." << strName << " = dict() ";
                    break;
                }

            case 'O' :
                {
                    CCOUT << "self." << strName << " = object() ";
                    break;
                }
            case 'h':
                {
                    CCOUT << "self." << strName << " = 0";
                    break;
                }
            case 'Q':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = 0";
                    else
                    {
                        guint64 qwVal = *pVal;
                        CCOUT << "self." << strName << " = " << qwVal;
                    }
                    break;
                }
            case 'q':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = 0";
                    else
                    {
                        gint64 qwVal = *pVal;
                        CCOUT << "self." << strName << " = " << qwVal;
                    }
                    break;
                }
            case 'D':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = 0";
                    else
                    {
                        guint32 dwVal = *pVal;
                        CCOUT << "self." << strName << " = " << dwVal;
                    }
                    break;
                }
            case 'd':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = 0";
                    else
                    {
                        gint32 iVal = *pVal;
                        CCOUT << "self." << strName << " = " << iVal;
                    }
                    break;
                }
            case 'W':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = 0";
                    else
                    {
                        guint16 wVal = *pVal;
                        CCOUT << "self." << strName << " = " << wVal;
                    }
                    break;
                }
            case 'w':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = 0";
                    else
                    {
                        gint16 sVal = *pVal;
                        CCOUT << "self." << strName << " = " << sVal;
                    }
                    break;
                }
            case 'b':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = True";
                    else
                    {
                        bool bVal = *pVal;
                        if( bVal )
                            CCOUT << "self." << strName << " = " << "True";
                        else
                            CCOUT << "self." << strName << " = " << "False";
                    }
                    break;
                }
            case 'B':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = 0";
                    else
                    {
                        guint8 byVal = *pVal;
                        CCOUT << "self." << strName << " = " << byVal;
                    }
                    break;
                }
            case 'f':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = .0";
                    else
                    {
                        float fVal = *pVal;
                        CCOUT << "self." << strName << " = " << fVal;
                    }
                    break;
                }
            case 'F':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = .0";
                    else
                    {
                        double dblVal = *pVal;
                        CCOUT << "self." << strName << " = " << dblVal;
                    }
                    break;
                }
            case 's':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "self." << strName << " = ''";
                    else
                    {
                        const char* szVal = pVal->ptr();
                        CCOUT << "self." << strName << " = \"" << szVal << "\"";
                    }
                    break;
                }
            case 'a':
                {
                    CCOUT << "self." << strName << " = bytearray()";
                    break;
                }
            case 'o':
                {
                    CCOUT << "self." << strName << " = None";
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
            NEW_LINE;
        }
        INDENT_DOWNL;

        CCOUT << "def Serialize( self ) -> [ int, dict ]:";
        INDENT_UPL;
        Wa( "osb = serijson.CSerialBase()" );
        Wa( "res = dict()" );
        NEW_LINE;
        CCOUT << "res[ \"" << JSON_ATTR_STRUCTID
            << "\" ] = self.GetStructId()";
        NEW_LINE;
        Wa( "res[ 'StructName' ] = self.GetStructName()" );
        Wa( "fields = dict()" );

        bool bCheck = false;
        for( guint32 i = 0; i < dwCount; i++ )
        {
            CFieldDecl* pfd =
                pfl->GetChild( i );

            CAstNodeBase* pType =
                pfd->GetType();

            stdstr strSig =
                pType->GetSignature();

            int ch = strSig[ 0 ];
            if( ch == '(' || ch == '[' || ch == 'O' )
            {
                bCheck = true;
                break;
            }
        }

        if( bCheck )
            Wa( "iRet = [ 0, None ]" );

        CCOUT << "while True:";
        INDENT_UPL;
        NEW_LINE;
        for( guint32 i = 0; i < dwCount; i++ )
        {
            CFieldDecl* pfd = pfl->GetChild( i );
            if( pfd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strName = pfd->GetName();
            CAstNodeBase* pType = pfd->GetType();
            if( pType == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strSig = pType->GetSignature();

            stdstr strDest = "fields[\"";
            strDest += strName + "\"]";
            stdstr strField = "self.";
            strField += strName;
            ret = EmitSerialBySig( m_pWriter,
                strDest, strField, strSig, true );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }
        Wa( "break" );
        INDENT_DOWNL;
        if( bCheck )
        {
            Wa( "if iRet[ 0 ] < 0 :" );
            Wa( "    return iRet" );
        }
        NEW_LINE;
        Wa( "res[ \"Fields\" ] = fields" );
        Wa( "return [ 0, res ]" );
        INDENT_DOWNL;
        NEW_LINE;

        CCOUT << "def Deserialize( self, val : dict ) -> int:";
        INDENT_UPL;
        NEW_LINE;
        Wa( "osb = serijson.CSerialBase()" );
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        CCOUT << "ret = osb.DeserialInt32( val[ \""
            << JSON_ATTR_STRUCTID <<"\" ] )";

        NEW_LINE;
        Wa( "if ret != self.GetStructId() :" );
        Wa( "    return -errno.EBADMSG" );
        Wa( "fields = val[ \"Fields\" ]" );
        NEW_LINE;

        if( bCheck )
            Wa( "iRet = [ 0, None ]" );

        CCOUT << "while True:";
        INDENT_UPL;
        for( guint32 i = 0; i < dwCount; i++ )
        {
            ObjPtr pObj = pfl->GetChild( i );
            CFieldDecl* pfd = pObj;
            if( pfd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strName = pfd->GetName();
            CAstNodeBase* pType = pfd->GetType();
            if( pType == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strDest = "self.";
            strDest += strName;
            stdstr strSrc = "fields[ \"";
            strSrc += strName + "\" ]";
            stdstr strSig = pType->GetSignature();

            ret = EmitDeserialBySig(
                m_pWriter, strDest, strSrc, strSig );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }
        if( ERROR( ret ) )
            break;
        Wa( "break" );
        INDENT_DOWNL;
        if( bCheck )
        {
            Wa( "if iRet[ 0 ] < 0 :" );
            Wa( "    return iRet[ 0 ]" );
        }
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter,
            true, false, "Err.ERROR_FAIL" );
        Wa( "return 0" );
        INDENT_DOWN;
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

static gint32 GenStructsFilePy2(
    CPyWriter2* m_pWriter, ObjPtr& pRoot )
{
    if( m_pWriter == nullptr ||
        pRoot.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CStatements* pStmts = pRoot;
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        auto pWriter = m_pWriter;
        pWriter->SelectStructsFile();
        Wa( "#Generated by RIDLC" );
        Wa( "from rpcf import serijson" );
        Wa( "from rpcf import ErrorCode as Err" );
        Wa( "import errno" );

        pWriter->NewLine();

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

        for( auto& elem : vecActStructs )
        {
            CDeclarePyStruct2 odps( pWriter, elem );
            ret = odps.Output();
            if( ERROR( ret ) )
                break;
        }

        if( vecActStructs.size() > 0 )
        {
            Wa( "from seribase import g_mapStructs" );
        }
        for( int i = 0; i < vecActStructs.size(); i++ )
        {
            CStructDecl* pStruct = vecActStructs[ i ];
            stdstr strName = pStruct->GetName(); 
            stdstr strMsgId =
                g_strAppName + "::" + strName;
            guint32 dwMsgId = GenClsid( strMsgId );
            stdstr strClsid = FormatClsid( dwMsgId );
            CCOUT << "g_mapStructs[ " << strClsid
                << " ] = " << strName;
            if( i < vecActStructs.size() - 1 )
                NEW_LINE;
        }

    }while( 0 );

    return ret;
}

static gint32 GenIfImplFile(
    CWriterBase* m_pWriter,
    ObjPtr pRoot )
{
    gint32 ret = 0;

    do{
        EmitImports( m_pWriter );
        EmitPyBuildReqHeader( m_pWriter );
        NEW_LINE;

        auto pWriter =
            static_cast< CPyWriter2*>( m_pWriter );
        CImplPyInterfaces2 oipifs(
            pWriter, pRoot );
        ret = oipifs.Output();

    }while( 0 );

    return ret;
}

static gint32 GenSvcFiles2(
    CPyWriter* pWriter, ObjPtr& pRoot )
{
    if( pWriter == nullptr ||
        pRoot.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{

        CStatements* pStmts = pRoot;
        if( unlikely( pStmts == nullptr ) )
        {
            ret = -EFAULT;
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
            // client base imlementation
            stdstr strCommon =
                pWriter->GetOutPath() +
                "/" + elem.first; 

            // server imlementation
            ret = pWriter->SelectImplFile(
                strCommon + "svr.py" );
            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    strCommon + "svr.py.new" );
                if( ERROR( ret ) )
                    break;
            }

            CImplPySvcSvr2 opss(
                pWriter, elem.second );
            ret = opss.Output();
            if( ERROR( ret ) )
                break;

            // client imlementation
            ret = pWriter->SelectImplFile(
                strCommon + "cli.py" );
            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    strCommon + "cli.py.new" );
                if( ERROR( ret ) )
                    break;
            }

            CImplPySvcProxy2 opsc(
                pWriter, elem.second );
            ret = opsc.Output();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

gint32 GenPyProj2(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot )
{
    if( strOutPath.empty() ||
        strAppName.empty() ||
        pRoot.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;

    do{
        struct stat sb;
        if( lstat( strOutPath.c_str(), &sb ) == -1 )
        {
            std::string strCmd = "mkdir -p ";
            strCmd += strOutPath;
            ret = system( strCmd.c_str() );
            ret = 0;
        }
        else
        {
            mode_t iFlags =
                ( sb.st_mode & S_IFMT );
            if( iFlags != S_IFDIR )
            {
                std::string strMsg =
                    "error '";
                strMsg += strOutPath;
                strMsg +=
                   "' is not a valid directory";
                printf( "%s\n",
                    strMsg.c_str() );
                break;
            }
        }

        SetStructRefs( pRoot );

        CPyWriter2 oWriter(
            strOutPath, strAppName, pRoot );

        ret = GenStructsFilePy2( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

        ret = GenInitFile( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

        oWriter.SelectMakefile();
        CPyExportMakefile2 opemk( &oWriter, pRoot );
        ret = opemk.Output();
        if( ERROR( ret ) )
            break;

        oWriter.SelectIfImpl();
        ret = GenIfImplFile( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

        ret = GenSvcFiles2( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

        CImplPyMainFunc2 opmf( &oWriter, pRoot );
        ret = opmf.Output();
        if( ERROR( ret ) )
            break;

        oWriter.SelectReadme();
        CExportPyReadme2 ordme( &oWriter, pRoot );
        ret = ordme.Output();

    }while( 0 );

    return ret;
}

gint32 CPyExportMakefile2::Output()
{
    Wa( "all:" );
    Wa( "\tmake -C fs" );
    return 0;
}

CImplPyInterfaces2::CImplPyInterfaces2(
    CPyWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'service' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplPyInterfaces2::Output()
{
    gint32 ret = 0;
    do{
        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfDecls( vecIfs );
        for( int i = 0; i < vecIfs.size(); ++i )
        {
            auto& elem = vecIfs[ i ];
            CInterfaceDecl* pifd = elem;
            if( pifd == nullptr )
                continue;
            if( pifd->RefCount() == 0 )
                continue;

            CImplPyIfSvrBase2 oifps(
                m_pWriter, elem );

            ret = oifps.Output();
            if( ERROR( ret ) )
                break;

            CImplPyIfProxyBase2 oifpp(
                m_pWriter, elem );

            ret = oifpp.Output();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

CImplPyIfSvrBase2::CImplPyIfSvrBase2(
    CPyWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interface' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplPyIfSvrBase2::Output()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        CCOUT << "class I" << strIfName << "_SvrImpl :";
        INDENT_UP;
        NEW_LINES( 2 );
        CCOUT << "_ifName_ = \"" <<  strIfName << "\"";
        NEW_LINE;

        ret = OutputDispMsg();
        if( ERROR( ret ) )
            break;

        ObjPtr pObj = m_pNode->GetMethodList();
        std::vector< ObjPtr > vecMethods;
        CMethodDecls* pmds = pObj;
        guint32 i = 0;
        for( ; i < pmds->GetCount(); i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            vecMethods.push_back( pObj );
        }

        if( vecMethods.empty() )
        {
            ret = -ENOENT;
            break;
        }

        for( auto& elem : vecMethods )
        {
            CImplPyMthdSvrBase2 oism(
                m_pWriter, elem );
            oism.Output();
        }
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyIfSvrBase2::OutputDispMsg()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        ObjPtr pObj = m_pNode->GetMethodList();
        std::vector< ObjPtr > vecMethods;
        CMethodDecls* pmds = pObj;
        guint32 i = 0;
        for( ; i < pmds->GetCount(); i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            vecMethods.push_back( pObj );
        }

        if( vecMethods.empty() )
        {
            ret = -ENOENT;
            break;
        }

        stdstr strClass = "I";
        strClass += strIfName + "_SvrImpl";
        CCOUT << "_funcMap_ = {";
        INDENT_UPL;
        for( guint32 i = 0; i < vecMethods.size(); i++ )
        {
            CMethodDecl* pNode = vecMethods[ i ];
            if( pNode == nullptr )
                continue;
            stdstr strName = pNode->GetName();
            CCOUT << "\"" << strName
                << "\": lambda self, req : "
                << strClass << "."
                << strName << "Wrapper( self, req )";
            if( i < vecMethods.size() - 1 )
            {
                CCOUT << ",";
                NEW_LINE;
            }
        }
        
        Wa( " }" );
        INDENT_DOWNL;

        CCOUT << "def DispatchMsg( self, oMsg : dict ):";
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        CCOUT << "strMethod = oMsg[ \""
            << JSON_ATTR_METHOD "\" ]";
        NEW_LINE;
        CCOUT << "method = " << strClass <<"._funcMap_[ strMethod ]";
        NEW_LINE;
        Wa( "method( self, oMsg )" );
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter,
            false, true, "-errno.EINVAL" );
        INDENT_DOWNL

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdSvrBase2::Output()
{
    if( m_pNode->IsEvent() )
        return OutputEvent();
    return OutputSync();
}

gint32 CImplPyMthdSvrBase2::OutputSync()
{
    gint32 ret = 0;

    do{
        stdstr strName = m_pNode->GetName();
        stdstr strIfName = m_pIf->GetName();
        CArgList* pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount = GetArgCount(
            m_pNode->GetInArgs() );
        CArgList* pOutArgs = m_pNode->GetOutArgs();
        guint32 dwOutCount = GetArgCount(
            m_pNode->GetOutArgs() );
        bool bNoReply = m_pNode->IsNoReply();

        Wa( "'''" );
        Wa( "Request handler" );
        Wa( "within which to run the business logic." );
        Wa( "Returning Err.STATUS_PENDING for an" );
        Wa( "asynchronous operation and returning other" );
        Wa( "error code will complete the request. if the" );
        Wa( "method returns STATUS_PENDING, make sure to" );
        CCOUT << "call \"" << strName << "CompleteCb\" later to";
        NEW_LINE;
        Wa( "complete the request. Otherwise, the client" );
        Wa( "will get a timeout error. The return value is a" );
        Wa( "list, with the first element as error code and" );
        Wa( "the second element as a list of the response" );
        Wa( "parameters." );
        Wa( "'''" );
        CCOUT << "def " << strName << "( self,";
        if( dwInCount > 0 )
        {
            CCOUT << " reqId : object,";
            INDENT_UPL;
            ret = EmitFormalArgListPy(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;
            INDENT_DOWN;
        }
        else
        {
            CCOUT << " reqId : object";
        }
        CCOUT << " ) -> Tuple[ int, list ] :";
        NEW_LINE;
        Wa( "    pass" );

        Wa( "'''" );
        Wa( "Request handler wrapper" );
        Wa( "which will deserialize the incoming parameters,");
        Wa( "call the actual request handler, and return the" );
        Wa( "serialized response." );
        Wa( "'''" );

        CCOUT << "def " << strName << "Wrapper( self";
        if( dwInCount > 0 )
            CCOUT << ", oReq : dict";
        CCOUT << " ) -> int :";
        INDENT_UPL;

        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        if( dwInCount == 0 )
        {
            CCOUT << "reqId = oReq[ \""
                << JSON_ATTR_REQCTXID << "\" ]";
            NEW_LINE;
            CCOUT << "ret = self." << strName << "( reqId );";
            NEW_LINE;
        }
        else
        {
            CCOUT << "reqId = oReq[ \""
                << JSON_ATTR_REQCTXID << "\" ]";
            NEW_LINE;
            Wa( "osb = serijson.CSerialBase()" );
            CCOUT << "oParams = oReq[ \""
                << JSON_ATTR_PARAMS << "\" ]";
            NEW_LINE;
            ret = EmitPyDeserialReq( m_pWriter,
                pInArgs, "args", "oParams" );
            if( ERROR( ret ) )
                break;

            CCOUT << "ret = self."
                << strName << "( reqId, *args )";
            NEW_LINE;
        }

        if( bNoReply )
        {
            Wa( "return 0" );
            INDENT_DOWNL;
            EmitExceptHandler( m_pWriter, false );
            INDENT_DOWNL;
            NEW_LINE;
            break;
        }

        Wa( "bPending = ( ret[ 0 ] ==" );
        Wa( "    Err.STATUS_PENDING )" );

        Wa( "if bPending :" );
        Wa( "    return Err.STATUS_PENDING" );

        Wa( "iRet = ret[ 0 ]" );
        Wa( "retArgs = ret[ 1 ]" );
        Wa( "if iRet < 0:" );
        CCOUT << "    retArgs = [ None ] * "
            << std::to_string( dwOutCount );
        NEW_LINE;
        CCOUT << "ret = self." << strName << "CompleteCb("
            << "iRet, reqId";
        if( dwOutCount == 0 )
            CCOUT << " )";
        else
        {
            CCOUT << ", *retArgs )";
        }
        NEW_LINE;
        Wa( "return ret" );
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, false );
        INDENT_DOWNL;
        NEW_LINE;

        Wa( "'''" );
        Wa( "Call this method when the request" );
        Wa( "is completed asynchronously" );
        Wa( "'''" );
        CCOUT << "def " << strName << "CompleteCb( self,";
        NEW_LINE;
        if( dwOutCount > 0 )
        {
            INDENT_UP;
            CCOUT << "ret : int, reqId : object,";
            ret = EmitFormalArgListPy(
                m_pWriter, pOutArgs );
            if( ERROR( ret ) )
                break;
            INDENT_DOWN;
        }
        else
        {
            CCOUT << "    ret : int, reqId : object";
        }
        CCOUT << " ) -> int :";
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        if( dwOutCount > 0 )
        {
            Wa( "if ret == 0 :" );
            Wa( "    resArgs = dict()" );
            CCOUT << "else:";
            INDENT_UPL;
            EmitPyArgPack( m_pWriter, pOutArgs, "argPack" );
            ret = EmitPySerialReq( m_pWriter,
                pOutArgs, "resArgs", "argPack" );
            if( ERROR( ret ) )
                break;
            INDENT_DOWNL;
        }
        else
        {
            Wa( "resArgs = dict()" );
        }

        Wa( "oResp = BuildReqHeader( reqId," );
        CCOUT << "    \"" << JSON_ATTR_METHOD << "\",";
        NEW_LINE;
        CCOUT << "    \"" << JSON_ATTR_IFNAME1 << "\",";
        NEW_LINE;
        Wa( "    ret, False, True )" );
        CCOUT << "oResp[ \""
            << JSON_ATTR_PARAMS<< "\" ] = resArgs";
        NEW_LINE;
        Wa( "self.sendResp( oResp )" );
        Wa( "return ret" );

        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, false );
        INDENT_DOWNL
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdSvrBase2::OutputEvent()
{
    gint32 ret = 0;
    do{
        Wa( "'''" );
        Wa( "Call this method whenever the sever has the" );
        Wa( "event to broadcast" );
        Wa( "'''" );
        stdstr strName = m_pNode->GetName();
        stdstr strIfName = m_pIf->GetName(); 
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount = GetArgCount( pInArgs );

        if( dwInCount == 0 )
        {
            CCOUT << "def " << strName << "( self )->int: ";
            INDENT_UPL;
            Wa( "reqId = random.randint( 0, 0x100000000 )" );
            Wa( "oEvent = BuildReqHeader( reqId," );
            CCOUT << "\"" << strName << "\",";
            NEW_LINE;
            CCOUT << "\"" << strIfName << "\",";
            NEW_LINE;
            CCOUT << "0, False, False )";
            NEW_LINE;
            CCOUT << "oEvent[ \""
                << JSON_ATTR_PARAMS<< "\" ] = []";
            NEW_LINE;
            Wa( "self.sendEvent( oEvent )" );
            Wa( "return 0" );
            break;
        }

        CCOUT << "def "<< strName << "( self,";
        INDENT_UPL;
        ret = EmitFormalArgListPy( m_pWriter, pInArgs );
        if( ERROR( ret ) )
            break;

        CCOUT << ")->int:";
        NEW_LINE;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        Wa( "osb = serijson.CSerialBase()" );

        EmitPyArgPack( m_pWriter, pInArgs, "argPack" );
        ret = EmitPySerialReq( m_pWriter,
            pInArgs, "resArgs", "argPack" );
        if( ERROR( ret ) )
            break;

        Wa( "reqId = random.randint( 0, 0x100000000 )" );
        Wa( "oEvent = BuildReqHeader( reqId," );
        CCOUT << "    \"" << strName << "\",";
        NEW_LINE;
        CCOUT << "    \"" << strIfName << "\",";
        NEW_LINE;
        CCOUT << "    0, False, False )";
        NEW_LINE;
        CCOUT << "oEvent[ \""
            << JSON_ATTR_PARAMS<< "\" ] = resArgs";
        NEW_LINE;

        Wa( "self.sendEvent( oEvent )" );
        CCOUT << "return 0";
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, false );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPyIfProxyBase2::CImplPyIfProxyBase2(
    CPyWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interface' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplPyIfProxyBase2::Output()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        CCOUT << "class I" << strIfName << "_CliImpl :";
        INDENT_UP;
        NEW_LINES( 2 );
        CCOUT << "_ifName_ = \"" <<  strIfName << "\"";
        NEW_LINE;

        ret = OutputDispMsg();
        if( ERROR( ret ) )
            break;

        ObjPtr pObj = m_pNode->GetMethodList();
        std::vector< ObjPtr > vecMethods;
        CMethodDecls* pmds = pObj;
        guint32 i = 0;
        for( ; i < pmds->GetCount(); i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            vecMethods.push_back( pObj );
        }

        if( vecMethods.empty() )
        {
            ret = -ENOENT;
            break;
        }

        for( auto& elem : vecMethods )
        {
            CImplPyMthdProxyBase2 oipm(
                m_pWriter, elem );
            oipm.Output();
        }
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyIfProxyBase2::OutputDispMsg()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        ObjPtr pObj = m_pNode->GetMethodList();
        std::vector< ObjPtr > vecMethods;
        CMethodDecls* pmds = pObj;
        guint32 i = 0;
        for( ; i < pmds->GetCount(); i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            CMethodDecl* pNode = pObj;
            if( pNode->IsEvent() || g_bAsyncProxy )
                vecMethods.push_back( pObj );
        }

        if( vecMethods.empty() )
        {
            ret = 0;
            break;
        }

        stdstr strClass = "I";
        strClass += strIfName + "_CliImpl";
        CCOUT << "_funcMap_ = {";
        INDENT_UPL;
        for( guint32 i = 0; i < vecMethods.size(); i++ )
        {
            CMethodDecl* pNode = vecMethods[ i ];
            stdstr strName = pNode->GetName();
            if( pNode->IsEvent() )
            {
                CCOUT << "\"" << strName
                    << "\": lambda self, req : "
                    << strClass << "."
                    << strName << "Wrapper( self, req )";
            }
            else if( g_bAsyncProxy )
            {
                CCOUT << "\"" << strName
                    << "\": lambda self, req : "
                    << strClass << "."
                    << strName << "CbWrapper( self, req )";
            }
            if( i < vecMethods.size() - 1 )
            {
                CCOUT << ",";
                NEW_LINE;
            }
        }
        
        Wa( " }" );
        INDENT_DOWNL;

        CCOUT << "def DispatchMsg( self, oMsg : dict ):";
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        CCOUT << "strMethod = oMsg[ \""
            << JSON_ATTR_METHOD "\" ]";
        NEW_LINE;
        CCOUT << "method = " << strClass <<"._funcMap_[ strMethod ]";
        NEW_LINE;
        Wa( "method( self, oMsg )" );
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter,
            false, true, "-errno.EINVAL" );
        Wa( "return" );
        INDENT_DOWNL

    }while( 0 );

    return ret;
}

CImplPyMthdProxyBase2::CImplPyMthdProxyBase2(
    CPyWriter* pWriter, ObjPtr& pNode )
    : super( pWriter )
{
    m_pWriter = pWriter;
    m_pNode = pNode;

    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'method' node" );
        throw std::runtime_error( strMsg );
    }
    CAstNodeBase* pMdl = m_pNode->GetParent();
    m_pIf = ObjPtr( pMdl->GetParent() );
}

gint32 CImplPyMthdProxyBase2::Output()
{
    if( m_pNode->IsEvent() )
        return OutputEvent();
    gint32 ret = OutputSync( g_bAsyncProxy );
    if( ERROR( ret ) )
        return ret;
    return OutputAsyncCbWrapper( g_bAsyncProxy );
}

gint32 CImplPyMthdProxyBase2::OutputEvent()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        stdstr strIfName = m_pIf->GetName(); 
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount = GetArgCount( pInArgs );

        Wa( "'''" );
        Wa( "Event handler's placeholder" );
        Wa( "within which to run event processing logic." );
        Wa( "Return code is ignored" );
        Wa( "'''" );
        CCOUT << "def " << strName << "( self,";
        if( dwInCount > 0 )
        {
            INDENT_UPL;
            CCOUT << "evtId : int,";
            ret = EmitFormalArgListPy(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;
            INDENT_DOWN;
        }
        else
        {
            CCOUT << " evtId : int";
        }
        CCOUT << " ) :";
        NEW_LINE;
        Wa( "    pass" );

        CCOUT << "def " << strName
            << "Wrapper( self, oEvent : object ) :";
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        CCOUT << "evtId = oEvent[ \""
            << JSON_ATTR_REQCTXID << "\" ]";
        NEW_LINE;
        if( dwInCount == 0 )
        {
            CCOUT << "self." << strName << "( evtId )";
        }
        else
        {
            Wa( "osb = serijson.CSerialBase()" );
            CCOUT << "oParams = oEvent[ \""
                << JSON_ATTR_PARAMS << "\" ]";
            NEW_LINE;
            CCOUT << "evtArgs = [None] * "
                << std::to_string( dwInCount );
            NEW_LINE;

            ret = EmitPyDeserialReq( m_pWriter,
                pInArgs, "evtArgs", "oParams" );
            if( ERROR( ret ) )
                break;
            CCOUT << "self."
                << strName << "( evtId, *evtArgs )";
        }
        NEW_LINE;
        CCOUT << "return";
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, false, true );
        INDENT_DOWNL;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdProxyBase2::OutputSync( bool bSync )
{
    gint32 ret = 0;
    do{
        Wa( "'''" );
        Wa( "Request Sender" );
        Wa( "client calls this method to send the request" );
        Wa( "'''" );
        stdstr strName = m_pNode->GetName();
        stdstr strIfName = m_pIf->GetName(); 
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount = GetArgCount( pInArgs );
        ObjPtr pOutArgs = m_pNode->GetOutArgs();
        guint32 dwOutCount = GetArgCount( pOutArgs );
        bool bNoReply = m_pNode->IsNoReply();

        if( dwInCount == 0 )
        {
            CCOUT << "def " << strName
                << "( self, reqId : int )->[ int, list ]: ";
        }
        else
        {
            CCOUT << "def "<< strName << "( self, reqId : int, ";
            INDENT_UPL;

            ret = EmitFormalArgListPy( m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;

            CCOUT << ")->[ int, list ]:";
            INDENT_DOWNL;
        }

        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        Wa( "oReq = dict()" );
        CCOUT << "oReq[ \""
            << JSON_ATTR_REQCTXID << "\" ] = reqId";
        NEW_LINE;
        CCOUT << "oReq[ \""
            << JSON_ATTR_MSGTYPE << "\" ] = \"req\"";
        NEW_LINE;
        CCOUT << "oReq[ \""
            << JSON_ATTR_IFNAME1 << "\" ] = \""
            << strIfName << "\"";
        NEW_LINE;
        CCOUT << "oReq[ \""
            << JSON_ATTR_METHOD << "\" ] = \""
            << strName << "\"";
        NEW_LINE;
        if( dwInCount == 0 )
        {
            CCOUT << "oReq[ \""
                << JSON_ATTR_PARAMS<< "\" ] = []";
            NEW_LINE;
        }
        else
        {
            Wa( "osb = serijson.CSerialBase()" );
            Wa(  "resArgs = dict()" );
            EmitPyArgPack( m_pWriter, pInArgs, "argPack" );
            ret = EmitPySerialReq( m_pWriter,
                pInArgs, "resArgs", "argPack" );
            if( ERROR( ret ) )
                break;
            CCOUT << "oReq[ \""
                << JSON_ATTR_PARAMS<< "\" ] = resArgs";
            NEW_LINE;
        }

        if( !bSync )
        {
            // async
            Wa( "ret = self.sendReq( oReq )" );
            Wa( "return [ ret, None ]" );
        }
        else if( bNoReply )
        {
            // noreply
            Wa( "self.sendReq( oReq )" );
            Wa( "return [ 0, None ]" );
        }
        else
        {
            // sync
            Wa( "ret = self.sendReq( oReq )" );
            Wa( "if ret < 0 :" );
            Wa( "    return [ ret, None ]" );

            // receive the response
            Wa( "ret = self.recvResp()" );
            Wa( "if ret[ 0 ] < 0:" );
            Wa( "    error = iRet[ 0 ]" );
            Wa( "    raise Exception( \"Error receiving resp %d\" \% error  )" );
            Wa( "oResp = ret[ 1 ]" );

            CCOUT << "reqIdr = oResp[ \""
               << JSON_ATTR_REQCTXID << "\" ]";
            NEW_LINE;
            Wa( "if reqId != reqIdr :" );
            Wa( "    error = -errno.EBADMSG" );
            Wa( "    raise Exception( \"Error reqId %d\" \% error  )" );
            CCOUT << "strType = oResp[ \""
               << JSON_ATTR_MSGTYPE << "\" ]";
            NEW_LINE;
            Wa( "if strType != \"resp\" :" );
            Wa( "    error = -errno.EBADMSG" );
            Wa( "    raise Exception( \"Error msg type %d\" \% error  )" );

            CCOUT << "strMethod = oResp[ \""
               << JSON_ATTR_METHOD << "\" ]";
            NEW_LINE;
            CCOUT << "if strMethod != \""<< strName <<"\" :";
            NEW_LINE;
            Wa( "    error = -errno.EBADMSG" );
            Wa( "    raise Exception( \"Error bad method %d\" \% error  )" );

            CCOUT << "strIfName = oResp[ \""
               << JSON_ATTR_IFNAME1 << "\" ]";
            NEW_LINE;
            CCOUT << "if strIfName != \""<< strIfName <<"\" :";
            NEW_LINE;
            Wa( "    error = -errno.EBADMSG" );
            Wa( "    raise Exception( \"Error bad ifName %d\" \% error  )" );

            CCOUT << "retCode = oResp[ \""
               << JSON_ATTR_RETCODE << "\" ]";
            NEW_LINE;

            if( dwOutCount == 0 )
            {
                Wa( "return [ retCode, [] );" );
            }
            else
            {
                Wa( "if retCode < 0:" );
                Wa( "    return [ retCode, None ];" );
                Wa( "else:" );
                INDENT_UPL;
                CCOUT << "args = [ None ] *"
                    << std::to_string( dwOutCount );
                CCOUT << "oParams = oResp[ \""
                    << JSON_ATTR_PARAMS << "\" ]";
                NEW_LINE;
                ret = EmitPyDeserialReq( m_pWriter,
                    pOutArgs, "args", "oParams" );
                Wa( "return [ 0, args ]" );
                INDENT_DOWNL; // retcode == 0
            }
        }
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, true );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdProxyBase2::OutputAsyncCbWrapper( bool bSync )
{
    gint32 ret = 0;
    do{
        bool bNoReply = m_pNode->IsNoReply();
        if( bSync || bNoReply )
            break;

        stdstr strName = m_pNode->GetName();
        stdstr strIfName = m_pIf->GetName(); 
        ObjPtr pOutArgs = m_pNode->GetOutArgs();
        guint32 dwOutCount = GetArgCount( pOutArgs );
        CCOUT << "def " << strName << "Callback(";
        NEW_LINE;
        CCOUT << "    self, ret : int, reqId : int";
        if( dwOutCount == 0 )
        {
            CCOUT << " ):";
        }
        else
        {
            CCOUT << ",";
            INDENT_UPL;
            ret = EmitFormalArgListPy(
                m_pWriter, pOutArgs );
            if( ERROR( ret ) )
                break;

            CCOUT << " ):";
            INDENT_DOWNL;
        }
        Wa( "    pass" );

        Wa( "'''" );
        Wa( "this callback wraper gets called when the" ); 
        Wa( "request is completed, timedout or canceled" );
        Wa( "'''" );
        CCOUT << "def " << strName
            << "CbWrapper( self, oResp : object ) : ";

        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );

        CCOUT << "reqIdr = oResp[ \""
           << JSON_ATTR_REQCTXID << "\" ]";
        NEW_LINE;
        CCOUT << "strType = oResp[ \""
           << JSON_ATTR_MSGTYPE << "\" ]";
        NEW_LINE;
        Wa( "if strType != \"resp\" :" );
        Wa( "    error = -errno.EBADMSG" );
        Wa( "    raise Exception( \"Error msg type %d\" \% error  )" );

        CCOUT << "strMethod = oResp[ \""
           << JSON_ATTR_METHOD << "\" ]";
        NEW_LINE;
        CCOUT << "if strMethod != \""<< strName <<"\" :";
        NEW_LINE;
        Wa( "    error = -errno.EBADMSG" );
        Wa( "    raise Exception( \"Error bad method %d\" \% error  )" );

        CCOUT << "strIfName = oResp[ \""
           << JSON_ATTR_IFNAME1 << "\" ]";
        NEW_LINE;
        CCOUT << "if strIfName != \""<< strIfName <<"\" :";
        NEW_LINE;
        Wa( "    error = -errno.EBADMSG" );
        Wa( "    raise Exception( \"Error bad ifName %d\" \% error  )" );

        CCOUT << "retCode = oResp[ \""
           << JSON_ATTR_RETCODE << "\" ]";
        NEW_LINE;

        if( dwOutCount == 0 )
        {
            CCOUT << "self." << strName << "Callback( retCode, reqIdr )";
            NEW_LINE;
            Wa( "return" );
        }
        else
        {
            CCOUT << "args = [None] * "<< dwOutCount;
            NEW_LINE;
            Wa( "if retCode == 0:" );
            INDENT_UPL;
            CCOUT << "oParams = oResp[ \""
                << JSON_ATTR_PARAMS << "\" ]";
            NEW_LINE;

            ret = EmitPyDeserialReq( m_pWriter,
                pOutArgs, "args", "oParams" );
            if( ERROR( ret ) )
                break;

            INDENT_DOWNL; // retcode == 0
            CCOUT << "self." << strName
                << "Callback( retCode, reqIdr, *args )";
            NEW_LINE;
            Wa( "return" );
        }

        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, true, false );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPySvcSvr2::CImplPySvcSvr2(
    CPyWriter* pWriter,
    ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'service' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplPySvcSvr2::OutputSvcSvrClass()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        CCOUT << "class C" << strName << "Server(";
        INDENT_UPL;
        
        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }
        
        guint32 dwCount = vecIfs.size();
        for( guint32 i = 0; i < dwCount; i++ )
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
            stdstr strIfName = pifd->GetName();

            CCOUT << "C" << strIfName << "svr";
            if( i < dwCount - 1 )
            {
                CCOUT << ",";
                NEW_LINE;
                continue;
            }
        }

        CCOUT << " ) :";
        NEW_LINE;
        Wa( "def __init__( self, strSvcPoint : str ) :" );
        INDENT_UPL;
        Wa( "error = 0" );
        Wa( "self.m_strPath = strSvcPoint" );
        Wa( "reqFile = strSvcPoint + \"/jreq_0\"" );
        Wa( "self.m_reqFp = open( reqFile, \"rb\" )" );
        Wa( "respFile = strSvcPoint + \"/jrsp_0\"" );
        Wa( "self.m_respFp = open( respFile, \"rb\" )" );
        Wa( "self.m_bExit = False" );
        Wa( "self.m_oLock = threading.lock()" );
        INDENT_DOWNL;
        
        Wa( "def IsExit() -> bool:" );
        Wa( "    self.m_oLock.acquire()" );
        Wa( "    bExit = self.bExit" );
        Wa( "    self.m_oLock.release()" );
        Wa( "    return bExit; " );

        Wa( "def SetExit() -> bool:" );
        Wa( "    self.m_oLock.acquire()" );
        Wa( "    self.m_bExit = True" );
        Wa( "    self.m_oLock.release()" );

        Wa( "def sendResp( self, oResp : object ) -> int:" );
        Wa( "    return iolib.sendResp( self.m_respFp, oResp )" );

        Wa( "def sendEvent( self, oEvent : object ) -> int:" );
        Wa( "    return iolib.sendEvent( self.m_respFp, oEvent )" );

        Wa( "def recvReq( self, oReq : object ) -> [ int, object ]:" );
        Wa( "    return iolib.recvReq( self.m_reqFp, oReq )" );

        Wa( "def getReqFp( self, oReq : object ) ->  object:" );
        Wa( "    return self.m_reqFp" );

        CCOUT << "def DispatchMsg( self, oReq : dict ):";
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );

        for( guint32 i = 0; i < vecIfs.size(); i++ )
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
            stdstr strIfName = pifd->GetName();
            CCOUT << "if \"" << strIfName << "\" == oReq[ \""
               << JSON_ATTR_IFNAME1 << "\" ] :";
            NEW_LINE;
            stdstr strClass = "I";
            strClass += strIfName + "_SvrImpl";
            CCOUT << "    " << strClass << ".DispatchMsg( self, oMsg )";
            NEW_LINE;
            Wa( "    return" );
            if( i < vecIfs.size() - 1 )
                NEW_LINE;
        }
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, false, true );
        INDENT_DOWNL;
        INDENT_DOWNL; // class

    }while( 0 );

    return ret;
}

gint32 CImplPySvcSvr2::Output()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        EmitImports( m_pWriter );
        CCOUT << "from ifimpl import *";
        NEW_LINE;

        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        for( auto elem : vecIfs )
        {
            CInterfRef* pIfRef = elem;
            ObjPtr pObj;
            ret = pIfRef->GetIfDecl( pObj );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }

            CImplPyIfSvr2 opis( m_pWriter, pObj );
            ret = opis.Output();
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            break;

        ret = OutputSvcSvrClass();

    }while( 0 );

    return ret;
}

CImplPyIfSvr2::CImplPyIfSvr2(
    CPyWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interface' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplPyIfSvr2::Output()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        CCOUT << "class C" << strIfName << "svr( I"
            << strIfName << "_SvrImpl ):";
        INDENT_UPL;
        bool bHasMethod = false;
        ObjPtr pObj = m_pNode->GetMethodList();
        std::vector< ObjPtr > vecMethods;
        CMethodDecls* pmds = pObj;
        guint32 i = 0;
        for( ; i < pmds->GetCount(); i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            vecMethods.push_back( pObj );
        }

        for( auto& elem : vecMethods )
        {
            CMethodDecl* pmd = elem;
            if( pmd->IsEvent() )
                continue;

            bHasMethod = true;
            CImplPyMthdSvr2 oms( m_pWriter, elem );
            oms.Output();
        }

        if( !bHasMethod )
            Wa( "pass" );

        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPyMthdSvr2::CImplPyMthdSvr2(
    CPyWriter* pWriter, ObjPtr& pNode )
    : super( pWriter )
{
    m_pWriter = pWriter;
    m_pNode = pNode;

    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'method' node" );
        throw std::runtime_error( strMsg );
    }
    CAstNodeBase* pMdl = m_pNode->GetParent();
    m_pIf = ObjPtr( pMdl->GetParent() );
}

gint32 CImplPyMthdSvr2::Output()
{
    gint32 ret = 0;

    do{
        Wa( "'''" );
        if( m_pNode->IsAsyncs() )
            Wa( "Asynchronous request handler" );
        else
            Wa( "Synchronous request handler" );
        Wa( "'''" );

        CMethodDecl* pmd = m_pNode;
        stdstr strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );

        ObjPtr pOutArgs = pmd->GetOutArgs();
        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        if( dwInCount > 0 )
        {
            CCOUT << "def " << strName << "( self, reqId : object,"; 
            INDENT_UPL;

            ret = EmitFormalArgListPy(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }
        else
        {
            CCOUT << "def " << strName << "( self, reqId : object"; 
            INDENT_UPL;
        }

        if( dwOutCount > 0 )
        {
            Wa( ") -> Tuple[ int, list ] :" );
            Wa( "#Implement this method here" );
        }
        else
            Wa( ") -> Tuple[ int, None ] :" );

        Wa( "return [ ErrorCode.ERROR_NOT_IMPL, None ]" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPyIfProxy2::CImplPyIfProxy2(
    CPyWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interface' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplPyIfProxy2::Output()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        CCOUT << "class C" << strIfName << "cli( I"
             << strIfName << "_CliImpl ):";
        INDENT_UPL;
        ObjPtr pObj = m_pNode->GetMethodList();
        std::vector< ObjPtr > vecMethods;
        CMethodDecls* pmds = pObj;
        bool bHasMethod = false;
        guint32 i = 0;
        for( ; i < pmds->GetCount(); i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            vecMethods.push_back( pObj );
        }

        if( vecMethods.empty() )
        {
            ret = -ENOENT;
            break;
        }

        for( auto& elem : vecMethods )
        {
            CMethodDecl* pmd = elem;
            if( !g_bAsyncProxy &&
                !pmd->IsEvent() )
                continue;

            bHasMethod = true;
            CImplPyMthdProxy2 oipm(
                m_pWriter, elem );
            oipm.Output();
        }

        if( !bHasMethod )
            Wa( "pass" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPyMthdProxy2::CImplPyMthdProxy2( 
    CPyWriter* pWriter, ObjPtr& pNode )
    : super( pWriter )
{
    m_pWriter = pWriter;
    m_pNode = pNode;

    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'method' node" );
        throw std::runtime_error( strMsg );
    }
    CAstNodeBase* pMdl = m_pNode->GetParent();
    m_pIf = ObjPtr( pMdl->GetParent() );
}

gint32 CImplPyMthdProxy2::OutputAsyncCallback()
{
    gint32 ret = 0;
    do{
        Wa( "'''" );
        Wa( "Asynchronous callback to receive the " );
        Wa( "request status, and reponse parameters" );
        Wa( "if any. And add code here to process the" );
        Wa( "request response" );
        Wa( "'''" );

        CMethodDecl* pmd = m_pNode;
        stdstr strName = pmd->GetName();

        ObjPtr pOutArgs = pmd->GetOutArgs();
        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        if( dwOutCount > 0 )
        {
            CCOUT << "def " << strName << "Callback( self,";
            INDENT_UPL;
            CCOUT << "reqId : int, ret : int, "; 
            ret = EmitFormalArgListPy(
                m_pWriter, pOutArgs );
            if( ERROR( ret ) )
                break;
            Wa( " ) :" );
        }
        else
        {
            CCOUT << "def " << strName << "Callback( self,";
            INDENT_UPL;
            CCOUT << "    ret : int, reqId : int ):"; 
            NEW_LINE;
        }

        Wa( "pass" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdProxy2::OutputEvent()
{
    gint32 ret = 0;

    do{
        Wa( "'''" );
        Wa( "Event handler" );
        Wa( "Add code here to process the event" );
        Wa( "'''" );

        CMethodDecl* pmd = m_pNode;
        stdstr strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );

        ObjPtr pOutArgs = pmd->GetOutArgs();
        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        if( dwInCount > 0 )
        {
            CCOUT << "def " << strName << "( self, evtId : int,"; 
            INDENT_UPL;

            ret = EmitFormalArgListPy(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }
        else
        {
            CCOUT << "def " << strName << "( self, evtId : int ): "; 
            INDENT_UPL;
        }

        Wa( ") :" );

        Wa( "return" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdProxy2::Output()
{
    if( g_bAsyncProxy )
        return OutputAsyncCallback();
    if( m_pNode->IsEvent() )
        return OutputEvent();
    return -ENOTSUP;
}

CImplPySvcProxy2::CImplPySvcProxy2(
    CPyWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'service' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplPySvcProxy2::OutputSvcProxyClass()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        CCOUT << "class C" << strName << "Proxy(";
        INDENT_UPL;
        
        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        guint32 dwCount = vecIfs.size();
        for( guint32 i = 0; i < dwCount; i++ )
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
            stdstr strIfName = pifd->GetName();

            CCOUT << "C" << strIfName << "cli";
            if( i < dwCount - 1 )
            {
                CCOUT << ",";
                NEW_LINE;
            }
        }

        CCOUT << ") :";
        NEW_LINE;
        Wa( "def __init__( self, strSvcPoint : str ) :" );
        INDENT_UPL;
        Wa( "self.m_strPath = strSvcPoint" );
        Wa( "reqFile = strSvcPoint + \"/jreq_0\"" );
        Wa( "self.m_reqFp = open( reqFile, \"rb\" )" );
        Wa( "respFile = strSvcPoint + \"/jrsp_0\"" );
        Wa( "self.m_respFp = open( respFile, \"rb\" )" );
        Wa( "evtFile = strSvcPoint + \"/jevt_0\"" );
        Wa( "self.m_evtFp = open( evtFile, \"rb\" )" );
        Wa( "self.m_bExit = False" );
        Wa( "self.m_oLock = threading.lock()" );
        INDENT_DOWNL;
        
        Wa( "def IsExit() -> bool:" );
        Wa( "    self.m_oLock.acquire()" );
        Wa( "    bExit = self.bExit" );
        Wa( "    self.m_oLock.release()" );
        Wa( "    return bExit; " );

        Wa( "def SetExit() -> bool:" );
        Wa( "    self.m_oLock.acquire()" );
        Wa( "    self.m_bExit = True" );
        Wa( "    self.m_oLock.release()" );

        Wa( "def sendReq( self, oResp : object ) -> int:" );
        Wa( "    return iolib.sendReq( self.m_reqFp, oResp )" );

        Wa( "def sendEvent( self, oEvent : object ) -> int:" );
        Wa( "    return iolib.sendEvent( self.m_respFp, oEvent )" );

        Wa( "def recvResp( self, oReq : object ) -> [ int, object ]:" );
        Wa( "    return iolib.recvResp( self.m_reqFp, oReq )" );

        Wa( "def getRespFp( self ) ->  object:" );
        Wa( "    return self.m_reqFp" );

        Wa( "def getEvtFp( self ) ->  object:" );
        Wa( "    return self.m_evtFp" );

        CCOUT << "def DispatchMsg( self, oResp : dict ):";
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );

        for( guint32 i = 0; i < vecIfs.size(); i++ )
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
            stdstr strIfName = pifd->GetName();
            CCOUT << "if \"" << strIfName << "\" == oResp[ \""
               << JSON_ATTR_IFNAME1 << "\" ] :";
            NEW_LINE;
            stdstr strClass = "I";
            strClass += strIfName + "_CliImpl";
            CCOUT << "    " << strClass << ".DispatchMsg( self, oMsg )";
            NEW_LINE;
            Wa( "    return" );
            if( i < vecIfs.size() - 1 )
                NEW_LINE;
        }
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, false, true );
        INDENT_DOWNL;
        INDENT_DOWNL;
    }while( 0 );

    return ret;
}

gint32 CImplPySvcProxy2::Output()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        EmitImports( m_pWriter );
        CCOUT << "from ifimpl import *";
        NEW_LINE;

        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        for( auto elem : vecIfs )
        {
            CInterfRef* pIfRef = elem;
            ObjPtr pObj;
            ret = pIfRef->GetIfDecl( pObj );
            if( ERROR( ret ) )
            {
                ret = 0;
                continue;
            }

            CImplPyIfProxy2 opip( m_pWriter, pObj );
            ret = opip.Output();
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            break;

        ret = OutputSvcProxyClass();

    }while( 0 );

    return ret;
}

CImplPyMainFunc2::CImplPyMainFunc2(
    CPyWriter* pWriter,
    ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'statements' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplPyMainFunc2::OutputThrdProcSvr(
    CServiceDecl* pSvc )
{
    gint32 ret = 0;
    do{
        bool bHasEvt = HasEvent( pSvc );
        bool bAsync = g_bAsyncProxy;
        if( !bHasEvt && !bAsync )
            break;

        Wa( "class ListeningThread(threading.Thread):" );
        INDENT_UPL;
        Wa( "def __init__(self , threadName, oSvr ):" );
        Wa( "    super(ListeningThread,self).__init__(name=threadName)" );
        Wa( "    self.m_oSvr = oSvr" );
        NEW_LINE;
        Wa( "def run(self):" );
        Wa( "    return self.handleReqs()" );

        NEW_LINE;
        CCOUT << "def handleReqs( self ):";
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        Wa( "fps = []" );
        Wa( "fMap = dict()" );
        Wa( "fp = self.m_oSvr.getReqFp()" );
        Wa( "fps.append( fp )" );
        Wa( "fmap[ fp.fileno ] = lambda oSvr, oMsg : oSvr.DispatchMsg( oMsg )" );
        CCOUT << "while True:";
        INDENT_UPL;
        Wa( "ioevents = select.select( fps, [], [] )" );
        Wa( "readable = ioevents[ 0 ]" );
        CCOUT << "for s in readable:";
        INDENT_UPL;
        Wa( "ret = iolib.recvMsg( s )" );
        Wa( "if ret[ 0 ] < 0:" );
        Wa( "    error = ret[ 0 ]" );
        Wa( "    raise Exception( \"Error read @\%d\" \% s.fileno )" );
        Wa( "for oMsg in ret[ 1 ] :" );
        Wa( "    fmap[ s.fileno ]( self.m_oSvr, oMsg )" );
        Wa( "bExit = self.m_oSvr.IsExit()" );
        Wa( "if bExit: " );
        CCOUT <<"    break";
        INDENT_DOWN;
        INDENT_DOWN;

        INDENT_DOWNL;// try
        EmitExceptHandler( m_pWriter, false, true );
        INDENT_DOWN;// run
        INDENT_DOWNL;// class

    }while( 0 );
    return ret;
}

gint32 CImplPyMainFunc2::OutputThrdProcCli(
    CServiceDecl* pSvc )
{
    gint32 ret = 0;
    do{
        bool bHasEvt = HasEvent( pSvc );
        bool bAsync = g_bAsyncProxy;
        if( !bHasEvt && !bAsync )
            break;

        Wa( "class MessageThread(threading.Thread):" );
        Wa( "    def __init__(self , threadName, oProxy ):" );
        Wa( "        super(MessageThread,self).__init__(name=threadName)" );
        Wa( "        self.m_oProxy = oProxy" );
        INDENT_UPL;
        CCOUT << "def run(self):";
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        Wa( "fps = []" );
        Wa( "fMap = dict()" );
        if( bHasEvt )
        {
            Wa( "fp = self.m_oProxy.getEvtFp()" );
            Wa( "fps.append( fp )" );
            Wa( "fmap[ fp.fileno ] = lambda oProxy, oMsg : oProxy.DispatchMsg( oMsg )" );
        }
        if( g_bAsyncProxy )
        {
            Wa( "fp = self.m_oProxy.getRespFp()" );
            Wa( "fps.append( fp )" );
            Wa( "fmap[ fp.fileno ] = lambda oProxy, oMsg : oProxy.DispatchMsg( oMsg )" );
        }
        CCOUT << "while True:";
        INDENT_UPL;
        Wa( "ioevents = select.select( fps, [], [] )" );
        Wa( "readable = ioevents[ 0 ]" );
        CCOUT << "for s in readable:";
        INDENT_UPL;
        Wa( "ret = iolib.recvMsg( s )" );
        Wa( "if ret[ 0 ] < 0:" );
        Wa( "    error = ret[ 0 ]" );
        Wa( "    raise Exception( \"Error read @\%d\" \% s.fileno )" );
        Wa( "for oMsg in ret[ 1 ] :" );
        Wa( "    fmap[ s.fileno ]( self.m_oProxy, oMsg )" );
        Wa( "bExit = self.m_oSvr.IsExit()" );
        Wa( "if bExit: " );
        CCOUT << "    break";
        INDENT_DOWN;
        INDENT_DOWN;

        INDENT_DOWNL;// try
        EmitExceptHandler( m_pWriter, false, true );
        INDENT_DOWN;// run
        INDENT_DOWNL;// class

    }while( 0 );
    return ret;
}

gint32 CImplPyMainFunc2::OutputCli(
    CServiceDecl* pSvc )
{
    gint32 ret = 0;
    do{
        NEW_LINE;

        bool bHasEvent = HasEvent( pSvc );
        if( bHasEvent || g_bAsyncProxy )
            OutputThrdProcCli( pSvc );

        CCOUT << "def maincli() :";
        INDENT_UPL;
        Wa( "ret = 0" );
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        Wa( "strSvcPt = sys.argv[ 1 ]" );
        Wa( "print( \"start to work here...\" )" );

        stdstr strName = pSvc->GetName();
        CCOUT << "oProxy = C" << strName << "Proxy(";
        NEW_LINE;
        Wa( "    strPath_ )" );
        
        if( bHasEvent || g_bAsyncProxy )
        {
            CCOUT << "oMsgThrd = MessageThread( \""
                << pSvc->GetName() <<"cliMsg\", oProxy )";
            NEW_LINE;
            Wa( "oMsgThrd.start()" );
            NEW_LINE;
        }

        Wa( "'''" );
        Wa( "adding your code here" );

        std::vector< ObjPtr > vecIfs;
        pSvc->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        auto& elem = vecIfs.front();
        do{
            CInterfRef* pIfRef = elem;
            ObjPtr pObj;
            ret = pIfRef->GetIfDecl( pObj );
            if( ERROR( ret ) )
            {
                ret = 0;
                break;
            }
            CInterfaceDecl* pIf = pObj;
            pObj = pIf->GetMethodList();
            std::vector< ObjPtr > vecMethods;

            CMethodDecls* pmds = pObj;
            guint32 i = 0;
            for( ; i < pmds->GetCount(); i++ )
            {
                ObjPtr pObj = pmds->GetChild( i );
                vecMethods.push_back( pObj );
            }
            if( vecMethods.empty() )
                break;

            CMethodDecl* pmd = nullptr;
            for( auto& elem : vecMethods )
            {
                pmd = vecMethods.front();
                if( pmd->IsEvent() )
                {
                    pmd = nullptr;
                    continue;
                }
                break;
            }

            if( pmd != nullptr )
            {
                stdstr strMName = pmd->GetName();
                std::vector< std::pair< stdstr, stdstr >> vecArgs;
                ObjPtr pInArgs = pmd->GetInArgs();
                guint32 dwInCount = GetArgCount( pInArgs );
                if( dwInCount == 0 )
                {
                    Wa( "Calling a proxy method like" );
                    CCOUT << "oProxy." << strMName << "()";
                }
                else
                {
                    Wa( "Calling a proxy method like" );
                    Wa( "reqId = 0x12345" );
                    CCOUT << "'oProxy." << strMName << "( reqId,";
                    if( dwInCount > 2 )
                    {
                        NEW_LINE;
                        CCOUT << "    ";
                    }
                    else
                    {
                        CCOUT << " ";
                    }
                    CArgList* pArgList = pInArgs;
                    for( int i = 0; i < dwInCount; i++ )
                    {
                        CFormalArg* pfa = pArgList->GetChild( i );
                        stdstr strName = pfa->GetName();
                        CCOUT << strName;
                        if( i < dwInCount - 1 )
                            CCOUT << ", ";
                        else
                            CCOUT << " )'";
                    }
                }
            }
            else if( bHasEvent )
            {
                Wa( "Just waiting and events will " );
                Wa( "be handled on the event thread" );
                NEW_LINE;
                Wa( "while True:" );
                CCOUT << "    time.sleep(1)";
            }
            NEW_LINE;

        }while( 0 );

        Wa( "'''" );
        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, false );
        Wa( "return ret" );
        INDENT_DOWNL;
        Wa( "ret = maincli()" );
        Wa( "quit( ret )" );

    }while( 0 );

    return ret;
}

gint32 CImplPyMainFunc2::OutputSvr(
    CServiceDecl* pSvc )
{
    gint32 ret = 0;
    do{
        NEW_LINE;

        bool bHasEvent = HasEvent( pSvc );
        OutputThrdProcSvr( pSvc );

        CCOUT << "def mainsvr() :";
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "error = 0" );
        Wa( "strSvcPt = sys.argv[ 1 ]" );
        Wa( "print( \"start to work here...\" )" );

        stdstr strName = pSvc->GetName();
        CCOUT << "oSvr = C" << strName << "Server(";
        NEW_LINE;
        Wa( "    strPath_ )" );

        CCOUT << "oSvrThrd = ListeningThread( \""
            << pSvc->GetName() <<"Svr\", oSvr )";
        NEW_LINE;
        if( bHasEvent)
        {
            // start a new thread to handle incoming reqs
            Wa( "oSvrThrd.start()" );
            NEW_LINE;

            // using the main thread to sendevent
            Wa( "'''" );
            Wa( "adding your code here" );
            Wa( "'''" );
        }
        else
        {
            Wa( "oSvrThrd.handleReqs()" );
        }

        INDENT_DOWNL;
        EmitExceptHandler( m_pWriter, false );
        Wa( "return 0" );
        INDENT_DOWNL;
        Wa( "ret = mainsvr()" );
        Wa( "quit( ret )" );

    }while( 0 );

    return ret;
}

gint32 CImplPyMainFunc2::Output()
{
    gint32 ret = 0;
    do{
        std::vector< ObjPtr > vecSvcs;
        ret = m_pNode->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
            break;
        if( vecSvcs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        m_pWriter->SelectMainCli();
        EmitImports( m_pWriter );

        Wa( "import os" );
        Wa( "import time" );
        Wa( "import threading" );
        Wa( "import select" );

        for( auto& elem : vecSvcs )
        {
            CServiceDecl* pNode = elem;
            stdstr strName = pNode->GetName();
            CCOUT << "from " << strName << "cli"
                << " import " << "C" << strName
                << "Proxy";
            NEW_LINE;
        }
        ret = OutputCli( vecSvcs.front() );
        if( ERROR( ret ) )
            break;

        m_pWriter->SelectMainSvr();
        EmitImports( m_pWriter );
        for( auto& elem : vecSvcs )
        {
            CServiceDecl* pNode = elem;
            stdstr strName = pNode->GetName();
            CCOUT << "from " << strName << "svr"
                << " import " << "C" << strName
                << "Server";
            NEW_LINE;
        }
        ret = OutputSvr( vecSvcs.front() );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

bool CImplPyMainFunc2::HasEvent(
    CServiceDecl* pSvc )
{
    gint32 ret = 0;
    bool bEvent = false;
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
            ;
            for( guint32 j = 0; j < pmds->GetCount(); j++ )
            {
                ObjPtr pObj = pmds->GetChild( j );
                CMethodDecl* pmd = pObj;
                if( pmd->IsEvent() )
                {
                    bEvent = true;
                    break;
                }
            }
            if( bEvent )
                break;
        }

    }while( 0 );
    return bEvent;
}

extern stdstr g_strLocale;
gint32 CExportPyReadme2::Output()
{
    if( g_strLocale == "en" )
        return Output_en();
    if( g_strLocale == "cn" )
        return Output_cn();

    return -ENOTSUP;
}

gint32 CExportPyReadme2::Output_en()
{
   gint32 ret = 0; 
   do{
        std::vector< ObjPtr > vecSvcs;
        ret = m_pNode->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
            break;

        std::vector< stdstr > vecSvcNames;
        for( auto& elem : vecSvcs )
        {
            CServiceDecl* psd = elem;
            if( psd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            vecSvcNames.push_back(
                psd->GetName() );
        }

        Wa( "### Introduction to the files:" );

        CCOUT << "* **maincli.py**, **mainsvr.py**: "
            << "Containing defintion of `maincli()` function for client, as the main "
            << "entry for client program "
            << "and definition of `mainsvr()` function server program respectively. ";
        NEW_LINE;
        CCOUT << "And you can make changes to the files to customize the program. "
            << "The `ridlc` will not touch them if they exist in the project directory, "
            << "when it runs again, and put the newly "
            << "generated code to `maincli.py.new` and `mainsvr.py.new`.";
        NEW_LINES( 2 );

        for( auto& elem : vecSvcNames )
        {
            CCOUT << "* **" << elem << "svr.py**, **" << elem << "cli.py**: "
                << "Containing the declarations and definitions of all the server/client side "
                << "methods that need to be implemented by you, mainly the request/event handlers, "
                << "for service `" << elem << "`.";
            NEW_LINE;
            CCOUT << "And you need to make changes to the files to implement the "
                << "functionality for server/client. "
                << "The `ridlc` will not touch them if they exist in the project directory, "
                << "when it runs again, and put the newly "
                << "generated code to `"<<elem  <<".py.new`.";
            NEW_LINES( 2 );
        }

        CCOUT << "* *ifimpl.py* : "
            << "Containing the declarations and definitions of all the server/client side "
            << "utilities and helpers for the interfaces of service.";
        NEW_LINE;
        CCOUT << "And please don't edit them, since they will be "
            << "overwritten by `ridlc` without backup.";
        NEW_LINES( 2 );

        CCOUT<< "* *" << g_strAppName << "structs.py*: "
            << "Containing all the declarations of the struct classes "
            << "declared in the ridl, with serialization methods implemented.";
        NEW_LINE;
        CCOUT << "And please don't edit it, since they will be "
            << "overwritten by `ridlc` without auto-backup.";
        NEW_LINES( 2 );

        CCOUT << "**Note 1**: the files in bold text need your further implementation. "
            << "And files in italic text do not. And of course, "
            << "you can still customized the italic files, but be aware they "
            << "will be rewritten after running RIDLC again.";
        NEW_LINES(2);
        CCOUT << "**Note 2**: Please refer to [this link](https://github.com/zhiming99/rpc-frmwrk#building-rpc-frmwrk)"
            << "for building and installation of RPC-Frmwrk";
        NEW_LINE;

   }while( 0 );

   return ret;
}

gint32 CExportPyReadme2::Output_cn()
{
   gint32 ret = 0; 
   do{
        std::vector< ObjPtr > vecSvcs;
        ret = m_pNode->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
            break;

        std::vector< stdstr > vecSvcNames;
        for( auto& elem : vecSvcs )
        {
            CServiceDecl* psd = elem;
            if( psd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            vecSvcNames.push_back(
                psd->GetName() );
        }

        Wa( "### 生成文件介绍:" );

        CCOUT << "* **maincli.py**, **mainsvr.py**: "
            << "分别包含对客户端和服务器端的程序入口`main()`函数的定义";
        NEW_LINE;
        CCOUT << "你可以对这两个文件作出修改，不必担心ridlc会冲掉你修改的内容。"
            << "`ridlc`再次编译时，如果发现目标目录存在该文件会把新生成的文件"
            << "名加上.new后缀。";
        NEW_LINES( 2 );

        for( auto& elem : vecSvcNames )
        {
            CCOUT << "* **" << elem << "svr.py**, **" << elem << "cli.py**: "
                << "分别包含有关service `"<<elem
                <<"`的服务器端和客户端的所有接口函数的声明和空的实现。"
                << "这些接口函数需要你的进一步实现, 其中主要包括"
                << "服务器端的请求处理和客户端的事件处理。";
            NEW_LINE;
            CCOUT << "你可以对这两个文件作出修改，不必担心`ridlc`会冲掉你修改的内容。"
                << "`ridlc`再次编译时，如果发现目标目录存在该文件，会为新生成的文件"
                << "的文件名加上.new后缀。";
            NEW_LINES( 2 );
        }

        CCOUT << "* *ifimpl.py*: 包含有关interface" 
            <<"的服务器端和客户端的所有辅助函数和底部支持功能的实现。";
        NEW_LINE;
        CCOUT << "这些函数和方法务必不要做进一步的修改。"
            << "`ridlc`在下一次运行时会重写里面的内容。";
        NEW_LINES( 2 );


        CCOUT<< "* *" << g_strAppName << "structs.py*: "
            << "包含一个ridl文件中声明的所有用到的struct，"
            << "以及序列/反序列化方法的实现.";
        NEW_LINE;
        CCOUT << "这个文件务必不要做进一步的修改。"
                << "`ridlc`在下一次运行时会重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT << "**注1**: 上文中的粗体字的文件是需要你进一步修改的文件. 斜体字的文件则不需要。"
            << "如果仍然有修改的必要，请注意这些文件有被`ridlc`或者`synccfg.py`改写的风险。";
        NEW_LINES(2);
        CCOUT << "**注2**: 有关配置系统搭建和设置请参考[此文。](https://github.com/zhiming99/rpc-frmwrk#building-rpc-frmwrk)。";
        NEW_LINE;

   }while( 0 );

   return ret;
}
