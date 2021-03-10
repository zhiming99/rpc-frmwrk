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

gint32 CArgListUtils::GetArgCount(
    ObjPtr& pArgs )
{
    if( pArgs.IsEmpty() )
        return 0;

    CArgList* pal = pArgs;
    if( pal == nullptr )
        return 0;

    return pal->GetCount();
}

std::string CArgListUtils::ToStringInArgs(
    ObjPtr& pArgs )
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
            std::string strVarName = pfa->GetName();
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

std::string CArgListUtils::ToStringOutArgs(
    ObjPtr& pArgs )
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
            std::string strVarName = pfa->GetName();
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

gint32 CArgListUtils::GenLocals(
    ObjPtr pArgList,
    std::vector< std::string >& vecLocals )
{
    CArgList* pinal = pArgList;
    if( pinal == nullptr )
        return -EINVAL;
   
    gint32 ret = 0; 

    do{
        guint32 i = 0;
        for( ; i < pinal->GetCount(); i++ )
        {
            ObjPtr pObj = pinal->GetChild( i );
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

    }while( 0 );

    return ret;
}

gint32 CArgListUtils::GetHstream(
    ObjPtr& pArgList,
    std::vector< ObjPtr >& vecHstms )
{
    CArgList* pinal = pArgList;
    if( pinal == nullptr )
        return -EINVAL;
   
    gint32 ret = 0; 

    do{
        guint32 i = 0;
        for( ; i < pinal->GetCount(); i++ )
        {
            ObjPtr pObj = pinal->GetChild( i );
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

    }while( 0 );

    return ret;
}

gint32 CArgListUtils::GetArgsForCall(
    ObjPtr& pArgList,
    std::vector< std::string >& vecArgs ) 
{
    CArgList* pinal = pArgList;
    if( pinal == nullptr )
        return -EINVAL;
   
    gint32 ret = 0; 

    do{
        guint32 i = 0;
        for( ; i < pinal->GetCount(); i++ )
        {
            ObjPtr pObj = pinal->GetChild( i );
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

            if( strSig[ 0 ] == 'h' )
                strVarName += ".m_hStream";

            vecArgs.push_back( strVarName );
        }

    }while( 0 );

    return ret;
}

gint32 CArgListUtils::GetArgTypes(
    ObjPtr pArgList,
    std::vector< std::string >& vecTypes )
{
    CArgList* pinal = pArgList;
    if( pinal == nullptr )
        return -EINVAL;
   
    gint32 ret = 0; 

    do{
        guint32 i = 0;
        for( ; i < pinal->GetCount(); i++ )
        {
            ObjPtr pObj = pinal->GetChild( i );
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

            vecTypes.push_back( strType );
        }

    }while( 0 );

    return ret;
}

gint32 CMethodWriter::GenActParams(
    ObjPtr& pArgList )
{
    if( GetArgCount( pArgList ) == 0 )
        return STATUS_SUCCESS;

    gint32 ret = 0;
    do{
        std::vector< std::string > vecArgs;
        ret = GetArgsForCall(
            pArgList, vecArgs );
        if( ERROR( ret ) )
            break;

        guint32 i = 0;
        for( ;i < vecArgs.size(); i++ )
        {
            CCOUT << vecArgs[ i ];
            if( i + 1 < vecArgs.size() )
            {
                CCOUT << ",";
                NEW_LINE;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CMethodWriter::GenActParams(
    ObjPtr& pArgList, ObjPtr& pArgList2 )
{
    guint32 dwCount =
        GetArgCount( pArgList );

    guint32 dwCount2 =
        GetArgCount( pArgList2 );

    if( dwCount == 0 && dwCount2 == 0 )
        return STATUS_SUCCESS;

    if( dwCount == 0 )
        return GenActParams( pArgList2 );

    if( dwCount2 == 0 )
        return GenActParams( pArgList );

    gint32 ret = 0;
    do{
        std::vector< std::string > vecArgs;
        ret = GetArgsForCall(
            pArgList, vecArgs );
        if( ERROR( ret ) )
            break;

        ret = GetArgsForCall(
            pArgList2, vecArgs );
        if( ERROR( ret ) )
            break;

        guint32 i = 0;
        for( ;i < vecArgs.size(); i++ )
        {
            CCOUT << vecArgs[ i ];
            if( i + 1 < vecArgs.size() )
            {
                CCOUT << ",";
                NEW_LINE;
            }
        }

    }while( 0 );

    return ret;
}

gint32 CMethodWriter::GenSerialArgs(
    ObjPtr& pArgList,
    const std::string& strBuf,
    bool bDeclare, bool bAssign )
{
    gint32 ret = 0;
    if( GetArgCount( pArgList ) == 0 )
        return -ENOENT;

    do{
        NEW_LINE;
        CCOUT << "BufPtr " << strBuf << "( true );";
        Wa( "CSeriBase oSerial;" );
        CEmitSerialCode oesc(
            m_pWriter, pArgList );

        std::vector< ObjPtr > vecHstms;
        ret = GetHstream(
            pArgList, vecHstms );
        if( ERROR( ret ) )
            break;

        for( auto& elem : vecHstms )
        {
            CFormalArg* pfa = elem;
            std::string strLocal =
                pfa->GetName();

            if( bDeclare )
                CCOUT << "HSTREAM " << strLocal;

            CCOUT << strLocal <<
                ".m_pIf = this;";

            if( bAssign )
            {
                CCOUT << strLocal
                    << ".m_hStream = "
                    << strLocal << "_h;";
            }
            NEW_LINE;
        }

        CCOUT << "do";
        BLOCK_OPEN;
        ret = oesc.OutputSerial(
            "oSerial", strBuf );
        if( ERROR( ret ) )
            break;

        BLOCK_CLOSE;
        CCOUT << "while( 0 );";

    }while( 0 );

    return ret;
}

gint32 CMethodWriter::GenDeserialArgs(
    ObjPtr& pArgList,
    const std::string& strBuf,
    bool bDeclare, bool bAssign )
{
    gint32 ret = 0;
    if( GetArgCount( pArgList ) == 0 )
        return -ENOENT;

    do{
        NEW_LINE;
        Wa( "CSeriBase oDeserial;" );
        CEmitSerialCode oedsc(
            m_pWriter, pArgList );

        std::vector< ObjPtr > vecHstms;
        ret = GetHstream(
            pArgList, vecHstms );
        if( ERROR( ret ) )
            break;

        for( auto& elem : vecHstms )
        {
            CFormalArg* pfa = elem;
            std::string strLocal =
                pfa->GetName();

            if( bDeclare )
                CCOUT << "HSTREAM " << strLocal;
            CCOUT << strLocal <<
                ".m_pIf = this;";

            NEW_LINE;
        }

        CCOUT << "do";
        BLOCK_OPEN;
        ret = oedsc.OutputDeserial(
            "oDeserial", strBuf );
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

        if( bAssign == false )
            break;

        if( vecHstms.size() )
            break;

        for( auto& elem : vecHstms )
        {
            CFormalArg* pfa = elem;
            std::string strLocal =
                pfa->GetName();

            // assign the stream handle to
            // the out parameter
            CCOUT << strLocal << "_h = "
                << strLocal << ".m_hStream;";

            NEW_LINE;
        }

    }while( 0 );

    return ret;
}

gint32 CMethodWriter::DeclLocals(
    ObjPtr& pArgList )
{
    std::vector< std::string > vecLocals;
    gint32 ret = GenLocals(
        pArgList, vecLocals );
    if( ERROR( ret ) )
        return ret;

    for( auto& elem : vecLocals )
    {
        CCOUT << elem << ";";
        NEW_LINE;
    }
    return STATUS_SUCCESS;
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
    CCppWriter* m_pWriter, ObjPtr& pRoot )
{
    if( m_pWriter == nullptr ||
        pRoot.IsEmpty() )
        return -EINVAL;

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

        Wa( "//Generated by ridlc" );
        Wa( "//This file content could be cleared by ridlc" );
        Wa( "#include \"seribase.h\"" );
        CCOUT << "#include \"" << strName << ".h\"";
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

                    CImplIufProxy oiufp(
                        m_pWriter, pObj );
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
                        CImplIfMethodProxy oiimp(
                            m_pWriter, pmd );
                        oiimp.Output();
                    }

                    CImplIufSvr oiufs(
                        m_pWriter, pObj );
                    oiufs.Output();

                    for( guint32 i = 0;
                        i < pmdl->GetCount(); i++ )
                    {
                        ObjPtr pmd = pmdl->GetChild( i );
                        CImplIfMethodSvr oiims(
                            m_pWriter, pmd );
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
            dwCount = GetArgCount( pArgs );
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
                <<"Wrapper( IEventSink* pCallback );";
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
        
        guint32 dwInCount = GetArgCount( pInArgs );

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

        NEW_LINE;

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

        guint32 dwInCount =
            GetArgCount( pInArgs );

        guint32 dwOutCount =
            GetArgCount( pInArgs );

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

        guint32 dwInCount = GetArgCount( pInArgs );
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
            Wa( "if( ERROR( ret ) ) break;" );

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
            case 'F':
            case 'b':
            case 'B':
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
            case 'F':
            case 'b':
            case 'B':
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
        strClass += m_pNode->GetName() + "_PImpl";
        std::string strIfName = m_pNode->GetName();
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
        strClass += m_pNode->GetName() + "_SImpl";
        std::string strIfName = m_pNode->GetName();
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

                ObjPtr pInArgs = pmd->GetInArgs();
                ObjPtr pOutArgs = pmd->GetOutArgs();

                guint32 dwCount = 0;
                if( !pmd->IsAsyncs() )
                {
                    dwCount = GetArgCount( pInArgs );
                    dwCount += GetArgCount( pOutArgs );
                }
                else
                {
                    dwCount = GetArgCount( pInArgs );
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
                        CCOUT << "ADD_USER_SERVICE_HANDLER_EX("
                            << dwCount << ",";
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
    m_pArgs = pNode;
    if( m_pArgs == nullptr )
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
            case 'F':
            case 'b':
            case 'B':
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

    return ret;
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
            case 'F':
            case 'b':
            case 'B':
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

    return ret;
}

CMethodWriter::CMethodWriter(
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

    CAstNodeBase* pParent = m_pNode->GetParent();
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

gint32 CImplIfMethodProxy::OutputEvent()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_PImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwCount = dwInCount;

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
            DeclLocals( pInArgs );

            NEW_LINE;

            ret = GenDeserialArgs(
                pInArgs, "pBuf", false, false );
            if( ERROR( ret ) )
                break;

            // call the user's handler
            CCOUT << strMethod << "(";
            GenActParams( pInArgs );

            if( dwCount == 0 )
                CCOUT << ");";
            else
                CCOUT << " );";

            NEW_LINE;
        }

        Wa( "return ret" );
        BLOCK_CLOSE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodProxy::OutputSync()
{
    gint32 ret = 0;
    std::string strClass = "I";
    std::string strIfName = m_pIf->GetName();
    strClass += strIfName + "_PImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );

    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( ";
        INDENT_UP;
        NEW_LINE;

        // gen the param list
        if( dwInCount == 0 && dwOutCount == 0 )
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
    
            CCOUT << strArgList << " );";
        }
        INDENT_DOWN;
        NEW_LINE;

        BLOCK_OPEN;

        Wa( "CParamList oOptions;" );
        Wa( "CfgPtr pResp;" );
        CCOUT << "oOptions[ propIfName ] = \""
            << strIfName << "\";";
        NEW_LINE;
        if( !bSerial )
            CCOUT << "oOptions[ propSeriProto ] = "
                << "( guint32 )seriNone;";
        else
            CCOUT << "oOptions[ propSeriProto ] = "
                << "( guint32 )seriRidl;";
        NEW_LINE;

        if( 0 == dwInCount + dwOutCount )
        {
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
            CCOUT << " ret = this->SyncCallEx("
                << "oOptions.GetCfg(), pResp, "
                << "\"" << strMethod << "\"";

            if( dwInCount > 0 )
            {
                CCOUT << ", ";
                INDENT_UP;
                GenActParams( pInArgs );
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
                GenActParams( pOutArgs );
                CCOUT << " );";
                INDENT_DOWN;
            }
        }
        else /* need serialize */
        {
            Wa( "//Serialize the input parameters" );
            ret = GenSerialArgs(
                pInArgs, "pBuf", true, true );
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
                Wa( "BufPtr pBuf2;" );
                Wa( "ret = oResp.GetProperty( 0, pBuf2 );" );
                Wa( "if( ERROR( ret ) ) return ret;" );

                if( ERROR( ret ) )
                    break;

                CCOUT << "do";
                ret = GenDeserialArgs(
                    pOutArgs, "pBuf2", true, true );
                BLOCK_CLOSE;
                Wa( "while( 0 );" );

                NEW_LINE;
                Wa( "return ret" );
            }
        }

        BLOCK_CLOSE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodProxy::OutputAsync()
{
    gint32 ret = 0;
    std::string strClass = "I";
    std::string strIfName = m_pIf->GetName();
    strClass += strIfName + "_PImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );

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

            CCOUT << strArgList << " )";
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

        NEW_LINES( 2 );
        
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
            << "nullptr, oReqCtx.GetCfg() );";
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
                GenActParams( pInArgs );
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
            ret = GenSerialArgs(
                pInArgs, "pBuf", true, true );

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

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodProxy::OutputAsyncCbWrapper()
{
    gint32 ret = 0;
    std::string strClass = "I";
    std::string strIfName = m_pIf->GetName();
    strClass += strIfName + "_PImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwOutCount = GetArgCount( pOutArgs );

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
        NEW_LINE;
        BLOCK_OPEN;

        Wa( "gint32 ret = 0;" );
        CCOUT << "do";
        BLOCK_OPEN;
        NEW_LINE; 
        Wa( "CCfgOpenerObj oReq( pIoReq );" );
        Wa( "ret = oReq.GetPointer(" );
        Wa( "propRespPtr, pResp );" );
        Wa( "if( ERROR( ret ) )" );
        Wa( "    break;" );

        Wa( "CCfgOpener oResp( pResp );" );
        Wa( "gint32 iRet = 0;" );
        Wa( "ret = oResp.GetIntProp( " );

        INDENT_UPL;
        CCOUT << "propReturnValue, ( guint32& ) iRet );";
        INDENT_DOWNL;

        Wa( "if( ERROR( ret ) )" );
        INDENT_UP;
        Wa( "break;" );
        INDENT_DOWN;

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
            DeclLocals( pOutArgs );

            Wa( "if( ERROR( iRet ) )" );
            BLOCK_OPEN;

            CCOUT << strMethod << "Callback( context, iRet,";
            NEW_LINE;
            INDENT_UP;

            GenActParams( pOutArgs );

            CCOUT << " )";
            NEW_LINE;
            INDENT_DOWN;

            Wa( "return iRet;" );
            BLOCK_CLOSE;

            Wa( "guint32 i = 0;" );
            Wa( "std::vector< BufPtr > vecParms" );
            CCOUT << "for( ; i <" << dwOutCount<< "; i++ )";
            NEW_LINE;
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

            CCOUT << " )";
            INDENT_DOWN;
            NEW_LINE;

            BLOCK_CLOSE;
            break;
        }
        else /* need serialization */
        {
            Wa( "BufPtr pBuf;" );
            Wa( "ret = oResp.GetProperty( 0, pBuf );" );
            Wa( "if( ERROR( ret ) ) break;" );

            DeclLocals( pOutArgs );
            ret = GenDeserialArgs( pOutArgs,
                "pBuf", false, false );
            if( ERROR( ret ) )
                break;

            NEW_LINE;
            CCOUT << "this->" << strMethod
                <<"Callback( context, iRet,";
            NEW_LINE;
            INDENT_UP;

            GenActParams( pOutArgs );

            CCOUT << " );";
            INDENT_DOWN;
            NEW_LINE;

            break;
        }

        INDENT_DOWN;
        NEW_LINE;
        BLOCK_CLOSE;
        Wa( "while( 0 );" );
        Wa( "return ret" );

        BLOCK_CLOSE;

    }while( 0 );
    
    return ret;
}

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
            if( ERROR( ret ) )
                break;

            ret = OutputAsyncCallback();
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
    gint32 ret = 0;
    std::string strClass = "I";
    std::string strIfName = m_pIf->GetName();
    strClass += strIfName + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    guint32 dwInCount = GetArgCount( pInArgs );

    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "( ";
        INDENT_UPL;

        // gen the param list
        if( dwInCount == 0 )
        {
            CCOUT << " )";
        }
        else
        {
            strArgList =
                ToStringInArgs( pInArgs );
            CCOUT << strArgList << " )";
        }

        INDENT_DOWNL;
        BLOCK_OPEN;

        Wa( "CParamList oOptions;" );
        CCOUT << "oOptions[ propSeriProto ] = ";
        if( bSerial )
            CCOUT << "( guint32 )seriRidl;";
        else
            CCOUT << "( guint32 )seriNone;";
        NEW_LINE;

        if( 0 == dwInCount )
        {
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
            CCOUT << " ret = this->SendEventEx("
                << "nullptr, oOptions.GetCfg(), iid("
                << strIfName << "), "
                << "\"" << strMethod << "\", ";

            INDENT_UP;
            ret = GenActParams( pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << " );";
            INDENT_DOWNL;
            Wa( "return ret;" );
        }
        else /* need serialize */
        {
            Wa( "//Serialize the input parameters" );
            ret = GenSerialArgs(
                pInArgs, "pBuf", true, true );
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

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvr::OutputSync()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );
    guint32 dwCount = dwInCount + dwOutCount;

    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        CCOUT << "gint32 " << strClass << "::"
            << strMethod << "Wrapper( ";
        INDENT_UPL;
        CCOUT << "IEventSink* pCallback";
        if( dwCount == 0 )
        {
        }
        else if( bSerial )
        {
            CCOUT << ", BufPtr& pBuf";
        }
        else
        {
            strArgList =
                ToStringInArgs( pInArgs );

            if( !strArgList.empty() )
                CCOUT << ", " << strArgList;
        }

        CCOUT << " )";
        INDENT_DOWNL;

        BLOCK_OPEN;

        if( dwCount == 0 )
        {
            CCOUT << "ret = this->"
                << strMethod << "();";

            Wa( "CParamList oResp;" );
            BLOCK_OPEN;
            Wa( "oResp[ propReturnValue ] = ret;" );
            Wa( "this->SetResponse( pCallback, oResp.GetCfg() )" );
            BLOCK_CLOSE;
        }
        else if( !bSerial )
        {
            CCOUT << "ret = this->"
                << strMethod << "(";
            INDENT_UPL;

            GenActParams( pInArgs, pOutArgs );

            if( dwInCount > 0 )
                CCOUT <<  " );";
            else
                CCOUT << ");";
            INDENT_DOWNL;

            Wa( "CParamList oResp;" );
            Wa( "oResp[ propReturnValue ] = ret;" );
            Wa( "if( SUCCEEDED( ret ) )" );
            BLOCK_OPEN;

            std::vector< std::string > vecArgs;
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

            DeclLocals( pInArgs );

            NEW_LINE;
            DeclLocals( pOutArgs );

            NEW_LINE;

            if( dwInCount > 0 )
            {
                ret = GenDeserialArgs(
                    pInArgs, "pBuf", false, false );
            }

            // call the user's handler
            CCOUT << "ret =" << strMethod << "(";
            INDENT_UP;
            NEW_LINE;

            GenActParams( pInArgs, pOutArgs );

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
            ret = GenSerialArgs(
                pOutArgs, "pBuf2", false, false );
            if( ERROR( ret ) )
                break;

            Wa( "oResp.push( pBuf );" );
            BLOCK_CLOSE;
        }

        Wa( "return ret" );
        BLOCK_CLOSE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvr::OutputAsync()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    guint32 dwInCount = GetArgCount( pInArgs );
    guint32 dwOutCount = GetArgCount( pOutArgs );

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

        DeclLocals( pInArgs );
        NEW_LINE;

        DeclLocals( pOutArgs );
        NEW_LINE;

        ret = GenDeserialArgs(
            pInArgs, "pBuf", false, false );
        if( ERROR( ret ) )
            break;

        // call the user's handler
        CCOUT << "ret ="
            << strMethod << "( pCallback,";

        INDENT_UP;
        NEW_LINE;

        GenActParams( pInArgs, pOutArgs );

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
        ret = GenSerialArgs(
            pOutArgs, "pBuf2", false, false );
        Wa( "oResp.push( pBuf );" );
        BLOCK_CLOSE;

        Wa( "return ret" );
        BLOCK_CLOSE;

    }while( 0 );
    
    return ret;
}

gint32 CImplIfMethodSvr::OutputAsyncCallback()
{
    gint32 ret = 0;
    std::string strClass = "I";
    strClass += m_pIf->GetName() + "_SImpl";
    std::string strMethod = m_pNode->GetName();

    ObjPtr pInArgs = m_pNode->GetInArgs();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();

    bool bSerial = m_pNode->IsSerialize();

    do{
        std::string strArgList;
        Wa( "// call me when you have completed" );
        Wa( "// the asynchronous operation" );

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

        Wa( "if( ret == STATUS_PENDING )" );
        INDENT_UP;
        Wa( "ret = ERROR_STATE;" );
        INDENT_DOWN;
        Wa( "else" );
        INDENT_UP;
        Wa( "ret = iRet;" );
        INDENT_DOWN;
        Wa( "CParamList oResp;" );
        Wa( "oResp[ propReturnValue ] = ret;" );

        if( bSerial )
        {
            Wa( "if( SUCCEEDED( ret ) )" );
            BLOCK_OPEN;
            Wa( "BufPtr pBuf( true );" );
            ret = GenSerialArgs(
                pOutArgs, "pBuf", true, true );
            if( ERROR( ret ) )
                break;
            Wa( "oResp.Push( pBuf )" );
            BLOCK_CLOSE;
        }
        else
        {
            Wa( "if( SUCCEEDED( ret ) )" );
            BLOCK_OPEN;
            std::vector< std::string > vecArgs;
            ret = GetArgsForCall( pOutArgs, vecArgs );
            if( ERROR( ret ) )
                break;

            for( auto& elem : vecArgs )
            {
                CCOUT << "oResp.Push( "
                    << elem << " );";
            }
            BLOCK_CLOSE;
        }

        Wa( "this->OnServiceComplete( " );
        INDENT_UPL;
        CCOUT << "( IConfigDb* )oResp.GetCfg(), pCallback )";
        INDENT_DOWNL;
        Wa( "return ret;" );

        BLOCK_CLOSE;

    }while( 0 );
    
    return ret;
}

