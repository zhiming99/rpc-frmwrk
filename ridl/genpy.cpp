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

std::map< char, stdstr > g_mapSig2Type =
{
    { '(' , "list" },
    { '[' , "map" },
    { 'O' ,"object" },
    { 'Q', "int" },
    { 'q', "int" },
    { 'D', "int" },
    { 'd', "int" },
    { 'W', "int" },
    { 'w', "int" },
    { 'b', "int" },
    { 'B', "int" },
    { 'f', "float" },
    { 'F', "float" },
    { 's', "str" },
    { 'a', "bytearray" },
    { 'o', "cpp.ObjPtr" },
    { 'h', "int" }
};

static gint32 EmitDeserialBySig(
    CWriterBase* pWriter,
    const stdstr& strSig )
{
    gint32 ret = 0;
    std::ofstream& ofp = *pWriter->m_curFp;
    switch( strSig[ 0 ] )
    {
    case '(' :
        {
            ofp << "ret = osb.DeserialArray( buf, offset, \""
                << strSig << "\" )";
            break;
        }
    case '[' :
        {
            ofp << "ret = osb.DeserialMap( buf, offset, \""
                << strSig << "\" )";
            break;
        }

    case 'O' :
        {
            ofp << "ret = osb.DeserialStruct( buf, offset"
                << " )";
            break;
        }
    case 'Q':
    case 'q':
        {
            ofp << "ret = osb.DeserialInt64( buf, offset )";
            break;
        }
    case 'D':
    case 'd':
        {
            ofp << "ret = osb.DeserialInt32( buf, offset )";
            break;
        }
    case 'W':
    case 'w':
        {
            ofp << "ret = osb.DeserialInt16( buf, offset )";
            break;
        }
    case 'b':
    case 'B':
        {
            ofp << "ret = osb.DeserialInt8( buf, offset )";
            break;
        }
    case 'f':
        {
            ofp << "ret = osb.DeserialFloat( buf, offset )";
            break;
        }
    case 'F':
        {
            ofp << "ret = osb.DeserialDouble( buf, offset )";
            break;
        }
    case 's':
        {
            ofp << "ret = osb.DeserialString( buf, offset )";
            break;
        }
    case 'a':
        {
            ofp << "ret = osb.DeserialBuf( buf, offset )";
            break;
        }
    case 'o':
        {
            ofp << "ret = osb.DeserialObjPtr( buf, offset )";
            break;
        }
    case 'h':
        {
            ofp << "ret = osb.DeserialHStream( buf, offset )";
            break;
        }
    default:
        {
            ret = -EINVAL;
            break;
        }
    }
    if( SUCCEEDED( ret ) )
        pWriter->NewLine();

    return ret;
}

static gint32 EmitSerialBySig(
    CWriterBase* pWriter,
    const stdstr& strField,
    const stdstr& strSig,
    bool bNoRet = false )
{
    gint32 ret = 0;
    std::ofstream& ofp = *pWriter->m_curFp;
    switch( strSig[ 0 ] )
    {
    case '(' :
        {
            ofp << "ret = osb.SerialArray( buf, "
                << strField << ", \"" << strSig << "\" )";
            pWriter->NewLine();
            ofp << "if ret < 0 :";
            pWriter->NewLine();
            if( bNoRet )
                ofp << "    break";
            else
                ofp << "    return ret";
            break;
        }
    case '[' :
        {
            ofp << "ret = osb.SerialMap( buf, "
                << strField << ", \"" << strSig << "\" )";
            pWriter->NewLine();
            ofp << "if ret < 0 :";
            pWriter->NewLine();
            if( bNoRet )
                ofp << "    break";
            else
                ofp << "    return ret";
            break;
        }

    case 'O' :
        {
            ofp << "ret = osb.SerialStruct( buf, " << strField << " )";
            pWriter->NewLine();
            ofp << "if ret < 0 :";
            pWriter->NewLine();
            if( bNoRet )
                ofp << "    break";
            else
                ofp << "    return ret";
            break;
        }
    case 'Q':
    case 'q':
        {
            ofp << "osb.SerialInt64( buf, " << strField << " )";
            break;
        }
    case 'D':
    case 'd':
        {
            ofp << "osb.SerialInt32( buf, " << strField << " )";
            break;
        }
    case 'W':
    case 'w':
        {
            ofp << "osb.SerialInt16( buf, " << strField << " )";
            break;
        }
    case 'b':
    case 'B':
        {
            ofp << "osb.SerialInt8( buf, " << strField << " )";
            break;
        }
    case 'f':
        {
            ofp << "osb.SerialFloat( buf, " << strField << " )";
            break;
        }
    case 'F':
        {
            ofp << "osb.SerialDouble( buf, " << strField << " )";
            break;
        }
    case 's':
        {
            ofp << "osb.SerialString( buf, " << strField << " )";
            break;
        }
    case 'a':
        {
            ofp << "osb.SerialBuf( buf, " << strField << " )";
            break;
        }
    case 'o':
        {
            ofp << "osb.SerialObjPtr( buf, " << strField << " )";
            break;
        }
    case 'h':
        {
            ofp << "osb.SerialHStream( buf, " << strField << " )";
            break;
        }
    default:
        {
            ret = -EINVAL;
            break;
        }
    }

    if( SUCCEEDED( ret ) )
        pWriter->NewLine();

    return ret;
}

static gint32 GetArgsAndSigs( CArgList* pArgList,
    std::vector< std::pair< stdstr, stdstr >>& vecArgs )
{
    if( pArgList == nullptr )
        return -EINVAL;
    gint32 ret = 0;
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
        std::string strVarName =
            pfa->GetName();

        pObj = pfa->GetType();
        CAstNodeBase* pNode = pObj;
        std::string strSig =
            pNode->GetSignature();
        vecArgs.push_back(
            std::pair< stdstr, stdstr >
                ( strVarName, strSig ) );
    }
    return ret;
}

static gint32 EmitFormalArgList(
    CWriterBase* pWriter, CArgList* pInArgs )
{
    gint32 ret = 0;
    std::ofstream& ofp = *pWriter->m_curFp;
    do{
        std::vector< std::pair< stdstr, stdstr >> vecArgs;
        ret = GetArgsAndSigs( pInArgs, vecArgs );
        if( ERROR( ret ) )
            break;

        for( guint32 i = 0; i < vecArgs.size(); i++ )
        {
            auto& elem = vecArgs[ i ];
            std::map< char, stdstr >::iterator itr =
                g_mapSig2Type.find( elem.second[ 0 ] ); 
            if( itr == g_mapSig2Type.end() )
            {
                ret = -ENOENT;
                break;
            }
            stdstr strType = itr->second;
            ofp << elem.first << " : " << strType;
            if( i < vecArgs.size() - 1 )
            {
                ofp << ",";
                pWriter->NewLine();
            }
        }

    }while( 0 );

    return ret;
}

CPyFileSet::CPyFileSet(
    const std::string& strOutPath,
    const std::string& strAppName )
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

        std::string strSvrPyBase = m_strPath +
            "/" + strSvcName + "svrbase.py";

        std::string strCliPyBase = m_strPath +
            "/" + strSvcName + "clibase.py";

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

        pstm = STMPTR( new std::ofstream(
            strCliPyBase,
            std::ofstream::out |
            std::ofstream::trunc) );

        idx += 1;
        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strCliPyBase ] = idx;

        pstm = STMPTR( new std::ofstream(
            strSvrPyBase,
            std::ofstream::out |
            std::ofstream::trunc) );

        idx += 1;
        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strSvrPyBase ] = idx;

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
    Wa( "def Deserialize( self, buf : bytearray, offset : int ) -> ( int, int ):" );
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

            stdstr strField = "self.";
            strField += strName;
            ret = EmitSerialBySig(
                m_pWriter, strField, strSig );
            if( ERROR( ret ) )
                break;

            if( i < dwCount - 1 )
                NEW_LINES( 2 );
        }
        NEW_LINE;
        Wa( "return 0" );
        INDENT_DOWNL;
        NEW_LINE;

        CCOUT << "def Deserialize( self, buf : bytearray, offset : int ) -> ( int, int ):";
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
            ret = EmitDeserialBySig(
                m_pWriter, strSig );
            if( ERROR( ret ) )
                break;
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
        pWriter->WriteLine0( "from seribase import CSerialBase" );
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

        if( vecActStructs.size() > 0 )
        {
            pWriter->WriteLine0(
                "from seribase import g_mapStructs" );
        }
        std::ofstream& ofp = *pWriter->m_curFp;
        for( int i = 0; i < vecActStructs.size(); i++ )
        {
            CStructDecl* pStruct = vecActStructs[ i ];
            stdstr strName = pStruct->GetName(); 
            stdstr strMsgId =
                g_strAppName + "::" + strName;
            guint32 dwMsgId = GenClsid( strMsgId );
            ofp << "g_mapStructs[ " << dwMsgId
                << " ] = \"" << strName << "\"";
            if( i < vecActStructs.size() - 1 )
                pWriter->NewLine();
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
        /*CCOUT << "from " << g_strAppName << "structs" << " import *";
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

        NEW_LINE;*/
        Wa( "print( 'initializing...' )" );

    }while( 0 );

    return ret;
}

CPyExportMakefile::CPyExportMakefile(
    CPyWriter* pWriter, ObjPtr& pNode )
    : super( pWriter, pNode )
{
    m_strFile = "./pymktpl";
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

CImplPyMthdProxyBase::CImplPyMthdProxyBase(
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

gint32 CImplPyMthdProxyBase::OutputEvent()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        CCOUT << "def " << strName << "Wrapper" << "( self,";
        NEW_LINE;
        CCOUT << "    callback : cpp.ObjPtr, buf : bytearray ) :";
        INDENT_UPL;
        NEW_LINE;
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );
        if( dwInCount == 0 )
        {
            CCOUT << "self." << strName << "()";
            NEW_LINE;
        }
        else
        {
            Wa( "offset = 0" );
            Wa( "listArgs = []" );
            Wa( "osb = CSerialBase( self )" );
            NEW_LINE;
            guint32 i = 0;
            CArgList* pArgList = pInArgs;
            for( ; i < dwInCount; i++ )
            {
                CFormalArg* pfa =
                    pArgList->GetChild( i );

                stdstr strArg = pfa->GetName();
                CAstNodeBase* pType =
                    pfa->GetType();

                stdstr strSig =
                    pType->GetSignature();

                ret = EmitDeserialBySig(
                    m_pWriter, strSig );
                if( ERROR( ret ) )
                    break;

                NEW_LINE;
                Wa( "if ret[ 0 ] is None :" );
                Wa( "    return" );
                Wa( "listArgs.append( ret[ 0 ] )" );
                if( i < dwInCount - 1 )
                    Wa( "offset = ret[ 1 ]  " );
            }
            CCOUT << "self."<< strName << "( *listArgs )";
            NEW_LINE;
        }
        Wa( "return" );
        INDENT_DOWNL;
        NEW_LINE;

    }while( 0 );

    return ret;
}

void CImplPyMthdProxyBase::EmitOptions()
{
    stdstr strName = m_pNode->GetName();
    Wa( "#preparing the options" );
    Wa( "oOptions = cpp.CParams()" );
    Wa( "oOptions.SetStrProp( cpp.propIfName, self._ifName_ )" );
    CCOUT << "oOptions.SetStrProp( cpp.propMethodName, \"" << strName << "\" )";
    NEW_LINE;
    Wa( "oOptions.SetIntProp( cpp.propSeriProto, cpp.seriRidl )" );
    Wa( "oOptions.SetBoolProp( cpp.propSysMethod, True )" );
    if( m_pNode->IsNoReply() )
        Wa( "oOptions.SetBoolProp( cpp.propNoReply, True )" );

    guint32 dwTimeoutSec =
        m_pNode->GetTimeoutSec();
    if( dwTimeoutSec == 0 )
        return;

    CCOUT << "dwTimeoutSec = " << dwTimeoutSec;
    NEW_LINE;
    Wa( "oOptions.SetIntProp(" );
    Wa( "    propTimeoutSec, dwTimeoutSec )" ); 
    Wa( "oOptions.RemoveProperty( propKeepAliveSec )" );
    Wa( "if dwTimeoutSec > 2 :" );
    Wa( "    oOptions.SetIntProp( propKeepAliveSec," );
    Wa( "        dwTimeoutSec / 2 )" );
}

gint32 CImplPyMthdProxyBase::OutputSync()
{
    gint32 ret = 0;
    do{
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
            CCOUT << "def " << strName << "( self, "; 
            INDENT_UPL;

            ret = EmitFormalArgList(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;

            NEW_LINE;
            Wa( ") -> list[ int, list ] :" );
            NEW_LINE;
            Wa( "osb = CSerialBase( self )" );
            Wa( "buf = bytearray()" );
            NEW_LINE;

            CArgList* pinal = pInArgs;
            for( guint32 i = 0; i < dwInCount; i++ )
            {
                CFormalArg* pfa = pinal->GetChild( i );
                if( pfa == nullptr )
                    continue;
                CAstNodeBase* pType = pfa->GetType();
                if( pType == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                stdstr strSig = pType->GetSignature();
                stdstr strArg = pfa->GetName();
                ret = EmitSerialBySig(
                    m_pWriter, strArg, strSig );
                if( ERROR( ret ) )
                    break;
            }
            if( ERROR( ret ) )
                break;

            Wa( "listArgs = [ buf, ]" );
        }
        else
        {
            CCOUT << "def " << strName << "( self )->list[int,list] :";
            INDENT_UPL;
            NEW_LINE;
            Wa( "listArgs = []" );
        }

        EmitOptions();

        Wa( "listResp = []" );
        Wa( "ret = self.MakeCallWithOpt( " );
        Wa( "    oOptions.GetCfg(), listArgs, listResp )" );
        if( dwOutCount == 0 )
        {
            Wa( "return ret" );
            INDENT_DOWNL;
            break;
        }
        Wa( "if ret[ 0 ] < 0 :" );
        Wa( "    return ret" );
        Wa( "buf = ret[ 1 ][ 0 ]" );
        Wa( "if buf is None or not isinstance( buf, bytearray ) : " );
        Wa( "   ret[ 0 ] = -errno.EBADMSG " );
        Wa( "   ret[ 1 ] = None" );
        Wa( "   return ret " );
        NEW_LINE;

        Wa( "offset = 0" );
        if( dwInCount == 0 )
            Wa( "osb = CSerialBase( self )" );

        CArgList* pal = pOutArgs;
        for( guint32 i = 0; i < dwOutCount; i++ )
        {
            ObjPtr pObj = pal->GetChild( i );
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
                continue;
            CAstNodeBase* pType = pfa->GetType();
            if( pType == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strSig = pType->GetSignature();
            ret = EmitDeserialBySig(
                m_pWriter, strSig );
            if( ERROR( ret ) )
                break;

            Wa( "listResp.append( ret[ 0 ] )" );
            Wa( "offset = ret[ 1 ]" );
            NEW_LINE;
        }

        Wa( "return [ 0, listResp ]" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdProxyBase::OutputAsync()
{
    gint32 ret = 0;
    do{
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
            CCOUT << "def " << strName << "( self, context, "; 
            INDENT_UPL;
            ret = EmitFormalArgList(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;

            NEW_LINE;
            Wa( ") -> list[ int, list ] :" );
            NEW_LINE;
            Wa( "osb = CSerialBase( self )" );
            Wa( "buf = bytearray()" );
            NEW_LINE;

            CArgList* pal = pInArgs;
            for( guint32 i = 0; i < dwInCount; i++ )
            {
                CFormalArg* pfa = pal->GetChild( i );
                if( pfa == nullptr )
                    continue;
                CAstNodeBase* pType = pfa->GetType();
                if( pType == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                stdstr strSig = pType->GetSignature();
                stdstr strArg = pfa->GetName();
                ret = EmitSerialBySig(
                    m_pWriter, strArg, strSig );
                if( ERROR( ret ) )
                    break;
            }
            if( ERROR( ret ) )
                break;

            Wa( "listArgs = [ buf, ]" );
        }
        else
        {
            CCOUT << "def " << strName << "( self, context )->list[int,list] :";
            INDENT_UPL;
            NEW_LINE;
            Wa( "listArgs = []" );
        }

        EmitOptions();

        Wa( "listResp = []" );
        Wa( "ret = self.MakeCallWithOptAsync( " );
        CCOUT << "    self." << strName << "CbWrapper, context,";
        NEW_LINE;
        Wa( "    oOptions.GetCfg(), listArgs, listResp )" );
        Wa( "if ret[ 0 ] == ErrorCode.STATUS_PENDING :" );
        Wa( "    return ret" );
        Wa( "if ret[ 0 ] < 0 :" );
        Wa( "    return ret" );
        if( dwOutCount == 0 )
        {
            Wa( "return ret" );
            INDENT_DOWNL;
            break;
        }
        Wa( "buf = ret[ 1 ][ 0 ]" );
        Wa( "if buf is None or not isinstance( buf, bytearray ) : " );
        Wa( "   ret[ 0 ] = -errno.EBADMSG " );
        Wa( "   ret[ 1 ] = None" );
        Wa( "   return ret " );
        NEW_LINE;

        Wa( "offset = 0" );
        if( dwInCount == 0 )
            Wa( "osb = CSerialBase( self )" );

        CArgList* pal = pOutArgs;
        for( guint32 i = 0; i < dwOutCount; i++ )
        {
            ObjPtr pObj = pal->GetChild( i );
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
                continue;
            CAstNodeBase* pType = pfa->GetType();
            if( pType == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strSig = pType->GetSignature();
            ret = EmitDeserialBySig(
                m_pWriter, strSig );
            if( ERROR( ret ) )
                break;

            Wa( "listResp.append( ret[ 0 ] )" );
            Wa( "offset = ret[ 1 ]" );
            NEW_LINE;
        }

        Wa( "return [ 0, listResp ]" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdProxyBase::OutputAsyncCbWrapper()
{
    gint32 ret = 0;
    do{
        CMethodDecl* pmd = m_pNode;
        stdstr strName = pmd->GetName();
        stdstr strIfName = m_pIf->GetName();
        ObjPtr pOutArgs = pmd->GetOutArgs();
        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        CCOUT << "def " << strName
            << "CbWrapper( self, context, ret, listArgs ) :"; 
        INDENT_UPL;

        Wa( "targetMethod = self.GetMethod(" );
        CCOUT << "    \"" << strIfName << "\", \"" << strName << "Cb\" )";
        NEW_LINE;
        Wa( "if targetMethod is None :" );
        Wa( "    return" );

        Wa( "listResp = [context, ret]" );
        if( dwOutCount == 0 )
        {
            Wa( "self.InvokeCallback( targetMethod, listResp );" );
            Wa( "return" );
            INDENT_DOWNL;
            break;
        }

        NEW_LINE;
        Wa( "if listArgs is None or len( listArgs ) < 1 :" );
        Wa( "    return" );
        Wa( "buf = listArgs[ 0 ]" );
        Wa( "if buf is None or not isinstance( buf, bytearray ):" );
        Wa( "    return" );
        NEW_LINE;

        Wa( "offset = 0" );
        Wa( "osb = CSerialBase( self )" );

        CArgList* pal = pOutArgs;
        for( guint32 i = 0; i < dwOutCount; i++ )
        {
            ObjPtr pObj = pal->GetChild( i );
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
                continue;
            CAstNodeBase* pType = pfa->GetType();
            if( pType == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strSig = pType->GetSignature();
            ret = EmitDeserialBySig(
                m_pWriter, strSig );
            if( ERROR( ret ) )
                break;

            Wa( "listResp.append( ret[ 0 ] )" );
            Wa( "offset = ret[ 1 ]" );
            NEW_LINE;
        }

        Wa( "self.InvokeCallback( targetMethod, listResp );" );
        Wa( "return" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdProxyBase::Output()
{
    if( m_pNode->IsEvent() )
    {
        return OutputEvent();
    }
    else if( m_pNode->IsAsyncp() )
    {
        gint32 ret = OutputAsync();
        if( ERROR( ret ) )
            return ret;
        return OutputAsyncCbWrapper();
    }
    return OutputSync();
}

CImplPyIfProxyBase::CImplPyIfProxyBase(
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

gint32 CImplPyIfProxyBase::Output()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        CCOUT << "class I" << strIfName << "_CliImpl :";
        INDENT_UP;
        NEW_LINES( 2 );
        CCOUT << "_ifName_ = \"" <<  strIfName << "\"";
        NEW_LINE;
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
            CImplPyMthdProxyBase oipm(
                m_pWriter, elem );
            oipm.Output();
        }
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPySvcProxyBase::CImplPySvcProxyBase(
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

#define OUTPUT_BANNER() \
    Wa( "#Generated by RIDLC, make sure to backup if "\
        "there are important changes" );\
    Wa( "from rpcf.rpcbase import *" ); \
    Wa( "from rpcf.proxy import PyRpcServices, ErrorCode" );\
    Wa( "from seribase import CSerialBase" );\
    Wa( "import errno" ); \
    NEW_LINE

gint32 CImplPySvcProxyBase::Output()
{
    gint32 ret = 0;

    do{
        OUTPUT_BANNER();

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

            CImplPyIfProxyBase oipb(
                m_pWriter, pObj );

            ret = oipb.Output();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

gint32 GenSvcFiles(
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

            pWriter->SelectImplFile(
                strCommon + "clibase.py" ); 

            CImplPySvcProxyBase opspb(
                pWriter, elem.second );
            ret = opspb.Output();
            if( ERROR( ret ) )
                break;

            // server base imlementation
            pWriter->SelectImplFile(
                strCommon + "svrbase.py" );

            CImplPySvcSvrBase opssb(
                pWriter, elem.second );
            ret = opssb.Output();
            if( ERROR( ret ) )
                break;

            // server imlementation
            pWriter->SelectImplFile(
                strCommon + "svr.py" );

            CImplPySvcSvr opss(
                pWriter, elem.second );
            ret = opss.Output();
            if( ERROR( ret ) )
                break;

            // client imlementation
            pWriter->SelectImplFile(
                strCommon + "cli.py" );

            CImplPySvcProxy opsc(
                pWriter, elem.second );
            ret = opsc.Output();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

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

        oWriter.SelectDrvFile();
        CExportDrivers oedrv( &oWriter, pRoot );
        ret = oedrv.Output();
        if( ERROR( ret ) )
            break;

        oWriter.SelectDescFile();
        CExportObjDesc oedesc( &oWriter, pRoot );
        ret = oedesc.Output();
        if( ERROR( ret ) )
            break;

        oWriter.SelectMakefile();
        CPyExportMakefile opemk( &oWriter, pRoot );
        ret = opemk.Output();
        if( ERROR( ret ) )
            break;

        ret = GenSvcFiles( &oWriter, pRoot );

    }while( 0 );

    return ret;
}

CImplPyMthdSvrBase::CImplPyMthdSvrBase(
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

gint32 CImplPyMthdSvrBase::OutputSync( bool bSync )
{
    gint32 ret = 0;

    if( bSync )
    {
        Wa( "'''" );
        Wa( "Synchronous request handler wrapper," );
        Wa( "which will deserialize the incoming parameters,");
        Wa( "call the actual request handler, and return the" );
        Wa( "serialized response. It does not accept" );
        Wa( "STATUS_PENDING on handler return. " );
        Wa( "'''" );
    }

    do{
        stdstr strName = m_pNode->GetName();
        stdstr strIfName = m_pIf->GetName();
        CCOUT << "def " << strName << "Wrapper( self,";
        NEW_LINE;
        CCOUT << "    callback : cpp.ObjPtr, buf : bytearray";
        NEW_LINE;
        CCOUT << "    ) -> list[ int, list ] :";
        INDENT_UPL;
        NEW_LINE;
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );
        ObjPtr pOutArgs = m_pNode->GetOutArgs();

        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        if( dwInCount == 0 )
        {
            if( dwOutCount > 0 )
                Wa( "osb = CSerialBase( self )" );
            Wa( "targetMethod = self.GetMethod(" );
            CCOUT << "    \"" << strIfName << "\", \"" << strName << "\" )";
            NEW_LINE;
            Wa( "if targetMethod is None :" );
            Wa( "    return [ -errno.EFAULT, None ]" );
            Wa( "ret = self.InvokeCallback( targetMethod, [ callback, ] );" );
        }
        else
        {
            Wa( "offset = 0" );
            Wa( "listArgs = [ callback, ]" );
            Wa( "osb = CSerialBase( self )" );
            NEW_LINE;
            guint32 i = 0;
            CArgList* pArgList = pInArgs;
            for( ; i < dwInCount; i++ )
            {
                CFormalArg* pfa =
                    pArgList->GetChild( i );

                CAstNodeBase* pType =
                    pfa->GetType();

                stdstr strSig =
                    pType->GetSignature();

                ret = EmitDeserialBySig(
                    m_pWriter, strSig );
                if( ERROR( ret ) )
                    break;

                Wa( "if ret[ 0 ] is None :" );
                Wa( "    return [ -errno.EFAULT, None ]" );
                Wa( "listArgs.append( ret[ 0 ] )" );
                NEW_LINE;
                if( i < dwInCount - 1 )
                    Wa( "offset = ret[ 1 ]  " );
            }
            Wa( "targetMethod = self.GetMethod(" );
            CCOUT << "    \"" << strIfName << "\", \"" << strName << "\" )";
            NEW_LINE;
            Wa( "if targetMethod is None :" );
            Wa( "    return [ -errno.EFAULT, None ]" );
            Wa( "ret = self.InvokeCallback(" );
            Wa( "    targetMethod, listArgs );" );
        }

        Wa( "bPending = ( ret[ 0 ] ==" );
        Wa( "    ErrorCode.STATUS_PENDING )" );

        if( bSync )
        {
            Wa( "if bPending :" );
            Wa( "    return [ -errno.EINVAL, [] ]" );
        }
        else
        {
            Wa( "if bPending:" );
            Wa( "    self.InstallCompNotify( callback," );
            CCOUT << "        self.On" << strName << "CanceledWrapper,";
            NEW_LINE;
            Wa( "        *listArgs )" );
            Wa( "    return [ ret[ 0 ], None ]" );
        }

        Wa( "if ret[ 0 ] < 0 :" );
        Wa( "    return [ ret[ 0 ], [] ]" );
        NEW_LINE;

        if( dwOutCount == 0 )
        {
            Wa( "return [ ret[ 0 ], [] ]" );
            INDENT_DOWNL;
            break;
        }

        Wa( "listResp = ret[ 1 ]" );
        Wa( "buf = bytearray()" );
        Wa( "iRet = ret[ 0 ]" );
        Wa( "ret = 0" );
        Wa( "while True:" );
        INDENT_UPL;
        CArgList* poutal = pOutArgs;
        for( guint32 i = 0; i < dwOutCount; i++ )
        {
            CFormalArg* pfa = poutal->GetChild( i );
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            std::string strVarName = "listResp[";
            strVarName += std::to_string( i ) + "]";

            CAstNodeBase* pNode = pfa->GetType();
            std::string strSig =
                pNode->GetSignature();

            ret = EmitSerialBySig(
                m_pWriter, strVarName,
                strSig, true );
            if( ERROR( ret ) )
                break;
            
        } 
        if( ERROR( ret ) )
            break;
        Wa( "break" );
        INDENT_DOWNL;
        Wa( "if ret < 0 :" );
        Wa( "    return [ ret, None ]" );
        Wa( "return [ iRet, [ buf, ] ]" );
        INDENT_DOWNL;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdSvrBase::OutputAsyncWrapper()
{
    Wa( "'''" );
    Wa( "Asynchronous request handler wrapper," );
    Wa( "which will deserialize the incoming parameters,");
    Wa( "call the actual request handler, and return the" );
    Wa( "serialized response. It accepts STATUS_PENDING" );
    Wa( "if the handler cannot finish the request" );
    Wa( "immediately" );
    Wa( "'''" );
    return OutputSync( false );
}

gint32 CImplPyMthdSvrBase::OutputAsyncCompHandler()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        ObjPtr pOutArgs = m_pNode->GetOutArgs();
        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        Wa( "'''" );
        Wa( "Call this method when the server has completed" );
        Wa( "the request, and about to return the response" );
        Wa( "'callback' is the one passed to the request handler" );
        Wa( "'ret' is the status code of the request" );
        Wa( "'''" );
        if( dwOutCount == 0 )
        {
            CCOUT << "def On" << strName << "Complete( self,";
            NEW_LINE;
            Wa( "    callback : cpp.ObjPtr, ret : int ): " );
            INDENT_UPL;
            Wa( "self.RidlOnServiceComplete(" );
            Wa( "    callback, ret, bytearray() )" );
            Wa( "return" );
            INDENT_DOWNL;
            break;
        }

        CCOUT << "def On" << strName << "Complete"
            << "( self, callback : cpp.ObjPtr, ret : int,";
        INDENT_UPL;

        ret = EmitFormalArgList( m_pWriter, pOutArgs );
        if( ERROR( ret ) )
            break;

        CCOUT << "):";
        NEW_LINE;
        Wa( "osb = CSerialBase( self )" );
        Wa( "buf = bytearray()" );
        Wa( "while True:" );
        INDENT_UPL;
        guint32 i = 0;
        CArgList* pArgList = pOutArgs;
        for( ; i < dwOutCount; i++ )
        {
            CFormalArg* pfa =
                pArgList->GetChild( i );

            stdstr strArg = pfa->GetName();
            CAstNodeBase* pType =
                pfa->GetType();

            stdstr strSig =
                pType->GetSignature();

            ret = EmitSerialBySig(
                m_pWriter, strArg, strSig, true );
            if( ERROR( ret ) )
                break;
            if( i < dwOutCount - 1 )
                NEW_LINES( 2 );
        }
        Wa( "break" );
        INDENT_DOWNL;
        Wa( "if ret < 0 :" );
        Wa( "    buf = bytearray()" );
        Wa( "self.RidlOnServiceComplete(" );
        Wa( "    callback, ret, buf )" );
        Wa( "return" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdSvrBase::OutputAsyncCHWrapper()
{
    gint32 ret = 0;
    do{
        Wa( "'''" );
        Wa( "This method is called when the request is" );
        Wa( "cancelled due to timeout or user request" );
        Wa( "it gives a chance to do some cleanup work." );
        Wa( "'''" );
        NEW_LINE;
        stdstr strName = m_pNode->GetName();
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );

        if( dwInCount == 0 )
        {
            CCOUT << "def On" << strName << "CanceledWrapper( self,";
            NEW_LINE;
            CCOUT << "    iRet : int, callback : cpp.ObjPtr ): ";
            NEW_LINE;
            CCOUT << "    self.On" << strName << "Canceled( self, ";
            NEW_LINE;
            CCOUT << "    callback, iRet ): ";
            NEW_LINE;
            break;
        }

        CCOUT << "def On" << strName << "CanceledWrapper( self,";
        NEW_LINE;
        CCOUT << "    iRet : int, callback : cpp.ObjPtr,";
        INDENT_UPL;

        ret = EmitFormalArgList( m_pWriter, pInArgs );
        if( ERROR( ret ) )
            break;

        CCOUT << "):";
        NEW_LINE;
        CCOUT << "self.On" << strName << "Canceled( self, ";
        INDENT_UPL;
        CCOUT << "callback, iRet,";
        NEW_LINE; 
        CArgList* pinal = pInArgs;
        for( guint32 i = 0; i < dwInCount; i++ )
        {
            CFormalArg* pfa = pinal->GetChild( i );
            CCOUT << pfa->GetName();
            if( i < dwInCount - 1 )
                CCOUT<< ", ";
        }

        CCOUT << " )";
        INDENT_DOWNL;
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdSvrBase::OutputEvent()
{
    gint32 ret = 0;
    do{
        Wa( "'''" );
        Wa( "Call this method whenever the sever has the" );
        Wa( "event to broadcast" );
        Wa( "'''" );
        NEW_LINE;
        stdstr strName = m_pNode->GetName();
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwOutCount =
            GetArgCount( pInArgs );

        if( dwOutCount == 0 )
        {
            CCOUT << "def " << strName
                << "( self, callback ): ";
            INDENT_UPL;
            Wa( "self.RidlSendEvent( self._ifName_," );
            CCOUT << "\"" << strName << "\", \"\", None )";
            break;
        }

        CCOUT << "def "<< strName << "( self,";
        NEW_LINE;
        Wa( "    callback : cpp.ObjPtr, ret : int," );
        INDENT_UPL;

        ret = EmitFormalArgList( m_pWriter, pInArgs );
        if( ERROR( ret ) )
            break;

        CCOUT << "):";
        NEW_LINE;
        Wa( "osb = CSerialBase( self )" );
        Wa( "buf = bytearray()" );
        Wa( "while True:" );
        INDENT_UPL;
        guint32 i = 0;
        CArgList* pArgList = pInArgs;
        for( ; i < dwOutCount; i++ )
        {
            CFormalArg* pfa =
                pArgList->GetChild( i );

            stdstr strArg = pfa->GetName();
            CAstNodeBase* pType =
                pfa->GetType();

            stdstr strSig =
                pType->GetSignature();

            ret = EmitSerialBySig(
                m_pWriter, strArg, strSig, true );
            if( ERROR( ret ) )
                break;
            if( i < dwOutCount - 1 )
                NEW_LINES( 2 );
        }
        Wa( "break" );
        INDENT_DOWNL;
        Wa( "if ret < 0 :" );
        Wa( "    return ret" );
        Wa( "self.RidlSendEvent( self._ifName_," );
        CCOUT << "    \"" << strName << "\", \"\", buf )";
        NEW_LINE;
        Wa( "return 0" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdSvrBase::Output()
{
    if( m_pNode->IsEvent() )
    {
        return OutputEvent();
    }
    else if( m_pNode->IsAsyncp() )
    {
        gint32 ret = 0;
        ret = OutputAsyncWrapper();
        if( ERROR( ret ) )
            return ret;

        ret = OutputAsyncCompHandler();
        if( ERROR( ret ) )
            return ret;

        ret = OutputAsyncCHWrapper();
        if( ERROR( ret ) )
            return ret;
    }
    return OutputSync();
}

CImplPyIfSvrBase::CImplPyIfSvrBase(
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

gint32 CImplPyIfSvrBase::Output()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        CCOUT << "class I" << strIfName << "_SvrImpl :";
        INDENT_UP;
        NEW_LINES( 2 );
        CCOUT << "_ifName_ = \"" <<  strIfName << "\"";
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
            CImplPyMthdSvrBase oism(
                m_pWriter, elem );
            oism.Output();
        }
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPySvcSvrBase::CImplPySvcSvrBase(
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

gint32 CImplPySvcSvrBase::Output()
{
    gint32 ret = 0;

    do{
        OUTPUT_BANNER();

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

            CImplPyIfSvrBase oisb(
                m_pWriter, pObj );

            ret = oisb.Output();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

CImplPyMthdSvr::CImplPyMthdSvr(
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

gint32 CImplPyMthdSvr::Output()
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
            CCOUT << "def " << strName << "( self, callback : cpp.ObjPtr, "; 
            INDENT_UPL;

            ret = EmitFormalArgList(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }
        else
        {
            CCOUT << "def " << strName << "( self, callback : cpp.ObjPtr ): "; 
            INDENT_UPL;
        }

        if( dwOutCount > 0 )
        {
            Wa( ") -> list[ int, list ] :" );
            Wa( "'''" );
            std::vector< std::pair< stdstr, stdstr > > vecArgs;
            ret = GetArgsAndSigs( pOutArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            Wa( "response parameters includes the following" );
            for( auto& elem : vecArgs )
            {
                CCOUT << elem.first << " : "
                    << g_mapSig2Type[ elem.second[ 0 ] ];
                NEW_LINE;
            }

            Wa( "Implement this method here" );
            Wa( "'''" );
        }
        else
            Wa( ") -> list[ int, None ] :" );

        Wa( "return [ -errno.ENOTSUP, None ]" );
        INDENT_DOWNL;

        if( m_pNode->IsAsyncs() )
            OutputAsyncCancelHandler();

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdSvr::OutputAsyncCancelHandler()
{
    gint32 ret = 0;
    do{
        Wa( "'''" );
        Wa( "This method is called when the async" );
        Wa( "request is cancelled due to timeout" );
        Wa( "or user request. Add your own cleanup" );
        Wa( "code here" );
        Wa( "'''" );
        NEW_LINE;
        stdstr strName = m_pNode->GetName();
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );

        if( dwInCount == 0 )
        {
            CCOUT << "def On" << strName << "Canceled( self,";
            NEW_LINE;
            CCOUT << "    callback : cpp.ObjPtr, iRet : int ): ";
            NEW_LINE;
            Wa( "pass" );
            break;
        }

        CCOUT << "def On" << strName << "Canceled( self,";
        NEW_LINE;
        CCOUT << "    callback : cpp.ObjPtr, iRet : int,";
        INDENT_UPL;

        ret = EmitFormalArgList( m_pWriter, pInArgs );
        if( ERROR( ret ) )
            break;

        CCOUT << " ):";
        NEW_LINE;
        Wa( "pass" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPyIfSvr::CImplPyIfSvr(
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

gint32 CImplPyIfSvr::Output()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        CCOUT << "class C" << strIfName << "svr( I"
            << strIfName << "_SvrImpl ):";
        INDENT_UP;
        NEW_LINES( 2 );
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
            CImplPyMthdSvr oms( m_pWriter, elem );
            oms.Output();
        }

        if( !bHasMethod )
            Wa( "pass" );

        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPySvcSvr::CImplPySvcSvr(
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

gint32 CImplPySvcSvr::OutputSvcSvrClass()
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

        guint32 i = 0;
        guint32 dwCount = vecIfs.size();
        for( ; i < dwCount; i++ )
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
            stdstr strIf = pifd->GetName();

            CCOUT << "C" << strIf << "svr,";
            NEW_LINE;
        }

        CCOUT << "PyRpcServer ) :";
        INDENT_UPL;
        Wa( "def __init__( self, pIoMgr, strDesc, strObjName ) :" );
        Wa( "    PyRpcServer.__init__( self," );
        CCOUT << "        pIoMgr, strDesc, strObjName )";
        INDENT_DOWN;
        INDENT_DOWN;
        NEW_LINE;
    }while( 0 );

    return ret;
}

gint32 CImplPySvcSvr::Output()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        OUTPUT_BANNER();
        Wa( "from rpcf.proxy import PyRpcServer" );
        CCOUT << "from " << strName
            << "svrbase import *";
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

            CImplPyIfSvr opis( m_pWriter, pObj );
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

gint32 CImplPyMthdProxy::OutputAsyncCallback()
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
            CCOUT << "def " << strName << "Cb( self,";
            NEW_LINE;
            CCOUT << "    context : cpp.ObjPtr, ret : int, "; 
            INDENT_UPL;

            ret = EmitFormalArgList(
                m_pWriter, pOutArgs );
            if( ERROR( ret ) )
                break;
            Wa( " ) :" );
        }
        else
        {
            CCOUT << "def " << strName << "Cb( self,";
            NEW_LINE;
            CCOUT << "    context : cpp.ObjPtr, ret : int ):"; 
            NEW_LINE;
            INDENT_UPL;
        }

        Wa( "pass" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdProxy::OutputEvent()
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
            CCOUT << "def " << strName << "( self, callback : cpp.ObjPtr, "; 
            INDENT_UPL;

            ret = EmitFormalArgList(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }
        else
        {
            CCOUT << "def " << strName << "( self, callback : cpp.ObjPtr ): "; 
            INDENT_UPL;
        }

        Wa( ") :" );

        Wa( "return" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CImplPyMthdProxy::Output()
{
    if( m_pNode->IsAsyncp() )
        return OutputAsyncCallback();
    if( m_pNode->IsEvent() )
        return OutputEvent();
    return -ENOTSUP;
}

CImplPyMthdProxy::CImplPyMthdProxy( 
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

CImplPyIfProxy::CImplPyIfProxy(
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

gint32 CImplPyIfProxy::Output()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        CCOUT << "class C" << strIfName << "cli( I"
             << strIfName << "_CliImpl ):";
        INDENT_UP;
        NEW_LINES( 2 );
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
            if( !pmd->IsAsyncp() )
                continue;

            CImplPyMthdProxy oipm(
                m_pWriter, elem );
            oipm.Output();
        }

        if( !bHasMethod )
            Wa( "pass" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

CImplPySvcProxy::CImplPySvcProxy(
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

gint32 CImplPySvcProxy::OutputSvcProxyClass()
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

        guint32 i = 0;
        guint32 dwCount = vecIfs.size();
        for( ; i < dwCount; i++ )
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
            stdstr strIf = pifd->GetName();

            CCOUT << "C" << strIf << "cli,";
            NEW_LINE;
        }

        CCOUT << "PyRpcProxy ) :";
        INDENT_UPL;
        Wa( "def __init__( self, pIoMgr, strDesc, strObjName ) :" );
        Wa( "    PyRpcProxy.__init__( self," );
        CCOUT << "        pIoMgr, strDesc, strObjName )";
        INDENT_DOWN;
        INDENT_DOWN;
        NEW_LINE;
    }while( 0 );

    return ret;
}
gint32 CImplPySvcProxy::Output()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        OUTPUT_BANNER();
        Wa( "from rpcf.proxy import PyRpcProxy" );
        CCOUT << "from " << strName
            << "clibase import *";
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

            CImplPyIfProxy opip( m_pWriter, pObj );
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
