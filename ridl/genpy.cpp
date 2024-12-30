/*
 * =====================================================================================
 *
 *       Filename:  genpy.cpp
 *
 *    Description:  implementations of proxy/server generator for Python
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
#include "genpy.h"
#include "gencpp2.h"
#include "jsondef.h"

extern std::string g_strAppName;
extern stdstr g_strCmdLine;
extern stdstr g_strLang;
extern gint32 SetStructRefs( ObjPtr& pRoot );
extern guint32 GenClsid( const std::string& strName );
extern bool g_bRpcOverStm;
extern bool g_bBuiltinRt;

std::map< char, stdstr > g_mapSig2PyType =
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
    { 'h', "int" },
    { 'v', "Variant" },
};

static gint32 EmitDeserialBySig(
    CWriterBase* pWriter,
    const stdstr& strSig )
{
    gint32 ret = 0;
    // std::ofstream& ofp = *pWriter->m_curFp;
    CWriterBase* m_pWriter = pWriter;
    switch( strSig[ 0 ] )
    {
    case '(' :
        {
            CCOUT << "ret = osb.DeserialArray( buf, offset, \""
                << strSig << "\" )";
            break;
        }
    case '[' :
        {
            CCOUT << "ret = osb.DeserialMap( buf, offset, \""
                << strSig << "\" )";
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
            CCOUT << "ret = osb.DeserialInt64( buf, offset )";
            break;
        }
    case 'D':
    case 'd':
        {
            CCOUT << "ret = osb.DeserialInt32( buf, offset )";
            break;
        }
    case 'W':
    case 'w':
        {
            CCOUT << "ret = osb.DeserialInt16( buf, offset )";
            break;
        }
    case 'b':
    case 'B':
        {
            CCOUT << "ret = osb.DeserialInt8( buf, offset )";
            break;
        }
    case 'f':
        {
            CCOUT << "ret = osb.DeserialFloat( buf, offset )";
            break;
        }
    case 'F':
        {
            CCOUT << "ret = osb.DeserialDouble( buf, offset )";
            break;
        }
    case 's':
        {
            CCOUT << "ret = osb.DeserialString( buf, offset )";
            break;
        }
    case 'a':
        {
            CCOUT << "ret = osb.DeserialBuf( buf, offset )";
            break;
        }
    case 'o':
        {
            CCOUT << "ret = osb.DeserialObjPtr( buf, offset )";
            break;
        }
    case 'h':
        {
            CCOUT << "ret = osb.DeserialHStream( buf, offset )";
            break;
        }
    case 'v':
        {
            CCOUT << "ret = osb.DeserialVariant( buf, offset )";
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

static gint32 EmitSerialBySig(
    CWriterBase* pWriter,
    const stdstr& strField,
    const stdstr& strSig,
    bool bNoRet = false )
{
    gint32 ret = 0;
    // std::ofstream& ofp = *pWriter->m_curFp;
    CWriterBase* m_pWriter = pWriter;
    switch( strSig[ 0 ] )
    {
    case '(' :
        {
            CCOUT << "ret = osb.SerialArray( buf, "
                << strField << ", \"" << strSig << "\" )";
            NEW_LINE;
            CCOUT << "if ret < 0 :";
            NEW_LINE;
            if( bNoRet )
                CCOUT << "    break";
            else
                CCOUT << "    return ret";
            break;
        }
    case '[' :
        {
            CCOUT << "ret = osb.SerialMap( buf, "
                << strField << ", \"" << strSig << "\" )";
            NEW_LINE;
            CCOUT << "if ret < 0 :";
            NEW_LINE;
            if( bNoRet )
                CCOUT << "    break";
            else
                CCOUT << "    return ret";
            break;
        }

    case 'O' :
        {
            CCOUT << "ret = osb.SerialStruct( buf, " << strField << " )";
            NEW_LINE;
            CCOUT << "if ret < 0 :";
            NEW_LINE;
            if( bNoRet )
                CCOUT << "    break";
            else
                CCOUT << "    return ret";
            break;
        }
    case 'Q':
    case 'q':
        {
            CCOUT << "osb.SerialInt64( buf, " << strField << " )";
            break;
        }
    case 'D':
    case 'd':
        {
            CCOUT << "osb.SerialInt32( buf, " << strField << " )";
            break;
        }
    case 'W':
    case 'w':
        {
            CCOUT << "osb.SerialInt16( buf, " << strField << " )";
            break;
        }
    case 'b':
    case 'B':
        {
            CCOUT << "osb.SerialInt8( buf, " << strField << " )";
            break;
        }
    case 'f':
        {
            CCOUT << "osb.SerialFloat( buf, " << strField << " )";
            break;
        }
    case 'F':
        {
            CCOUT << "osb.SerialDouble( buf, " << strField << " )";
            break;
        }
    case 's':
        {
            CCOUT << "osb.SerialString( buf, " << strField << " )";
            break;
        }
    case 'a':
        {
            CCOUT << "osb.SerialBuf( buf, " << strField << " )";
            break;
        }
    case 'o':
        {
            CCOUT << "ret = osb.SerialObjPtr( buf, " << strField << " )";
            NEW_LINE;
            CCOUT << "if ret < 0 :";
            NEW_LINE;
            if( bNoRet )
                CCOUT << "    break";
            else
                CCOUT << "    return ret";
            break;
        }
    case 'h':
        {
            CCOUT << "ret = osb.SerialHStream( buf, " << strField << " )";
            NEW_LINE;
            CCOUT << "if ret < 0 :";
            NEW_LINE;
            if( bNoRet )
                CCOUT << "    break";
            else
                CCOUT << "    return ret";
            break;
        }
    case 'v':
        {
            CCOUT << "ret = osb.SerialVariant( buf, " << strField << " )";
            NEW_LINE;
            CCOUT << "if ret < 0 :";
            NEW_LINE;
            if( bNoRet )
                CCOUT << "    break";
            else
                CCOUT << "    return ret";
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

gint32 GetArgsAndSigs( CArgList* pArgList,
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

gint32 EmitFormalArgListPy(
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
                g_mapSig2PyType.find( elem.second[ 0 ] ); 
            if( itr == g_mapSig2PyType.end() )
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

stdstr GetTypeName( CAstNodeBase* pType )
{
    EnumClsid iClsid = pType->GetClsid();
    if( iClsid == clsid( CStructDecl ) )
    {
        CStructDecl* pStruct =
            static_cast< CStructDecl* >( pType );
        return pStruct->GetName();
    }
    else if( iClsid == clsid( CStructRef ) )
    {
        CStructRef* pStruct = ObjPtr( pType );
            static_cast< CStructRef* >( pType );
        return pStruct->GetName();
    }
    else if( iClsid == clsid( CArrayType ) )
    {
        CArrayType* pArray =
            static_cast< CArrayType* >( pType );

        CAstNodeBase* pet = pArray->GetElemType();
        stdstr strElem = GetTypeName( pet );
        return stdstr( "(" ) + strElem + ")";
    }
    else if( iClsid == clsid( CMapType ) )
    {
        CMapType* pMap =
            static_cast< CMapType* >( pType );
        
        CAstNodeBase* pkt = pMap->GetKeyType();
        stdstr strKey = GetTypeName( pkt );

        CAstNodeBase* pet = pMap->GetElemType();
        stdstr strElem = GetTypeName( pet );
        return stdstr( "[" ) + strKey +
            "," + strElem + "]";
    }

    CPrimeType* pPrime =
        static_cast< CPrimeType* >( pType );

    if( g_strLang == "py" )
        return pPrime->ToStringPy();
    else if( g_strLang == "js" )
        return pPrime->ToStringJs();
    return pPrime->ToStringCpp();
}

static gint32 EmitRespList(
    CWriterBase* pWriter, CArgList* pOutArgs )
{
    gint32 ret = 0;
    std::ofstream& ofp = *pWriter->m_curFp;
    do{
        pWriter->WriteLine0( "'''" );
        std::vector< std::pair< stdstr, stdstr > > vecArgs;
        ret = GetArgsAndSigs( pOutArgs, vecArgs );
        if( ERROR( ret ) )
            break;

        pWriter->WriteLine0(
            "the response parameters includes" );

        std::map< char, stdstr >::iterator itr;
        stdstr strType;
        for( int i = 0; i < vecArgs.size(); i++ )
        {
            auto& elem = vecArgs[ i ];
            if( elem.second[ 0 ] != 'O' &&
                elem.second[ 0 ] != '(' &&
                elem.second[ 0 ] != '[' )
            {
                itr = g_mapSig2PyType.find(
                    elem.second[ 0 ] );

                if( itr != g_mapSig2PyType.end() )
                    strType = itr->second;
                else
                    strType = "unknown";
            }
            else
            {
                CFormalArg* pfa =
                    pOutArgs->GetChild( i );

                CAstNodeBase* pType =
                    pfa->GetType();

                strType = GetTypeName( pType );
            }
            ofp << elem.first << " : " << strType;

            pWriter->NewLine();
        }
        pWriter->WriteLine0( "'''" );

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
        strOutPath, strAppName + "structs.py",
        false );

    GEN_FILEPATH( m_strObjDesc,
        strOutPath, strAppName + "desc.json",
        false );

    GEN_FILEPATH( m_strDriver,
        strOutPath, "driver.json",
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

    pstm = STMPTR( new std::ofstream(
        m_strReadme,
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
                    stdstr strObjInit = "object()";
                    do{
                        CStructRef* pRef = ObjPtr( pType );
                        if( pRef == nullptr )
                            break;

                        // the struct_ref node has a name
                        // either to be the struct name or
                        // an alias name, we need to check
                        // both cases to find out the actual
                        // struct name. 
                        ObjPtr pTrueType;
                        stdstr strTypeName = pRef->GetName();
                        ret = g_mapDecls.GetDeclNode(
                            strTypeName, pTrueType );

                        if( SUCCEEDED( ret ) )
                        {
                            CStructDecl* pDecl = pTrueType;
                            strObjInit = pDecl->GetName() + "( self )";
                            break;
                        }

                        // this is an alias, lookup into alias map
                        ret = g_mapAliases.GetAliasType(
                            strTypeName, pTrueType );
                        pRef = pTrueType;
                        if( pRef != nullptr )
                        {
                            strObjInit = pRef->GetName() + "( self )";
                            break;
                        }

                        CStructDecl* pDecl = pTrueType;
                        if( pDecl != nullptr )
                        {
                            strObjInit = pDecl->GetName() + "( self )";
                        }

                    }while( 0 );

                    CCOUT << "self." << strName << " = " << strObjInit ;
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
            case 'v':
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
        CCOUT << "osb.SerialInt32( buf, " << strName << ".GetStructId() )";
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
        CCOUT << "ret = osb.DeserialInt32( buf, offset )";
        NEW_LINE;
        Wa( "if ret[ 0 ] is None :" );
        Wa( "    return ( -errno.ENOENT, 0 )" );
        CCOUT << "if ret[ 0 ] != " << strName <<".GetStructId() :";
        NEW_LINE;
        CCOUT << "    return ( -errno.EBADMSG, 0 )";
        NEW_LINE;
        Wa( "offset = ret[ 1 ]" );
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

static gint32 GenStructsFilePy(
    CPyWriter* m_pWriter, ObjPtr& pRoot )
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
        if( strSeribase != m_pWriter->GetOutPath() )
        {
            stdstr strCmd = "cp ";
            strCmd += strSeribase +
                " " + m_pWriter->GetOutPath();
            system( strCmd.c_str() );
        }

        m_pWriter->SelectStructsFile();
        EMIT_DISCLAIMER_PY;
        CCOUT << "# " << g_strCmdLine;
        NEW_LINE;
        Wa( "from rpcf.proxy import PyRpcServices" );
        Wa( "from seribase import CSerialBase" );
        Wa( "from seribase import Variant" );
        Wa( "import errno" );

        NEW_LINE;

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
            CDeclarePyStruct odps( m_pWriter, elem );
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
        Wa( "#GENERATED BY RIDLC, MAKE SURE TO BACKUP IF THERE ARE IMPORTANT CHANGES" );
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
    stdstr strIfName = stdstr( "I" ) +
        m_pIf->GetName() + "_CliImpl._ifName_";
    Wa( "#preparing the options" );
    Wa( "oOptions = cpp.CParamList()" );
    CCOUT << "oOptions.SetStrProp( cpp.propIfName, "
        << strIfName << " )";
    NEW_LINE;
    CCOUT << "oOptions.SetStrProp( cpp.propMethodName, \""
        << strName << "\" )";
    NEW_LINE;
    Wa( "oOptions.SetIntProp( cpp.propSeriProto, cpp.seriRidl )" );
    if( m_pNode->IsNoReply() )
        Wa( "oOptions.SetBoolProp( cpp.propNoReply, True )" );

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
        CCOUT << "oOptions.SetIntProp( cpp.propTimeoutSec, " <<
            dwTimeoutSec << " )";
        NEW_LINE;
        CCOUT << "oOptions.SetIntProp( cpp.propKeepAliveSec, " <<
            dwKeepAliveSec << " )";
        NEW_LINE;
    }
}

gint32 CImplPyMthdProxyBase::OutputSync( bool bSync )
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
            if( bSync )
                CCOUT << "def " << strName << "( self, "; 
            else
                CCOUT << "def " << strName << "( self, context, "; 
            INDENT_UPL;

            ret = EmitFormalArgListPy(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;

            NEW_LINE;
            Wa( ") -> Tuple[ int, list ] :" );

            NEW_LINE;
            if( dwOutCount > 0 )
                EmitRespList( m_pWriter, pOutArgs );
            Wa( "osb = CSerialBase( self )" );
            Wa( "buf = bytearray()" );
            Wa( "ret = 0" );
            CCOUT << "while True:";
            INDENT_UPL;

            bool bCheck = false;
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
                int ch = strSig[ 0 ];
                if( ch == '(' ||
                    ch == '[' || ch == 'O' )
                    bCheck = true;

                stdstr strArg = pfa->GetName();
                ret = EmitSerialBySig(
                    m_pWriter, strArg, strSig, true );
                if( ERROR( ret ) )
                    break;
            }
            if( ERROR( ret ) )
                break;

            Wa( "break" );
            INDENT_DOWNL;
            if( bCheck )
            {
                Wa( "if ret < 0 :" );
                Wa( "    return [ ret, None ]" );
            }

            Wa( "listArgs = [ buf, ]" );
        }
        else
        {
            if( bSync )
                CCOUT << "def " << strName << "( self )->Tuple[int,list] :";
            else
                CCOUT << "def " << strName << "( self, context )->Tuple[int,list] :";
            INDENT_UPL;
            NEW_LINE;
            if( dwOutCount > 0 )
                EmitRespList( m_pWriter, pOutArgs );
            Wa( "listArgs = []" );
        }

        EmitOptions();

        Wa( "listResp = [ None, None ]" );
        if( bSync && dwOutCount > 0 )
        {
            Wa( "ret = self.MakeCallWithOpt( " );
        }
        else if( bSync && dwOutCount == 0 )
        {

            Wa( "return self.MakeCallWithOpt( " );
        }
        else if( !bSync && dwOutCount > 0 )
        {
            Wa( "ret = self.MakeCallWithOptAsync( " );
            CCOUT << "    self." << strName << "CbWrapper, context,";
            NEW_LINE;
        }
        else // !bSync && dwOutCount == 0
        {
            Wa( "return self.MakeCallWithOptAsync( " );
            CCOUT << "    self." << strName << "CbWrapper, context,";
            NEW_LINE;
        }
        Wa( "    oOptions.GetCfg(), listArgs, listResp )" );
        if( !bSync && dwOutCount > 0 )
        {
            Wa( "if ret[ 0 ] == ErrorCode.STATUS_PENDING :" );
            Wa( "    return ret" );
        }
        if( dwOutCount == 0 )
        {
            INDENT_DOWNL;
            break;
        }
        Wa( "if ret[ 0 ] < 0 :" );
        Wa( "    return ret" );
        Wa( "buf = ret[ 1 ][ 0 ]" );
        Wa( "if buf is None or not isinstance( buf, bytearray ) : " );
        Wa( "    return [ -errno.EBADMSG, None ] " );
        NEW_LINE;

        Wa( "rmtret = ret" );
        Wa( "offset = 0" );
        if( dwInCount == 0 )
            Wa( "osb = CSerialBase( self )" );

        Wa( "listRet = []" );
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

            Wa( "if ret[ 0 ] is None :" );
            Wa( "    return [ -errno.EBADMSG, None ]" );
            Wa( "listRet.append( ret[ 0 ] )" );
            Wa( "offset = ret[ 1 ]" );
            NEW_LINE;
        }

        Wa( "if len( rmtret ) == 3:" );
        Wa( "    return [ 0, listRet, rmtret ]" );
        Wa( "return [ 0, listRet ]" );
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

        if( dwOutCount > 0 )
            CCOUT << "def " << strName
                << "CbWrapper( self, context, ret, ridlBuf ) :"; 
        else
            CCOUT << "def " << strName
                << "CbWrapper( self, context, ret ) :"; 
        INDENT_UPL;

        Wa( "targetMethod = self.GetMethod(" );
        CCOUT << "    \"" << strIfName << "\", \"" << strName << "Cb\" )";
        NEW_LINE;
        Wa( "if targetMethod is None :" );
        Wa( "    return" );

        Wa( "listResp = [ context, ret ]" );
        if( dwOutCount == 0 )
        {
            Wa( "self.InvokeCallback( targetMethod, listResp )" );
            Wa( "return" );
            INDENT_DOWNL;
            break;
        }
        NEW_LINE;
        Wa( "if ret < 0:" );
        if( dwOutCount > 1 )
            CCOUT << "    listResp.extend( [None] * " << dwOutCount << ")";
        else
            CCOUT << "    listResp.extend( [None] )";
        NEW_LINE;
        Wa( "    self.InvokeCallback( targetMethod, listResp )" );
        Wa( "    return" );

        NEW_LINE;
        Wa( "if ridlBuf is None or len( ridlBuf ) == 0 :" );
        Wa( "    return" );
        Wa( "buf = ridlBuf" );
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

            Wa( "if ret[ 0 ] is None :" );
            Wa( "    return" );
            Wa( "listResp.append( ret[ 0 ] )" );
            Wa( "offset = ret[ 1 ]" );
            NEW_LINE;
        }

        Wa( "self.InvokeCallback( targetMethod, listResp )" );
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
        gint32 ret = OutputSync( false );
        if( ERROR( ret ) )
            return ret;
        return OutputAsyncCbWrapper();
    }
    return OutputSync( true );
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
    EMIT_DISCLAIMER_PY; \
    CCOUT << "# " << g_strCmdLine;\
    NEW_LINE;\
    Wa( "from typing import Tuple" );\
    Wa( "from rpcf.rpcbase import *" ); \
    Wa( "from rpcf.proxy import *" );\
    Wa( "from seribase import CSerialBase" );\
    Wa( "from seribase import Variant" );\
    CCOUT << "from " << g_strAppName << "structs import *"; \
    NEW_LINE; \
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

static gint32 GenSvcFiles(
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
            ret = pWriter->SelectImplFile(
                strCommon + "svr.py" );
            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    strCommon + "svr.py.new" );
                if( ERROR( ret ) )
                    break;
            }

            CImplPySvcSvr opss(
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

            CImplPySvcProxy opsc(
                pWriter, elem.second );
            ret = opsc.Output();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

gint32 SyncCfg( const stdstr& strPath )
{
    sync();
    stdstr strCmd = "make -sC ";
    strCmd += strPath;
    gint32 ret = system( strCmd.c_str() );
    if( ret < 0 )
        ret = -errno;
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

        ret = GenStructsFilePy( &oWriter, pRoot );
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
        if( !g_bRpcOverStm )
        {
            CExportObjDesc oedesc( &oWriter, pRoot );
            ret = oedesc.Output();
        }
        else
        {
            CExportObjDesc2 oedesc( &oWriter, pRoot );
            ret = oedesc.OutputROS();
        }
        if( ERROR( ret ) )
            break;

        oWriter.SelectMakefile();
        CPyExportMakefile opemk( &oWriter, pRoot );
        ret = opemk.Output();
        if( ERROR( ret ) )
            break;

        ret = GenSvcFiles( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

        CImplPyMainFunc opmf( &oWriter, pRoot );
        ret = opmf.Output();
        if( ERROR( ret ) )
            break;

        oWriter.SelectReadme();
        CExportPyReadme ordme( &oWriter, pRoot );
        ret = ordme.Output();

    }while( 0 );

    if( SUCCEEDED( ret ) )
        SyncCfg( strOutPath );

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
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );
        ObjPtr pOutArgs = m_pNode->GetOutArgs();
        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        CCOUT << "def " << strName << "Wrapper( self,";
        NEW_LINE;
        if( dwInCount > 0 )
            CCOUT << "    callback : cpp.ObjPtr, buf : bytearray";
        else
            CCOUT << "    callback : cpp.ObjPtr";
        NEW_LINE;
        CCOUT << "    ) -> Tuple[ int, list ] :";
        INDENT_UPL;
        NEW_LINE;

        Wa( "oReqCtx_ = PyReqContext()" );
        Wa( "oReqCtx_.oCallback = callback" );
        if( dwInCount == 0 )
        {
            if( dwOutCount > 0 )
                Wa( "osb = CSerialBase( self )" );
            Wa( "targetMethod = self.GetMethod(" );
            CCOUT << "    \"" << strIfName << "\", \"" << strName << "\" )";
            NEW_LINE;
            Wa( "if targetMethod is None :" );
            Wa( "    return [ -errno.EFAULT, None ]" );
            Wa( "ret = self.InvokeCallback( targetMethod, [ oReqCtx_, ] )" );
        }
        else
        {
            Wa( "offset = 0" );
            Wa( "listArgs = [ oReqCtx_, ]" );
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
                Wa( "    return [ -errno.EBADMSG, None ]" );
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
            Wa( "    targetMethod, listArgs )" );
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
            Wa( "    self.oInst.SetInvTimeout(" );

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
                CCOUT << "        callback, " << dwTimeoutSec
                    << ", " << dwKeepAliveSec << " )";
            }
            else
            {
                CCOUT << "        callback, 0, 0 )";
            }

            NEW_LINE;
            Wa( "    self.InstallCancelNotify( callback," );
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
        CCOUT << "while True:";
        INDENT_UPL;
        CArgList* poutal = pOutArgs;
        bool bCheck = false;
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

            int ch = strSig[ 0 ];
            if( ch == '(' ||
                ch == '[' || ch == 'O' )
                bCheck = true;

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
        if( bCheck )
        {
            Wa( "if ret < 0 :" );
            Wa( "    return [ ret, None ]" );
        }
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
            Wa( "    oReqCtx_ : PyReqContext, iRet : int ):" );
            INDENT_UPL;
            Wa( "self.RidlOnServiceComplete(" );
            Wa( "    oReqCtx_.oCallback, iRet, None )" );
            Wa( "return" );
            INDENT_DOWNL;
            break;
        }

        CCOUT << "def On" << strName << "Complete"
            << "( self, oReqCtx_ : PyReqContext, iRet : int,";
        INDENT_UPL;

        ret = EmitFormalArgListPy( m_pWriter, pOutArgs );
        if( ERROR( ret ) )
            break;

        CCOUT << "):";
        NEW_LINE;
        Wa( "if iRet < 0:" );
        Wa( "    self.RidlOnServiceComplete(" );
        Wa( "    oReqCtx_.oCallback, iRet, None )" );
        Wa( "    return" );
        Wa( "osb = CSerialBase( self )" );
        Wa( "buf = bytearray()" );
        Wa( "ret = 0" );
        CCOUT << "while True:";
        INDENT_UPL;
        guint32 i = 0;
        bool bCheck = false;
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

            int ch = strSig[ 0 ];
            if( ch == '(' ||
                ch == '[' || ch == 'O' )
                bCheck = true;

            ret = EmitSerialBySig(
                m_pWriter, strArg, strSig, true );
            if( ERROR( ret ) )
                break;
        }
        Wa( "break" );
        INDENT_DOWNL;
        if( bCheck )
        {
            Wa( "if ret < 0 :" );
            Wa( "    buf = None" );
        }
        Wa( "self.RidlOnServiceComplete(" );
        Wa( "    oReqCtx_.oCallback, iRet, buf )" );
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
        stdstr strName = m_pNode->GetName();
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );

        if( dwInCount == 0 )
        {
            CCOUT << "def On" << strName << "CanceledWrapper( self,";
            NEW_LINE;
            CCOUT << "    iRet : int, callback : PyReqContext ):";
            NEW_LINE;
            CCOUT << "    self.On" << strName << "Canceled( self,";
            NEW_LINE;
            CCOUT << "        callback, iRet )";
            NEW_LINE;
            break;
        }

        CCOUT << "def On" << strName << "CanceledWrapper( self,";
        NEW_LINE;
        CCOUT << "    iRet : int, callback : PyReqContext,";
        INDENT_UPL;

        ret = EmitFormalArgListPy( m_pWriter, pInArgs );
        if( ERROR( ret ) )
            break;

        CCOUT << "):";
        NEW_LINE;
        CCOUT << "self.On" << strName << "Canceled(";
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
        stdstr strName = m_pNode->GetName();
        stdstr strIfName = stdstr( "I" ) +
            m_pIf->GetName() + "_SvrImpl._ifName_";
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );

        if( dwInCount == 0 )
        {
            CCOUT << "def " << strName
                << "( self, callback : object ): ";
            INDENT_UPL;
            Wa( "ret = self.RidlSendEvent( callback, " );
            CCOUT << strIfName << ", \"" 
                << strName << "\", \"\", None )";
            break;
        }

        CCOUT << "def "<< strName << "( self,";
        NEW_LINE;
        CCOUT << "    callback : object,";
        INDENT_UPL;

        ret = EmitFormalArgListPy( m_pWriter, pInArgs );
        if( ERROR( ret ) )
            break;

        CCOUT << "):";
        NEW_LINE;
        Wa( "osb = CSerialBase( self )" );
        Wa( "buf = bytearray()" );
        CCOUT << "while True:";
        INDENT_UPL;
        guint32 i = 0;
        CArgList* pArgList = pInArgs;
        bool bCheck = false;
        for( ; i < dwInCount; i++ )
        {
            CFormalArg* pfa =
                pArgList->GetChild( i );

            stdstr strArg = pfa->GetName();
            CAstNodeBase* pType = pfa->GetType();

            stdstr strSig =
                pType->GetSignature();

            int ch = strSig[ 0 ];
            if( ch == '(' ||
                ch == '[' || ch == 'O' )
                bCheck = true;

            ret = EmitSerialBySig(
                m_pWriter, strArg, strSig, true );

            if( ERROR( ret ) )
                break;
        }
        Wa( "break" );
        INDENT_DOWNL;
        if( bCheck )
        {
            Wa( "if ret < 0 :" );
            Wa( "    return ret" );
        }
        Wa( "ret = self.RidlSendEvent( callback, " );
        CCOUT << "    " << strIfName << ",";
        NEW_LINE;
        CCOUT << "    \"" << strName << "\", \"\", buf )";
        NEW_LINE;
        Wa( "return ret" );
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
    else if( m_pNode->IsAsyncs() )
    {
        gint32 ret = 0;
        ret = OutputAsyncWrapper();
        if( ERROR( ret ) )
            return ret;

        ret = OutputAsyncCompHandler();
        if( ERROR( ret ) )
            return ret;

        return  OutputAsyncCHWrapper();
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
            CCOUT << "def " << strName << "( self, oReqCtx : PyReqContext,"; 
            INDENT_UPL;

            ret = EmitFormalArgListPy(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }
        else
        {
            CCOUT << "def " << strName << "( self, oReqCtx : PyReqContext"; 
            INDENT_UPL;
        }

        if( dwOutCount > 0 )
        {
            Wa( ") -> Tuple[ int, list ] :" );
            EmitRespList( m_pWriter, pOutArgs );
            Wa( "#Implement this method here" );
        }
        else
            Wa( ") -> Tuple[ int, None ] :" );

        Wa( "return [ ErrorCode.ERROR_NOT_IMPL, None ]" );
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
        stdstr strName = m_pNode->GetName();
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );

        if( dwInCount == 0 )
        {
            CCOUT << "def On" << strName << "Canceled( self,";
            NEW_LINE;
            CCOUT << "    oReqCtx : PyReqContext, iRet : int ): ";
            NEW_LINE;
            Wa( "    pass" );
            break;
        }

        CCOUT << "def On" << strName << "Canceled( self,";
        NEW_LINE;
        CCOUT << "    oReqCtx : PyReqContext, iRet : int,";
        INDENT_UPL;

        ret = EmitFormalArgListPy( m_pWriter, pInArgs );
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
        NEW_LINE;
        Wa( "def __init__( self, pIoMgr, strDesc, strObjName ) :" );
        Wa( "    PyRpcServer.__init__( self," );
        CCOUT << "        pIoMgr, strDesc, strObjName )";
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
            CCOUT << "    context : object, ret : int, "; 
            INDENT_UPL;

            ret = EmitFormalArgListPy(
                m_pWriter, pOutArgs );
            if( ERROR( ret ) )
                break;
            Wa( " ) :" );
        }
        else
        {
            CCOUT << "def " << strName << "Cb( self,";
            NEW_LINE;
            CCOUT << "    context : object, ret : int ):"; 
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
            CCOUT << "def " << strName << "( self,"; 
            INDENT_UPL;

            ret = EmitFormalArgListPy(
                m_pWriter, pInArgs );
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }
        else
        {
            CCOUT << "def " << strName << "( self ): "; 
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
            if( !pmd->IsAsyncp() &&
                !pmd->IsEvent() )
                continue;

            bHasMethod = true;
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
        NEW_LINE;
        Wa( "def __init__( self, pIoMgr, strDesc, strObjName ) :" );
        Wa( "    PyRpcProxy.__init__( self," );
        CCOUT << "        pIoMgr, strDesc, strObjName )";
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

CImplPyMainFunc::CImplPyMainFunc(
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

gint32 CImplPyMainFunc::EmitUsage( bool bProxy )
{
    gint32 ret = 0;
    do{
        CCOUT << "def Usage():";
        INDENT_UPL;
        CCOUT << "print( ( \"Usage: \%s [OPTION]\\n\" +";
        INDENT_UPL;
        Wa( "\"\\t [ -a to enable authentication ]\\n\" +" );
        Wa( "\"\\t [ -d to run as a daemon ]\\n\" +" );
        if( !bProxy )
            Wa( "\"\\t [ -g to enable logging ]\\n\" +" );
#ifdef FUSE3
        if( !bProxy && g_bBuiltinRt )
            Wa( "\"\\t [ -m to specify a directory as the mountpoint for rpcfs ]\\n\" +" );
#endif
        if( g_bBuiltinRt )
        {
            if( bProxy )
            {
                Wa( "\"\\t [ -i <ip address> to specify the destination ip address. ]\\n\"" );
                Wa( "\"\\t [ -p <port number> to specify the tcp port number to ]\\n\"" );
#ifdef KRB5
                Wa( "\"\\t [ -k to run as a kinit proxy. ]\\n\"" );
                Wa( "\"\\t [ -l <user name> login with the user name and then quit. ]\\n\"" );
#endif
            }
            else
            {
                Wa( "\"\\t [ -i <ip address> to specify the ip address to bind ]\\n\"" );
                Wa( "\"\\t [ -p <port number> to specify the tcp port number to listen on ]\\n\"" );
            }
            Wa( "\"\\t [ --driver <path> to specify the path to the customized 'driver.json'. ]\\n\"" );
            Wa( "\"\\t [ --objdesc <path> to specify the path to the object description file. ]\\n\"" );
            Wa( "\"\\t [ --router <path> to specify the path to the customized 'router.json'. ]\\n\"" );
            if( bProxy )
            {
                Wa( "\"\\t [ --instname <name> to specify the server instance name to connect'. ]\\n\"" );
                Wa( "\"\\t [ --sainstname <name> to specify the stand-alone router instance name to connect'. ]\\n\"" );
                Wa( "\"\\t [ --nodbus to start without dbus connection, suitable for non-dbus environment'. ]\\n\"" );
            }
            else
            {
                Wa( "\"\\t [ --instname <name> to specify the string as the name of this server instance'. ]\\n\"" );
            }
        }
        Wa( "\"\\t [ -h this help ]\\n\" ) \% ( sys.argv[ 0 ] )," );
        CCOUT << "file=sys.stderr )";
        INDENT_DOWN;
        INDENT_DOWNL;
    }while( 0 );
    return ret;
}

gint32 CImplPyMainFunc::EmitGetOpt( bool bProxy )
{
    gint32 ret = 0;
    do{
        Wa( "params=dict()" );
        Wa( "argv = sys.argv[1:]" );
        Wa( "try:" );
        stdstr strShort = "hadi:p:";
        if( !bProxy )
            strShort = "hadgi:p:";
#ifdef FUSE3
        if( !bProxy && g_bBuiltinRt )
            strShort = "hadgi:p:m:";
#endif
#ifdef KRB5
        if( bProxy && g_bBuiltinRt )
            strShort += "kl:";
#endif
        CCOUT << "    opts, args = getopt.getopt(argv, \""<< strShort <<"\","; 
        NEW_LINE;
        if( bProxy && g_bBuiltinRt )
            CCOUT << "        [ \"driver=\", \"objdesc=\", \"router=\", \"instname=\", \"sainstname=\", \"nodbus\" ] )";
        else if( !bProxy && g_bBuiltinRt )
            CCOUT << "        [ \"driver=\", \"objdesc=\", \"router=\", \"instname=\" ] )";
        NEW_LINE;

        Wa( "except:" );
        Wa( "    Usage()" );
        Wa( "    return -errno.EINVAL" );
#ifdef FUSE3
        if( !bProxy && g_bBuiltinRt )
            Wa( "bMount = False" );
#endif
        Wa( "for opt, arg in opts:" );
        Wa( "    if opt in ('-a'):" );
        Wa( "        params[ 'bAuth' ] = True" );
        if( !bProxy )
        {
            Wa( "    elif opt in ('-g'):" );
            Wa( "        params[ 'logging' ] = True" );
        }

#ifdef KRB5
        if( bProxy && g_bBuiltinRt )
        {
            Wa( "    elif opt in ('-k'):" );
            Wa( "        params[ 'bKProxy' ] = True" );
            Wa( "    elif opt in ('-l'):" );
            Wa( "        params[ 'bKProxy' ] = True" );
            Wa( "        params[ 'UserName' ] = arg" );
        }
#endif
        Wa( "    elif opt in ('-d'):" );
        Wa( "        params[ 'bDaemon' ] = True" );
        Wa( "    elif opt in ('-i'):" );
        Wa( "        params[ 'ipaddr' ] = arg" );
        Wa( "    elif opt in ('-p'):" );
        Wa( "        params[ 'portnum' ] = arg" );
#ifdef FUSE3
        if( !bProxy && g_bBuiltinRt )
        {
            Wa( "    elif opt in ('-m'):" );
            Wa( "        params[ 'MountPoint' ] = arg" );
            Wa( "        bMount = True" );
        }
#endif
        CCOUT << "    elif opt in ('--driver', '--router', '--objdesc' ):";
        INDENT_UP;
        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
        Wa( "stinfo = os.stat( arg )" );
        Wa( "if not stat.S_ISREG( stinfo.st_mode ) and not stat.S_ISLNK( stinfo.st_mode):" );
        CCOUT << "    raise Exception( \"Error invalid file '%s'\" \% arg )";
        INDENT_DOWNL;
        Wa( "except Exception as err:" );
        Wa( "    print( err )" );
        Wa( "    Usage()" );
        Wa( "    sys.exit( 1 )" );
        Wa( "if opt == '--driver':" );
        Wa( "    params[ 'driver' ] = arg" );
        Wa( "elif opt == '--router':" );
        Wa( "    params[ 'router' ] = arg" );
        Wa( "else:" );
        CCOUT << "    params[ 'objdesc' ] = arg";
        INDENT_DOWN;
        INDENT_DOWNL;
        Wa( "    elif opt == '--instname':" );
        Wa( "        params[ 'instname' ] = arg" );
        Wa( "        if 'sainstname' in params:" );
        Wa( "            print( 'Error specifying both instname and sainstname')" );
        Wa( "            Usage()" );
        Wa( "            sys.exit( 1 )" );
        Wa( "    elif opt == '--sainstname':" );
        Wa( "        params[ 'sainstname' ] = arg" );
        Wa( "        if 'instname' in params:" );
        Wa( "            print( 'Error specifying both instname and sainstname')" );
        Wa( "            Usage()" );
        Wa( "            sys.exit( 1 )" );
        if( bProxy && g_bBuiltinRt )
        {
            Wa( "    elif opt == '--nodbus':" );
            Wa( "        params[ 'nodbus' ] = True" );
        }
        Wa( "    elif opt in ('-h'):" );
        Wa( "        Usage()" );
        Wa( "        sys.exit( 0 )" );
        Wa( "    else:" );
        Wa( "        Usage()" );
        Wa( "        sys.exit( 1 )" );
#ifdef KRB5
        if( bProxy && g_bBuiltinRt )
        {
            Wa( "if 'bKProxy' in params and not 'bAuth' in params:" );
            Wa( "    print( \"Error '-k' or '-l' is only valid when '-a' is present \")" );
            Wa( "    Usage()" );
            Wa( "    sys.exit( 1 )" );
        }
#endif

    }while( 0 );

    return ret;
}


gint32 CImplPyMainFunc::EmitProxySampleCode(
    std::vector< ObjPtr >& vecSvcs )
{
    gint32 ret = 0;
    do{
        Wa( "'''" );
        Wa( "adding your code here" );

        std::vector< ObjPtr > vecIfs;
        CServiceDecl* pSvc = vecSvcs[ 0 ];
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

            bool bHasEvent = false;
            CMethodDecl* pmd = nullptr;
            for( auto& elem : vecMethods )
            {
                pmd = vecMethods.front();
                if( pmd->IsEvent() )
                {
                    pmd = nullptr;
                    bHasEvent = true;
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
                    ret = GetArgsAndSigs( pInArgs, vecArgs );
                    if( ERROR( ret ) )
                        break;

                    Wa( "Calling a proxy method like" );
                    CCOUT << "'oProxy." << strMName << "(";
                    if( vecArgs.size() > 2 )
                    {
                        NEW_LINE;
                        CCOUT << "    ";
                    }
                    else
                    {
                        CCOUT << " ";
                    }
                    for( int i = 0; i < vecArgs.size(); i++ )
                    {
                        auto& elem = vecArgs[ i ];
                        CCOUT << elem.first;
                        if( i < vecArgs.size() - 1 )
                            CCOUT << ", ";
                        else
                            CCOUT << " )'";
                    }
                }
            }
            else if( bHasEvent )
            {
                Wa( "Just waiting and events will " );
                Wa( "be handled in the background" );
                NEW_LINE;
                Wa( "import time" );
                Wa( "while( oProxy.oInst.GetState() == cpp.stateConnected ):" );
                CCOUT << "    time.sleep(1)";
            }
            NEW_LINE;

        }while( 0 );

        Wa( "'''" );
    }while( 0 );
    return ret;
}


#define EMIT_CALL_TO_USERMAIN( bProxy ) \
do{ \
    stdstr strSuffix; \
    if( bProxy )\
        strSuffix = "cli";\
    else\
        strSuffix = "svr";\
    CCOUT << "ret = main" << strSuffix << "(";\
    stdstr strPrefix;\
    if( bProxy ) \
        strPrefix = "oProxy"; \
    else \
        strPrefix = "oServer"; \
    if( vecSvcs.size() == 1 ) \
    { \
        CCOUT << " " << strPrefix << " )"; \
    } \
    else for( int i = 0; i < vecSvcs.size(); i++ ) \
    { \
        if( i > 0 ) \
            CCOUT << "    " << strPrefix \
                << std::to_string( i ); \
        else  \
            CCOUT << " " << strPrefix; \
        if( i < vecSvcs.size() - 1 ) \
        { \
            CCOUT << ","; \
            NEW_LINE; \
        } \
        else \
        { \
            CCOUT << " )"; \
        } \
    } \
}while( 0 )

#define EMIT_SIGHANDLER \
do{ \
    Wa( "import signal" ); \
    Wa( "bExit = False" ); \
    Wa( "def SigHandler( signum, frame ):" ); \
    Wa( "    global bExit" ); \
    Wa( "    bExit = True" ); \
    NEW_LINE; \
}while( 0 )

gint32 CImplPyMainFunc::EmitKProxyLoop()
{
    if( g_bBuiltinRt )
    {
        CCOUT << "def KProxyLoop( strUserName : str = None ) -> int:";
        INDENT_UPL;
        Wa( "global bExit" );
        Wa( "if strUserName is None:" );
        Wa( "    while not bExit:" );
        Wa( "        time.sleep( 3 )" );
        Wa( "    print( \"\\n\" )" );
        CCOUT << "    return 0";
        NEW_LINE;
        Wa( "strCmd = \"kinit -ki \" + strUserName" );
        CCOUT << "return -os.system( strCmd )";
        INDENT_DOWNL;
    }
    return 0;
}
gint32 CImplPyMainFunc::OutputCli(
    std::vector< ObjPtr >& vecSvcs )
{
    gint32 ret = 0;
    do{
        if( vecSvcs.empty() )
        {
            ret = -EINVAL;
            break;
        }
        Wa("import os" );
        Wa("import time" );
        if( g_bBuiltinRt )
        {
            Wa( "import sys" );
            Wa( "import getopt" );
            Wa( "import stat" );
            NEW_LINE;
            EmitUsage( true );
        }
        NEW_LINE;
        EMIT_SIGHANDLER;

        EmitKProxyLoop();
        NEW_LINE;

        CCOUT << "def MainEntryCli() :";
        INDENT_UPL;
        Wa( "ret = 0" );
        Wa( "signal.signal( signal.SIGINT, SigHandler)" );
        stdstr strModName = g_strAppName + "cli";
        if( g_bRpcOverStm && !g_bBuiltinRt )
        {
            CCOUT << "oContext = PyRpcContext( '" << strModName << "' )";
            NEW_LINE;
        }
        else if( g_bBuiltinRt )
        {
            CCOUT << "try:";
            INDENT_UPL;
            ret = EmitGetOpt( true );
            if( ERROR( ret ) )
                break;
            CCOUT << "params[ '" << JSON_ATTR_MODNAME
                << "' ] = \"" << strModName << "\"";
            NEW_LINE;
            CCOUT << "params[ 'AppName' ] = \""
                << g_strAppName << "\"";
            NEW_LINE;
            CCOUT << "params[ '"
                << JSON_ATTR_ROUTER_ROLE << "' ] = 1";
            NEW_LINE;
            Wa( "if 'objdesc' in params:" );
            Wa( "    strPath_ = params[ 'objdesc' ]" );
            Wa( "else:" );
            Wa( "    strPath_ = os.path.dirname( os.path.realpath( __file__ ) )" );
            CCOUT << "    strPath_ += '/" << g_strAppName << "desc.json'";
            NEW_LINE;
            Wa( "ret, strNewDesc = \\" );
            Wa( "    UpdateObjDesc( strPath_, params )" );
            Wa( "if ret < 0:" );
            Wa( "    print( 'Error UpdateObjDesc', ret )" );
            Wa( "    return ret" );
            Wa( "if isinstance( strNewDesc, str ) and len( strNewDesc ):" );
            Wa( "    strPath_ = strNewDesc" );
            Wa( "    params[ 'objdesc' ] = strNewDesc" );
            Wa( "oContext = PyRpcContext( params )" );
        }
        else
        {
            Wa( "oContext = PyRpcContext( 'PyRpcProxy' )" );
        }
        CCOUT << "with oContext as ctx:";
        INDENT_UPL;
        CCOUT << "if ctx.status < 0:";
        INDENT_UPL;
        Wa( "ret = ctx.status" );
        Wa( "print( os.getpid(), " );
        Wa( "    \"Error start PyRpcContext %d\" % ret )" );
        CCOUT << "return ret";
        INDENT_DOWNL;
        NEW_LINE;

        Wa( "print( \"start to work here...\" )" );
        if( !g_bBuiltinRt )
        {
            Wa( "strPath_ = os.path.dirname( os.path.realpath( __file__) )" );
            CCOUT << "strPath_ += '/" << g_strAppName << "desc.json'";
            NEW_LINE;
        }
        else
        {
#ifdef KRB5
            Wa( "if 'bKProxy' in params and params[ 'bKProxy' ]:" );
            Wa( "    if 'UserName' in params:" );
            Wa( "        strUser = params[ 'UserName' ]" );
            Wa( "    else:" );
            Wa( "        strUser = None" );
            Wa( "    return KProxyLoop( strUser )" );
#endif
        }

        for( auto elem : vecSvcs )
        {
            CServiceDecl* pSvc = elem;
            stdstr strName = pSvc->GetName();
            stdstr strVar = "oProxy_";
            strVar += strName;

            CCOUT << strVar << " = C" << strName << "Proxy( ctx.pIoMgr,";
            NEW_LINE;
            CCOUT << "    strPath_, '" << strName << "' )" ;
            NEW_LINE;
            CCOUT << "ret = "<< strVar << ".GetError()";
            NEW_LINE;
            Wa( "if ret < 0 :" );
            Wa( "    return ret" );
            NEW_LINE;
        }
        if( vecSvcs.size() == 1 )
        {
            CServiceDecl* pSvc = vecSvcs[ 0 ];
            stdstr strName = pSvc->GetName();
            stdstr strVar = "oProxy_";
            strVar += strName;
            CCOUT << "with " << strVar << " as oProxy:";
        }
        else
        {  
            CCOUT << "with(";
            INDENT_UPL
            for( int i = 0; i < vecSvcs.size(); i++ )
            {
                CServiceDecl* pSvc = vecSvcs[ i ];
                stdstr strName = pSvc->GetName();
                stdstr strVar = "oProxy_";
                strVar += strName;
                CCOUT << strVar << " as oProxy";
                if( i > 0 )
                    CCOUT << i;
                if( i < vecSvcs.size() - 1 )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }
                else
                {
                    NEW_LINE;
                    CCOUT << "):";
                }
            }
            INDENT_DOWN;
        }

        INDENT_UPL;
        Wa( "global bExit" );
        CCOUT << "try:";
        INDENT_UPL;
        if( vecSvcs.size() == 1 )
        {
            Wa( "ret = oProxy.GetError()" );
            Wa( "if ret < 0 :" );
            Wa( "    raise Exception( 'start proxy failed' )");
            Wa( "state = oProxy.oInst.GetState()" );
            Wa( "while state == cpp.stateRecovery :" );
            Wa( "    time.sleep( 1 )" );
            Wa( "    state = oProxy.oInst.GetState()" );
            Wa( "    if bExit:" );
            Wa( "        break" );
            Wa( "if state != cpp.stateConnected or bExit:" );
            Wa( "    return ErrorCode.ERROR_STATE" );
        }
        else
        {
            for( int i = 0; i < vecSvcs.size(); i++ )
            {
                stdstr strVar = "oProxy";
                if( i > 0 )
                    strVar += std::to_string( i );
                CCOUT << "ret = " << strVar << ".GetError()";
                NEW_LINE;
                Wa( "if ret < 0 :" );
                CCOUT << "    raise Exception( 'start " << strVar <<" failed' )";
                NEW_LINE;
                CCOUT << "state = " << strVar << ".oInst.GetState()";
                NEW_LINE;
                Wa( "while state == cpp.stateRecovery :" );
                Wa( "    time.sleep( 1 )" );
                CCOUT << "    state = "<< strVar << ".oInst.GetState()";
                NEW_LINE;
                Wa( "    if bExit:" );
                Wa( "        break" );
                Wa( "if state != cpp.stateConnected or bExit:" );
                Wa( "    return ErrorCode.ERROR_STATE" );
                if( i < vecSvcs.size() - 1 )
                    NEW_LINE;
            }
        }
        EMIT_CALL_TO_USERMAIN( true );
        INDENT_DOWNL;
        Wa( "except Exception as err:" );
        Wa( "    print( err )" );
        INDENT_DOWNL;
        if( vecSvcs.size() == 1 )
        {
            CServiceDecl* pSvc = vecSvcs[ 0 ];
            stdstr strName = pSvc->GetName();
            CCOUT << "oProxy_" << strName << " = None";
        }
        else
        {
            for( int i = 0; i < vecSvcs.size(); i++ )
            {
                CServiceDecl* pSvc = vecSvcs[ i ];
                stdstr strName = pSvc->GetName();
                stdstr strVar = "oProxy_";
                strVar += strName;
                if( i > 0 )
                    strVar += std::to_string( i );
                CCOUT << strVar << " = None";
                if( i < vecSvcs.size() - 1 )
                    NEW_LINE;
            }
        }
        INDENT_DOWNL;
        Wa( "oContext = None" );
        if( g_bBuiltinRt )
        {
            INDENT_DOWNL;
            CCOUT << "finally:";
            INDENT_UPL;
            CCOUT << "if 'objdesc' in params:";
            INDENT_UPL;
            Wa( "strDesc = params[ 'objdesc' ]" );
            Wa( "if strDesc[:12] == '/tmp/rpcfod_':" );
            CCOUT << "    os.unlink( strDesc )";
            INDENT_DOWN;
            INDENT_DOWNL;
        }
        Wa( "return ret" );
        INDENT_DOWNL;
        EmitDefineUserMain( vecSvcs, true );
        Wa( "if __name__ == '__main__' :" );
        Wa( "    ret = MainEntryCli()" );
        Wa( "    quit( -ret )" );


    }while( 0 );

    return ret;
}

gint32 CImplPyMainFunc::EmitDefineUserMain( 
    std::vector< ObjPtr >& vecSvcs,
    bool bProxy )
{
    gint32 ret = 0;
    do{
        stdstr strPrefix, strSuffix;
        Wa( "#------customize the method below for your own purpose----" ); \
        if( bProxy )
        {
            CCOUT << "def maincli( ";
            strPrefix = "oProxy";
            strSuffix = "Proxy";
        }
        else
        {
            CCOUT << "def mainsvr( ";
            strPrefix = "oServer";
            strSuffix = "Server";
        }
        INDENT_UPL;
        if( vecSvcs.size() == 1 )
        {
            CServiceDecl* pSvc = vecSvcs[ 0 ];
            stdstr strSvcName = pSvc->GetName();
            CCOUT << strPrefix << ": C" 
                << strSvcName << strSuffix
                << " ) -> int:";
        }
        else for( int i = 0; i < vecSvcs.size(); i++ )
        {
            CServiceDecl* pSvc = vecSvcs[ i ];
            stdstr strSvcName = pSvc->GetName();
            stdstr strVar = strPrefix;
            if( i > 0 )
                strVar += std::to_string( i );
            CCOUT << strVar << ": C" 
                << strSvcName << strSuffix;

            if( i < vecSvcs.size() - 1 )
            {
                CCOUT << ",";
                NEW_LINE;
            }
            else
            {
                CCOUT << " ) -> int:";
            }
        }
        NEW_LINE;
        if( bProxy )
            EmitProxySampleCode( vecSvcs );
        else
            EmitMainloopServer( vecSvcs );
        Wa( "return 0" );
        INDENT_DOWNL;

    }while( 0 );

    return ret;

}

gint32 CImplPyMainFunc::EmitMainloopServer( 
    std::vector< ObjPtr >& vecSvcs )
{
    do{
        Wa( "'''" );
        Wa( "make change to the following code" );
        Wa( "snippet for your own purpose" );
        Wa( "'''" );
        Wa( "global bExit" );
        if( vecSvcs.size() == 1 )
        {
            Wa( "while ( cpp.stateConnected ==" );
            Wa( "    oServer.oInst.GetState() ):" );
        }
        else
        {
            CCOUT << "while(";
            INDENT_UPL;
            for( int i = 0; i < vecSvcs.size(); ++i )
            {
                stdstr strVar = "oServer";
                if( i > 0 )
                    strVar += std::to_string( i );
                CCOUT << "cpp.stateConnected == "
                    << strVar << ".oInst.GetState()";
                if( i < vecSvcs.size() - 1 )
                {
                    CCOUT << " and";
                    NEW_LINE;
                }
                else
                {
                    NEW_LINE;
                    CCOUT << "):";
                    INDENT_DOWNL;
                }
            }
        }
        Wa( "    time.sleep( 1 )" );
        Wa( "    if bExit:" );
        CCOUT << "        break";
        NEW_LINE;
    }while( 0 );
    return 0;
}

gint32 CImplPyMainFunc::OutputSvr(
    std::vector< ObjPtr >& vecSvcs )
{
    gint32 ret = 0;
    do{
        if( vecSvcs.empty() )
        {
            ret = -EINVAL;
            break;
        }
        Wa("import os" );
        CCOUT << "import time";
        NEW_LINE;
        if( g_bBuiltinRt )
        {
            Wa( "import sys" );
            Wa( "import getopt" );
            Wa( "import stat" );
            NEW_LINE;
            EmitUsage( false );
        }
        NEW_LINE;

        EMIT_SIGHANDLER;

        CCOUT << "def MainEntrySvr() :";
        INDENT_UPL;
        Wa( "ret = 0" );
        Wa( "signal.signal( signal.SIGINT, SigHandler)" );
        stdstr strModName = g_strAppName + "svr";
        if( g_bRpcOverStm && !g_bBuiltinRt )
        {
            CCOUT << "oContext = PyRpcContext( '" << strModName << "' )";
            NEW_LINE;
        }
        else if( g_bBuiltinRt )
        {
            CCOUT << "try:";
            INDENT_UPL;
            ret = EmitGetOpt( false );
            if( ERROR( ret ) )
                break;
            CCOUT << "params[ '" << JSON_ATTR_MODNAME
                << "' ] = \"" << strModName << "\"";
            NEW_LINE;
            CCOUT << "params[ 'AppName' ] = \""
                << g_strAppName << "\"";
            NEW_LINE;
            CCOUT << "params[ '"
                << JSON_ATTR_ROUTER_ROLE << "' ] = 2";
            NEW_LINE;
            Wa( "if 'driver' in params:" );
            Wa( "    strCfg = params[ 'driver' ]" );
            Wa( "else:" );
            Wa( "    strCfg = './driver.json'" );
            Wa( "if 'objdesc' in params:" );
            Wa( "    strPath_ = params[ 'objdesc' ]" );
            Wa( "else:" );
            Wa( "    strPath_ = os.path.dirname( os.path.realpath( __file__ ) )" );
            CCOUT << "    strPath_ += '/" << g_strAppName << "desc.json'";
            NEW_LINE;
            Wa( "ret, strNewDesc = \\" );
            Wa( "    UpdateObjDesc( strPath_, params )" );
            Wa( "if ret < 0:" );
            Wa( "    print( 'Error UpdateObjDesc', ret )" );
            Wa( "    return ret" );
            Wa( "if isinstance( strNewDesc, str ) and len( strNewDesc ):" );
            Wa( "    params[ 'objdesc' ] = strNewDesc" );
            CCOUT << "if 'ipaddr' in params or 'portnum' in params:";
            INDENT_UPL;
            Wa( "ret, strNewCfg = \\" );
            Wa( "    UpdateDrvCfg( strCfg, params )" );
            Wa( "if ret < 0:" );
            Wa( "    print( 'Error UpdateDrvCfg', ret )" );
            Wa( "    return ret" );
            Wa( "if isinstance( strNewCfg, str ) and len( strNewCfg ):" );
            Wa( "    params[ 'driver' ] = strNewCfg" );
            INDENT_DOWNL;

            Wa( "oContext = PyRpcContext( params )" );
        }
        else
        {
            Wa( "oContext = PyRpcContext( 'PyRpcServer' )" );
        }
        CCOUT << "with oContext as ctx:";
        INDENT_UPL;
        CCOUT << "if ctx.status < 0:";
        INDENT_UPL;
        Wa( "ret = ctx.status" );
        Wa( "print( os.getpid()," ); 
        Wa( "    \"Error start PyRpcContext %d\" % ret )" );
        CCOUT << "return ret";
        INDENT_DOWNL;
        NEW_LINE;

        Wa( "print( \"start to work here...\" )" );
        if( g_bBuiltinRt )
        {
            Wa( "if 'objdesc' in params:" );
            Wa( "    strPath_ = params[ 'objdesc' ]" );
            Wa( "else:" );
            Wa( "    strPath_ = os.path.dirname( os.path.realpath( __file__ ) )" );
            CCOUT << "    strPath_ += '/" << g_strAppName << "desc.json'";
            NEW_LINE;
        }
        else
        {
            Wa( "strPath_ = os.path.dirname( os.path.realpath( __file__) )" );
            CCOUT << "strPath_ += '/" << g_strAppName << "desc.json'";
            NEW_LINE;
        }
        for( auto elem : vecSvcs )
        {
            CServiceDecl* pSvc = elem;
            stdstr strName = pSvc->GetName();
            stdstr strVar = "oServer_";
            strVar += strName;

            CCOUT << strVar << " = C" << strName << "Server( ctx.pIoMgr,";
            NEW_LINE;
            CCOUT << "    strPath_, '" << strName << "' )" ;
            NEW_LINE;
            CCOUT << "ret = "<< strVar << ".GetError()";
            NEW_LINE;
            Wa( "if ret < 0 :" );
            Wa( "    return ret" );
            NEW_LINE;
        }
        if( vecSvcs.size() == 1 )
        {
            CServiceDecl* pSvc = vecSvcs[ 0 ];
            stdstr strName = pSvc->GetName();
            stdstr strVar = "oServer_";
            strVar += strName;
            CCOUT << "with " << strVar << " as oServer:";
        }
        else
        {
            CCOUT << "with(";
            INDENT_UPL
            for( int i = 0; i < vecSvcs.size(); i++ )
            {
                CServiceDecl* pSvc = vecSvcs[ i ];
                stdstr strName = pSvc->GetName();
                stdstr strVar = "oServer_";
                strVar += strName;
                CCOUT << strVar << " as oServer";
                if( i > 0 )
                    CCOUT << i;
                if( i < vecSvcs.size() - 1 )
                {
                    CCOUT << ",";
                    NEW_LINE;
                }
                else
                {
                    NEW_LINE;
                    CCOUT << "):";
                }
            }
            INDENT_DOWN;
        }

        INDENT_UPL;
        CCOUT << "try:";
        INDENT_UPL;
#ifdef FUSE3
        if( g_bBuiltinRt )
        {
            CCOUT << "if bMount:";
            INDENT_UPL;
            for( gint32 i = 0; i < vecSvcs.size(); i++ )
            {
                CServiceDecl* pSvc = vecSvcs[ i ];
                stdstr strName = pSvc->GetName();
                CCOUT << "AddSvcStatFile( oServer";
                if( i > 0 )
                    CCOUT << i;
                CCOUT << ".oObj, \"" << strName << "\" )";
                NEW_LINE;
            }
            Wa( "fuseif_mainloop(" );
            CCOUT << "    sys.argv[ 0 ], params[ 'MountPoint' ] )";
            INDENT_DOWNL;
            CCOUT << "else :";
            INDENT_UPL;
            EMIT_CALL_TO_USERMAIN( false );
            INDENT_DOWN;
        }
        else
        {
            EMIT_CALL_TO_USERMAIN( false );
        }
#else
        EMIT_CALL_TO_USERMAIN( false );
#endif
        INDENT_DOWNL;
        Wa( "except Exception as err:" );
        Wa( "    print( err )" );
        INDENT_DOWNL;
        Wa( "print( \"Server loop ended...\" )" );
        if( vecSvcs.size() == 1 )
        {
            CServiceDecl* pSvc = vecSvcs[ 0 ];
            stdstr strName = pSvc->GetName();
            CCOUT << "oServer_" 
                << strName <<" = None";
        }
        else
        {
            for( int i = 0; i < vecSvcs.size(); i++ )
            {
                CServiceDecl* pSvc = vecSvcs[ i ];
                stdstr strName = pSvc->GetName();
                stdstr strVar = "oServer_";
                strVar += strName;
                CCOUT << strVar << " = None";
                if( i < vecSvcs.size() - 1 )
                    NEW_LINE;
            }
        }
        INDENT_DOWNL;
        Wa( "oContext = None" );
        if( g_bBuiltinRt )
        {
            INDENT_DOWNL;
            CCOUT << "finally:";
            INDENT_UPL;
            CCOUT << "if 'objdesc' in params:";
            INDENT_UPL;
            Wa( "strVal = params[ 'objdesc' ]" );
            Wa( "if strVal[:12] == '/tmp/rpcfos_':" );
            CCOUT << "    os.unlink( strVal )";
            INDENT_DOWNL;
            CCOUT << "if 'driver' in params:";
            INDENT_UPL;
            Wa( "strVal = params[ 'driver' ]" );
            Wa( "if strVal[:13] == '/tmp/rpcfdrv_':" );
            CCOUT << "    os.unlink( strVal )";
            INDENT_DOWN;
            INDENT_DOWNL;
        }
        Wa( "return ret" );
        INDENT_DOWNL;

        EmitDefineUserMain( vecSvcs, false );

        Wa( "if __name__ == '__main__' :" );
        Wa( "    ret = MainEntrySvr()" );
        Wa( "    quit( -ret )" );

    }while( 0 );

    return ret;
}

gint32 CImplPyMainFunc::Output()
{
    gint32 ret = 0;
    do{
        std::vector< ObjPtr > vecSvcs;
        ret = m_pNode->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
            break;

        m_pWriter->SelectMainCli();
        OUTPUT_BANNER();
        for( auto& elem : vecSvcs )
        {
            CServiceDecl* pNode = elem;
            stdstr strName = pNode->GetName();
            CCOUT << "from " << strName << "cli"
                << " import " << "C" << strName
                << "Proxy";
            NEW_LINE;
        }
        ret = OutputCli( vecSvcs );
        if( ERROR( ret ) )
            break;

        m_pWriter->SelectMainSvr();
        OUTPUT_BANNER();
        for( auto& elem : vecSvcs )
        {
            CServiceDecl* pNode = elem;
            stdstr strName = pNode->GetName();
            CCOUT << "from " << strName << "svr"
                << " import " << "C" << strName
                << "Server";
            NEW_LINE;
        }
        ret = OutputSvr( vecSvcs );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

extern stdstr g_strLocale;
gint32 CExportPyReadme::Output()
{
    if( g_strLocale == "en" )
        return Output_en();
    if( g_strLocale == "cn" )
        return Output_cn();

    return -ENOTSUP;
}

gint32 CExportPyReadme::Output_en()
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
            << "entry point for client program "
            << "and definition of `mainsvr()` function for server program respectively. ";
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

        CCOUT<< "* *" << g_strAppName << "structs.py*: "
            << "Containing all the declarations of the struct classes "
            << "declared in the ridl, with serialization methods implemented.";
        NEW_LINE;
        CCOUT << "And please don't edit it, since they will be "
            << "overwritten by `ridlc` without auto-backup.";
        NEW_LINES( 2 );

        CCOUT<< "* *" << g_strAppName << "desc.json*: "
            << "Containing the configuration parameters for all "
            << "the services declared in the ridl file";
        NEW_LINE;
        CCOUT << "And please don't edit it, since they will be "
            << "overwritten by `ridlc` and synccfg.py without backup.";
        NEW_LINES( 2 );

        CCOUT << "* *driver.json*: "
            << "Containing the configuration parameters for all "
            << "the ports and drivers";
        NEW_LINE;
        CCOUT << "And please don't edit it, since they will be "
            << "overwritten by `ridlc` and synccfg.py without backup.";
        NEW_LINES( 2 );

        CCOUT << "* *Makefile*: "
            << "The Makefile will just synchronize the configurations "
            << "with the local system settings. And it does nothing else.";
        NEW_LINE;
        CCOUT << "And please don't edit it, since it will be "
            << "overwritten by `ridlc` and synccfg.py without backup.";
        NEW_LINES( 2 );

        for( auto& elem : vecSvcNames )
        {
            CCOUT << "* *" << elem << "svrbase.py*, *"<< elem << "clibase.py* : "
                << "Containing the declarations and definitions of all the server/client side "
                << "utilities and helpers for the interfaces of service `" << elem << "`.";
            NEW_LINE;
            CCOUT << "And please don't edit them, since they will be "
                << "overwritten by `ridlc` without backup.";
            NEW_LINES( 2 );

        }

        CCOUT << "* *seribase.py*: "
            << "Containing the utility classes for serializations.";
        NEW_LINE;
        CCOUT << "And please don't edit it, since they will be "
            << "overwritten by `ridlc`.";
        NEW_LINES( 2 );

        CCOUT << "* *synccfg.py*: "
            << "a small python script to synchronous settings "
            << "with the system settings, just ignore it.";
        NEW_LINES(2);
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

gint32 CExportPyReadme::Output_cn()
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

        for( auto& elem : vecSvcNames )
        {
            CCOUT << "* *" << elem << "svrbase.py*, *"<< elem << "clibase.py* : "
                << "分别包含有关service `"<<elem
                <<"`的服务器端和客户端的所有辅助函数和底部支持功能的实现。";
            NEW_LINE;
            CCOUT << "这些函数和方法务必不要做进一步的修改。"
                << "`ridlc`在下一次运行时会重写里面的内容。";
            NEW_LINES( 2 );

        }

        CCOUT<< "* *" << g_strAppName << "structs.py*: "
            << "包含一个ridl文件中声明的所有用到的struct，"
            << "以及序列/反序列化方法的实现.";
        NEW_LINE;
        CCOUT << "这个文件务必不要做进一步的修改。"
                << "`ridlc`在下一次运行时会重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT<< "* *" << g_strAppName << "desc.json*: "
            << "包含本应用相关的配置信息, 和所有定义的服务(service)的配置参数。";
        NEW_LINE;
        CCOUT << "这个文件务必不要做进一步的修改。"
            << "`ridlc`或者`synccfg.py`都会在在下一次运行时重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT << "* *driver.json*: "
            << "包含本应用相关的配置信息,主要是底层的iomanager的配置信息。";
        NEW_LINE;
        CCOUT << "这个文件务必不要做进一步的修改。"
                << "`ridlc`或者`synccfg.py`都会在在下一次运行时重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT << "* *Makefile*: "
            << "该Makefile会用当前系统配置的信息更新本目录下的json配置文件。";
        NEW_LINE;
        CCOUT << "这个文件务必不要做进一步的修改。"
                << "`ridlc`或者`synccfg.py`都会在在下一次运行时重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT << "* *seribase.py*: "
            << "此文件包含序列化相关的支持和实现, 会随着ridl内容文件的改变而改变。";
        NEW_LINE;
        CCOUT << "所以此文件务必不要做进一步的修改.";
        NEW_LINES( 2 );

        CCOUT << "* *synccfg.py*: "
            << "一个小的Python脚本，用来同步本应用配置信息。";
        NEW_LINES(2);
        CCOUT <<"* **运行:** 在命令行输入`python3 mainsvr.py`启动服务器。"
            << " 在另一个命令行输入 `python3 maincli`以启动客户端程序。"
            << "在运行前，务必运行一下`make`命令同步程序的配置文件，"
            << "即`"<< g_strAppName <<"desc.json`和`driver.json`。";
        NEW_LINES(2);
        CCOUT << "**注1**: 上文中的粗体字的文件是需要你进一步修改的文件. 斜体字的文件则不需要。"
            << "如果仍然有修改的必要，请注意这些文件有被`ridlc`或者`synccfg.py`改写的风险。";
        NEW_LINES(2);
        CCOUT << "**注2**: 有关配置系统搭建和设置请参考[此文。](https://github.com/zhiming99/rpc-frmwrk#building-rpc-frmwrk)。";
        NEW_LINE;

   }while( 0 );

   return ret;
}
