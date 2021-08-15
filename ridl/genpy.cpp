/*
 * =====================================================================================
 *
 *       Filename:  genpy.cpp
 *
 *    Description:  implementations of proxy/server generator for C++
 *
 *        Version:  1.0
 *        Created:  08/14/2021 02:57:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <sys/stat.h>
#include "rpc.h"
using namespace rpcf;
#include "genpy.h"

extern std::string g_strAppName;
extern gint32 SetStructRefs( ObjPtr& pRoot );
extern guint32 GenClsid( const std::string& strName );

CPyFileSet::CPyFileSet(
    const std::string& strOutPath,
    const std::string& strAppName )
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
        strOutPath, strAppName + "structs.py" );

    GEN_FILEPATH( m_strObjDesc,
        strOutPath, strAppName + "desc.json" );

    GEN_FILEPATH( m_strDriver,
        strOutPath, "driver.json" );

    GEN_FILEPATH( m_strMakefile,
        strOutPath, "Makefile" );

    GEN_FILEPATH( m_strMainCli,
        strOutPath, "maincli.py" );

    GEN_FILEPATH( m_strMainSvr, 
        strOutPath, "mainsvr.py" );

    m_strPath = strOutPath;

    gint32 ret = OpenFiles();
    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "internal error open files" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CPyFileSet::OpenFiles()
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
        m_strObjDesc,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strDriver,
        std::ofstream::out |
        std::ofstream::trunc) );

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

    return STATUS_SUCCESS;
}

gint32 CPyFileSet::AddSvcImpl(
    const std::string& strSvcName )
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
        m_mapSvcImp[ strSvcName + "svr" + strExt ] = idx;

        pstm = STMPTR( new std::ofstream(
            strCliPy,
            std::ofstream::out |
            std::ofstream::trunc) );

        idx += 1;
        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strSvcName + "cli" + strExt ] = idx;

    }while( 0 );

    return ret;
}

CPyFileSet::~CPyFileSet()
{
    for( auto& elem : m_vecFiles )
    {
        if( elem != nullptr )
            elem->close();
    }
    m_vecFiles.clear();
}

CDeclarePyStruct::CDeclarePyStruct(
    CPyWriter* pWriter,
    ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'struct' node" );
        throw std::runtime_error( strMsg );
    }
}

void CDeclarePyStruct::OutputStructBase()
{
    CCOUT << "class CStructBase :";
    INDENT_UPL;
    Wa( "def __init__( self, pIf : PyRpcServices = None ) :" );
    Wa( "    self.pIf = pIf" );
    Wa( "def Serialize( self, buf : bytearray ) -> int:" );
    Wa( "    pass" );
    Wa( "def Deserialize( self, buf : bytearray, offset : int ) -> tuple( int, int ):" );
    Wa( "    pass" );
    INDENT_DOWNL
}

gint32 CDeclarePyStruct::Output()
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

        Wa( "#Message identity" );

        Wa( "@classmethod" );
        Wa( "def GetStructId( cls ) -> int:" );
        CCOUT << "    return "<< dwMsgId;

        NEW_LINES( 2 );

        Wa( "@classmethod" );
        Wa( "def GetStructName( cls ) -> str:" );
        CCOUT << "    return \""<< strMsgId << "\"";

        NEW_LINES( 2 );

        BufPtr pMsgId( true );
        *pMsgId = dwMsgId;
        m_pNode->SetProperty(
            PROP_MESSAGE_ID, pMsgId );

        CCOUT << "def __init__( self, pIf : PyRpcServices = None ) :";
        INDENT_UPL;
        CCOUT << "super().__init__( pIf )";
        NEW_LINE;

        guint32 i = 0;
        for( ; i < dwCount; i++ )
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
            case 'Q':
            case 'q':
            case 'D':
            case 'd':
            case 'W':
            case 'w':
            case 'b':
            case 'B':
                {
                    CCOUT << "self." << strName << " = 0";
                    break;
                }
            case 'f':
            case 'F':
                {
                    CCOUT << "self." << strName << " = .0";
                    break;
                }
            case 's':
                {
                    CCOUT << "self." << strName << " = \"\"";
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

        CCOUT << "def Serialize( self, buf : bytearray ) -> int:";
        INDENT_UPL;
        NEW_LINE;
        Wa( "osb = CSerialBase( self.pIf )" );
        NEW_LINE;
        for( i = 0; i < dwCount; i++ )
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
            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    CCOUT << "ret = osb.SerialArray( buf, self." << strName
                        << ", \"" << strSig << "\" )";
                    NEW_LINE;
                    CCOUT << "if ret < 0 :";
                    NEW_LINE;
                    CCOUT << "    return ret";
                    break;
                }
            case '[' :
                {
                    CCOUT << "ret = osb.SerialMap( buf, self." << strName
                        << ", \"" << strSig << "\" )";
                    NEW_LINE;
                    CCOUT << "if ret < 0 :";
                    NEW_LINE;
                    CCOUT << "    return ret";
                    break;
                }

            case 'O' :
                {
                    CCOUT << "ret = osb.SerialStruct( buf, self." << strName
                    << " )";
                    NEW_LINE;
                    CCOUT << "if ret < 0 :";
                    NEW_LINE;
                    CCOUT << "    return ret";
                    break;
                }
            case 'Q':
            case 'q':
                {
                    CCOUT << "osb.SerialInt64( buf, self." << strName << " )";
                    break;
                }
            case 'D':
            case 'd':
                {
                    CCOUT << "osb.SerialInt32( buf, self." << strName << " )";
                    break;
                }
            case 'W':
            case 'w':
                {
                    CCOUT << "osb.SerialInt16( buf, self." << strName << " )";
                    break;
                }
            case 'b':
            case 'B':
                {
                    CCOUT << "osb.SerialInt8( buf, self." << strName << " )";
                    break;
                }
            case 'f':
                {
                    CCOUT << "osb.SerialFloat( buf, self." << strName << " )";
                    break;
                }
            case 'F':
                {
                    CCOUT << "osb.SerialDouble( buf, self." << strName << " )";
                    break;
                }
            case 's':
                {
                    CCOUT << "osb.SerialString( buf, self." << strName << " )";
                    break;
                }
            case 'a':
                {
                    CCOUT << "osb.SerialBuf( buf, self." << strName << " )";
                    break;
                }
            case 'o':
                {
                    CCOUT << "osb.SerialObjPtr( buf, self." << strName << " )";
                    break;
                }
            case 'h':
                {
                    CCOUT << "osb.SerialHStream( buf, self." << strName << " )";
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
            if( i < dwCount - 1 )
                NEW_LINES( 2 );
        }
        NEW_LINE;
        Wa( "return 0" );
        INDENT_DOWNL;
        NEW_LINE;

        CCOUT << "def Deserialize( self, buf : bytearray, offset : int ) -> tuple( int, int ):";
        INDENT_UPL;
        NEW_LINE;
        Wa( "osb = CSerialBase( self.pIf )" );
        NEW_LINE;
        for( i = 0; i < dwCount; i++ )
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
            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    CCOUT << "ret = osb.DeserialArray( buf, offset"
                        << ", \"" << strSig << "\" )";
                    break;
                }
            case '[' :
                {
                    CCOUT << "ret = osb.DeserialMap( buf, offset"
                        << ", \"" << strSig << "\" )";
                    break;
                }

            case 'O' :
                {
                    CCOUT << "ret = osb.DeserialStruct( buf, offset"
                        << " )";
                    break;
                }
            case 'Q':
            case 'q':
                {
                    CCOUT << "ret = osb.DeserialInt64( buf, offset" << " )";
                    break;
                }
            case 'D':
            case 'd':
                {
                    CCOUT << "ret = osb.DeserialInt32( buf, offset" << " )";
                    break;
                }
            case 'W':
            case 'w':
                {
                    CCOUT << "ret = osb.DeserialInt16( buf, offset" << " )";
                    break;
                }
            case 'b':
            case 'B':
                {
                    CCOUT << "ret = osb.DeserialInt8( buf, offset" << " )";
                    break;
                }
            case 'f':
                {
                    CCOUT << "ret = osb.DeserialFloat( buf, offset" << " )";
                    break;
                }
            case 'F':
                {
                    CCOUT << "ret = osb.DeserialDouble( buf, offset" << " )";
                    break;
                }
            case 's':
                {
                    CCOUT << "ret = osb.DeserialString( buf, offset" << " )";
                    break;
                }
            case 'a':
                {
                    CCOUT << "ret = osb.DeserialBuf( buf, offset" << " )";
                    break;
                }
            case 'o':
                {
                    CCOUT << "ret = osb.DeserialObjPtr( buf, offset" << " )";
                    break;
                }
            case 'h':
                {
                    CCOUT << "ret = osb.DeserialHStream( buf, offset" << " )";
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
            NEW_LINE;
            Wa( "if ret[ 0 ] is None :" );
            Wa( "    return ( -errno.ENOENT, 0 )" );
            CCOUT << "self." << strName << " = ret[ 0 ]";
            NEW_LINE;
            CCOUT << "offset = ret[ 1 ]";
            NEW_LINES( 2 );
        }

        Wa( "return ( 0, offset )" );
        INDENT_DOWNL;
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 GenStructsFile(
    CPyWriter* pWriter, ObjPtr& pRoot )
{
    if( pWriter == nullptr ||
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

        stdstr strSeribase = "./seribase.py";
        ret = access( strSeribase.c_str(), F_OK );
        if( ret == -1 )
        {
            stdstr strPath;
            ret = FindInstCfg( strSeribase, strPath );
            if( ERROR( ret ) )
                break;
            strSeribase = strPath;
        }
        stdstr strCmd = "cp ";
        strCmd += strSeribase + " " + pWriter->GetOutPath();
        system( strCmd.c_str() );

        pWriter->SelectStructsFile();
        pWriter->WriteLine0( "#Generated by RIDLC, "
            "make sure to backup if there are important changes" );

        pWriter->WriteLine0( "from rpcf.proxy import PyRpcServices" );
        pWriter->WriteLine0( "from .seribase import CSerialBase" );
        pWriter->WriteLine0( "import errno" );

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

        bool bFirst = true;
        for( auto& elem : vecActStructs )
        {
            CDeclarePyStruct odps( pWriter, elem );
            if( bFirst )
            {
                odps.OutputStructBase();
                bFirst = false;
            }
            ret = odps.Output();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

CExportInitPy::CExportInitPy(
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

gint32 CExportInitPy::Output()
{
    gint32 ret = 0;
    do{
        Wa( "#Generated by RIDLC, make sure to backup if there are important changes" );
        CCOUT << "from ." << g_strAppName << "structs" << " import *";
        NEW_LINE;
        CStatements* pStmts = m_pNode;
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }
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
        CCOUT << "g_mapStructs = {";
        INDENT_UPL; 
        for( int i = 0; i < vecActStructs.size(); i++ )
        {
            CStructDecl* pStruct = vecActStructs[ i ];
            if( pStruct == nullptr )
                continue;
            stdstr strName = pStruct->GetName(); 
            stdstr strMsgId =
                g_strAppName + "::" + strName;
            guint32 dwMsgId = GenClsid( strMsgId );
            CCOUT << dwMsgId << " : " << strName;
            if( i < vecActStructs.size() - 1 )
            {
                CCOUT << ",";
                NEW_LINE;
            }
        }

        INDENT_DOWNL;
        Wa( "}" );

    }while( 0 );

    return ret;
}

gint32 GenInitFile(
    CPyWriter* pWriter, ObjPtr& pRoot )
{
    if( pWriter == nullptr ||
        pRoot.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;

    pWriter->SelectInitFile();
    CExportInitPy oip( pWriter, pRoot );
    ret = oip.Output();

    return ret;
}

gint32 GenPyProj(
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

        CPyWriter oWriter(
            strOutPath, strAppName, pRoot );

        ret = GenStructsFile( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

        ret = GenInitFile( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

        /*oWriter.SelectDrvFile();
        CExportDrivers oedrv( &oWriter, pRoot );
        ret = oedrv.Output();

        oWriter.SelectDescFile();
        CExportObjDesc oedesc( &oWriter, pRoot );
        ret = oedesc.Output();
        */

    }while( 0 );

    return ret;
}
