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
#define PROP_MESSAGE_ID             3

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
            std::string strVarName = pfa->GetName;
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
                if( dwName == TOK_STRING )
                {
                    strType = std::string( "const " ) +
                        pType->ToStringCpp() + "&";
                }
                else if( dwName == TOK_OBJPTR ||
                    dwName == TOK_BYTEARR )
                {
                    strType = pType->ToStringCpp();
                    strType += "&";
                }
                else if( dwName == TOK_HSTREAM )
                {
                    strType = "HANDLE";
                    strVarName += "_h";
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

std::string ToStringOutArgs( ObjPtr& pArgs )
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
            std::string strVarName = pfa->GetName;
            if( dwClsid != clsid( CPrimeType ) )
            {
                CAstNodeBase* pType = pObj;
                strType = pType->ToStringCpp() + "&";
            }
            else 
            {
                CPrimeType* pType = pObj;
                guint32 dwName = pType->GetName();
                if( dwName != TOK_HSTREAM )
                {
                    strType = pType->ToStringCpp() + "&";
                }
                else
                {
                    strType = "HANDLE&";
                    strVarName += "_h";
                }
            }

            strVal +=
                strType + " " + strVarName;

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

gint32 CFileSet::AddSvcImpl(
    const std::string& strSvcName )
{
    if( strSvcName.empty() )
        return -EINVAL;

    gint32 idx = m_vecFiles.size();
    std::string strCpp = m_strPath +
        "/" + strSvcName + ".cpp";

    std::string strHeader = m_strPath +
        "/" + strSvcName + ".h";

    STMPTR pstm( new std::ofstream(
        strHeader,
        std::ofstream::out |
        std::ofstream::trunc ) );

    m_vecFiles.push_back( std::move( pstm ) );
    m_mapSvcImp[ strSvcName + ".h" ] = idx++;

    pstm = STMPTR( new std::ofstream(
        strCpp,
        std::ofstream::out |
        std::ofstream::trunc) );

    m_mapSvcImp[ strSvcName + ".cpp" ] = idx++;
    return 0;
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
            ret = pWriter->SelectImplFile(
                elem.first + ".h" ); 

            if( ERROR( ret ) )
                break;

            CDeclServiceImpl osi( pWriter, elem.second );
            ret = osi.Output();
        }

    }while( 0 );

    return ret;
}

gint32 GenCppFile(
    CCppWriter* pWriter, ObjPtr& pRoot )
{
    if( pWriter == nullptr ||
        pRoot.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        pWriter->SelectCppFile();

        CStatements* pStmts = pRoot;
        if( pStmts == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        std::string strName = pStmts->GetName();

        Wa( "//Generated by ridlc" );
        Wa( "//This file content could be cleared by ridlc" );
        Wa( "#include \"seribase.h\"" );
        CCOUT << "#include \"" << strName << ".h\";
        NEW_LINE;
        CCOUT << "using namespace rpcfrmwrk;";
        NEW_LINES( 2 );

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
                    CImplSerialStruct oiss(
                        pWriter, pObj );
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

                    CImplIufProxy oiufp(
                        pWriter, pObj );
                    oiufp.Output();

                    CInterfaceDecl* pifd = pObj;
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
                        CImplIfMethodProxy* oiimp(
                            pWriter, pmd );
                        oiimp.Output();
                    }

                    CImplIufSvr oiufs(
                        pWriter, pObj );
                    oiufs.Output();

                    for( guint32 i = 0;
                        i < pmdl->GetCount(); i++ )
                    {
                        ObjPtr pmd = pmdl->GetChild( i );
                        CImplIfMethodSvr* oiims(
                            pWriter, pmd );
                        oiims.Output();
                    }

                    break;
                }
            case clsid( CServiceDecl ):
            case clsid( CTypedefDecl ):
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
        Wa( ": public CStructBase" );
        BLOCK_OPEN;
    
        ObjPtr pFileds =
            m_pNode->GetFieldList();

        CFieldList* pfl = pFileds;
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

        Wa( "//Message identity" );
        CCOUT << "guint32 m_dwMsgId = "
            << dwMsgId << ";";
        NEW_LINE;

        CCOUT << "std::string m_strMsgId = "
            << "\"" << strMsgId << "\";";
        NEW_LINE;

        BufPtr pMsgId( true );
        *pMsgId = dwMsgId;
        m_pNode->SetProperty(
            PROP_MESSAGE_ID, pMsgId );

        Wa( "// data members" );
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
            std::string strLine =
                pfd->ToStringCpp();
            CCOUT<< strLine << ";"; 
            NEW_LINE;
        }

        NEW_LINE;
        Wa( "// methods" );
        // declare two methods to implement
        CCOUT<< "gint32 Serialize(";
        INDENT_UP;
        NEW_LINE;
        CCOUT<< " BufPtr& pBuf ) const override;";
        INDENT_DOWN;
        NEW_LINE;
        CCOUT<< "gint32 Deserialize(";
        INDENT_UP;
        NEW_LINE;
        CCOUT<<" BufPtr& pBuf, guint32 dwSize ) override;"; 
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
        guint32 dwCount = 0;
        std::string strArgList;
        if( !pArgs.IsEmpty() )
        {
            strArgList = ToStringInArgs( pArgs );
            strDecl += strArgList;
            CArgList* pinal = pArgs;
            dwCount = pinal->GetCount();
        }
        strDecl += " )";
        CCOUT << strDecl <<" = 0;";
        pmd->SetAbstDecl( strDecl );
        if( pmd->IsSerialize() && dwCount > 0 )
        { 
            NEW_LINE;
            // extra step the deserialize before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName
                <<"Wrapper( IEventSink* pCallback, "
                << "BufPtr& pBuf );";
        }
        else if( dwCount == 0 )
        {
            NEW_LINE;
            // extra step the deserialize before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName
                <<"Wrapper( IEventSink* pCallback );"
        }
        else if( !pmd->IsSerialize() )
        {
            NEW_LINE;
            // extra step the deserialize before
            // calling event handler.
            Wa( "//RPC event handler wrapper" );
            CCOUT << "gint32 " << strName
                <<"Wrapper( IEventSink* pCallback, "
                << strArgList << " ;";
        }

        NEW_LINE;

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
        
        CArgList* pinal = pInArgs;
        CArgList* poutal = pOutArgs;
        guint dwInCount = 0;
        guint dwOutCount = 0;

        if( pinal != nullptr )
            dwInCount = pinal->GetCount();

        if( poutal != nullptr )
            dwOutCount = poutal->GetCount();

        Wa( "//RPC Aync Req Sender" );
        CCOUT << "gint32 " << strName << "( ";
        INDENT_UP;
        NEW_LINE;

        CCOUT << "IConfigDb* context";
        if( dwInCount > 0 )
        {
            CCOUT << ", "
                << ToStringInArgs( pInArgs );
        }

        CCOUT << ");";
        INDENT_DOWN;

        if( pmd->IsSerialize() &&
            !pInArgs.IsEmpty() )
        {
            NEW_LINE;
            CCOUT << "gint32" << strName
                << "Dummy( BufPtr& pBuf )";
            NEW_LINE;
            Wa( "{ return STATUS_SUCCESS; }" );
        }

        NEW_LINES;

        Wa( "//Async callback wrapper" );
        CCOUT << "gint32 " << strName
            << "CbWrapper" << "( ";
        INDENT_UP;

        NEW_LINE;
        Wa( "IEventSink* pCallback, " );
        Wa( "IEventSink* pIoReq," );
        Wa( "IConfigDb* pReqCtx );" );

        INDENT_DOWN;
        NEW_LINES( 2 );

        Wa( "//RPC Async Req callback" );
        Wa( "//TODO: implement me by adding" );
        Wa( "//response processing code" );

        std::string strDecl;
        strDecl += "virtual gint32 " + strName
            + "Callback" + "( ";
        strDecl += "IConfigDb* context, ";
        strDecl += "gint32 iRet";

        if( !pOutArgs.IsEmpty() )
        {
            strDecl += std::string( ", " ) +
                ToStringInArgs( pOutArgs );
        }
        strDecl += ")";
        CCOUT << strDecl << " = 0;";
        pmd->SetAbstDecl( strDecl );
        NEW_LINES( 2 );

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
            CCOUT << ToStringOutArgs( pOutArgs );
        }

        CCOUT << ");";

        if( pmd->IsSerialize() &&
            !pInArgs.IsEmpty() )
        {
            CCOUT << "gint32" << strName
                << "Dummy( BufPtr& pBuf )";
            NEW_LINE;
            Wa( "{ return STATUS_SUCCESS; }" );
        }
        NEW_LINES( 2 );

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
        NEW_LINE;
        Wa( "const EnumClsid GetIid() const" );
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
                ret = OutputSync( pmd );
                if( ERROR( ret ) )
                    break;
            }
            else 
            {
                ret = OutputAsync( pmd );
            }
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

        CArgList* pinal = pInArgs;
        CArgList* poutal = pOutArgs;
        guint32 dwInCount = 0;
        guint32 dwOutCount = 0;

        if( pinal != nullptr )
            dwInCount += pinal->GetCount();

        if( poutal != nullptr )
            dwOutCount += poutal->GetCount();

        Wa( "//RPC Sync Request Handler Wrapper" );
        CCOUT << "gint32 " << strName
             << "Wrapper( IEventSink* pCallback";

        if( pmd->IsSerialize() &&
            dwInCount > 0 )
        {
            CCOUT << ", BufPtr& pBuf ";
        }
        else if( dwInCount > 0 )
        {
            CCOUT << ", ";
            CCOUT << ToStringInArgs( pInArgs );
        }

        CCOUT << ");"; 
        NEW_LINES( 2 );

        Wa( "//RPC Sync Request Handler" );
        Wa( "//TODO: implement me" );
        std::string strDecl =
            std::string( "virtual gint32 " )
            + strName + "( ";
        bool bComma = false;
        if( dwInCount > 0 )
        {
            strDecl += ToStringInArgs( pInArgs );
            bComma = true;
        }
        if( dwOutCount > 0 )
        {
            if( bComma )
                strDecl += ", ";
            strDecl += ToStringOutArgs( pOutArgs );
        }
        strDecl += ")";
        CCOUT << strDecl << " = 0;";
        pmd->SetAbstDecl( strDecl, false );
        NEW_LINES( 2 );

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

        CArgList* pinal = pInArgs;
        CArgList* poutal = pOutArgs;
        guint32 dwInCount = 0;
        guint32 dwOutCount = 0;

        if( pinal != nullptr )
            dwInCount += pinal->GetCount();

        if( poutal != nullptr )
            dwOutCount += poutal->GetCount();

        if( pmd->IsSerialize() && dwInCount > 0 )
        {
            Wa( "//RPC Aync Req Handler wrapper" );
            CCOUT << "gint32 "
                << strName << "Wrapper( "
                << "IEventSink* pCallback"
                << ", BufPtr& pBuf );";
            NEW_LINES( 2 );
        }

        Wa( "//RPC Async Req callback" );
        Wa( "//Call this method when you have" );
        Wa( "//finished the async operation" );
        Wa( "//with all the return value set" );
        Wa( "//or an error code" );

        CCOUT << "virtual gint32 " << strName
            << "Callback" << "( ";

        CCOUT << "IEventSink* pCallback, "
            << "gint32 iRet";

        if( dwInCount > 0 )
        {
            CCOUT << ", "
                << ToStringInArgs( pOutArgs );
        }
        CCOUT << " );";
        NEW_LINES( 2 );

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
        if( !pOutArgs.IsEmpty() )
        {
            strDecl += std::string( ", " ) +
                ToStringOutArgs( pOutArgs );
        }
        strDecl += ")";
        CCOUT << strDecl << " = 0;";
        pmd->SetAbstDecl( strDecl, false );
        NEW_LINES( 2 );

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

        std::vector< std::string > vecPMethods;
        ret = FindAbstMethod( vecPMethods, true );
        if( ERROR( ret ) )
            break;

        std::vector< std::string > vecSMethods;
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

        std::string strClass, strBase;
        if( !vecPMethods.empty() )
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
                Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
                Wa( "{ return super::OnStreamReady( hChannel ); " );
                NEW_LINE;
                Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
                Wa( "{ return super::OnStmClosing( hChannel ); }" );
            }

            for( auto& elem : vecPMethods )
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

        if( vecSMethods.empty() )
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
            Wa( "gint32 OnStreamReady( HANDLE hChannel ) override" );
            Wa( "{ return super::OnStreamReady( hChannel ); " );
            NEW_LINE;
            Wa( "gint32 OnStmClosing( HANDLE hChannel ) override" );
            Wa( "{ return super::OnStmClosing( hChannel ); }" );
            NEW_LINE;
        }

        for( auto& elem : vecSMethods )
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

CImplSerialStruct::CImplSerialStruct(
    CCppWriter* pWriter, ObjPtr& pNode )
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

gint32 CImplSerialStruct::OutputSerial()
{
    gint32 ret = 0;
    do{
        CCOUT << "gint32 " << m_pNode->GetName()
            << "::" << "Serialize( BufPtr& pBuf )";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "if( pBuf.IsEmpty() )" );
        INDENT_UP;
        Wa( "return -EINVAL;" );
        INDENT_DOWN;
        Wa( "gint32 ret = 0;" );
        CCOUT << "do";
        BLOCK_OPEN;

        ObjPtr pFileds =
            m_pNode->GetFieldList();

        CFieldList* pfl = pFileds;
        guint32 i = 0;
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

            pObj = pfd->GetType();
            std::string strName = pfd->GetName();
            CAstNodeBase* pTypeNode = pObj;

            std::string strSig =
                pTypeNode->GetSignature();

            if( strSig.empty() )
            {
                ret = -EINVAL;
                break;
            }

            CCOUT << "ret = Serialize( pBuf, m_dwMsgId );";
            NEW_LINE;
            Wa( "if( ERROR( ret ) ) break;" )

            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    CCOUT << "ret = SerializeArray( "
                        << "pBuf, " << strName << ", \""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case '[' :
                {
                    CCOUT << "ret = SerializeMap( "
                        << "pBuf, " << strName << ", \""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }

            case 'O' :
                {
                    CCOUT << "ret = SerialStruct( "
                        << "pBuf, " << strName << " );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'h':
                {
                    CCOUT << "ret = this->" << strName
                        <<".Serialize( pBuf );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    NEW_LINE;
                    break;
                }
            case 'Q':
            case 'q':
            case 'D':
            case 'd':
            case 'W':
            case 'w':
            case 'f':
            case 'D':
            case 'b':
            case 'B':
            case 'h':
            case 's':
            case 'a':
            case 'o':
                {
                    CCOUT << "ret = this->Serialize"
                        << "( pBuf, "
                        << strName << " );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    NEW_LINE;
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
        }

        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINE;

        Wa( "return ret" );

        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

gint32 CImplSerialStruct::OutputDeserial()
{
    gint32 ret = 0;
    do{
        CCOUT << "gint32 " << m_pNode->GetName()
            << "::" << "Deserialize(";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "BufPtr& pBuf, guint32 dwSize )";
        INDENT_DOWN;
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "if( pBuf.IsEmpty() )" );
        INDENT_UP;
        Wa( "return -EINVAL;" );
        INDENT_DOWN;
        Wa( "gint32 ret = 0;" );
        CCOUT << "do";
        BLOCK_OPEN;

        ObjPtr pFileds =
            m_pNode->GetFieldList();

        CFieldList* pfl = pFileds;
        guint32 i = 0;
        guint32 dwCount = pfl->GetCount();
        if( dwCount == 0 )
        {
            ret = -ENOENT;
            break;
        }

        Wa( "guint32 dwMsgId = 0;" );
        CCOUT << "ret = Deserialize( pBuf, dwMsgId );";
        NEW_LINE;
        Wa( "if( ERROR( ret ) ) return ret;" );
        Wa( "if( m_dwMsgId != dwMsgId ) return -EINVAL;" );

        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pfl->GetChild( i );
            CFieldDecl* pfd = pObj;
            if( pfd == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            pObj = pfd->GetType();
            std::string strName = pfd->GetName();
            CAstNodeBase* pTypeNode = pObj;

            std::string strSig =
                pTypeNode->GetSignature();

            if( strSig.empty() )
            {
                ret = -EINVAL;
                break;
            }

            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    CCOUT << "ret = DesrialArray( "
                        << "pBuf, " << strName << ", \""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case '[' :
                {
                    CCOUT << "ret = DeserialMap( "
                        << "pBuf, " << strName << ", \""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }

            case 'O' :
                {
                    CCOUT << "ret = DeserialStruct( "
                        << "pBuf, " << strName << " );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'h':
                {
                    CCOUT << "ret = this->"
                        << strName
                        <<".Deserialize( pBuf );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    NEW_LINE;
                    break;
                }
            case 'Q':
            case 'q':
            case 'D':
            case 'd':
            case 'W':
            case 'w':
            case 'f':
            case 'D':
            case 'b':
            case 'B':
            case 'h':
            case 's':
            case 'a':
            case 'o':
                {
                    CCOUT << "ret = this->Deserialize"
                        << "( pBuf, "
                        << strName << " );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    NEW_LINE;
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
        }

        BLOCK_CLOSE;
        CCOUT << "while( 0 );";
        NEW_LINE;

        Wa( "return ret" );

        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

gint32 CImplSerialStruct::Output()
{
    gint32 ret = OutputSerial();
    if( ERROR( ret ) )
        return ret;
    NEW_LINE;
    ret = OutputDeserial();
    if( ERROR( ret ) )
        return ret;
    NEW_LINE;
    return ret;
}

CImplIufProxy::CImplIufProxy(
    CCppWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplIufProxy::Output()
{
    gint32 ret = 0;
    do{
        std::string strClass = "I";
        strClass += pNode->GetName() + "_PImpl";
        std::string strIfName = pNode->GetName();
        CCOUT << "gint32 " << strClass
            << "::" << "InitUserFuncs()";
        NEW_LINE;
        BLOCK_OPEN;

        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 i = 0;
        pmds->GetCount();
        guint32 dwCount = pmds->GetCount();

        std::vector< ObjPtr > vecEvent;
        std::vector< ObjPtr > vecReq;

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
                vecEvent.push_back( pObj );
            }
            else 
            {
                vecReq.push_back( pObj );
            }
        }

        if( !vecReq.empty() )
        {
            CCOUT << "BEGIN_IFPROXY_MAP( "
                << strIfName << ", false );";
            NEW_LINE;
            for( auto& elem : vecReq )
            {
                CMethodDecl* pmd = elem;
                std::string strMethod = pmd->GetName();

                ObjPtr pObj = pmd->GetInArgs();
                CArgList* pal = pObj;
                guint32 dwCount = 0;
                if( pal == nullptr )
                    dwCount = 0;
                else
                    dwCount = pal->GetCount();
                
                if( dwCount == 0 )
                {
                    CCOUT << "ADD_USER_PROXY_METHOD_EX( 0,";
                    INDENT_UP;
                    NEW_LINE;
                    CCOUT << strClass << "::"
                        << strMethod << ",";
                }
                else if( pmd->IsSerialize() )
                {
                    CCOUT << "ADD_USER_PROXY_METHOD_EX( 1,";
                    INDENT_UP;
                    NEW_LINE;
                    CCOUT << strClass << "::"
                        << strMethod << "Dummy,";
                }
                else
                {
                    CCOUT << "ADD_USER_PROXY_METHOD_EX( "
                        << dwCount;
                    INDENT_UP;
                    NEW_LINE;
                    CCOUT << strClass << "::"
                        << strMethod;
                }
                CCOUT << "\"" << strMethod << "\" );";
                INDENT_DOWN;
                NEW_LINE;
            }

            Wa( "END_IFPROXY_MAP;" );
        }

        if( !vecEvent.empty() )
        {
            CCOUT << "BEGIN_IFHANDLER_MAP( "
                << strIfName << " );";
            NEW_LINE;
            for( auto& elem : vecEvent )
            {
                CMethodDecl* pmd = elem;
                std::string strMethod = pmd->GetName();
                
                CCOUT << "ADD_USER_EVENT_HANDLER(";
                INDENT_UP;
                NEW_LINE;

                // either serializable or not, we
                // will use a wrapper to remove
                // `IEventSink' before calling the
                // true event handler
                CCOUT << strClass << "::"
                    << strMethod << "Wrapper,";

                NEW_LINE;
                CCOUT << "\"" << strMethod << "\" );";
                INDENT_DOWN;
                NEW_LINE;
            }

            Wa( "END_IFHANDLER_MAP;" );
        }

        Wa( "return STATUS_SUCCESS;" );
        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

CImplIufSvr::CImplIufSvr(
    CCppWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplIufSvr::Output()
{
    gint32 ret = 0;
    do{
        std::string strClass = "I";
        strClass += pNode->GetName() + "_SImpl";
        std::string strIfName = pNode->GetName();
        CCOUT << "gint32 " << strClass
            << "::" << "InitUserFuncs()";
        NEW_LINE;
        BLOCK_OPEN;

        ObjPtr pMethods =
            m_pNode->GetMethodList();

        CMethodDecls* pmds = pMethods;
        guint32 i = 0;
        pmds->GetCount();
        guint32 dwCount = pmds->GetCount();

        std::vector< ObjPtr > vecHandlers;

        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = pmds->GetChild( i );
            vecHandlers.push_back( pObj );
        }

        if( !vecHandlers.empty() )
        {
            CCOUT << "BEGIN_IFHANDLER_MAP( "
                << strIfName << " );";
            NEW_LINE;
            for( auto& elem : vecHandlers )
            {
                CMethodDecl* pmd = elem;
                std::string strMethod = pmd->GetName();

                ObjPtr pObj == pmd->GetInArgs();
                CArgList* pinal = pObj; 

                pObj == pmd->GetOutArgs();
                CArgList* poutal = pObj;

                guint32 dwCount = 0;
                if( !pmd->IsAsyncs() )
                {
                    if( pinal != nullptr )
                        dwCount += pinal->GetCount();
                    if( poutal != nullptr )
                        dwCount += poutal->GetCount();
                }
                else
                {
                    if( pinal == nullptr )
                        dwCount = 0;
                    else
                        dwCount = pinal->GetCount();
                }
                
                if( pmd->IsAsyncs() )
                {
                    if( dwCount == 0 )
                    {
                        CCOUT << "ADD_USER_SERVICE_HANDLER(";
                        INDENT_UP;
                        NEW_LINE;
                        CCOUT << strClass << "::"
                            << strMethod << ",";
                    }
                    else if( pmd->IsSerialize() )
                    {
                        CCOUT << "ADD_USER_SERVICE_HANDLER(";
                        INDENT_UP;
                        NEW_LINE;
                        CCOUT << strClass << "::"
                            << strMethod << "Wrapper,";
                    }
                    else
                    {
                        dwInCount = pinal->GetCount();
                        CCOUT << "ADD_USER_SERVICE_HANDLER_EX("
                            << dwInCount << ",";
                        INDENT_UP;
                        NEW_LINE;
                        CCOUT << strClass << "::"
                            << strMethod << ",";
                    }
                }
                else
                {
                    CCOUT << "ADD_USER_SERVICE_HANDLER(";
                    INDENT_UP;
                    NEW_LINE;
                    CCOUT << strClass << "::"
                        << strMethod << "Wrapper,";
                }
                CCOUT << "\"" << strMethod << "\" );";
                INDENT_DOWN;
                NEW_LINE;
            }

            Wa( "END_IFHANDLER_MAP;" );
        }


        Wa( "return STATUS_SUCCESS;" );
        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

CEmitSerialCode::CEmitSerialCode(
    CCppWriter* pWriter, ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CEmitSerialCode::OutputSerial(
    const std::string& strObj,
    const std::string strBuf )
{
    gint32 ret = 0;
    do{

        guint32 i = 0;
        guint32 dwCount = m_pArgs->GetCount();
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = m_pArgs->GetChild( i );
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            pObj = pfa->GetType();
            std::string strName = pfa->GetName();
            CAstNodeBase* pTypeNode = pObj;

            std::string strSig =
                pTypeNode->GetSignature();

            if( strSig.empty() )
            {
                ret = -EINVAL;
                break;
            }

            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    CCOUT << "ret = " << strObj
                        << ".SerializeArray( "
                        << strBuf << ", " << strName << ", \""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case '[' :
                {
                    CCOUT << "ret = "<< strObj
                        << ".SerializeMap( "
                        << strBuf << ", " << strName << ", \""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }

            case 'O' :
                {
                    CCOUT << "ret = " << strObj
                        << ".SerialStruct( "
                        << strBuf << ", " << strName << " );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'h':
                {
                    CCOUT << "ret = " << strName
                        <<".Serialize( " << strBuf << " );";

                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    NEW_LINE;
                    break;
                }
            case 'Q':
            case 'q':
            case 'D':
            case 'd':
            case 'W':
            case 'w':
            case 'f':
            case 'D':
            case 'b':
            case 'B':
            case 'h':
            case 's':
            case 'a':
            case 'o':
                {
                    CCOUT << "ret = " << strObj
                        << ".Serialize( " << strBuf << ", "
                        << strName << " );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    NEW_LINE;
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
        }

    }while( 0 );
}

gint32 CEmitSerialCode::OutputDeserial(
    const std::string& strObj,
    const std::string strBuf )
{
    gint32 ret = 0;
    do{
        guint32 i = 0;
        guint32 dwCount = m_pArgs->GetCount();
        for( ; i < dwCount; i++ )
        {
            ObjPtr pObj = m_pArgs->GetChild( i );
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            pObj = pfa->GetType();
            std::string strName = pfa->GetName();
            CAstNodeBase* pTypeNode = pObj;

            std::string strSig =
                pTypeNode->GetSignature();

            if( strSig.empty() )
            {
                ret = -EINVAL;
                break;
            }

            switch( strSig[ 0 ] )
            {
            case '(' :
                {
                    CCOUT << "ret = " << strObj << ".DesrialArray( "
                        << strBuf << ", " << strName << ", \""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case '[' :
                {
                    CCOUT << "ret = " << strObj << ".DeserialMap( "
                        << strBuf << ", " << strName << ", \""
                        << strSig << "\" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }

            case 'O' :
                {
                    CCOUT << "ret = " << strObj << ".DeserialStruct( "
                        << strBuf << ", " << strName << " );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    break;
                }
            case 'h':
                {
                    CCOUT << "ret = " << strName
                        <<".Deserialize( " << strBuf <<" );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    NEW_LINE;
                    break;
                }
            case 'Q':
            case 'q':
            case 'D':
            case 'd':
            case 'W':
            case 'w':
            case 'f':
            case 'D':
            case 'b':
            case 'B':
            case 'h':
            case 's':
            case 'a':
            case 'o':
                {
                    CCOUT << "ret = " << strObj << ".Deserialize"
                        << "( " << strBuf << ", "
                        << strName << " );";
                    NEW_LINE;
                    Wa( "if( ERROR( ret ) ) break;" );
                    NEW_LINE;
                    break;
                }
            default:
                {
                    ret = -EINVAL;
                    break;
                }
            }
        }
    }while( 0 );
}

CImplIfMethodProxy::CImplIfMethodProxy(
    CCppWriter* pWriter,
    ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }

    CAstNodeBase* pParent = pNode->GetParent();
    if( pParent == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'method_decls' node" );
        throw std::runtime_error( strMsg );
    }
    pParent = pParent->GetParent();
    if( pParent == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }

    m_pIf = ObjPtr( pParent );
    if( m_pIf == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }
};


gint32 CImplIfMethodProxy::Output()
{
    gint32 ret = 0;

    do{
        NEW_LINE;

        if( m_pNode->IsEvent() )
        {
            ret = OutputEvent();
            break;
        }
        else if( m_pNode->IsAsyncp() )
        {
            ret = OutputAsync();
            if( ERROR( ret ) )
                break;

            ret = OutputAsyncCbWrapper();
        }
        else
        {
            ret = OutputSync();
        }

        NEW_LINE;

    }while( 0 );

    return ret;
}

static gint32 GenLocals( ObjPtr pArgList,
    std::vector< std::string >& vecLocals )
{
    CArgList* pinal = pInArgs;
    if( pinal == nullptr )
        return -EINVAL;
   
    gint32 ret = 0; 

    do{
        guint32 i = 0;
        for( ; i < pinal->GetCount(); i++ )
        {
            ObjPtr pObj = pinal->GetChild();
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pObj = pfa->GetType();
            CAstNodeBase* pNode = pObj;
            std::string strType =
                pNode->ToStringCpp();

            std::string strLocal =
                pfa->GetName();

            strType += " ";
            vecLocals.push_back( strType );
        }

    }while( 0 )

    return ret;
}

static gint32 GetHstream( ObjPtr& pArgList,
    std::vector< ObjPtr >& vecHstms )
{
    CArgList* pinal = pInArgs;
    if( pinal == nullptr )
        return -EINVAL;
   
    gint32 ret = 0; 

    do{
        guint32 i = 0;
        for( ; i < pinal->GetCount(); i++ )
        {
            ObjPtr pObj = pinal->GetChild();
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pObj = pfa->GetType();
            CAstNodeBase* pNode = pObj;
            std::string strSig =
                pNode->GetSignature();

            if( strSig[ 0 ] == 'h' )
            {
                vecHstms.push_back(
                    ObjPtr( pfa ) );
            }
        }

    }while( 0 )

    return ret;
}

static gint32 GetArgsForCall( ObjPtr& pArgList,
    std::vector< std::string >& vecArgs ) 
{
    CArgList* pinal = pInArgs;
    if( pinal == nullptr )
        return -EINVAL;
   
    gint32 ret = 0; 

    do{
        guint32 i = 0;
        for( ; i < pinal->GetCount(); i++ )
        {
            ObjPtr pObj = pinal->GetChild();
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

            if( strSig[ 0 ] != 'h' )
                strVarName += ".m_hStream";

            vecArgs.push_back( strVarName );
        }

    }while( 0 )

    return ret;
}

static gint32 GetArgTypes( ObjPtr pArgList,
    std::vector< std::string >& vecTypes )
{
    CArgList* pinal = pInArgs;
    if( pinal == nullptr )
        return -EINVAL;
   
    gint32 ret = 0; 

    do{
        guint32 i = 0;
        for( ; i < pinal->GetCount(); i++ )
        {
            ObjPtr pObj = pinal->GetChild();
            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pObj = pfa->GetType();
            CAstNodeBase* pNode = pObj;
            std::string strType =
                pNode->ToStringCpp();

            vecLocals.push_back( strType );
        }

    }while( 0 )

    return ret;
}

gint32 CImplIfMethodProxy::OutputEvent()
{
    ObjPtr pInArgs = m_pNode->GetInArgs();
    CArgList* pinal = pInArgs;
    if( pinal == nullptr )
        return 0;

    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_PImpl";
    strMethod = m_pNode->GetName();

    guint32 dwCount = pinal->GetCount();
    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Wrapper( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IEventSink* pCallback";
        if( dwCount == 0 )
        {
            CCOUT << " )";
        }
        else if( bSerial )
        {
            CCOUT << ", BufPtr& pBuf )";
        }
        else
        {
            strArgList =
                ToStringInArgs( pInArgs );
            CCOUT << ", " << strArgList;
        }
        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        if( dwCount == 0 )
        {
            CCOUT << "return this->"
                << strMethod << "();";
        }
        else if( !bSerial )
        {
            CCOUT << "return this->"
                << strMethod << "Wrapper(";
            INDENT_UP;
            NEW_LINE;
            CCOUT << strArgList << " );";
        }
        else
        {
            Wa( "gint32 ret = 0;" );
            std::vector< string > vecLocals;
            ret = GenLocals(
                pInArgs, vecLocals );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecLocals )
            {
                CCOUT << elem << ";";
                NEW_LINE;
            }
            NEW_LINE;

            Wa( "CSeriBase oDeserial;" );
            CEmitSerialCode odes(
                m_pWriter, pInArgs );

            std::vector< ObjPtr > vecHstms;
            ret = GetHstream(
                pInArgs, vecHstms );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecHstms )
            {
                CFormalArg* pfa = elem;
                std::string strLocal =
                    pfa->GetName();

                CCOUT << strLocal <<
                    ".m_pIf = this;";
                NEW_LINE;
            }

            CCOUT << "do";
            BLOCK_OPEN;
            ret = odes.OutputDeserial(
                "oDeserial" );
            if( ERROR( ret ) )
                break;

            BLOCK_CLOSE;
            CCOUT << "while( 0 );";

            Wa( "if( ERROR( ret ) )" );
            INDENT_UP;
            NEW_LINE;
            Wa( "return ret;" );
            INDENT_DOWN;
            NEW_LINE;

            std::vector< std::string > vecArgs;
            ret = GetArgsForCall(
                pInArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            // call the user's handler
            CCOUT << strMethod << "(";

            guint32 i = 0;
            for( ; i < vecArgs.size(); i++ )
            {
                CCOUT << " " << vecArgs[ i ];
                if( i + 1 < vecArgs.size() )
                    CCOUT << ", ";
            }

            if( vecArgs.empty() )
                CCOUT << ");";
            else
                CCOUT << " );";

            NEW_LINE;
        }

        Wa( "return ret" );
        BLOCK_CLOSE;

    }while( 0 )
    
    return ret;
}

gint32 CImplIfMethodProxy::OutputSync()
{
    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    gint32 ret = 0;
    std::string strClass = "I";
    strIfName += m_pIf->GetName();
    strClass += strIfName + "_PImpl";
    strMethod = m_pNode->GetName();

    CArgList* pinal = pInArgs;
    CArgList* poutal = pOutArgs;
    guint32 dwInCount = 0;
    guint32 dwOutCount = 0;

    if( pinal != nullptr )
        dwInCount += pinal->GetCount();

    if( poutal != nullptr )
        dwOutCount += poutal->GetCount();

    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( ";
        INDENT_UP;
        NEW_LINE;

        // gen the param list
        if( dwInCount == 0 &&
            dwOutCount == 0 )
        {
            CCOUT << " )";
        }
        else
        {
            bool bComma = false;
            if( dwInCount > 0 )
            {
                bComma = true;
                strArgList =
                    ToStringInArgs( pInArgs );
            }

            if( dwOutCount > 0 )
            {
                if( bComma )
                    strArgList += ", ";

                strArgList +=
                    ToStringOutArgs( pOutArgs );
            }
    
            CCOUT << strArgList << " )
        }
        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        if( 0 == dwInCount + dwOutCount )
        {
            Wa( "CParamList oOptions;" );
            Wa( "CfgPtr pResp;" );
            CCOUT << "oOptions[ propIfName ] = \""
                << strIfName << "\";";
            NEW_LINE;
            CCOUT << "oOptions[ propSeriProto ] = "
                << "( guint32 )seriNone;";
            NEW_LINE;
            CCOUT << " ret = this->SyncCallEx("
                << "oOptions.GetCfg(), pResp, "
                << "\"" << strMethod << "\" );";
            NEW_LINE;
            Wa( "if( ERROR( ret ) ) return ret;" );
            Wa( "guint32 dwRet = 0;" );
            Wa( "CCfgOpener oResp( ( IConfigDb* )pResp );" );
            Wa( "ret = oResp.GetIntProp( propReturnValue, dwRet );" );
            Wa( "if( ERROR( ret ) ) return ret;" );
            Wa( "ret = ( gint32 )dwRet" );
            Wa( "return ret" );
        }
        else if( !bSerial )
        {
            Wa( "CParamList oOptions;" );
            Wa( "CfgPtr pResp;" );
            CCOUT << "oOptions[ propIfName ] = \""
                << strIfName << "\";";
            NEW_LINE;
            CCOUT << "oOptions[ propSeriProto ] = "
                << "( guint32 )seriNone;";
            NEW_LINE;
            CCOUT << " ret = this->SyncCallEx("
                << "oOptions.GetCfg(), pResp, "
                << "\"" << strMethod << "\"";

            if( dwInCount > 0 )
            {
                CCOUT << ", ";
                INDENT_UP;
                std::vector< std::string > vecArgs;
                ret = GetArgsForCall(
                    pInArgs, vecArgs );
                if( ERROR( ret ) )
                    break;

                guint32 i = 0;
                for( ; i < vecArgs.size(); i++ )
                {
                    NEW_LINE;
                    CCOUT << elem;
                    if( i + 1 < vecArgs.size() )
                        CCOUT << ", ";
                }
                INDENT_DOWN;
            }

            CCOUT << " );";
            NEW_LINE;

            Wa( "if( ERROR( ret ) ) return ret;" );
            Wa( "guint32 dwRet = 0;" );
            Wa( "CCfgOpener oResp( ( IConfigDb* )pResp );" );
            Wa( "ret = oResp.GetIntProp( propReturnValue, dwRet );" );
            Wa( "if( ERROR( ret ) ) return ret;" );
            Wa( "ret = ( gint32 )dwRet" );
            if( dwOutCount == 0 )
            {
                Wa( "return ret;" );
            }
            else
            {
                Wa( "if( ERROR( ret ) ) return ret;" );
                Wa( "ret = this->FillArgs( pResp, ret" );

                CCOUT << ", ";
                INDENT_UP;
                std::vector< std::string > vecArgs;
                ret = GetArgsForCall(
                    pOutArgs, vecArgs );
                if( ERROR( ret ) )
                    break;

                guint32 i = 0;
                for( ; i < vecArgs.size(); i++ )
                {
                    NEW_LINE;
                    CCOUT << elem;
                    if( i + 1 < vecArgs.size() )
                        CCOUT << ", ";
                }
                CCOUT << " );";
                INDENT_DOWN;
            }
        }
        else /* need serialize */
        {
            Wa( "CParamList oOptions;" );
            Wa( "CfgPtr pResp;" );
            CCOUT << "oOptions[ propIfName ] = \""
                << strIfName << "\";";
            NEW_LINE;
            CCOUT << "oOptions[ propSeriProto ] = "
                << "( guint32 )seriRidl;";
            NEW_LINES( 2 );

            Wa( "//Serialize the input parameters" );
            Wa( "BufPtr pBuf( true ); " );

            Wa( "CSeriBase oSerial;" );
            CEmitSerialCode oesc(
                m_pWriter, pInArgs );

            std::vector< ObjPtr > vecHstms;
            ret = GetHstream(
                pInArgs, vecHstms );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecHstms )
            {
                CFormalArg* pfa = elem;
                std::string strLocal =
                    pfa->GetName();

                // Init the hstream structure for
                // serialization 
                CCOUT << "HSTREAM " << strLocal;
                CCOUT << strLocal <<
                    ".m_pIf = this;";
                CCOUT << strLocal << ".m_hStream = "
                    << strLocal << "_h";
                NEW_LINE;
            }

            NEW_LINE;
            ret = oesc.OutputSerial( "oSerial" );
            if( ERROR( ret ) )
                break;

            CCOUT << " ret = this->SyncCallEx("
                << "oOptions.GetCfg(), pResp, "
                << "\"" << strMethod << "\", "
                << "pBuf );";

            NEW_LINE;
            Wa( "if( ERROR( ret ) ) return ret;" );
            Wa( "guint32 dwRet = 0;" );
            Wa( "CCfgOpener oResp( ( IConfigDb* )pResp );" );
            Wa( "ret = oResp.GetIntProp( propReturnValue, dwRet );" );
            Wa( "if( ERROR( ret ) ) return ret;" );
            Wa( "ret = ( gint32 )dwRet" );
            if( dwOutCount == 0 )
                Wa( "return ret" );
            else
            {
                Wa( "BufPtr pBuf2( true );" );
                Wa( "ret = oResp.GetProperty( 0, pBuf );" );
                Wa( "if( ERROR( ret ) ) return ret;" );
                CCOUT << "do";
                BLOCK_OPEN;

                CEmitSerialCode oedsc(
                    m_pWriter, pOutArgs );
                ret = oedsc.OutputDeserial( "oSerial", "pBuf2" );
                if( ERROR( ret ) )
                    break;

                BLOCK_CLOSE;
                Wa( "while( 0 );" );

                Wa( "if( ERROR( ret ) )" );
                INDENT_UP;
                NEW_LINE;
                Wa( "return ret;" );
                INDENT_DOWN;
                NEW_LINE;

                // pass the stream handle to the
                // output parameter
                vecHstms.clear();
                ret = GetHstream(
                    pOutArgs, vecHstms );
                if( ERROR( ret ) )
                    break;

                for( auto& elem : vecHstms )
                {
                    CFormalArg* pfa = elem;
                    std::string strLocal =
                        pfa->GetName();

                    // assign the stream handle to
                    // the out parameter
                    CCOUT << strLocal << "_h = "
                        strLocal << ".m_hStream;";

                    NEW_LINE;
                }
                NEW_LINE;
                Wa( "return ret" );
            }
        }

        BLOCK_CLOSE;

    }while( 0 )
    
    return ret;
}

gint32 CImplIfMethodProxy::OutputAsync()
{
    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();
    CArgList* pinal = pInArgs;
    CArgList* poutal = pOutArgs;

    gint32 ret = 0;
    std::string strClass = "I";
    strIfName += m_pIf->GetName();
    strClass += strIfName + "_PImpl";
    strMethod = m_pNode->GetName();

    guint32 dwInCount = 0;
    guint32 dwOutCount = 0;

    if( pinal != nullptr )
        dwInCount += pinal->GetCount();

    if( poutal != nullptr )
        dwOutCount += poutal->GetCount();

    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( IConfigDb* context";
        INDENT_UP;
        NEW_LINE;

        // gen the param list
        if( dwInCount == 0 )
        {
            CCOUT << " )";
        }
        else
        {
            strArgList =
                ToStringInArgs( pInArgs );

            CCOUT << strArgList << " )
        }
        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        Wa( "CParamList oOptions;" );
        Wa( "CfgPtr pResp;" );
        CCOUT << "oOptions[ propIfName ] = ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "DBUS_IF_NAME( \""
            << strIfName << "\" );";
        INDENT_DOWN;
        NEW_LINE;
        CCOUT << "oOptions[ propSeriProto ] = ";
        if( bSerial )
            CCOUT << "( guint32 )seriRidl;";
        else
            CCOUT << "( guint32 )seriNone;";

        NEW_LINE( 2 );
        
        Wa( "CParamList oReqCtx;" );
        Wa( "ObjPtr pTemp( context );" );
        Wa( "oReqCtx.Push( pTemp );" );
        Wa( "TaskletPtr pRespCb;" );
        CCOUT << "ret = NEW_PROXY_RESP_HANDLER2(";
        INDENT_UP;
        NEW_LINE;
        Wa(  "pRespCb, ObjPtr( this ), " );
        CCOUT << "&" << strClass << "::"
            << strMethod << "CbWrapper, "
            << "nullptr, oReqCtx.GetCfg() );
        INDENT_DOWN;

        Wa( "if( ERROR( ret ) ) return ret;" );
        NEW_LINE;

        if( dwInCount == 0 )
        {
            Wa( " ret = this->AsyncCall(" );
            INDENT_UP;
            Wa( "( IEventSink* )pRespCb, " );
            Wa( "oOptions.GetCfg(), pResp," );
            CCOUT << "\"" << strMethod << "\" );";

            NEW_LINE;
            Wa( "if( ret == STATUS_PENDING ) return ret;" );
            Wa( "( *pRespCb )( eventCancelTask )" );
            Wa( "return ret" );
        }
        else if( !bSerial )
        {
            Wa( " ret = this->AsyncCall(" );
            INDENT_UP;
            Wa( "( IEventSink* )pRespCb, " );
            Wa( "oOptions.GetCfg(), pResp," );
            CCOUT << "\"" << strMethod << "\"";

            if( dwInCount > 0 )
            {
                CCOUT << ", ";
                INDENT_UP;
                std::vector< std::string > vecArgs;
                ret = GetArgsForCall(
                    pInArgs, vecArgs );
                if( ERROR( ret ) )
                    break;

                guint32 i = 0;
                for( ; i < vecArgs.size(); i++ )
                {
                    NEW_LINE;
                    CCOUT << elem;
                    if( i + 1 < vecArgs.size() )
                        CCOUT << ", ";
                }
                INDENT_DOWN;
            }

            CCOUT << " );";
            NEW_LINE;

            Wa( "if( ret == STATUS_PENDING ) return ret;" );
            Wa( "( *pRespCb )( eventCancelTask )" );
            Wa( "return ret" );
        }
        else /* need serialize */
        {
            Wa( "//Serialize the input parameters" );
            Wa( "BufPtr pBuf( true ); " );

            Wa( "CSeriBase oSerial;" );
            CEmitSerialCode oesc(
                m_pWriter, pInArgs );

            std::vector< ObjPtr > vecHstms;
            ret = GetHstream(
                pInArgs, vecHstms );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecHstms )
            {
                CFormalArg* pfa = elem;
                std::string strLocal =
                    pfa->GetName();

                // Init the hstream structure for
                // serialization 
                CCOUT << "HSTREAM " << strLocal;
                CCOUT << strLocal <<
                    ".m_pIf = this;";
                CCOUT << strLocal << ".m_hStream = "
                    << strLocal << "_h";
                NEW_LINE;
            }

            NEW_LINE;
            ret = oesc.OutputSerial( "oSerial" );
            if( ERROR( ret ) )
                break;

            NEW_LINE;

            Wa( " ret = this->AsyncCall(" );
            INDENT_UP;
            Wa( "( IEventSink* )pRespCb, " );
            Wa( "oOptions.GetCfg(), pResp," );
            CCOUT << "\"" << strMethod << "\"";
            NEW_LINE;
            CCOUT << "pBuf );";

            NEW_LINE;
            Wa( "if( ret == STATUS_PENDING ) return ret;" );
            Wa( "( *pRespCb )( eventCancelTask )" );
            Wa( "return ret" );
        }

        BLOCK_CLOSE;

    }while( 0 )
    
    return ret;
}

gint32 CImplIfMethodProxy::OutputAsyncCbWrapper()
{
    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();
    CArgList* pinal = pInArgs;
    CArgList* poutal = pOutArgs;

    gint32 ret = 0;
    std::string strClass = "I";
    strIfName += m_pIf->GetName();
    strClass += strIfName + "_PImpl";
    strMethod = m_pNode->GetName();

    guint32 dwInCount = 0;
    guint32 dwOutCount = 0;

    if( pinal != nullptr )
        dwInCount += pinal->GetCount();

    if( poutal != nullptr )
        dwOutCount += poutal->GetCount();

    bool bSerial = m_pNode->IsSerialize();

    do{
        Wa( "//Async callback wrapper" );
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "CbWrapper" << "( ";
        INDENT_UP;
        NEW_LINE;
        Wa( "IEventSink* pCallback, " );
        Wa( "IEventSink* pIoReq," );
        Wa( "IConfigDb* pReqCtx )" );

        INDENT_DOWN;
        NEW_LINES( 2 );

        Wa( "gint32 ret = 0;" );
        CCOUT << "do{";
        INDENT_UP;
        NEW_LINE; 
        Wa( "CCfgOpenerObj oReq( pIoReq );" )
        Wa( "ret = oReq.GetPointer(" );
        Wa( "propRespPtr, pResp );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );

        Wa( "CCfgOpener oResp( pResp );" );
        Wa( "gint32 iRet = 0;" );
        Wa( "ret = oResp.GetIntProp( propReturnValue," );
        CCOUT << "( guint32& ) iRet );";
        NEW_LINE;

        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );

        Wa( "IConfigDb* context = nullptr" );
        Wa( "CCfgOpener oReqCtx( pReqCtx );" );
        Wa( "ret = oReqCtx->GetPointer( 0, context );" );
        Wa( "if( ERROR( ret ) ) break;" );

        // gen the param list
        if( dwOutCount == 0 )
        {
             CCOUT << strMethod
                 <<"Callback( context, iRet );";
        }
        else if( !bSerial )
        {
            std::vector< std::string > vecLocals;
            ret = GenLocals( pOutArgs, vecLocals );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecLocals )
            {
                CCOUT << vecLocals[ i ] << ";"
                NEW_LINE;
            }

            Wa( "if( ERROR( iRet ) )" );
            BLOCK_OPEN;

            CCOUT << strMethod << "Callback( context, iRet";
            NEW_LINE;
            INDENT_UP

            std::vector< std::string > vecArgs;
            ret = GetArgsForCall( pOutArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            // pass some random values
            for( auto& elem : vecArgs )
            {
                CCOUT << ",";
                NEW_LINE;
                CCOUT << elem;
            }

            CCOUT << " )";
            NEW_LINE;
            INDENT_DOWN;

            Wa( "return iRet;" );

            BLOCK_CLOSE;

            Wa( "guint32 i = 0;" );
            Wa( "std::vector< BufPtr > vecParms" );
            Wa( "for( ; i <" << dwOutCount<< "; i++ )" );
            BLOCK_OPEN;
            Wa( "BufPtr pVal;" );
            Wa( "ret = oResp.GetProperty( i, pVal );" );
            Wa( "if( ERROR( ret ) break;" );
            Wa( "vecParams.push_back( pVal );" ); 
            BLOCK_CLOSE;
            Wa( "if( ERROR( ret ) break;" );

            CCOUT << strMethod << "Callback( context, iRet";
            INDENT_UP;

            std::vector< std::string > vecTypes;
            ret = GetArgTypes( pOutArgs, vecTypes );
            if( ERROR( ret ) ||
                vecTypes.size() != dwOutCount )
            {
                ret = -ERANGE;
                break;
            }
            for( guint32 i = 0; i < dwOutCount; i++ )
            {
                CCOUT << ",";
                NEW_LINE;
                CCOUT << "CastTo< " <<  vecTypes[ i ] << ">(";
                CCOUT << "vecParams[" << i << "] )";
            }

            INDENT_DOWN;
            NEW_LINE;

            BLOCK_CLOSE;
            break;
        }
        else /* need serialization */
        {
            std::vector< std::string > vecLocals;
            ret = GenLocals( pOutArgs, vecLocals );
            if( ERROR( ret ) )
                break;

            // declare locals for output params
            for( auto& elem : vecLocals )
            {
                CCOUT << vecLocals[ i ] << ";"
                NEW_LINE;
            }

            Wa( "if( ERROR( iRet ) )" );
            BLOCK_OPEN;

            CCOUT << "this->" << strMethod
                << "Callback( context, iRet";
            NEW_LINE;
            INDENT_UP

            std::vector< std::string > vecArgs;
            ret = GetArgsForCall( pOutArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            // pass some random values on error
            for( auto& elem : vecArgs )
            {
                CCOUT << ",";
                NEW_LINE;
                CCOUT << elem;
            }

            CCOUT << " )";
            NEW_LINE;
            INDENT_DOWN;

            Wa( "return iRet;" );
            BLOCK_CLOSE;

            std::vector< ObjPtr > vecHstms;
            ret = GetHstream(
                pInArgs, vecHstms );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecHstms )
            {
                CFormalArg* pfa = elem;
                std::string strLocal =
                    pfa->GetName();

                CCOUT << strLocal <<
                    ".m_pIf = this;";
                NEW_LINE;
            }
            NEW_LINE;

            Wa( "CSeriBase oDeserial;" );
            CEmitSerialCode odes(
                m_pWriter, pOutArgs );

            CCOUT << "do";
            BLOCK_OPEN;
            ret = odes.OutputDeserial(
                "oDeserial" );
            if( ERROR( ret ) )
                break;

            BLOCK_CLOSE;
            CCOUT << "while( 0 );";

            Wa( "if( ERROR( ret ) )" );
            INDENT_UP;
            NEW_LINE;
            Wa( "return ret;" );
            INDENT_DOWN;
            NEW_LINE;

            NEW_LINE;
            CCOUT << "this->" << strMethod
                <<"Callback( context, iRet";
            NEW_LINE;
            INDENT_UP

            std::vector< std::string > vecArgs;
            ret = GetArgsForCall( pOutArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            // pass some random values
            for( auto& elem : vecArgs )
            {
                CCOUT << ",";
                NEW_LINE;
                CCOUT << elem;
            }

            CCOUT << " );";
            INDENT_DOWN;
            NEW_LINE;

            break;
        }

        INDENT_DOWN;
        NEW_LINE;
        Wa( "}while( 0 );" );

        BLOCK_CLOSE;

    }while( 0 )
    
    return ret;
}

CImplIfMethodSvr::CImplIfMethodSvr(
    CCppWriter* pWriter,
    ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }

    CAstNodeBase* pParent = pNode->GetParent();
    if( pParent == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'method_decls' node" );
        throw std::runtime_error( strMsg );
    }
    pParent = pParent->GetParent();
    if( pParent == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }

    m_pIf = ObjPtr( pParent );
    if( m_pIf == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'interf_decl' node" );
        throw std::runtime_error( strMsg );
    }
};


gint32 CImplIfMethodSvr::Output()
{
    gint32 ret = 0;

    do{
        NEW_LINE;

        if( m_pNode->IsEvent() )
        {
            ret = OutputEvent();
            break;
        }
        else if( m_pNode->IsAsyncp() )
        {
            ret = OutputAsync();
        }
        else
        {
            ret = OutputSync();
        }

        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplIfMethodSvr::OutputEvent()
{
    ObjPtr pInArgs = m_pNode->GetInArgs();

    gint32 ret = 0;
    std::string strClass = "I";
    strIfName += m_pIf->GetName();
    strClass += strIfName + "_SImpl";
    strMethod = m_pNode->GetName();

    CArgList* pinal = pInArgs;
    CArgList* poutal = pOutArgs;
    guint32 dwInCount = 0;

    if( pinal != nullptr )
        dwInCount += pinal->GetCount();

    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( ";
        INDENT_UP;
        NEW_LINE;

        // gen the param list
        if( dwInCount == 0 &&
            dwOutCount == 0 )
        {
            CCOUT << " )";
        }
        else
        {
            bool bComma = false;
            if( dwInCount > 0 )
            {
                bComma = true;
                strArgList =
                    ToStringInArgs( pInArgs );
            }

            CCOUT << strArgList << " )
        }
        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        if( 0 == dwInCount )
        {
            Wa( "CParamList oOptions;" );
            CCOUT << "oOptions[ propSeriProto ] = "
                << "( guint32 )seriNone;";
            NEW_LINE;
            CCOUT << " ret = this->SendEventEx("
                << "nullptr, oOptions.GetCfg(), iid("
                << strIfName << "), "
                << "\"" << strMethod << "\", "
                << "\"\" ) ";
            NEW_LINE;
            Wa( "return ret" );
        }
        else if( !bSerial )
        {
            Wa( "CParamList oOptions;" );
            CCOUT << "oOptions[ propSeriProto ] = "
                << "( guint32 )seriNone;";
            NEW_LINE;
            CCOUT << " ret = this->SendEventEx("
                << "nullptr, oOptions.GetCfg(), iid("
                << strIfName << "), "
                << "\"" << strMethod << "\", ";

            if( dwInCount > 0 )
            {
                CCOUT << ", ";
                INDENT_UP;
                std::vector< std::string > vecArgs;
                ret = GetArgsForCall(
                    pInArgs, vecArgs );
                if( ERROR( ret ) )
                    break;

                guint32 i = 0;
                for( ; i < vecArgs.size(); i++ )
                {
                    NEW_LINE;
                    CCOUT << elem;
                    if( i + 1 < vecArgs.size() )
                        CCOUT << ", ";
                }
            }

            CCOUT << " );";
            INDENT_DOWN;

            NEW_LINE;
            Wa( "return ret;" );
        }
        else /* need serialize */
        {
            Wa( "CParamList oOptions;" );
            CCOUT << "oOptions[ propSeriProto ] = "
                << "( guint32 )seriRidl;";
            NEW_LINE;

            Wa( "//Serialize the input parameters" );
            Wa( "BufPtr pBuf( true ); " );

            Wa( "CSeriBase oSerial;" );
            CEmitSerialCode oesc(
                m_pWriter, pInArgs );

            std::vector< ObjPtr > vecHstms;
            ret = GetHstream(
                pInArgs, vecHstms );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecHstms )
            {
                CFormalArg* pfa = elem;
                std::string strLocal =
                    pfa->GetName();

                // Init the hstream structure for
                // serialization 
                CCOUT << "HSTREAM " << strLocal;
                CCOUT << strLocal <<
                    ".m_pIf = this;";
                CCOUT << strLocal << ".m_hStream = "
                    << strLocal << "_h";
                NEW_LINE;
            }

            NEW_LINE;
            ret = oesc.OutputSerial( "oSerial" );
            if( ERROR( ret ) )
                break;

            CCOUT << " ret = this->SendEventEx("
                << "nullptr, oOptions.GetCfg(), iid("
                << strIfName << "), "
                << "\"" << strMethod << "\", "
                << "pBuf );";

            NEW_LINE;
            Wa( "return ret" );
        }

        BLOCK_CLOSE;

    }while( 0 )
    
    return ret;
}

gint32 CImplIfMethodSvr::OutputSync()
{
    ObjPtr pInArgs = m_pNode->GetInArgs();
    CArgList* pinal = pInArgs;
    if( pinal == nullptr )
        return 0;

    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    strMethod = m_pNode->GetName();

    ObjPtr pInArgs = pmd->GetInArgs();
    ObjPtr pOutArgs = pmd->GetOutArgs();

    CArgList* pinal = pInArgs;
    CArgList* poutal = pOutArgs;
    guint32 dwInCount = 0;
    guint32 dwOutCount = 0;

    if( pinal != nullptr )
        dwInCount += pinal->GetCount();

    if( poutal != nullptr )
        dwOutCount += poutal->GetCount();

    guint32 dwCount = dwInCount + dwOutCount;

    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Wrapper( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IEventSink* pCallback";
        if( dwCount == 0 )
        {
            CCOUT << " )";
        }
        else if( bSerial )
        {
            CCOUT << ", BufPtr& pBuf )";
        }
        else
        {
            strArgList =
                ToStringInArgs( pInArgs );
            if( !strArgList.empty() )
                CCOUT << ", " << strArgList 

            CCOUT << " )";
        }

        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        if( dwCount == 0 )
        {
            CCOUT << "ret = this->"
                << strMethod << "();";

            Wa( "CParamList oResp;" );
            INDENT_UP;
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "oResp[ propReturnValue ] = ret;" );
            Wa( "this->SetResponse( pCallback, oResp.GetCfg() )" );
            BLOCK_CLOSE;
        }
        else if( !bSerial )
        {
            CCOUT << "return this->"
                << strMethod << "(";
            INDENT_UP;

            std::vector< std::string > vecArgs;
            ret = GetArgsForCall(
                pInArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            bool bFirst = true;
            for( auto& elem : vecArgs )
            {
                if( !bFirst )
                    CCOUT << ",";
                NEW_LINE;
                CCOUT << elem;
            }

            if( vecArgs.size() )
                CCOUT <<  " );";
            else
                CCOUT << ");";
 
            Wa( "CParamList oResp;" );
            Wa( "oResp[ propReturnValue ] = ret;" );
            Wa( "if( SUCCEEDED( ret ) )" );
            INDENT_UP;
            NEW_LINE;
            BLOCK_OPEN;

            vecArgs.clear();
            ret = GetArgsForCall( pOutArgs, vecArgs );
            for( auto& elem : vecArgs )
            {
                CCOUT << "oParams.Push( " << elem << " );";
                NEW_LINE;
            }
            Wa( "this->SetResponse( pCallback, oResp.GetCfg() )" );
            BLOCK_CLOSE;

        }
        else
        {
            Wa( "gint32 ret = 0;" );

            std::vector< string > vecInLocals;
            ret = GenLocals(
                pInArgs, vecInLocals );
            if( ERROR( ret ) )
                break;

            std::vector< string > vecOutLocals;
            ret = GenLocals(
                pOutArgs, vecOutLocals );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecInLocals )
            {
                CCOUT << elem << ";";
                NEW_LINE;
            }

            for( auto& elem : vecOutLocals )
            {
                CCOUT << elem << ";";
                NEW_LINE;
            }

            NEW_LINE;

            Wa( "CSeriBase oDeserial;" );
            CEmitSerialCode odes(
                m_pWriter, pInArgs );

            std::vector< ObjPtr > vecHstms;
            ret = GetHstream(
                pInArgs, vecHstms );
            if( ERROR( ret ) )
                break;

            ret = GetHstream(
                pOutArgs, vecHstms );

            for( auto& elem : vecHstms )
            {
                CFormalArg* pfa = elem;
                std::string strLocal =
                    pfa->GetName();

                CCOUT << strLocal <<
                    ".m_pIf = this;";
                NEW_LINE;
            }

            CCOUT << "do";
            BLOCK_OPEN;
            ret = odes.OutputDeserial(
                "oDeserial" );
            if( ERROR( ret ) )
                break;

            BLOCK_CLOSE;
            CCOUT << "while( 0 );";

            Wa( "if( ERROR( ret ) )" );
            INDENT_UP;
            NEW_LINE;
            Wa( "return ret;" );
            INDENT_DOWN;
            NEW_LINE;

            std::vector< std::string > vecInArgs;
            ret = GetArgsForCall(
                pInArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            std::vector< std::string > vecOutArgs;
            ret = GetArgsForCall(
                pOutArgs, vecOutArgs );
            if( ERROR( ret ) )
                break;

            // call the user's handler
            CCOUT << "ret ="
                << strMethod << "(";

            INDENT_UP;
            guint32 i = 0;
            bool bComma = false;
            for( ; i < dwInCount; i++ )
            {
                bComma = true;
                NEW_LINE;
                CCOUT << " " << vecInArgs[ i ];
                if( i + 1 < vecInArgs.size() )
                    CCOUT << ", ";
            }

            for( ; i < dwOutCount; i++ )
            {
                if( bComma )
                {
                    CCOUT << ", ";
                    bComma = false;
                }
                CCOUT << " " << vecOutArgs[ i ];
                if( i + 1 < vecOutArgs.size() )
                    CCOUT << ", ";
            }

            if( dwInCount + dwOutCount == 0 )
                CCOUT << ");";
            else
                CCOUT << " );";

            INDENT_DOWN;
            NEW_LINE;

            Wa( "CParamList oResp;" );
            Wa( "oResp[ propReturnValue ] = ret;" );
            Wa( "if( SUCCEEDED( ret ) )" );
            INDENT_UP;
            NEW_LINE;
            BLOCK_OPEN;

            Wa( "BufPtr pBuf2( true )" );
            Wa( "CSeriBase oSerial;" );
            CEmitSerialCode oesc(
                m_pWriter, pInArgs );
            Wa( "do" );
            BLOCK_OPEN;
            ret = oesc.OutputSerial( "oSerial", "pBuf2" );
            if( ERROR( ret ) )
                break;

            BLOCK_CLOSE;
            Wa( "while( 0 );" );
            Wa( "if( ERROR( ret ) ) return ret" );
            Wa( "oResp.push( pBuf );" );
            BLOCK_CLOSE;
        }

        Wa( "return ret" );
        BLOCK_CLOSE;

    }while( 0 )
    
    return ret;
}

gint32 CImplIfMethodSvr::OutputAsync()
{
    ObjPtr pInArgs = m_pNode->GetInArgs();
    CArgList* pinal = pInArgs;
    if( pinal == nullptr )
        return 0;

    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    strMethod = m_pNode->GetName();

    ObjPtr pInArgs = pmd->GetInArgs();
    ObjPtr pOutArgs = pmd->GetOutArgs();

    CArgList* pinal = pInArgs;
    CArgList* poutal = pOutArgs;
    guint32 dwInCount = 0;
    guint32 dwOutCount = 0;

    if( pinal != nullptr )
        dwInCount += pinal->GetCount();

    if( poutal != nullptr )
        dwOutCount += poutal->GetCount();

    guint32 dwCount = dwInCount + dwOutCount;

    bool bSerial = m_pNode->IsSerialize();

    do{
        if( !bSerial )
            break;

        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Wrapper( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IEventSink* pCallback"
            << ", BufPtr& pBuf )";

        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );

        std::vector< string > vecInLocals;
        ret = GenLocals(
            pInArgs, vecInLocals );
        if( ERROR( ret ) )
            break;

        std::vector< string > vecOutLocals;
        ret = GenLocals(
            pOutArgs, vecOutLocals );
        if( ERROR( ret ) )
            break;

        for( auto& elem : vecInLocals )
        {
            CCOUT << elem << ";";
            NEW_LINE;
        }

        for( auto& elem : vecOutLocals )
        {
            CCOUT << elem << ";";
            NEW_LINE;
        }

        NEW_LINE;

        Wa( "CSeriBase oDeserial;" );
        CEmitSerialCode odes(
            m_pWriter, pInArgs );

        std::vector< ObjPtr > vecHstms;
        ret = GetHstream(
            pInArgs, vecHstms );
        if( ERROR( ret ) )
            break;

        ret = GetHstream(
            pOutArgs, vecHstms );

        for( auto& elem : vecHstms )
        {
            CFormalArg* pfa = elem;
            std::string strLocal =
                pfa->GetName();

            CCOUT << strLocal <<
                ".m_pIf = this;";
            NEW_LINE;
        }

        CCOUT << "do";
        BLOCK_OPEN;
        ret = odes.OutputDeserial(
            "oDeserial" );
        if( ERROR( ret ) )
            break;

        BLOCK_CLOSE;
        CCOUT << "while( 0 );";

        Wa( "if( ERROR( ret ) )" );
        INDENT_UP;
        NEW_LINE;
        Wa( "return ret;" );
        INDENT_DOWN;
        NEW_LINE;

        std::vector< std::string > vecInArgs;
        ret = GetArgsForCall(
            pInArgs, vecArgs );
        if( ERROR( ret ) )
            break;

        std::vector< std::string > vecOutArgs;
        ret = GetArgsForCall(
            pOutArgs, vecOutArgs );
        if( ERROR( ret ) )
            break;

        // call the user's handler
        CCOUT << "ret ="
            << strMethod << "( pCallback";

        INDENT_UP;
        guint32 i = 0;
        bool bComma = false;
        for( ; i < dwInCount; i++ )
        {
            bComma = true;
            NEW_LINE;
            CCOUT << " " << vecInArgs[ i ];
            if( i + 1 < vecInArgs.size() )
                CCOUT << ", ";
        }

        for( ; i < dwOutCount; i++ )
        {
            if( bComma )
            {
                CCOUT << ", ";
                bComma = false;
            }
            CCOUT << " " << vecOutArgs[ i ];
            if( i + 1 < vecOutArgs.size() )
                CCOUT << ", ";
        }

        if( dwInCount + dwOutCount == 0 )
            CCOUT << ");";
        else
            CCOUT << " );";

        INDENT_DOWN;
        NEW_LINE;

        Wa( "if( ret == STATUS_PENDING ) return ret;" );
        Wa( "CParamList oResp;" );
        Wa( "oResp[ propReturnValue ] = ret;" );
        Wa( "if( SUCCEEDED( ret ) )" );
        INDENT_UP;
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "BufPtr pBuf2( true )" );
        Wa( "CSeriBase oSerial;" );
        CEmitSerialCode oesc(
            m_pWriter, pInArgs );
        Wa( "do" );
        BLOCK_OPEN;
        ret = oesc.OutputSerial( "oSerial", "pBuf2" );
        if( ERROR( ret ) )
            break;

        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        Wa( "if( ERROR( ret ) ) return ret" );
        Wa( "oResp.push( pBuf );" );
        BLOCK_CLOSE;

        Wa( "return ret" );
        BLOCK_CLOSE;

    }while( 0 )
    
    return ret;
}

gint32 CImplIfMethodSvr::OutputAsyncCallback()
{
    ObjPtr pInArgs = m_pNode->GetInArgs();
    CArgList* pinal = pInArgs;
    if( pinal == nullptr )
        return 0;

    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    strMethod = m_pNode->GetName();

    ObjPtr pInArgs = pmd->GetInArgs();
    ObjPtr pOutArgs = pmd->GetOutArgs();

    CArgList* pinal = pInArgs;
    CArgList* poutal = pOutArgs;
    guint32 dwInCount = 0;
    guint32 dwOutCount = 0;

    if( pinal != nullptr )
        dwInCount += pinal->GetCount();

    if( poutal != nullptr )
        dwOutCount += poutal->GetCount();

    guint32 dwCount = dwInCount + dwOutCount;

    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Callback( ";
        INDENT_UP;
        NEW_LINE;
        CCOUT << "IEventSink* pCallback"
            << ", gint32 iRet";

        strArgList =
            ToStringInArgs( pOutArgs );

        if( !strArgList.empty() )
            CCOUT << ", " << strArgList;

        CCOUT << " )";

        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;
        Wa( "gint32 ret = 0;" );

        if( bSerial )
        {
            NEW_LINE;
            Wa( "BufPtr pBuf( true );" );
            Wa( "CSeriBase oSerial;" );
            CEmitSerialCode oesc(
                m_pWriter, pInArgs );

            std::vector< ObjPtr > vecHstms;
            ret = GetHstream(
                pInArgs, vecHstms );
            if( ERROR( ret ) )
                break;

            ret = GetHstream(
                pOutArgs, vecHstms );

            for( auto& elem : vecHstms )
            {
                CFormalArg* pfa = elem;
                std::string strLocal =
                    pfa->GetName();

                CCOUT << "HSTREAM " << strLocal;
                CCOUT << strLocal <<
                    ".m_pIf = this;";

                CCOUT << strLocal
                    << ".m_hStream = "
                    << strLocal << "_h;";

                NEW_LINE;
            }

            CCOUT << "do";
            BLOCK_OPEN;
            ret = oesc.OutputSerial( "oSerial" );
            if( ERROR( ret ) )
                break;

            BLOCK_CLOSE;
            CCOUT << "while( 0 );";

            Wa( "if( ERROR( ret ) )" );
            INDENT_UP;
            NEW_LINE;
            Wa( "return ret;" );
            INDENT_DOWN;
            NEW_LINE;


            Wa( "ret = iRet;" );
            Wa( "if( ret == STATUS_PENDING ) ret = ERROR_STATE;" );
            Wa( "CParamList oResp;" );
            Wa( "oResp[ propReturnValue ] = ret;" );
            Wa( "if( SUCCEEDED( ret ) )" );
            INDENT_UP;
            NEW_LINE;
                Wa( "oResp.Push( pBuf )" );
            INDENT_DOWN;
            NEW_LINE

            Wa( "this->OnServiceComplete( "
            INDENT_UP;
            NEW_LINE;
                CCOUT << "oResp.GetCfg(), pCallback )";
            INDENT_DOWN;
            NEW_LINE

            Wa( "return ret;" );

            break;
        }
        else
        {
            Wa( "ret = iRet;" );
            Wa( "if( ret == STATUS_PENDING ) ret = ERROR_STATE;" );
            Wa( "CParamList oResp;" );
            Wa( "oResp[ propReturnValue ] = ret;" );
            Wa( "if( SUCCEEDED( ret ) )" );
            INDENT_UP;
            NEW_LINE;

            std::vector< std::string > vecArgs;
            ret = GetArgsForCall( pOutArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecArgs )
            {
                CCOUT << "oResp.Push( "
                    << vecArgs[ i ] << " );";
            }

            INDENT_DOWN;
            NEW_LINE

            Wa( "this->OnServiceComplete( " );
            INDENT_UP;
            NEW_LINE;
                CCOUT << "( IConfigDb* )oResp.GetCfg(), pCallback )";
            INDENT_DOWN;
            NEW_LINE

            Wa( "return ret;" );
        }
        BLOCK_CLOSE;

    }while( 0 )
    
    return ret;
}

