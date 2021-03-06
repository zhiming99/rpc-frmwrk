/*
 * =====================================================================================
 *
 *       Filename:  gencpp.cpp
 *
 *    Description:  implementations of proxy/server generator for C++
 *
 *        Version:  1.0
 *        Created:  02/25/2021 01:34:32 PM
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
#include "rpc.h"
using namespace rpcfrmwrk;
#include "astnode.h"
#include "gencpp.h"
#include "sha1.h"

extern std::string g_strAppName;

#define PROP_ABSTMETHOD_PROXY       0
#define PROP_ABSTMETHOD_SERVER      1
#define PROP_PROXY_METHOD           2
#define PROP_SERVICE_HANDLER        3
#define PROP_EVENT_HANDLER          4

std::string ToStringInArgs( ObjPtr& pArgs )
{
    CArgList* pal = pArgs;
    std::string strVal;
    if( pal == nullptr )
        return strVal;

    do{
        guint32 dwCount = pal->GetCount();
        for( guint32 i = 0; i < dwCount; i++ )
        {
            ObjPtr pObj = pal->GetChild( i );
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
                continue;
            pObj = pfa->GetType();
            guint32 dwClsid = pObj->GetClsid();
            std::string strType;
            if( dwClsid != clsid( CPrimeType ) )
            {
                CAstNodeBase* pType = pObj;
                strType = std::string( "const " ) +
                    pType->ToStringCpp() + "&";
            }
            else 
            {
                CPrimeType* pType = pObj;
                guint32 dwName = pType->GetName();
                if( dwName == TOK_STRING ||
                    dwName == TOK_BYTEARR )
                {
                    strType = std::string( "const " ) +
                        pType->ToStringCpp() + "&";
                }
                else if( dwName == TOK_OBJPTR )
                {
                    strType = pType->ToStringCpp();
                    strType += "&";
                }
                else if( dwName == TOK_HSTREAM )
                {
                    strType = "HANDLE";
                }
                else
                {
                    strType = pType->ToStringCpp();
                }
            }

            strVal +=
                strType + " " + pfa->GetName();

            if( i + 1 < dwCount )
                strVal += ", ";
        }

    }while( 0 );

    return strVal;
}

guint32 GenClsid( const std::string& strName )
{
    SHA1 sha;
    std::string strHash = g_strAppName + strName;
    sha.Input( strHash.c_str(), strHash.size() );

    unsigned char digest[20];
    sha.Result( (unsigned*)digest );
    unsigned char bottom = ( ( ( guint32 )
        clsid( UserClsidStart ) ) >> 24 ) + 1;
    unsigned char ceiling = 0xFF;
    guint32 i = 0;
    for( ; i < sizeof( digest ); ++i )
        if( digest[ i ] >= bottom &&
            digest[ i ] < ceiling )
            break;

    if( sizeof( digest ) - i < sizeof( guint32 ) )
    {
        std::string strMsg = DebugMsg( -1,
            "internal error the SHA1 hash is"
            " conflicting" );
        throw std::runtime_error( strMsg );
    }
    guint32 iClsid = 0;

    memcpy( &iClsid,
        digest + i, sizeof( iClsid ) );

    return iClsid;
}

CFileSet::CFileSet(
    const std::string& strOutPath,
    const std::string& strAppName )
{
    if( strAppName.empty() )
    {
        std::string strMsg =
            "appname is empty";
        throw std::invalid_argument( strMsg );
    }

    m_strAppHeader =
        strOutPath + "/" + strAppName + ".h";

    m_strAppCpp =
        strOutPath + "/" + strAppName + ".cpp";

    m_strObjDesc =
        strOutPath + "/" + strAppName +
        "desc.json";

    m_strDriver =
        strOutPath + "/" + strAppName +
        "drv.json";

    m_strMakefile =
        strOutPath + "/" + "Makefile" ;

    m_strPath = strOutPath;

    gint32 ret = OpenFiles();
    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "internal error open files" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CFileSet::OpenFiles()
{
    STMPTR pstm( new std::ofstream(
        m_strAppHeader,
        std::ofstream::out |
        std::ofstream::trunc ) );

    m_vecFiles.push_back( std::move( pstm ) );

    pstm = STMPTR( new std::ofstream(
        m_strAppCpp,
        std::ofstream::out |
        std::ofstream::trunc) );

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

    return STATUS_SUCCESS;
}

CFileSet::~CFileSet()
{
    for( auto& elem : m_vecFiles )
    {
        if( elem != nullptr )
            elem->close();
    }
    m_vecFiles.clear();
}

gint32 GenHeaderFile(
    CCppWriter* pWriter, ObjPtr& pRoot )
{
    if( pWriter == nullptr ||
        pRoot.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        pWriter->SelectHeaderFile();

        CHeaderPrologue oHeader(
            pWriter, pRoot );

        ret = oHeader.Output();
        if( ERROR( ret ) )
            break;

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
                    CDeclareStruct ods(
                        pWriter, pObj );
                    ret = ods.Output();
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

                    CDeclInterfProxy odip(
                        pWriter, pObj );
                    ret = odip.Output();
                    if( ERROR( ret ) )
                        break;

                    CDeclInterfSvr odis(
                        pWriter, pObj );

                    ret = odis.Output();
                    break;
                }
            case clsid( CServiceDecl ):
                {
                    CDeclService ods(
                        pWriter, pObj );
                    ret = ods.Output();
                    if( ERROR( ret ) )
                        break;

                    CDeclServiceImpl odsi(
                        pWriter, pObj );
                    ret = odsi.Output();
                    break;
                }
            case clsid( CAppName ) :
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

    }while( 0 );

    return ret;
}

gint32 GenCppProj(
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
        CCppWriter oWriter(
            strOutPath, strAppName, pRoot );

        ret = GenHeaderFile( &oWriter, pRoot );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

gint32 CDeclareClassIds::Output()
{
    bool bFirst = true;
    gint32 ret = STATUS_SUCCESS;

    NEW_LINE;
    Wa( "enum EnumMyClsid" );
    BLOCK_OPEN;
    std::vector< ObjPtr > vecSvcs;
    ret = m_pStmts->GetSvcDecls( vecSvcs );
    if( ERROR( ret ) )
    {
        printf(
        "error no service declared\n" );
        return ret;
    }
    std::vector< std::string > vecNames;
    for( auto& elem : vecSvcs )
    {
        CServiceDecl* pSvc = elem;
        vecNames.push_back(
            pSvc->GetName() );
    }
        
    if( vecNames.size() > 1 )
    {
        std::sort( vecNames.begin(),
            vecNames.end() );
    }

    std::string strAppName =
        m_pStmts->GetName();

    std::string strVal = strAppName;
    for( auto& elem : vecNames )
        strVal += elem;

    guint32 dwClsid = GenClsid( strVal );
    for( auto& elem : vecSvcs )
    {
        CServiceDecl* pSvc = elem;
        if( pSvc == nullptr )
            continue;

        std::string strSvcName =
            pSvc->GetName();
        
        if( bFirst )
        {
            CCOUT << "DECL_CLSID( "
                << "C" << strSvcName
                << "_CliSkel"
                << " ) = "
                << dwClsid
                << ";";

            bFirst = false;
        }
        else
        {
            CCOUT << "DECL_CLSID( "
                << "C" << strSvcName
                << "_CliSkel" << " );";
        }

        NEW_LINE;

        CCOUT << "DECL_CLSID( "
            << "C" << strSvcName
            << "_SvrSkel" << " );";

        NEW_LINE;

        CCOUT << "DECL_CLSID( "
            << "C" << strSvcName
            << "_CliImpl" << " );";

        NEW_LINE;

        CCOUT << "DECL_CLSID( "
            << "C" << strSvcName
            << "_SvrImpl" << " );";

        NEW_LINE;
    }

    NEW_LINE;
    std::vector< ObjPtr > vecIfs;
    m_pStmts->GetIfDecls( vecIfs );
    for( auto& elem : vecIfs )
    {
        CInterfaceDecl* pifd = elem;
        if( pifd == nullptr )
            continue;

        std::string strIfName =
            pifd->GetName();
        
        CCOUT << "DECL_IID( "
            << strIfName
            << " );";
        NEW_LINE;
    }
    BLOCK_CLOSE;
    CCOUT << ";";
    NEW_LINES( 2 );

    return ret;
}

CDeclareTypedef::CDeclareTypedef(
    CCppWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'typedef' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CDeclareTypedef::Output()
{
    std::string strVal =
        m_pNode->ToStringCpp();
    if( strVal.empty() )
        return ERROR_FAIL;
    Wa( strVal );
    NEW_LINE;
    return STATUS_SUCCESS;
}

CDeclareStruct::CDeclareStruct(
    CCppWriter* pWriter,
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

gint32 CDeclareStruct::Output()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "struct " ); 
        Wa( strName );
        Wa( ": public CSerialBase" );
        BLOCK_OPEN;
    
        ObjPtr pFileds =
            m_pNode->GetFieldList();

        CFieldList* pfl = pFileds;
        guint32 i = 0;pfl->GetCount();
        guint32 dwCount = pfl->GetCount();
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pfl->GetChild( i );
            CFieldDecl* pfd = pObj;
            if( pfd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            std::string strLine =
                pfd->ToStringCpp();
            CCOUT<< strLine << ";"; 
            NEW_LINE;
        }

        NEW_LINES( 2 );
        // declare two methods to implement
        CCOUT<< "gint32 Serialize(";
        INDENT_UP;
        NEW_LINE;
        CCOUT<< " BufPtr& pBuf ) const;";
        INDENT_DOWN;
        NEW_LINE;
        CCOUT<< "gint32 Deserialize(";
        INDENT_UP;
        NEW_LINE;
        CCOUT<<" BufPtr& pBuf, guint32 dwSize );"; 
        INDENT_DOWN;
        NEW_LINE;

        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

CDeclInterfProxy::CDeclInterfProxy(
    CCppWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "interface declaration" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CDeclInterfProxy::Output()
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
        bool bStream = m_pNode->IsStream();
        if( bStream )
        {
            strBase =
            "CStreamProxyWrapper"; 
        }
        else
        {
            strBase =
            "CAggInterfaceProxy"; 
        }

        INDENT_UP;
        NEW_LINE;
        if( bStream )
            CCOUT<< "public:" << strBase;
        else
            CCOUT<< "public: virtual " << strBase;

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
            CCOUT << "virtbase( pCfg ), "
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
        Wa( "gint32 InitUserFuncs();" );

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
            else if( !pmd->IsAsyncp() )
            {
                ret = OutputSync( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else 
            {
                ret = OutputAsync( pmd );
            }
            NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclInterfProxy::OutputEvent(
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
        Wa( "//TODO: implement me" );
        std::string strDecl =
            std::string( "virtual gint32 " ) +
            strName + "( ";
        if( !pArgs.IsEmpty() )
        {
            strDecl += ToStringInArgs( pArgs );
        }
        strDecl += " )";
        CCOUT << strDecl <<" = 0;";
        pmd->SetAbstDecl( strDecl );
        BufPtr pBuf( true );
        *pBuf = strName;
        if( pmd->IsSerialize() &&
            !pArgs.IsEmpty() )
        { 
            NEW_LINE;
            // extra step the deserialize before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName
                <<"Wrapper( BufPtr& pBuf );";
            *pBuf = strName + "Wrapper";
        }

        NEW_LINE;

        // for InitUserFuncs
        pmd->SetProperty(
            PROP_EVENT_HANDLER, pBuf );

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy::OutputAsync(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();
        BufPtr piuf( true );

        Wa( "//RPC Aync Req Sender" );
        CCOUT << "gint32 " << strName << "( ";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "ObjPtr& context";
        if( !pInArgs.IsEmpty() )
        {
            CCOUT << ", "
                << ToStringInArgs( pInArgs );
        }

        CCOUT << ");";
        INDENT_DOWN;
        NEW_LINE;

        *piuf = strName;
        pmd->SetProperty(
            PROP_PROXY_METHOD, piuf );

        Wa( "//Async callback wrapper" );
        CCOUT << "gint32 " << strName
            << "CbWrapper" << "( ";
        INDENT_UP;

        NEW_LINE;
        CCOUT << "IEventSink* pCallback, ";
        NEW_LINE;
        CCOUT << "IEventSink* pIoReq,";
        NEW_LINE;
        CCOUT << "IConfigDb* pReqCtx );";

        INDENT_DOWN;
        NEW_LINE;

        Wa( "//RPC Async Req callback" );
        Wa( "//TODO: implement me by adding" );
        Wa( "//response processing code" );

        std::string strDecl;
        strDecl += "virtual gint32 " + strName
            + "Callback" + "( ";
        strDecl += "ObjPtr& context, ";
        strDecl += "gint32 iRet";

        if( !pOutArgs.IsEmpty() )
        {
            strDecl += std::string( ", " ) +
                ToStringInArgs( pOutArgs );
        }
        strDecl += ")";
        CCOUT << strDecl << " = 0;";
        pmd->SetAbstDecl( strDecl );
        NEW_LINE;


    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfProxy::OutputSync(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();

        Wa( "//RPC Sync Request Sender" );
        CCOUT << "gint32 " << strName << "( ";

        bool bComma = false;
        if( !pInArgs.IsEmpty() )
        {
            CCOUT << ToStringInArgs( pInArgs );
            bComma = true;
        }
        if( !pOutArgs.IsEmpty() )
        {
            if( bComma )
                CCOUT << ", ";
            CArgList* poutal = pOutArgs;
            CCOUT << poutal->
                ToStringCppRef( true );
        }

        CCOUT << ");";
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

CDeclInterfSvr::CDeclInterfSvr(
    CCppWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "interface declaration" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CDeclInterfSvr::Output()
{
    gint32 ret = 0;
    do{
        std::string strName =
            m_pNode->GetName();

        AP( "class " ); 
        std::string strClass = "I";
        strClass += strName + "_SImpl";
        CCOUT << strClass;
        std::string strBase;
        bool bStream = m_pNode->IsStream();
        if( bStream )
        {
            strBase = "CStreamServerWrapper"; 
        }
        else
        {
            strBase = "CAggInterfaceServer"; 
        }

        INDENT_UP;
        NEW_LINE;
        if( bStream )
            CCOUT<< "public:" << strBase;
        else
            CCOUT<< "public: virtual " << strBase;

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
            CCOUT << "virtbase( pCfg ), "
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
        Wa( "gint32 InitUserFuncs();" );
        Wa( "const EnumClsid GetIid() const" );

        CCOUT<< "{ return iid( "
            << strName <<" ); }";
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
            else if( !pmd->IsAsyncs() )
            {
                ret = OutputSync( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else 
            {
                ret = OutputAsync( pmd );
            }
            NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}

gint32 CDeclInterfSvr::OutputEvent(
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
        if( !pArgs.IsEmpty() )
        {
            CCOUT << ToStringInArgs( pArgs );
        }
        CCOUT << ");";
        NEW_LINE;

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr::OutputSync(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();
        BufPtr piuf( true );

        if( !pInArgs.IsEmpty() ||
            !pOutArgs.IsEmpty() )
        {
            Wa( "//RPC Sync Request Handler Wrapper" );
            CCOUT << "gint32 " << strName
                 << "Wrapper( IEventSink* pCallback";
            if( pmd->IsSerialize() &&
                !pInArgs.IsEmpty() )
            {
                CCOUT << ", BufPtr& pBuf ";
            }
            else
            {
                if( !pInArgs.IsEmpty() )
                {
                    CCOUT << ", ";
                    CCOUT << ToStringInArgs( pInArgs );
                }
            }
            CCOUT << ");"; 
            NEW_LINE;
            *piuf = strName + "Wrapper";
        }

        Wa( "//RPC Sync Request Handler" );
        Wa( "//TODO: implement me" );
        std::string strDecl =
            std::string( "virtual gint32 " )
            + strName + "( ";
        bool bComma = false;
        if( !pInArgs.IsEmpty() )
        {
            strDecl += ToStringInArgs( pInArgs );
            bComma = true;
        }
        if( !pOutArgs.IsEmpty() )
        {
            if( bComma )
                strDecl += ", ";
            CArgList* poutal = pOutArgs;
            strDecl += poutal->
                ToStringCppRef( true );
        }
        strDecl += ")";
        CCOUT << strDecl << " = 0;";
        pmd->SetAbstDecl( strDecl, false );
        NEW_LINE;

        if( piuf->empty() )
            *piuf = strName;

        pmd->SetProperty(
            PROP_SERVICE_HANDLER, piuf );

    }while( 0 );

    return STATUS_SUCCESS;
}

gint32 CDeclInterfSvr::OutputAsync(
    CMethodDecl* pmd )
{
    if( pmd == nullptr )
        return -EINVAL;
    do{
        std::string strName, strArgs;
        strName = pmd->GetName();
        ObjPtr pInArgs = pmd->GetInArgs();
        ObjPtr pOutArgs = pmd->GetOutArgs();
        BufPtr piuf( true );

        if( pmd->IsSerialize() &&
            !pInArgs.IsEmpty() )
        {
            Wa( "//RPC Aync Req Handler wrapper" );
            CCOUT << "gint32 "
                << strName << "Wrapper( "
                << "IEventSink* pCallback"
                << ", BufPtr& pBuf );";
            *piuf = strName + "Wrapper";
        }

        Wa( "//RPC Async Req callback" );
        Wa( "//Call this method when you have" );
        Wa( "//finished the async operation" );
        Wa( "//with all the return value set" );
        Wa( "//or an error code" );

        CCOUT << "virtual gint32 " << strName
            << "Callback" << "( ";

        if( !pOutArgs.IsEmpty() )
        {
            CCOUT << "IEventSink pCallback,";
            CCOUT << "gint32 iRet";
            CCOUT << ", "
                << ToStringInArgs( pOutArgs );
        }
        CCOUT << ");";
        NEW_LINE;

        Wa( "//RPC Aync Req Handler" );
        Wa( "//TODO: adding code to emit your async" );
        Wa( "//operation, keep a copy of pCallback and" );
        Wa( "//return STATUS_PENDING" );
        std::string strDecl =
            std::string( "virtual gint32 " ) +
            strName + "( ";
        strDecl += "IEventSink* pCallback";
        if( !pInArgs.IsEmpty() )
        {
            strDecl += std::string( ", " ) +
                ToStringInArgs( pInArgs );
        }
        strDecl += ")";
        CCOUT << strDecl << " = 0;";
        pmd->SetAbstDecl( strDecl, false );
        NEW_LINE;

        if( piuf->empty() )
            *piuf = strName;

        pmd->SetProperty(
            PROP_SERVICE_HANDLER, piuf );

    }while( 0 );

    return STATUS_SUCCESS;
}

CDeclService::CDeclService(
    CCppWriter* pWriter,
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

gint32 CDeclService::Output()
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
        if( m_pNode->IsStream() )
            Wa( "CStreamProxyAsync," );
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

        CCOUT << "DECLARE_AGGREGATED_PROXY(";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "C" << strSvcName << "_SvrSkel,";
        NEW_LINE;
        if( m_pNode->IsStream() )
            Wa( "CStreamServerAsync," );
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

CDeclServiceImpl::CDeclServiceImpl(
    CCppWriter* pWriter, ObjPtr& pNode )
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

gint32 CDeclServiceImpl::FindAbstMethod(
    std::vector< std::string >& vecMethods,
    bool bProxy )
{
    gint32 ret = 0;
    do{
        std::vector< ObjPtr > vecIfs;
        m_pNode->GetIfRefs( vecIfs );
        if( vecIfs.empty() )
        {
            ret = -ENOENT;
            break;
        }

        for( auto& elem : vecIfs )
        {
            CInterfRef* pifr = elem;
            ObjPtr pifdo;
            ret = pifr->GetIfDecl( pifdo );
            if( ERROR( ret ) )
                break;

            CInterfaceDecl* pifd = pifdo;
            ObjPtr pObj = pifd->GetMethodList();
            CMethodDecls* pmds = pObj;
            guint32 i = 0;
            bool bFirst = true;
            for( ; i < pmds->GetCount(); i++ )
            {
                ObjPtr p = pmds->GetChild( i );
                CMethodDecl* pmd = p;
                if( pmd == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                std::string strDecl =
                    pmd->GetAbstDecl( bProxy );
                if( strDecl.empty() )
                    continue;
                if( bFirst )
                {
                    std::string strComment = "// ";
                    strComment += pifd->GetName();
                    vecMethods.push_back( strComment );
                    bFirst = false;
                }
                vecMethods.push_back( strDecl );
            }
        }

    }while( 0 );

    return ret;
}

gint32 CDeclServiceImpl::Output()
{
    guint32 ret = STATUS_SUCCESS;
    do{
        std::string strSvcName =
            m_pNode->GetName();

        std::vector< std::string > vecMethods;
        ret = FindAbstMethod( vecMethods, true );
        if( ERROR( ret ) )
            break;

        std::string strClass, strBase;
        if( !vecMethods.empty() )
        {
            strClass = "C";
            strClass += strSvcName + "_CliImpl";
            strBase = "C";
            strBase += strSvcName + "_CliSkel";
            CCOUT << "class " << strClass;

            INDENT_UP;
            NEW_LINE;

            CCOUT << "public " << strBase;

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
            CCOUT << "virtbase( pCfg ), "
                << "super( pCfg )";
            INDENT_DOWN;
            NEW_LINE;
            CCOUT << "{}";
            NEW_LINES( 2 );

            if( m_pNode->IsStream() )
            {
                Wa( "/* The following two methods are important for */" );
                Wa( "/* streaming transfer. rewrite them if necessary */" );
                Wa( "gint32 OnStreamReady( HANDLE hChannel )" );
                Wa( "{ return super::OnStreamReady( hChannel ); " );
                Wa( "gint32 OnStmClosing( HANDLE hChannel )" );
                Wa( "{ return super::OnStmClosing( hChannel ); }" );
                NEW_LINE;
            }

            for( auto& elem : vecMethods )
            {
                if( elem[ 0 ] == '/' && elem[ 1 ] == '/' )
                    NEW_LINE;

                CCOUT << elem << ";";
                NEW_LINE;
            }

            BLOCK_CLOSE;
            CCOUT << ";";
            NEW_LINES( 2 );
        }

        vecMethods.clear();
        ret = FindAbstMethod( vecMethods, false );
        if( ERROR( ret ) )
            break;

        if( vecMethods.empty() )
            break;

        strClass = "C";
        strClass += strSvcName + "_SvrImpl";
        strBase = "C";
        strBase += strSvcName + "_SvrSkel";

        CCOUT << "class " << strClass;

        INDENT_UP;
        NEW_LINE;

        CCOUT << "public " << strBase;

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
        CCOUT << "virtbase( pCfg ), "
            << "super( pCfg )";
        INDENT_DOWN;
        NEW_LINE;
        CCOUT << "{}";
        NEW_LINES( 2 );

        if( m_pNode->IsStream() )
        {
            Wa( "/* The following two methods are important for */" );
            Wa( "/* streaming transfer. rewrite them if necessary */" );
            Wa( "gint32 OnStreamReady( HANDLE hChannel )" );
            Wa( "{ return super::OnStreamReady( hChannel ); " );
            NEW_LINE;
            Wa( "gint32 OnStmClosing( HANDLE hChannel )" );
            Wa( "{ return super::OnStmClosing( hChannel ); }" );
            NEW_LINE;
        }

        for( auto& elem : vecMethods )
        {
            if( elem[ 0 ] == '/' && elem[ 1 ] == '/' )
                NEW_LINE;

            CCOUT << elem << ";";
            NEW_LINE;
        }
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINES( 2 );

    }while( 0 );

    return ret;
}
