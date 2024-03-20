/*
 * =====================================================================================
 *
 *       Filename:  genjs.cpp
 *
 *    Description:  Implementations of classes and functions for proxy generator
 *                  for JavaScript
 *
 *        Version:  1.0
 *        Created:  02/29/2024 09:08:28 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2024 Ming Zhi( woodhead99@gmail.com )
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
#include "genjs.h"
#include "gencpp2.h"
#include "jsondef.h"

stdstr g_strJsLibPath = "";
stdstr g_strWebPath = "";
extern stdstr g_strCmdLine;
extern stdstr g_strAppName;
extern gint32 SetStructRefs( ObjPtr& pRoot );
extern gint32 SyncCfg( const stdstr& strPath );
extern guint32 GenClsid( const std::string& strName );
extern bool g_bRpcOverStm;

extern stdstr GetTypeName( CAstNodeBase* pType );
extern stdstr GetTypeSig( ObjPtr& pObj );

stdstr GetTypeSigJs(
    CAstNodeBase* pNode, bool bRestore = false )
{
    ObjPtr pObj = pNode;
    if( bRestore )
        g_strLang = "java";
    stdstr strSig = GetTypeSig( pObj );
    if( bRestore )
        g_strLang = "js";
    return strSig;
}

gint32 GetArgsAndSigsJs( CArgList* pArgList,
    std::vector< std::pair< stdstr, stdstr >>& vecArgs,
    bool bRestore = false )
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
        stdstr strSig = GetTypeSigJs( pObj, bRestore );
        vecArgs.push_back(
            std::pair< stdstr, stdstr >
                ( strVarName, strSig ) );
    }
    return ret;
}

const stdstr& GetJsLibPath()
{
    if( !g_strJsLibPath.empty() )
        return g_strJsLibPath;

    stdstr strPath;
    gint32 ret = FindInstCfg(
        "driver.json", strPath );
    if( ERROR( ret ) )
        return g_strJsLibPath;
    strPath = GetDirName( strPath );
    g_strJsLibPath = strPath + "/jslib";
    return g_strJsLibPath;
}

std::map< char, stdstr > g_mapSig2JsType =
{
    { '(', "Array" },
    { '[', "Map" },
    { 'O', "Object" },
    { 'Q', "Uint64" },
    { 'q', "Int64" },
    { 'D', "Uint32" },
    { 'd', "Int32" },
    { 'W', "Uint16" },
    { 'w', "Int16" },
    { 'b', "bool" },
    { 'B', "Uint8" },
    { 'f', "float" },
    { 'F', "double" },
    { 's', "String" },
    { 'a', "Uint8Array" },
    { 'o', "ObjPtr" },
    { 'h', "HStream" }
};

CJsFileSet::CJsFileSet(
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

    m_strMainCli =
        strOutPath + "/maincli.js";

    GEN_FILEPATH( m_strStructsJs,
        strOutPath, strAppName + "structs.js",
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
        strOutPath, "maincli.js",
        true );

    /*GEN_FILEPATH( m_strMainSvr, 
        strOutPath, "mainsvr.js",
        true );*/

    GEN_FILEPATH( m_strReadme, 
        strOutPath, "README.md",
        false );

    GEN_FILEPATH( m_strWebCfg, 
        strOutPath, "webpack.config.js",
        false );

    GEN_FILEPATH( m_strHtml,
        strOutPath, strAppName + ".html",
        true );

    m_strPath = strOutPath;
    gint32 ret = OpenFiles();
    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "internal error open files" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CJsFileSet::OpenFiles()
{
    STMPTR pstm( new std::ofstream(
        m_strStructsJs,
        std::ofstream::out |
        std::ofstream::trunc ) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm= STMPTR( new std::ofstream(
        m_strMainCli,
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

    /*pstm = STMPTR( new std::ofstream(
        m_strMainCli,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strMainSvr,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );*/

    pstm = STMPTR( new std::ofstream(
        m_strReadme,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strWebCfg,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strHtml,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_vecFiles.push_back( std::move( pstm ) );

    return STATUS_SUCCESS;
}

gint32 CJsFileSet::AddSvcImpl(
    const std::string& strSvcName )
{
    if( strSvcName.empty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        std::string strExt = ".js";
        std::string strCliJs = m_strPath +
            "/" + strSvcName + "cli.js";

        std::string strCliJsBase = m_strPath +
            "/" + strSvcName + "clibase.js";

        ret = access( strCliJs.c_str(), F_OK );
        if( ret == 0 )
        {
            strExt = ".js.new";
            strCliJs = m_strPath + "/" +
                strSvcName + "cli.js.new";
        }

        gint32 idx = m_vecFiles.size();
        STMPTR pstm = STMPTR( new std::ofstream(
            strCliJs,
            std::ofstream::out |
            std::ofstream::trunc) );

        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strCliJs ] = idx;

        idx += 1;
        pstm = STMPTR( new std::ofstream(
            strCliJsBase,
            std::ofstream::out |
            std::ofstream::trunc) );

        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strCliJsBase ] = idx;

    }while( 0 );

    return ret;
}

CJsFileSet::~CJsFileSet()
{
    for( auto& elem : m_vecFiles )
    {
        if( elem != nullptr )
            elem->close();
    }
    m_vecFiles.clear();
}
CDeclareJsStruct::CDeclareJsStruct(
    CJsWriter* pWriter,
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

void CDeclareJsStruct::OutputStructBase()
{
    Wa( "class CStructBase extends CObjBase" );
    BLOCK_OPEN;
    Wa( "constructor( pIf )" );
    BLOCK_OPEN;
    Wa( "super();" );
    Wa( "if( pIf !== undefined )" );
    Wa( "    this.m_pIf = pIf;" );
    Wa( "else" );
    Wa( "    this.m_pIf = null;" );
    CCOUT << "return;";
    BLOCK_CLOSE;
    NEW_LINE;
    Wa( "SetInterface( pIf )" );
    CCOUT << "{ this.m_pIf = pIf; }";
    BLOCK_CLOSE;
    NEW_LINE;
}

static gint32 EmitDeserialBySigJs(
    CWriterBase* m_pWriter,
    const stdstr& strSig )
{
    gint32 ret = 0;
    switch( strSig[ 0 ] )
    {
    case '(' :
        {
            CCOUT << "ret = osb.DeserialArray( buf, offset, \""
                << strSig << "\" );";
            break;
        }
    case '[' :
        {
            CCOUT << "ret = osb.DeserialMap( buf, offset, \""
                << strSig << "\" );";
            break;
        }

    case 'O' :
        {
            CCOUT << "ret = osb.DeserialStruct( buf, offset"
                << " );";
            break;
        }
    case 'Q':
    case 'q':
        {
            CCOUT << "ret = osb.DeserialUint64( buf, offset );";
            break;
        }
    case 'D':
    case 'd':
        {
            CCOUT << "ret = osb.DeserialUint32( buf, offset );";
            break;
        }
    case 'W':
    case 'w':
        {
            CCOUT << "ret = osb.DeserialUint16( buf, offset );";
            break;
        }
    case 'b':
    case 'B':
        {
            CCOUT << "ret = osb.DeserialUint8( buf, offset );";
            break;
        }
    case 'f':
        {
            CCOUT << "ret = osb.DeserialFloat( buf, offset );";
            break;
        }
    case 'F':
        {
            CCOUT << "ret = osb.DeserialDouble( buf, offset );";
            break;
        }
    case 's':
        {
            CCOUT << "ret = osb.DeserialString( buf, offset );";
            break;
        }
    case 'a':
        {
            CCOUT << "ret = osb.DeserialBuf( buf, offset );";
            break;
        }
    case 'o':
        {
            CCOUT << "ret = osb.DeserialObjPtr( buf, offset );";
            break;
        }
    case 'h':
        {
            CCOUT << "ret = osb.DeserialHStream( buf, offset );";
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

static gint32 EmitSerialBySigJs(
    CWriterBase* m_pWriter,
    const stdstr& strField,
    const stdstr& strSig )
{
    gint32 ret = 0;
    switch( strSig[ 0 ] )
    {
    case '(' :
        {
            CCOUT << "osb.SerialArray( "
                << strField << ", \"" << strSig << "\" )";
            break;
        }
    case '[' :
        {
            CCOUT << "osb.SerialMap( "
                << strField << ", \"" << strSig << "\" )";
            break;
        }

    case 'O' :
        {
            CCOUT << "osb.SerialStruct( " << strField << " )";
            break;
        }
    case 'Q':
    case 'q':
        {
            CCOUT << "osb.SerialUint64( " << strField << " )";
            break;
        }
    case 'D':
    case 'd':
        {
            CCOUT << "osb.SerialUint32( " << strField << " )";
            break;
        }
    case 'W':
    case 'w':
        {
            CCOUT << "osb.SerialUint16( " << strField << " )";
            break;
        }
    case 'b':
    case 'B':
        {
            CCOUT << "osb.SerialUint8( " << strField << " )";
            break;
        }
    case 'f':
        {
            CCOUT << "osb.SerialFloat( " << strField << " )";
            break;
        }
    case 'F':
        {
            CCOUT << "osb.SerialDouble( " << strField << " )";
            break;
        }
    case 's':
        {
            CCOUT << "osb.SerialString( " << strField << " )";
            break;
        }
    case 'a':
        {
            CCOUT << "osb.SerialBuf( " << strField << " )";
            break;
        }
    case 'o':
        {
            CCOUT << "osb.SerialObjPtr( " << strField << " )";
            break;
        }
    case 'h':
        {
            CCOUT << "osb.SerialHStream( " << strField << " )";
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

static gint32 EmitFormalArgListJs(
    CWriterBase* m_pWriter, 
    ObjPtr& pmdecl, bool bIn )
{
    gint32 ret = 0;
    do{
        CMethodDecl* pmd = pmdecl;
        ObjPtr pArgs = bIn ?
            pmd->GetInArgs():pmd->GetOutArgs();

        CArgListUtils oau;
        guint32 dwCount = oau.GetArgCount( pArgs );
        if( dwCount == 0 )
            break;

        INDENT_UPL;
        std::vector< std::pair< stdstr, stdstr > > vecArgs;
        ret = GetArgsAndSigsJs( pArgs, vecArgs );
        if( ERROR( ret ) )
            break;
        for( int i = 0; i < vecArgs.size(); i++ )
        {
            auto& elem = vecArgs[ i ];
            CCOUT << elem.first;
            if( i < vecArgs.size() - 1 )
                CCOUT << ",";
            CArgList* pal = pArgs;
            CFormalArg* pfa = pal->GetChild( i );

            CAstNodeBase* pType = pfa->GetType();

            stdstr strType = GetTypeName( pType );
            CCOUT << " // " << strType;
            if( i < vecArgs.size() - 1 )
                NEW_LINE;
        }
        INDENT_DOWNL;

    }while( 0 );

    return ret;
}

gint32 CDeclareJsStruct::Output()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        CCOUT << "class " << strName
            <<  " extends CStructBase";
        NEW_LINE;
        BLOCK_OPEN;
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

        Wa( "// struct identity" );

        Wa( "GetStructId()" );
        CCOUT << "{ return "<< strClsid << "; }";

        NEW_LINES( 2 );

        Wa( "GetStructName()" );
        CCOUT << "{ return \""<< strMsgId << "\"; }";

        NEW_LINES( 2 );

        Variant oVar;
        oVar = dwMsgId;
        m_pNode->SetProperty(
            PROP_MESSAGE_ID, oVar );

        Wa( "constructor( pIf )" );
        BLOCK_OPEN;
        CCOUT << "super( pIf )";
        NEW_LINE;
        CCOUT << "this.SetClsid( " << strClsid << " );";
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
            stdstr strSig = GetTypeSigJs( pType );
            BufPtr& pVal = pfd->GetVal();

            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    CCOUT << "this." << strName << " = []; ";
                    break;
                }
            case '[' :
                {
                    CCOUT << "this." << strName << " = new Map(); ";
                    break;
                }

            case 'O' :
                {
                    CCOUT << "this." << strName << " = new Object(); ";
                    break;
                }
            case 'h':
                {
                    CCOUT << "this." << strName << " = 0;";
                    break;
                }
            case 'Q':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = 0;";
                    else
                    {
                        guint64 qwVal = *pVal;
                        CCOUT << "this." << strName << " = " << qwVal << ";";
                    }
                    break;
                }
            case 'q':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = 0;";
                    else
                    {
                        gint64 qwVal = *pVal;
                        CCOUT << "this." << strName << " = " << qwVal << ";";
                    }
                    break;
                }
            case 'D':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = 0;";
                    else
                    {
                        guint32 dwVal = *pVal;
                        CCOUT << "this." << strName << " = " << dwVal << ";";
                    }
                    break;
                }
            case 'd':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = 0;";
                    else
                    {
                        gint32 iVal = *pVal;
                        CCOUT << "this." << strName << " = " << iVal << ";";
                    }
                    break;
                }
            case 'W':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = 0;";
                    else
                    {
                        guint16 wVal = *pVal;
                        CCOUT << "this." << strName << " = " << wVal << ";";
                    }
                    break;
                }
            case 'w':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = 0;";
                    else
                    {
                        gint16 sVal = *pVal;
                        CCOUT << "this." << strName << " = " << sVal << ";";
                    }
                    break;
                }
            case 'b':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = true;";
                    else
                    {
                        bool bVal = *pVal;
                        if( bVal )
                            CCOUT << "this." << strName << " = " << "true;";
                        else
                            CCOUT << "this." << strName << " = " << "false;";
                    }
                    break;
                }
            case 'B':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = 0;";
                    else
                    {
                        guint8 byVal = *pVal;
                        CCOUT << "this." << strName << " = " << byVal << ";";
                    }
                    break;
                }
            case 'f':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = .0;";
                    else
                    {
                        float fVal = *pVal;
                        CCOUT << "this." << strName << " = " << fVal << ";";
                    }
                    break;
                }
            case 'F':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = .0;";
                    else
                    {
                        double dblVal = *pVal;
                        CCOUT << "this." << strName << " = " << dblVal << ";";
                    }
                    break;
                }
            case 's':
                {
                    if( pVal.IsEmpty() )
                        CCOUT << "this." << strName << " = '';";
                    else
                    {
                        const char* szVal = pVal->ptr();
                        CCOUT << "this." << strName << " = \"" << szVal << "\";";
                    }
                    break;
                }
            case 'a':
                {
                    // bytearray
                    CCOUT << "this." << strName << " = null;";
                    break;
                }
            case 'o':
                {
                    CCOUT << "this." << strName << " = null;";
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
            if( i < dwCount - 1 )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "Serialize()" );
        BLOCK_OPEN;
        Wa( "var osb = new CSerialBase( this.m_pIf );" );
        NEW_LINE;
        CCOUT << "osb.SerialUint32( this.GetStructId() );";
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
            stdstr strSig = GetTypeSigJs( pType );

            stdstr strField = "this.";
            strField += strName;
            ret = EmitSerialBySigJs(
                m_pWriter, strField, strSig );
            if( ERROR( ret ) )
                break;
        }
        NEW_LINE;
        CCOUT << "return osb.m_oBuf;";
        BLOCK_CLOSE;
        NEW_LINE;

        Wa( "Deserialize( buf, offset )" );
        BLOCK_OPEN;
        Wa( "var ret;" );
        Wa( "var osb = new CSerialBase( this.m_pIf );" );
        NEW_LINE;
        CCOUT << "ret = osb.DeserialUint32( buf, offset );";
        NEW_LINE;
        Wa( "if( ret === null )" );
        CCOUT << "    throw new Error( 'Error bad format struct' );";
        NEW_LINE;
        CCOUT << "if( ret[ 0 ] !== this.GetStructId() )";
        NEW_LINE;
        CCOUT << "    throw new Error( 'Error unknow struct id' );";
        NEW_LINE;
        Wa( "offset += 4;" );
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
            stdstr strSig = GetTypeSigJs( pType );
            ret = EmitDeserialBySigJs(
                m_pWriter, strSig );
            if( ERROR( ret ) )
                break;
            Wa( "if( ret[ 0 ] === null )" );
            CCOUT << "    throw new Error( 'Error failed to "
                << "deserialize field \""<<strName<< "\"' );";
            NEW_LINE;
            CCOUT << "this." << strName << " = ret[ 0 ];";
            NEW_LINE;
            CCOUT << "offset = ret[ 1 ]";
            NEW_LINES( 2 );
        }
        if( ERROR( ret ) )
            break;

        CCOUT << "return [ this, offset ];";
        BLOCK_CLOSE;
        NEW_LINE;
        ret = OutputRestore();
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CDeclareJsStruct::OutputRestore()
{
    gint32 ret = 0;
    do{
        Wa( "Restore( oVal )" );
        BLOCK_OPEN;

        ObjPtr pFields =
            m_pNode->GetFieldList();

        CFieldList* pfl = pFields;
        guint32 dwCount = pfl->GetCount();
        if( dwCount == 0 )
        {
            ret = -ENOENT;
            break;
        }

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
            stdstr strSig = GetTypeSigJs( pType, true );
            BufPtr& pVal = pfd->GetVal();
            if( i > 0 )
                NEW_LINE;

            CCOUT << "if( oVal."<< strName <<" !== undefined )";
            INDENT_UPL;
            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    if( strSig.find( 'O' ) == stdstr::npos )
                    {
                        CCOUT << "this." << strName
                            << " = oVal."<< strName <<";";
                    }
                    else
                    {
                        CCOUT << "this." << strName
                            << " = CStructBase.RestoreArray( oVal."
                            << strName <<", '" << strSig << "' );";
                    }
                    break;
                }
            case '[' :
                {
                    if( strSig.find( 'O' ) == stdstr::npos )
                    {
                        CCOUT << "this." << strName
                            << " = oVal."<< strName <<";";
                    }
                    else
                    {
                        CCOUT << "this." << strName
                            << " = CStructBase.RestoreMap( oVal."
                            << strName <<", '" << strSig << "' );";
                    }
                    break;
                }

            case 'O' :
                {
                    CStructRef* pstr = ObjPtr( pType );
                    ObjPtr pObj;
                    ret = pstr->GetStructType( pObj );
                    if( ERROR( ret ) )
                        break;
                    CStructDecl* pst = pObj;
                    CCOUT << "this." << strName
                        << " = new " << pst->GetName() <<"(); ";
                    NEW_LINE;
                    CCOUT << "this." << strName
                        << ".Restore( oVal." << strName <<" );";
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
            case 'f':
            case 'F':
            case 's':
            case 'a':
            case 'o':
                {
                    CCOUT << "this." << strName
                        << " = oVal."<< strName <<";";
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
            INDENT_DOWN;
            if( i < dwCount - 1 )
                NEW_LINE;
        }
        BLOCK_CLOSE;

    }while( 0 );

    return 0;
}

static gint32 GenStructsFileJs(
    CJsWriter* m_pWriter, ObjPtr& pRoot )
{
    if( m_pWriter == nullptr ||
        pRoot.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    bool bHasStruct = true;
    do{
        stdstr strLibPath = GetJsLibPath();
        CStatements* pStmts = pRoot;
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        m_pWriter->SelectStructsFile();
        EMIT_DISCLAIMER;
        CCOUT << "// " << g_strCmdLine;
        NEW_LINE;
        CCOUT << "const { CSerialBase, CStructBase } = require( '"
            << strLibPath << "/combase/seribase' );";
        NEW_LINE;
        CCOUT << "const { errno } = require( '"
            << strLibPath << "/combase/enums' );";
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
        if( vecActStructs.empty() )
            bHasStruct = false;
        for( auto& elem : vecActStructs )
        {
            CDeclareJsStruct odps( m_pWriter, elem );
            ret = odps.Output();
            if( ERROR( ret ) )
                break;
        }

        NEW_LINE;
        if( vecActStructs.size() > 0 )
        {
            CCOUT << "const { IClassFactory } = require( '"
                << strLibPath << "/combase/factory')";
            NEW_LINE;
        }
        CCOUT << "class C" << g_strAppName <<"Factory "
            <<  "extends IClassFactory";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "constructor()" );
        CCOUT << "{ super(); }";

        if( vecActStructs.size() > 0 )
        {
            NEW_LINE;
            Wa( "CreateInstance( iClsid )" );
            BLOCK_OPEN;
            std::vector< stdstr > vecClsids;
            Wa( "switch( iClsid )" );
            Wa( "{" );
            for( int i = 0; i < vecActStructs.size(); i++ )
            {
                CStructDecl* pStruct = vecActStructs[ i ];
                stdstr strName = pStruct->GetName(); 
                stdstr strMsgId =
                    g_strAppName + "::" + strName;
                guint32 dwMsgId = GenClsid( strMsgId );
                stdstr strClsid = FormatClsid( dwMsgId );
                CCOUT << "case " << strClsid << ":";
                INDENT_UPL;
                CCOUT << "return new " << strName << "();";
                INDENT_DOWNL;
            }
            Wa( "default:" );
            Wa( "    return null;" );
            CCOUT << "}";
            BLOCK_CLOSE;
        }
        BLOCK_CLOSE;

        for( int i = 0; i < vecActStructs.size(); i++ )
        {
            if( i == 0 )
                NEW_LINE;
            CStructDecl* pStruct = vecActStructs[ i ];
            stdstr strName = pStruct->GetName(); 
            CCOUT << "exports." << strName << " = " << strName << ";";
            if( i < vecActStructs.size() - 1 )
                NEW_LINE;
        }
    }while( 0 );

    if( bHasStruct )
    {
        NEW_LINES(2);
        CCOUT << "globalThis.RegisterFactory( new C" << g_strAppName << "Factory() );";
    }
    NEW_LINE;

    return ret;

}

CJsExportMakefile::CJsExportMakefile(
    CJsWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
}

gint32 CJsExportMakefile::Output()
{
    gint32 ret = 0;
    do{
        Wa( "update :" );
        Wa( "\tpython3 synccfg.py" );
        NEW_LINE;

        Wa( "debug :" );
        Wa( "\t@if [ ! -d node_modules ]; then ln -s `npm -g root`;fi" );
        Wa( "\t@sed -i \"s@mode:[[:blank:]]*'production'@mode: 'development'@g\" webpack.config.js" );
        Wa( "\tnpm exec webpack" );
        NEW_LINE;

        Wa( "release :" );
        Wa( "\t@if [ ! -d node_modules ]; then ln -s `npm -g root`;fi" );
        Wa( "\t@sed -i \"s@mode:[[:blank:]]*'development'@mode: 'production'@g\" webpack.config.js" );
        Wa( "\tnpm exec webpack" );

    }while( 0 );
    return ret;
}

static void OUTPUT_BANNER(
    CJsWriter* m_pWriter, CServiceDecl* pSvc, bool bDisclaimer = true )
{
    stdstr strLibPath = GetJsLibPath();
    if( bDisclaimer )
    {
        EMIT_DISCLAIMER;
        CCOUT << "// " << g_strCmdLine;
    }
    NEW_LINE;
    CCOUT << "const { CConfigDb2 } = require( '"
        << strLibPath <<"/combase/configdb' );";
    NEW_LINE;
    CCOUT << "const { messageType } = require( '"
        << strLibPath << "/dbusmsg/constants' );";
    NEW_LINE;
    if( bDisclaimer )
        CCOUT << "const { randomInt, ERROR, Int32Value, USER_METHOD } = require( '"
            << strLibPath <<"/combase/defines' );";
    else
        CCOUT << "const { ERROR, Int32Value, USER_METHOD } = require( '"
            << strLibPath <<"/combase/defines' );";
    NEW_LINE;
    CCOUT << "const {EnumClsid, errno, EnumPropId, EnumCallFlags, "<<
        "EnumTypeId, EnumSeriProto} = require( '"
        << strLibPath <<"/combase/enums' );";
    NEW_LINE;
    CCOUT << "const {CSerialBase} = require( '"
        << strLibPath <<"/combase/seribase' );";
    NEW_LINE;
    CCOUT << "const {CInterfaceProxy} = require( '"
        << strLibPath <<"/ipc/proxy' )";
    NEW_LINE;
    CCOUT << "const {Buffer} = require( 'buffer' );";
    NEW_LINE;
    CCOUT << "const { DBusIfName, DBusDestination2, DBusObjPath } = require( '"
        << strLibPath << "/rpc/dmsg' );";
    NEW_LINE;
    do{ 
        ObjPtr pRoot; 
        do{ 
            pRoot = pSvc->GetParent(); 
            if( pRoot.IsEmpty() ) 
                break; 
        }while( pRoot->GetClsid() != 
            clsid( CStatements ) ); 
        if( pRoot.IsEmpty() ) 
            break; 
        CStatements* pStmts = pRoot; 
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
        if( vecActStructs.empty() ) break; 
        CCOUT << "const { "; 
        for( auto& elem : vecActStructs ) 
        { 
            CStructDecl* psd = elem; 
            CCOUT << psd->GetName() << ", "; 
        } 
        CCOUT << "} = require( './" 
            << g_strAppName << "structs' );"; 
        NEW_LINE; 
    }while( 0 );
    return;
}

CImplJsSvcProxyBase::CImplJsSvcProxyBase(
    CJsWriter* pWriter, ObjPtr& pNode )
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

gint32 CImplJsSvcProxyBase::EmitBuildEventTable()
{
    gint32 ret = 0;
    do{ 
        CServiceDecl* psd = m_pNode;
        std::vector< ObjPtr > vecIfs;
        psd->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        std::vector< std::pair< ObjPtr, ObjPtr > > vecEvents;
        for( int i = 0; i < vecIfs.size(); i++ )
        {
            auto& elem = vecIfs[ i ];
            CInterfRef* pIfRef = elem;
            ObjPtr pObj;
            ret = pIfRef->GetIfDecl( pObj );
            if( ERROR( ret ) )
                break;
            CInterfaceDecl* pifd = pObj;
            ObjPtr pmdlobj = pifd->GetMethodList();
            if( pmdlobj.IsEmpty() )
            {
                ret = -ENOENT;
                break;
            }

            stdstr strIfName = pifd->GetName();
            CMethodDecls* pmdl = pmdlobj;
            for( guint32 i = 0; i < pmdl->GetCount(); i++ )
            {
                ObjPtr pmd = pmdl->GetChild( i );
                if( pmd.IsEmpty() )
                {
                    ret = -ENOENT;
                    break;
                }
                CMethodDecl* pmdecl = pmd;
                if( !pmdecl->IsEvent() )
                    continue;
                vecEvents.push_back({ pObj, pmd });
            }
        }
        if( vecEvents.empty() )
        {
            ret = -ENOENT;
            break;
        }
        Wa( "BuildEventTable()" );
        BLOCK_OPEN;
        for( int i = 0; i < vecEvents.size(); i++ )
        {
            auto& elem = vecEvents[ i ];
            CInterfaceDecl* pifd = elem.first;
            stdstr strIfName = pifd->GetName();
            CMethodDecl* pmdecl = elem.second;
            stdstr strName = pmdecl->GetName();
            CCOUT << "this.m_arrDispEvtTable.set( '"
                << strIfName << "::" << strName << "', this."
                << strName << "Wrapper );";
            if( i < vecEvents.size() - 1 )
                NEW_LINE;
        }
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );
    return ret;
}

gint32 CImplJsSvcProxyBase::EmitSvcBaseCli()
{
    gint32 ret = 0;
    do{
        CServiceDecl* psd = m_pNode;
        stdstr strClass = "C";
        strClass += psd->GetName() + "clibase";
        CCOUT << "class " << strClass
            << " extends CInterfaceProxy";
        NEW_LINE;
        BLOCK_OPEN;
        ret = EmitBuildEventTable();
        CCOUT << "constructor( oIoMgr, strObjDescPath, strObjName, oParams )";
        NEW_LINE;
        BLOCK_OPEN;
        CCOUT << "super( oIoMgr, strObjDescPath, strObjName );";
        if( SUCCEEDED( ret ) )
        {
            NEW_LINE;
            CCOUT << "this.BuildEventTable();";
        }
        else
        {
            ret = STATUS_SUCCESS;
        }
        NEW_LINES( 2 );

        std::vector< ObjPtr > vecIfs;
        psd->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        for( int i = 0; i < vecIfs.size(); i++ )
        {
            auto& elem = vecIfs[ i ];
            CInterfRef* pIfRef = elem;
            ObjPtr pObj;
            ret = pIfRef->GetIfDecl( pObj );
            if( ERROR( ret ) )
                break;
            CInterfaceDecl* pifd = pObj;
            ObjPtr pmdlobj = pifd->GetMethodList();
            if( pmdlobj.IsEmpty() )
            {
                ret = -ENOENT;
                break;
            }

            stdstr strIfName = pifd->GetName();
            CMethodDecls* pmdl = pmdlobj;
            for( guint32 j = 0;
                j < pmdl->GetCount(); j++ )
            {
                ObjPtr pmd = pmdl->GetChild( j );
                if( pmd.IsEmpty() )
                {
                    ret = -ENOENT;
                    break;
                }
                CMethodDecl* pmdecl = pmd;
                stdstr strMethod = pmdecl->GetName();
                bool bEvent = pmdecl->IsEvent();
                if( bEvent )
                    strMethod += "Wrapper";
                CCOUT << strClass << ".prototype." << strMethod
                    << " = I" << strIfName << "_CliImpl.prototype."
                    << strMethod << ";";
                if( !bEvent )
                {
                    NEW_LINE;
                    CCOUT << strClass << ".prototype." << strMethod
                        << "CbWrapper = I" << strIfName << "_CliImpl.prototype."
                        << strMethod << "CbWrapper;";
                }
                if( j < pmdl->GetCount()-1 )
                    NEW_LINE;
            }
            if( i < vecIfs.size() - 1 )
                NEW_LINES( 2 );
        }
        BLOCK_CLOSE;
        BLOCK_CLOSE;
        NEW_LINE;

        CCOUT << "exports." << strClass
            << " = " << strClass << ";";
        NEW_LINE;

    }while( 0 );

    return ret;
}

CImplJsIfProxyBase::CImplJsIfProxyBase(
    CJsWriter* pWriter, ObjPtr& pNode )
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

gint32 CImplJsIfProxyBase::Output()
{
    gint32 ret = 0;
    do{
        stdstr strIfName = m_pNode->GetName();
        CCOUT << "class I" << strIfName << "_CliImpl";
        NEW_LINE;
        BLOCK_OPEN;
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
            CImplJsMthdProxyBase oipm(
                m_pWriter, elem );
            oipm.Output();
        }
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplJsSvcProxyBase::Output()
{
    gint32 ret = 0;

    do{
        OUTPUT_BANNER( m_pWriter, m_pNode );

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

            CImplJsIfProxyBase oipb(
                m_pWriter, pObj );

            ret = oipb.Output();
            if( ERROR( ret ) )
                break;
        }

        ret = EmitSvcBaseCli();

    }while( 0 );

    return ret;
}

static gint32 GenSvcFiles(
    CJsWriter* pWriter, ObjPtr& pRoot )
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
                strCommon + "clibase.js" ); 

            CImplJsSvcProxyBase opspb(
                pWriter, elem.second );
            ret = opspb.Output();
            if( ERROR( ret ) )
                break;

            // client imlementation
            ret = pWriter->SelectImplFile(
                strCommon + "cli.js" );
            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    strCommon + "cli.js.new" );
                if( ERROR( ret ) )
                    break;
            }

            CImplJsSvcProxy opsc(
                pWriter, elem.second );
            ret = opsc.Output();
            if( ERROR( ret ) )
                break;/* */
        }

    }while( 0 );

    return ret;
}
gint32 GenJsProj(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot )
{
    if( strOutPath.empty() ||
        strAppName.empty() ||
        pRoot.IsEmpty() )
        return -EINVAL;

    if( g_strWebPath.empty() )
    {
        fprintf( stderr,
            "Error missing the URL to the object description file" );
        return -EINVAL;
    }

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

        CJsWriter oWriter(
            strOutPath, strAppName, pRoot );

        ret = GenStructsFileJs( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

        // ret = GenIndexFile( &oWriter, pRoot );
        // if( ERROR( ret ) )
        //     break;

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
        CJsExportMakefile opemk( &oWriter, pRoot );
        ret = opemk.Output();
        if( ERROR( ret ) )
            break;

        ret = GenSvcFiles( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

        CImplJsMainFunc opmf( &oWriter, pRoot );
        ret = opmf.Output();
        if( ERROR( ret ) )
            break;

        oWriter.SelectReadme();
        CExportJsReadme ordme( &oWriter, pRoot );
        ret = ordme.Output();
        if( ERROR( ret ) )
            break;

        oWriter.SelectWebCfg();
        CExportJsWebpack owp( &oWriter, pRoot );
        ret = owp.Output();
        if( ERROR( ret ) )
            break;

        oWriter.SelectSampleHtml();
        CExportJsSampleHtml ojsh( &oWriter, pRoot );
        ret = ojsh.Output();
        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( SUCCEEDED( ret ) )
        SyncCfg( strOutPath );

    return ret;
}

CImplJsMthdProxyBase::CImplJsMthdProxyBase(
    CJsWriter* pWriter, ObjPtr& pNode )
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

// output the hint for output responses
static gint32 EmitRespList(
    CWriterBase* m_pWriter, CArgList* pOutArgs )
{
    gint32 ret = 0;
    do{
        Wa( "/*" );
        std::vector< std::pair< stdstr, stdstr > > vecArgs;
        ret = GetArgsAndSigsJs( pOutArgs, vecArgs );
        if( ERROR( ret ) )
            break;

        Wa( "* The response parameters:" );

        std::map< char, stdstr >::iterator itr;
        stdstr strType;
        for( int i = 0; i < vecArgs.size(); i++ )
        {
            auto& elem = vecArgs[ i ];
            if( elem.second[ 0 ] != 'O' &&
                elem.second[ 0 ] != '(' &&
                elem.second[ 0 ] != '[' )
            {
                itr = g_mapSig2JsType.find(
                    elem.second[ 0 ] );

                if( itr != g_mapSig2JsType.end() )
                {
                    strType = itr->second;
                }
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
            CCOUT << "* " << elem.first << " : " << strType;

            NEW_LINE;
        }
        Wa( "*/" );

    }while( 0 );

    return ret;
}

gint32 CImplJsMthdProxyBase::OutputSync()
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
            CCOUT << strName << "( oContext, "; 
            ObjPtr pNode = m_pNode;
            EmitFormalArgListJs( m_pWriter, pNode, true );
            Wa( ")" );
            BLOCK_OPEN;
            if( dwOutCount > 0 )
                EmitRespList( m_pWriter, pOutArgs );
            Wa( "var oReq = new CConfigDb2();" );
            Wa( "var osb = new CSerialBase( this );" );
            Wa( "var ret = 0" );
            CArgList* pal = pInArgs;
            bool bDecl = false;
            for( guint32 i = 0; i < dwInCount; i++ )
            {
                // restore if necessary
                CFormalArg* pfa = pal->GetChild( i );
                if( pfa == nullptr )
                    continue;
                CAstNodeBase* pType = pfa->GetType();
                if( pType == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                stdstr strArg = pfa->GetName();
                stdstr strSig = GetTypeSigJs( pType, true );
                if( strSig.find( 'O' ) == stdstr::npos )
                    continue;
                if( !bDecl )
                {
                    bDecl = true;
                    CCOUT << "var oBack;";
                    NEW_LINE;
                }

                if( strSig[ 0 ] == 'O' )
                {
                    CCOUT << "oBack = " << strArg << ";";
                    NEW_LINE;
                    CStructRef* pstr = ObjPtr( pType );
                    ObjPtr pObj;
                    ret = pstr->GetStructType( pObj );
                    if( ERROR( ret ) )
                        break;
                    CStructDecl* pst = pObj;
                    CCOUT << strArg << " = new "
                        << pst->GetName() << "();";
                    NEW_LINE;
                    CCOUT << strArg << ".Restore( oBack );";
                }
                else if( strSig[ 0 ] == '(' )
                {
                    NEW_LINE;
                    CCOUT << "oBack = " << strArg << ";";
                    NEW_LINE;
                    CCOUT << strArg
                        << " = CStructBase.RestoreArray( oBack );";
                }
                else if( strSig[ 0 ] == '[' )
                {
                    NEW_LINE;
                    CCOUT << "oBack = " << strArg << ";";
                    NEW_LINE;
                    CCOUT << strArg
                        << " = CStructBase.RestoreMap( oBack );";
                }
            }
            if( bDecl )
                NEW_LINE;

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
                stdstr strSig = GetTypeSigJs( pType );
                stdstr strArg = pfa->GetName();
                ret = EmitSerialBySigJs(
                    m_pWriter, strArg, strSig );
                if( ERROR( ret ) )
                    break;
            }
            if( ERROR( ret ) )
                break;
            Wa( "var ridlBuf = osb.GetRidlBuf();" );
            Wa( "oReq.Push( {t: EnumTypeId.typeByteArr, v:ridlBuf} );" );
        }
        else
        {
            CCOUT << strName << "( oContext )";
            BLOCK_OPEN;
            if( dwOutCount > 0 )
                EmitRespList( m_pWriter, pOutArgs );
        }
        stdstr strIfName = m_pIf->GetName();

        Wa( "oReq.SetString( EnumPropId.propIfName," );
        CCOUT << "    DBusIfName( '" << strIfName <<"' ) );";
        NEW_LINE;
        Wa( "oReq.SetString( EnumPropId.propObjPath," );
        Wa( "    this.m_strObjPath );" );
        Wa( "oReq.SetString( EnumPropId.propSrcDBusName," );
        Wa( "    this.m_strSender );" );
        Wa( "oReq.SetUint32( EnumPropId.propSeriProto," );
        Wa( "    EnumSeriProto.seriRidl )" );
        Wa( "oReq.SetString( EnumPropId.propDestDBusName," );
        Wa( "    this.m_strDest );" );
        Wa( "oReq.SetString( EnumPropId.propMethodName," );
        CCOUT << "USER_METHOD(\""<< strName << "\") );";
        NEW_LINE;
        Wa( "var oCallOpts = new CConfigDb2();" );

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
            dwKeepAliveSec > 0 )
        {
            CCOUT << "oCallOpts.SetUint32( EnumPropId.propTimeoutSec, " <<
                dwTimeoutSec << " );";
            NEW_LINE;
            CCOUT << "oCallOpts.SetUint32( EnumPropId.propKeepAliveSec, " <<
                dwKeepAliveSec << " );";
            NEW_LINE;
        }
        else
        {
            CCOUT << "oCallOpts.SetUint32( "
                << "EnumPropId.propTimeoutSec, this.m_dwTimeoutSec );";
            NEW_LINE;
            CCOUT << "oCallOpts.SetUint32( "
                << "EnumPropId.propTimeoutSec, this.m_dwKeepAliveSec );";
            NEW_LINE;
        }

        Wa( "oCallOpts.SetUint32( EnumPropId.propCallFlags," );
        Wa( "    EnumCallFlags.CF_ASYNC_CALL |" );
        if( !m_pNode->IsNoReply() )
            Wa( "    EnumCallFlags.CF_WITH_REPLY |" );
        Wa( "    EnumCallFlags.CF_KEEP_ALIVE |" );
        Wa( "    messageType.methodCall );" );
        Wa( "oReq.SetObjPtr(" );
        Wa( "    EnumPropId.propCallOptions, oCallOpts);" );
        Wa( "return this.m_funcForwardRequest(" );
        CCOUT << "    oReq, this."<< strName << "CbWrapper, oContext );";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplJsMthdProxyBase::OutputAsyncCbWrapper()
{
    gint32 ret = 0;
    do{
        CMethodDecl* pmd = m_pNode;
        stdstr strName = pmd->GetName();
        stdstr strIfName = m_pIf->GetName();
        ObjPtr pOutArgs = pmd->GetOutArgs();
        guint32 dwOutCount =
            GetArgCount( pOutArgs );

        CCOUT << strName << "CbWrapper( oContext, oResp )"; 
        NEW_LINE;
        BLOCK_OPEN;
        CCOUT << "try";
        BLOCK_OPEN;
        Wa( "var ret = oResp.GetProperty( EnumPropId.propReturnValue );" );
        Wa( "if( ERROR( ret ) )" );
        BLOCK_OPEN;
        Wa( "if( oContext.m_oCallback === undefined )" );
        CCOUT << "    this."<< strName << "Callback( oContext, ret );";
        NEW_LINE;
        Wa( "else" );
        CCOUT << "    oContext.m_oCallback.bind(this)( oContext, ret );";
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "else" );
        BLOCK_OPEN;
        Wa( "var args = [ oContext, ret ];" );
        if( dwOutCount > 0 )
        {
            Wa( "var buf = Buffer.from( oResp.GetProperty(0) );" );
            Wa( "var osb = new CSerialBase( this );" );
            Wa( "var offset = 0" );
        }

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
            stdstr strSig = GetTypeSigJs( pType );
            ret = EmitDeserialBySigJs(
                m_pWriter, strSig );
            if( ERROR( ret ) )
                break;
            Wa( "if( ret[ 0 ] === null )" );
            CCOUT << "    throw new Error( "
                <<"'Error failed to deserialize response for I"
                << strIfName << "::" << strName << "' );";
            NEW_LINE;
            Wa( "args.push( ret[ 0 ] );" );
            Wa( "offset = ret[ 1 ];" );
            if( i < dwOutCount - 1 )
                NEW_LINE;
        }
        
        Wa( "if( oContext.m_oCallback === undefined )" );
        CCOUT << "    this." << strName << "Callback.apply( this, args );";
        NEW_LINE;
        Wa( "else" );
        CCOUT << "    oContext.m_oCallback.apply( this, args );";
        if( dwOutCount > 0 )
        {
            NEW_LINE;
            CCOUT << "ret = args[ 1 ];";
        }
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "oContext.m_iRet = Int32Value( ret );" );
        Wa( "if( ERROR( ret ) && oContext.m_oReject )" );
        Wa( "    oContext.m_oReject( oContext );" );
        Wa( "else if( oContext.m_oResolve )" );
        Wa( "    oContext.m_oResolve( oContext );" );
        BLOCK_CLOSE;
        NEW_LINE;
        CCOUT << "catch( e )";
        BLOCK_OPEN;
        CCOUT << "console.log( e );";
        NEW_LINE;
        Wa( "oContext.m_iRet = -errno.EFAULT;" );
        Wa( "if( oContext.m_oResolve )" );
        CCOUT << "    oContext.m_oReject( oContext );";
        BLOCK_CLOSE;
        NEW_LINE;
        CCOUT << "return;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplJsMthdProxyBase::OutputEvent()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        stdstr strIfName = m_pIf->GetName();
        CCOUT << strName << "Wrapper" << "("
            << " ridlBuf )";
        NEW_LINE;
        BLOCK_OPEN;
        CCOUT << "try";
        BLOCK_OPEN;
        ObjPtr pInArgs = m_pNode->GetInArgs();
        guint32 dwInCount =
            GetArgCount( pInArgs );
        if( dwInCount == 0 )
        {
            CCOUT << "this." << strName << "();";
        }
        else
        {
            Wa( "var offset = 0;" );
            Wa( "var args = [];" );
            Wa( "var osb = CSerialBase( this );" );
            Wa( "var ret;" );
            Wa( "var buf = ridlBuf;" );
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
                    GetTypeSigJs( pType );

                ret = EmitDeserialBySigJs(
                    m_pWriter, strSig );
                if( ERROR( ret ) )
                    break;

                Wa( "if( ret[ 0 ] === null )" );
                CCOUT << "    throw new Error( "
                    <<"'Error failed to deserialize event for I"
                    << strIfName << "::" << strName << "' );";
                NEW_LINE;
                Wa( "args.push( ret[ 0 ] )" );
                if( i < dwInCount - 1 )
                    Wa( "offset = ret[ 1 ];" );
            }
            CCOUT << "this."<< strName << ".apply( this, args )";
        }
        BLOCK_CLOSE;
        NEW_LINE;
        CCOUT << "catch( e )";
        BLOCK_OPEN;
        CCOUT << "console.log( e );";
        BLOCK_CLOSE;
        NEW_LINE;
        CCOUT << "return;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplJsMthdProxyBase::Output()
{
    gint32 ret = 0;
    if( m_pNode->IsEvent() )
    {
        ret = OutputEvent();
        NEW_LINE;
    }
    else
    {
        ret = OutputSync();
        if( ERROR( ret ) )
            return ret;
        NEW_LINE;
        ret = OutputAsyncCbWrapper();
        NEW_LINE;
    }
    return ret;
}

CImplJsSvcProxy::CImplJsSvcProxy(
    CJsWriter* pWriter, ObjPtr& pNode )
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

gint32 CImplJsSvcProxy::OutputSvcProxyClass()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        stdstr strClass = "C";
        strClass += strName + "_CliImpl";
        CCOUT << "class " << strClass << " extends C" << strName << "clibase";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "constructor( oIoMgr, strObjDescPath, strObjName, oParams )" );
        BLOCK_OPEN;
        CCOUT << "super( oIoMgr, strObjDescPath, strObjName, oParams );";
        if( m_pNode->IsStream() )
        {
            NEW_LINE;
            CCOUT << "this.OnDataReceived = this.OnDataReceivedImpl;";
            NEW_LINE;
            CCOUT << "this.OnStmClosed = this.OnStmClosedImpl;";
        }
        BLOCK_CLOSE;
        NEW_LINES( 2 );
        Wa( "// The following are callbacks and event handlers" );
        Wa( "// which need to be filled out by you" );
        NEW_LINE;

        CArgListUtils oau;
        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        guint32 dwCount = vecIfs.size();
        std::vector< std::pair< ObjPtr, ObjPtr > > vecFuncs;
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
            stdstr strIf = pifd->GetName();

            ObjPtr pmdlsobj = pifd->GetMethodList();

            CMethodDecls* pmdls = pmdlsobj;
            for( guint32 i = 0; i < pmdls->GetCount(); i++ )
            {
                ObjPtr pmd = pmdls->GetChild( i );
                if( pmd.IsEmpty() )
                {
                    ret = -ENOENT;
                    break;
                }
                vecFuncs.push_back({ pObj, pmd });
            }
        }

        for( guint32 i = 0; i < vecFuncs.size(); i++ )
        {
            auto& elem = vecFuncs[ i ];
            CInterfaceDecl* pifd = elem.first;
            CMethodDecl* pmd = elem.second;
            stdstr strMethod = pmd->GetName();
            bool bEvent = pmd->IsEvent();
            if( !bEvent )
                strMethod += "Callback";
            CCOUT << "// " << pifd->GetName()
                << "::" << strMethod;
            NEW_LINE;
            if( bEvent )
            {
                CCOUT << "// event handler";
                NEW_LINE;
                CCOUT << strMethod << "(";
                ObjPtr pInArgs = pmd->GetInArgs();
                guint32 dwInCount =
                    oau.GetArgCount( pInArgs );

                if( dwInCount > 0 )
                {
                    EmitFormalArgListJs(
                        m_pWriter, elem.second, true );
                    Wa( ")" );
                }
                else
                {
                    CCOUT << ")";
                    NEW_LINE;
                }
                BLOCK_OPEN;
                CCOUT << "// add code here";
                BLOCK_CLOSE;
            }
            else
            {
                CCOUT << "// request callback";
                NEW_LINE;

                CCOUT << strMethod << "( oContext, ret";
                ObjPtr pOutArgs = pmd->GetOutArgs();
                guint32 dwOutCount =
                    oau.GetArgCount( pOutArgs );

                if( dwOutCount > 0 )
                {
                    CCOUT << ",";
                    EmitFormalArgListJs(
                        m_pWriter, elem.second, false );
                    Wa( ")" );
                }
                else
                {
                    CCOUT << " )";
                    NEW_LINE;
                }
                BLOCK_OPEN;
                Wa( "// add code here" );
                CCOUT << "// 'oContext' is the parameter passed to this."
                    << pmd->GetName();
                NEW_LINE;
                CCOUT << "// 'ret' is the status code of the request";
                NEW_LINE;
                CCOUT << "// if ret == errno.STATUS_SUCCESS, the following "
                    << "parameters are valid response from the server";
                NEW_LINE;
                CCOUT << "// if ret < 0, the following parameters are undefined";
                BLOCK_CLOSE;
            }
            if( i < vecFuncs.size() - 1 )
                NEW_LINES(2);
        }
        if( m_pNode->IsStream() )
        {
            if( vecFuncs.size() )
                NEW_LINES(2);
            Wa( "/**" );
            Wa( "* callback called when data arrives from the stream channel `hStream`" );
            Wa( "* @param {number} hStream the stream channel the data comes in " );
            Wa( "* @param {Buffer} oBuf the payload in Uint8Array or Buffer" );
            Wa( "*/" );
            Wa( "OnDataReceivedImpl( hStream, oBuf )" );
            BLOCK_OPEN;
            Wa( "// add code here" );
            CCOUT << "// to handle the incoming byte stream";
            BLOCK_CLOSE;
            NEW_LINES( 2 );

            Wa( "/**" );
            Wa( "* callback called when the stream channel `hStream` is closed" );
            Wa( "* @param {number} hStream the stream channel closed ");
            Wa( "*/" );
            Wa( "OnStmClosedImpl( hStream )" );
            BLOCK_OPEN;
            Wa( "// add code here" );
            CCOUT << "// to do some cleanup work if necessary";
            BLOCK_CLOSE;
        }
        BLOCK_CLOSE;
        NEW_LINES( 2 );
        CCOUT << "exports." << strClass
            << " = " << strClass << ";";
        NEW_LINE;
    }while( 0 );

    return ret;
}

gint32 CImplJsSvcProxy::Output()
{
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        OUTPUT_BANNER( m_pWriter, m_pNode );
        CCOUT << "const { C" << strName
            << "clibase } = require ( './"<< strName << "clibase' )";
        NEW_LINE;

        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        ret = OutputSvcProxyClass();

    }while( 0 );

    return ret;
}

CImplJsMainFunc::CImplJsMainFunc(
    CJsWriter* pWriter,
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

gint32 CImplJsMainFunc::Output()
{
    gint32 ret = 0;
    do{
        std::vector< ObjPtr > vecSvcs;
        ret = m_pNode->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
            break;

        m_pWriter->SelectMainFuncFile();

        EMIT_DISCLAIMER;
        CCOUT << "// " << g_strCmdLine;
        NEW_LINE;

        stdstr strLibPath = GetJsLibPath();
        CCOUT << "const { randomInt } = require( '"
            << strLibPath << "/combase/defines' );";
        NEW_LINE;
        CCOUT << "const {CoCreateInstance, RegisterFactory}=require( '"
            << strLibPath <<"/combase/factory' );";
        NEW_LINE;
        CCOUT << "const {CIoManager} = require( '"
            << strLibPath <<"/ipc/iomgr' );";
        NEW_LINES( 2 );
        Wa( "// Start the iomanager" );
        Wa( "globalThis.g_iMsgIdx = randomInt( 0xffffffff );" );
        Wa( "globalThis.CoCreateInstance=CoCreateInstance;" );
        Wa( "globalThis.RegisterFactory=RegisterFactory;" );
        Wa( "globalThis.g_oIoMgr = new CIoManager();" );
        Wa( "globalThis.g_oIoMgr.Start()" );
        NEW_LINE;

        OUTPUT_BANNER( m_pWriter, vecSvcs[ 0 ], false );
        Wa( "// Start the client(s)" );

        for( auto& elem : vecSvcs )
        {
            CServiceDecl* pNode = elem;
            stdstr strName = pNode->GetName();
            CCOUT << "const { C" << strName << "_CliImpl } = "
                << "require( './" << strName << "cli' )";
            NEW_LINE;
        }

        NEW_LINE;
        ret = OutputCli( vecSvcs );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CImplJsMainFunc::EmitProxySampleCode(
    ObjPtr& pSvcObj )
{
    gint32 ret = 0;
    do{
        std::vector< ObjPtr > vecIfs;
        CServiceDecl* pSvc = pSvcObj;
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
                NEW_LINES(2);
                Wa( "/*" );

                stdstr strVar = "o";
                strVar += pSvc->GetName() + "_cli";
                stdstr strMName = pmd->GetName();
                std::vector< std::pair< stdstr, stdstr >> vecArgs;
                ObjPtr pInArgs = pmd->GetInArgs();
                guint32 dwInCount = GetArgCount( pInArgs );
                Wa( "* sample code to make a request" );
                if( dwInCount == 0 )
                {
                    CCOUT << strVar << "." << strMName << "();";
                }
                else
                {
                    ret = GetArgsAndSigsJs( pInArgs, vecArgs );
                    if( ERROR( ret ) )
                        break;

                    CCOUT << strVar << "." << strMName << "(";
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
                            CCOUT << " );";
                    }
                }
                if( !pmd->IsNoReply() )
                {
                    NEW_LINE;
                    CCOUT << "* and the response goes to '" 
                        << strVar << "." << strMName << "Callback'";
                }
                NEW_LINE;
                Wa( "*/" );
                NEW_LINE;
                Wa( "/*" );
                Wa( "* sample code to make a request with promise" );
                CCOUT << "return new Promise( ( resolve, reject )=>";
                BLOCK_OPEN;
                Wa( "var oContext = new Object();" );
                Wa( "oContext.m_oResolve = resolve;" );
                Wa( "oContext.m_oReject = reject;" );
                CCOUT << "return ";
                if( dwInCount == 0 )
                {
                    CCOUT << strVar << "." << strMName
                        << "();";
                }
                else
                {
                    CCOUT << strVar << "." << strMName << "(";
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
                            CCOUT << " );";
                    }
                }
                BLOCK_CLOSE;
                CCOUT << ").then(( oContext)=>";
                BLOCK_OPEN;
                CCOUT << "console.log( 'request " << strMName 
                    << " is done with status ' + oContext.m_iRet );";
                BLOCK_CLOSE;
                CCOUT << ").catch((e)=>";
                BLOCK_OPEN;
                CCOUT << "console.log(e);";
                BLOCK_CLOSE;
                CCOUT << ")";
                NEW_LINE;
                CCOUT << "*/";
                NEW_LINE;
            }
        }while( 0 );
    }while( 0 );
    return ret;
}

gint32 CImplJsMainFunc::OutputCli(
    std::vector< ObjPtr >& vecSvcs )
{
    gint32 ret = 0;
    do{
        if( vecSvcs.empty() )
        {
            Wa( "return undefined;" );
            break;
        }
        bool bSingleSvc = ( vecSvcs.size() == 1 );
        stdstr strReturn = "oProxy";
        if( bSingleSvc )
        {
            CCOUT << "var oProxy = null;";
        }
        else
        {
            strReturn = "arrProxies";
            CCOUT << "var arrProxies = [];";
        }
        NEW_LINE;

        CCOUT << "var strObjDesc = '" << g_strWebPath << "/" << g_strAppName<<"desc.json';";
        NEW_LINE;
        CCOUT << "var strAppName = '" << g_strAppName << "';";
        NEW_LINE;
        for( guint32 i = 0;i < vecSvcs.size(); i++ )
        {
            CServiceDecl* pSvc = vecSvcs[ i ];
            CCOUT << "var str" << pSvc->GetName() << "ObjName = '"
                << pSvc->GetName() << "';";
            NEW_LINE;
            CCOUT << "var oParams" << i << 
                " = globalThis.CoCreateInstance( EnumClsid.CConfigDb2 );";
            NEW_LINE;
            CCOUT << "oParams" << i << ".SetString( "
                << "EnumPropId.propObjInstName, '"
                << pSvc->GetName() << "' );";
            NEW_LINE;

            stdstr strVar = "o";
            strVar += pSvc->GetName() + "_cli";
            CCOUT << "var " << strVar << " = new C"
                << pSvc->GetName() << "_CliImpl( globalThis.g_oIoMgr,";
            NEW_LINE;
            CCOUT << "    strObjDesc, str" << pSvc->GetName()
                << "ObjName, oParams" << i << " );";
            NEW_LINE;

            NEW_LINE;
            Wa( " // start the client object" );
            
            CCOUT << strVar << ".Start().then((retval)=>";
            BLOCK_OPEN;
            Wa( "if( ERROR( retval ) )" );
            BLOCK_OPEN;
            Wa( "console.log( retval );" );
            CCOUT << "return;";
            BLOCK_CLOSE;
            NEW_LINE;
            if( bSingleSvc )
            {
                CCOUT << strReturn << " = " << strVar << ";";
            }
            else
            {
                CCOUT << strReturn << ".push( " << strVar << " );";
            }
            
            if( pSvc->IsStream() )
            {
                NEW_LINE;
                Wa( "/*" );
                Wa( "* sample code to open a stream channel, it" );
                Wa( "* can be done elsewhere when starting is complete" );
                Wa( "var oStmCtx;" );
                CCOUT << "oStmCtx.m_oProxy = " << strVar << ";";
                NEW_LINE;
                CCOUT << "return " << strVar
                    << ".m_funcOpenStream( oStmCtx).then((oCtx)=>";
                BLOCK_OPEN;
                CCOUT << "console.log( 'OpenStream successfully with "
                    << "handle=' + oCtx.m_hStream );";
                NEW_LINE;
                CCOUT << "return Promise.resolve( oCtx );";
                BLOCK_CLOSE;
                CCOUT << ").catch((oCtx)=>";
                BLOCK_OPEN;
                CCOUT << "console.log( 'OpenStream failed with "
                    << "error=' + oCtx.m_iRet );";
                NEW_LINE;
                CCOUT << "return Promise.resolve( oCtx );";
                BLOCK_CLOSE;
                CCOUT << ")";
                NEW_LINE;
                CCOUT << "*/";
            }
            EmitProxySampleCode( vecSvcs[ i ] );
            NEW_LINE;
            CCOUT << "globalThis." << strReturn
                << " = " << strReturn << ";";
            BLOCK_CLOSE;
            CCOUT << ").catch((e)=>";
            BLOCK_OPEN;
            Wa( "console.log( 'Start Proxy failed' );" );
            CCOUT << "return Promise.resolve(e);";
            BLOCK_CLOSE;
            CCOUT << ")";
            NEW_LINE;
        }
        NEW_LINE;

    }while( 0 );
    return ret;
}

extern stdstr g_strLocale;
gint32 CExportJsReadme::Output()
{
    if( g_strLocale == "en" )
        return Output_en();
    if( g_strLocale == "cn" )
        return Output_cn();

    return -ENOTSUP;
}

gint32 CExportJsReadme::Output_en()
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

        CCOUT << "* **maincli.js**: "
            << "Containing creation of the iomanager and client objects, and"
            << "starting the client objects for rpc communications";
        NEW_LINE;
        CCOUT << "And you can make changes to the files to customize the program. "
            << "The `ridlc` will not touch them if they exist in the project directory, "
            << "when it runs again, and put the newly generated code to `maincli.js.new`.";
        NEW_LINES( 2 );

        for( auto& elem : vecSvcNames )
        {
            CCOUT << "* **" << elem << "cli.js**: "
                << "Containing the declarations and definitions of all the client side "
                << "methods that need to be implemented by you, mainly the request/event handlers, "
                << "for service `" << elem << "`.";
            NEW_LINE;
            CCOUT << "And you need to make changes to the files to implement the "
                << "business logics for client. "
                << "The `ridlc` will not touch them if they exist in the project directory, "
                << "when it runs again, and put the newly "
                << "generated code to `"<<elem  <<"cli.js.new`.";
            NEW_LINES( 2 );
        }

        CCOUT<< "* *" << g_strAppName << "structs.js*: "
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
            << "overwritten by `ridlc` and synccfg.js without backup.";
        NEW_LINES( 2 );

        CCOUT << "* *Makefile*: "
            << "The Makefile has 3 targets, `debug`, `release`, and `update`." 
            << "`debug` and `release` will package the project to the `dist` "
            << "directory. **So the `dist` directory, `" << g_strAppName
            << ".html`, and `"<< g_strAppName << "desc.json` are the file set "
            << "to deploy to the web server.** The target 'update' will update "
            << "the content of the json files in this directory with system "
            << "settings.";
        NEW_LINE;
        CCOUT << "And please don't edit it, since it will be "
            << "overwritten by `ridlc` and synccfg.py without backup.";
        NEW_LINES( 2 );

        CCOUT << "* *webpack.config.js*: "
            << "the file is used by webpack to pack all the stuffs to a single"
            << "js file '" << g_strAppName << ".js'" << " for deployment on the "
            << "web server";
        NEW_LINE;
        CCOUT << "And please don't edit it, since it will be "
            << "overwritten by `ridlc` without backup.";
        NEW_LINES( 2 );

        CCOUT << "* *"<< g_strAppName <<".html*: "
            << "This is a sample html file, to demonstrate how the webpage or external caller " 
            << "interact with the client object.";
        NEW_LINE;
        CCOUT << "And if this file is already present, the ridlc will "
            << "generate the new sample html to the `.new` file next time";
        NEW_LINES( 2 );


        for( auto& elem : vecSvcNames )
        {
            CCOUT << "* *"<< elem << "clibase.js* : "
                << "Containing the declarations and definitions of all the client side "
                << "utilities and helpers for the interfaces of service `" << elem << "`.";
            NEW_LINE;
            CCOUT << "And please don't edit them, since they will be "
                << "overwritten by `ridlc` without backup.";
            NEW_LINES( 2 );

        }
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

gint32 CExportJsReadme::Output_cn()
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

        CCOUT << "* **maincli.js**: "
            << "包含建立`iomanager`和各个`proxy`对象, 以及启动`proxy`的代码。"
            << "`ridlc`会留意这个文件作的改动。 当`ridlc`再次编译时, "
            << "如果发现目标目录存在该文件会把新生成的文件名加上.new后缀，避免覆盖原文件。";
        NEW_LINE;
        CCOUT << "你可以对这两个文件作出修改, 不必担心ridlc会冲掉你修改的内容。"
            << "`ridlc`再次编译时，如果发现目标目录存在该文件会把新生成的文件"
            << "名加上.new后缀。";
        NEW_LINES( 2 );

        for( auto& elem : vecSvcNames )
        {
            CCOUT << "* **" << elem << "cli.js**: "
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
            CCOUT << "* *"<< elem << "clibase.js* : "
                << "分别包含有关service `"<<elem
                <<"`的客户端的所有辅助函数和底部支持功能的实现。";
            NEW_LINE;
            CCOUT << "这些函数和方法不要做进一步的修改。"
                << "`ridlc`在下一次运行时会重写里面的内容。";
            NEW_LINES( 2 );

        }

        CCOUT<< "* *" << g_strAppName << "structs.js*: "
            << "包含一个ridl文件中声明的所有用到的struct,"
            << "以及序列/反序列化方法的实现.";
        NEW_LINE;
        CCOUT << "这个文件不要做进一步的修改。"
                << "`ridlc`在下一次运行时会重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT<< "* *" << g_strAppName << "desc.json*: "
            << "包含本应用相关的配置信息, 和所有定义的服务(service)的配置参数。";
        NEW_LINE;
        CCOUT << "这个文件不要做进一步的修改。"
            << "`ridlc`或者`synccfg.py`都会在在下一次运行时重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT << "* *driver.json*: "
            << "包含本应用相关的配置信息,主要是底层的iomanager的配置信息。";
        NEW_LINE;
        CCOUT << "这个文件不要做进一步的修改。"
                << "`ridlc`或者`synccfg.py`都会在在下一次运行时重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT << "* *Makefile*: "
            << "工程文件. 此文件有三个`target`, `update`更新配置， "
            << "`debug` 指示webpack打包整个项目, 保留文件的格式, 便于调试。"
            << "`release`指示webpack以紧凑格式打包整个项目。"
            << "**注意: 执行完make后生成的`dist`目录, `"<< g_strAppName << ".html`"
            << "和`" << g_strAppName << "desc.json`是部署到web服务器的文件集合。**";
        NEW_LINE;
        CCOUT << "这个文件不要做进一步的修改。"
                << "`ridlc`或者`synccfg.py`都会在在下一次运行时重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT << "* *webpack.config.js*: "
            << "该文件用于使用webpack打包整个工程为一个js文件 '" << g_strAppName
            << ".js'用于在web服务器上的部署。";
        NEW_LINE;
        CCOUT << "这个文件不要做进一步的修改。"
                << "`ridlc`会在在下一次运行时重写里面的内容。";
        NEW_LINES( 2 );

        CCOUT << "* *"<< g_strAppName <<".html*: "
            << "该文件是一个例子html, 用于展示如何在页面或者打包后外部调用者如何使用客户端程序"; 
        NEW_LINE;
        CCOUT << "这个文件如果存在的, `ridlc`下一次编译的输出会写入`.new`文件里。";
        NEW_LINES( 2 );


        CCOUT << "* *synccfg.py*: "
            << "一个小的Python脚本, 用来同步本应用配置信息。";
        NEW_LINES(2);
        CCOUT << "**注1**: 上文中的粗体字的文件是需要你进一步修改的文件. 斜体字的文件则不需要。"
            << "如果仍然有修改的必要，请注意这些文件有被`ridlc`或者`synccfg.py`改写的风险。";
        NEW_LINES(2);
        CCOUT << "**注2**: 有关配置系统搭建和设置请参考[此文。](https://github.com/zhiming99/rpc-frmwrk#building-rpc-frmwrk)。";
        NEW_LINE;

   }while( 0 );

   return ret;
}

gint32 CExportJsWebpack::Output()
{
    gint32 ret = 0;
    do{
        Wa( "let path = require( 'path' );" );
        Wa( "const webpack = require( 'webpack' );" );
        Wa( "module.exports = " );
        BLOCK_OPEN;
        Wa( "// replace 'development' with 'production' for a human-unreadable version" );
        Wa( "mode: 'development'," );
        Wa( "entry: './maincli.js'," );
        CCOUT << "resolve:";
        BLOCK_OPEN;
        Wa( "modules: [path.resolve(__dirname," );
        CCOUT << "    'node_modules'), 'node_modules']";
        BLOCK_CLOSE;
        Wa( "," );
        Wa( "output:" );
        BLOCK_OPEN;
        Wa( "path:path.resolve(__dirname, 'dist'),");
        CCOUT << "filename: '" << g_strAppName << ".js',";
        NEW_LINE;
        Wa( "globalObject: 'this'," );
        Wa( "library:" );
        BLOCK_OPEN;
        CCOUT << "name: '" << g_strAppName << "'," ;
        NEW_LINE;
        CCOUT << "type: 'var',";
        BLOCK_CLOSE;
        BLOCK_CLOSE;
        CCOUT << ",";
        NEW_LINE;
        CCOUT << "plugins: [";
        INDENT_UPL;
        Wa( "new webpack.ProvidePlugin(" );
        BLOCK_OPEN;
        CCOUT << "process: 'process/browser',";
        BLOCK_CLOSE;
        CCOUT << "),";
        NEW_LINE;
        Wa( "new webpack.ProvidePlugin(" );
        BLOCK_OPEN;
        CCOUT << "Buffer: ['buffer', 'Buffer'],";
        BLOCK_CLOSE;
        CCOUT << "),";
        INDENT_DOWNL;
        CCOUT << "],";
        BLOCK_CLOSE;

    }while( 0 );
    return ret;
}

gint32 CExportJsSampleHtml::Output()
{
    gint32 ret = 0;
    do{
        std::vector< ObjPtr > vecSvcs;
        ret = m_pNode->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
            break;

        Wa( "<!--" );
        EMIT_DISCLAIMER_INNER( "" );
        NEW_LINE;
        CCOUT << "* command line: " << g_strCmdLine;
        NEW_LINE;
        CCOUT << "* npm dependency: browserify buffer exports minify "
            << "long lz4 process put safe-buffer stream xxhash xxhashjs webpack webpack-cli";
        NEW_LINE;
        Wa( "-->" );
        Wa( "<!DOCTYPE html>" );
        Wa( "<html><head><meta charset=\"utf-8\" /></head></html>" );
        Wa( "<body>" );
        Wa( "<p id=\"log\">Sample HTML<br /></p>" );
        CCOUT << "<script src=\"dist/" << g_strAppName << ".js\">";
        NEW_LINE;
        Wa( "</script>" );
        Wa( "<script>" );
        CCOUT << "console.log( \"hello, " << g_strAppName << "!\");";
        NEW_LINE;
        Wa( "var iChecks = 0;" );
        CCOUT << "function checkProxyState()";
        BLOCK_OPEN;
        stdstr strVar = "oProxy";

        if( vecSvcs.size() > 1 )
            strVar = "arrProxies";
        CCOUT << "if( globalThis." << strVar << " === undefined )";
        BLOCK_OPEN;
        Wa( "setTimeout( checkProxyState, 2000 );" );
        CCOUT << "console.log( \"Waiting RPC connection ready...\" );";
        BLOCK_CLOSE;
        if( vecSvcs.size() == 1 )
        {
            CCOUT << "else if( globalThis." << strVar << ".m_iState === 5 )";
            BLOCK_OPEN;
            CCOUT << "console.log( \"RPC connection is down...\" );";
            NEW_LINE;
            CCOUT << "globalThis.oProxy = null;";
            BLOCK_CLOSE; 
        }
        CCOUT << "else";
        BLOCK_OPEN;
        Wa( "if( iChecks++ === 0 )" );
        CCOUT << "    console.log( \"RPC connection ready \" + globalThis." << strVar << " );";
        NEW_LINE;
        CCOUT << "setTimeout( checkProxyState, 5000 );";
        BLOCK_CLOSE;
        BLOCK_CLOSE;
        NEW_LINE;
        Wa( "setTimeout( checkProxyState, 2000 );" );
        Wa( "//# sourceURL=rpcf://htmlcode/checkstate.js" );
        Wa( "</script>" );
        Wa( "</body>" );

    }while( 0 );
    return ret;
}
