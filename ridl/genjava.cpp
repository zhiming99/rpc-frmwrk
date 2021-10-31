/*
 * =====================================================================================
 *
 *       Filename:  genjava.cpp
 *
 *    Description:  implementations classes of proxy/server generator for Java
 *
 *        Version:  1.0
 *        Created:  10/22/2021 10:47:05 PM
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
#include "genjava.h"

extern CDeclMap g_mapDecls;
extern ObjPtr g_pRootNode;
extern CAliasMap g_mapAliases;
extern stdstr g_strPrefix;

extern std::string g_strAppName;
extern gint32 SetStructRefs( ObjPtr& pRoot );
extern guint32 GenClsid( const std::string& strName );

gint32 CJTypeHelper::GetArrayTypeText(
    ObjPtr& pObj, stdstr& strText )
{
    if( pObj.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CArrayType* pat = pObj;
        ObjPtr pElemType =
            pat->GetElemType();
        if( pElemType.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        stdstr strElemText;
        ret = GetTypeText(
            pElemType, strElemText );
        if( ERROR( ret ) )
            break;

        strText = strElemText + "[]";
        break;

    }while( 0 );

    return ret;
}

gint32 CJTypeHelper::GetMapTypeText(
    ObjPtr& pObj, stdstr& strText )
{
    if( pObj.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CMapType* pmt = pObj;
        ObjPtr pKeyType =
            pmt->GetKeyType();
        if( pKeyType.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        stdstr strKeyText;
        ret = GetTypeText( pKeyType, strKeyText );
        if( ERROR( ret ) )
            break;

        if( m_mapTypeCvt.find( strKeyText ) )
        {
            strKeyText =
                m_mapTypeCvt[ strKeyText ];
        }

        ObjPtr pValType =
            pmt->GetElemType();
        if( pValType.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        stdstr strValText;
        ret = GetTypeText( pValType, strValText );
        if( ERROR( ret ) )
            break;

        if( m_mapTypeCvt.find( strValText ) )
        {
            strValText =
                m_mapTypeCvt[ strValText ];
        }

        strText = "Map< ";
        strText += strKeyText +
            ", " + strValText + " >";

    }while( 0 );

    return ret;
}

gint32 CJTypeHelper::GetStructTypeText(
    ObjPtr& pObj, stdstr& strText )
{
    CStructDecl* psd = pObj;
    if( psd == nullptr )
        return -EINVAL;
    strText = psd->GetName();
    return 0;
}

gint32 CJTypeHelper::GetTypeText(
    ObjPtr& pObj, stdstr& strText )
{
    stdstr strSig;
    stdstr strTypeText;
    gint32 ret = 0;
    if( pObj.IsEmpty() )
        return -EINVAL;
    do{
        EnumClsid iClsid = pObj->GetClsid();
        switch( iClsid )
        {
        case clsid( CArrayType ):
            {
                ret = GetArrayTypeText(
                    pObj, strTypeText );
                break;
            }
        case clsid( CMapType ):
            {
                ret = GetMapTypeText(
                    pObj, strTypeText );
                break;
            }
        case clsid( CStructDecl ):
            {
                CStructDecl* pNode = pObj;
                if( pNode == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                strTypeText = pNode->GetName();
                break;
            }
        case clsid( CStructRef ):
            {
                CStructRef* pRef = pObj;
                if( pRef == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                stdstr strRef = pNode->GetName();
                ObjPtr pRefType;
                stdstr strName = pRef->GetName();
                ret = g_mapDecls.GetDeclNode(
                    strName, pRefType );
                if( SUCCEEDED( ret ) )
                {
                    ret = GetStructTypeText(
                        pRefType, strTypeText );
                    break;
                }
                ret = g_mapAliases.GetAliasType(
                    strName, pRefType );
                if( ERROR( ret ) )
                    break;

                ret = GetTypeText(
                    pRefType, strTypeText );
                break;
            }
        case clsid( CPrimeType ):
            {
                CPrimeType* pType = pObj;
                stdstr strSig =
                    pType->GetSignature();
                char ch = strSig[ 0 ];
                std::map< char, stdstr >::iterator
                itr = m_mapSig2JTp.find( ch );
                if( itr == m_mapSig2JTp.end() )
                {
                    ret = -ENOENT;
                    break;
                }
                strTypeText = itr->second;
                break;
            }
        default:
            ret = -EINVAL;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CJTypeHelper::GetFormalArgList(
    ObjPtr& pArgs,
    std::vector< STRPAIR >& vecArgs )
{
    if( pArgs.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CArgList* pArgList = pArgs;
        if( pArgList == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        std::vector<STRPAIR> vecArgs;
        guint32 dwCount = pArgList->GetCount();
        if( dwCount == 0 )
            break;

        for( guint32 i = 0; i < dwCount; i++ )
        {
            ObjPtr pObj =
                pArgList->GetChild( i );

            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            ObjPtr pType = pfa->GetType();

            stdstr strType, strArgName;
            ret = GetTypeText(
                pType, strType );
            if( ERROR( ret ) )
                break;

            strArgName = pfa->GetName();
            vecArgs.push_back(
                {strType, strArgName});
        }

    }while( 0 );

    return ret;
}

gint32 CJTypeHelper::GetActArgList(
    ObjPtr& pArgs,
    std::vector< STRPAIR >& vecArgs )
{
    if( pArgs.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CArgList* pArgList = pArgs;
        if( pArgList == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        std::vector<stdstr> vecArgs;
        guint32 dwCount = pArgList->GetCount();
        if( dwCount == 0 )
            break;

        for( guint32 i = 0; i < dwCount; i++ )
        {
            ObjPtr pObj =
                pArgList->GetChild( i );

            CFormalArg* pfa = pObj;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            strArgName = pfa->GetName();
            vecArgs.push_back( strArgName );
        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitFormalArgList(
    ObjPtr& pArgs )
{
    if( pArgs.IsEmpty() || pWriter == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    std::vector< STRPAIR > vecArgs;
    do{
        ret = CJTypeHelper::GetFormalArgListJava(
            pArgs, vecArgs );
        if( ERROR( ret ) )
            break;

        if( vecArgs.empty() )
            break;

        int i = 0;
        for( ; i < vecArgs.size(); i++ )
        {
            auto& elem = vecArgs[ i ];
            CCOUT << elem.first << " "
                << elem.second;

            if( i < vecArgs.size() - 1 )
            {
                CCOUT << ",";
                NEW_LINE;
            }
            else
            {
                CCOUT << " ";
            }
        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitActArgList(
    ObjPtr& pArgs )
{
    if( pArgs.IsEmpty() || pWriter == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    std::vector< stdstr > vecArgs;
    do{
        ret = CJTypeHelper::GetActArgList(
            pArgs, vecArgs );
        if( ERROR( ret ) )
            break;

        if( vecArgs.empty() )
            break;

        int i = 0;
        for( ; i < vecArgs.size(); i++ )
        {
            auto& elem = vecArgs[ i ];
            CCOUT << elem;
            if( i < vecArgs.size() - 1 )
            {
                CCOUT << ",";
                NEW_LINE;
            }
            else
            {
                CCOUT << " ";
            }
        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitNewBufPtrSerial(
    guint32 dwSize )
{
    gint32 ret = 0;
    do{
        Wa( "BufPtr _pBuf = new BufPtr( true );" );
        CCOUT << "_pBuf.Resize(" << dwSize << ");";
        NEW_LINE;
        Wa( "Integer _offset = 0;" );

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitByteBufferForDeserial(
    const stdstr& strBuf )
{
    gint32 ret = 0;
    do{
        CCOUT<< "int iSize = " << strBuf << ".length;";
        NEW_LINE;
        Wa( "if( iSize > RC.MAX_BUF_SIZE )" );
        Wa( "{ ret = -RC.ERANGE; break;}" );
        Wa(
            "ByteBuffer _bbuf = new ByteBuffer();" );
        CCOUT << "_bbuf.allocate( " << iSize << ");";
        NEW_LINE;
        CCOUT << "_bbuf.put( " << strBuf << " );";
        NEW_LINE;
        Wa( "_bbuf.flip();");

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitSerialArgs(
    ObjPtr& pArgs,
    const stdstr& strName,
    bool bCast )
{
    if( pArgs.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CArgList* pal = pArgs;
        CArgListUtils oau;
        gint32 iCount = oau.GetArgCount( pal );
        if( iCount == 0 )
            break;

        for( gint32 i = 0; i < iCount; i++ )
        {
            ObjPtr pChild = pal->GetChild( i );
            CFormalArg* pfa = pChild;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strType;
            ObjPtr pType = pfa->GetType();
            stdstr strSig =
                GetTypeSig( pType );

            if( strName.empty() )
                strName = pfa->GetName();
            else
            {
                strName = strName + "[ " +
                    std::to_string( i ) + " ]";
            }

            stdstr strCast;
            if( bCast )
            {
                ret = EmitCastArgFromObject(
                    pfa, strName, strCast );
                if( ERROR( ret ) )
                    break;
                strName = strCast;
            }

            switch( strSig[ 0 ] )
            {
            case 'Q':
            case 'q':
                CCOUT << "ret = _osh.serialInt64( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'D':
            case 'd':
                CCOUT << "ret = _osh.serialInt32( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'W':
            case 'w':
                CCOUT << "ret = _osh.serialInt16( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'f':
                CCOUT << "ret = _osh.serialFloat( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'F':
                CCOUT << "ret = _osh.serialDouble( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'b':
                CCOUT << "ret = _osh.serialBool( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'B':
                CCOUT << "ret = _osh.serialInt8( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'h':
                CCOUT << "ret = _osh.serialHStream( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 's':
                CCOUT << "ret = _osh.serialString( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'a':
                CCOUT << "ret = _osh.serialBuf( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'o':
                CCOUT << "ret = _osh.serialObjPtr( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'O':
                CCOUT << "ret = _osh.serialStruct( _pBuf, _offset, "
                    << strName << " );";
                break;
            case '(':
                CCOUT << "ret = _osh.serialArray( _pBuf, _offset, "
                    << strName << ", \"" << strSig << "\");";
                break;
            case '[':
                CCOUT << "ret = _osh.serialMap( _pBuf, _offset, "
                    << strName << ", \"" << strSig << "\");";
                break;
            default
                ret = -EINVAL;
                break;
            }
            if( ERROR( ret ) )
                break;
            NEW_LINE;
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );

        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitDeserialArgs(
    ObjPtr& pArgs, bool bDecl )
{
    if( pArgs.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CArgList* pal = pArgs;
        CArgListUtils oau;
        gint32 iCount = oau.GetArgCount( pal );
        if( iCount == 0 )
            break;

        for( gint32 i = 0; i < iCount; i++ )
        {
            ObjPtr pChild = pal->GetChild( i );
            CFormalArg* pfa = pChild;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strType;
            ObjPtr pType = pfa->GetType();
            stdstr strSig =
                GetTypeSig( pType );

            stdstr strTypeText;
            ret = CJTypeHelper::GetTypeText(
                    pType, strTypeText );
            if( ERROR( ret ) )
                break;

            stdstr strName = pfa->GetName();
            if( bDecl )
            {
                strName =
                    strTypeText + " " + strName;
            }
            switch( strSig[ 0 ] )
            {
            case 'Q':
            case 'q':
                CCOUT << strName " = _osh.deserialInt64( _bbuf );"
                break;
            case 'D':
            case 'd':
                CCOUT << strName " = _osh.deserialInt32( _bbuf );"
                break;
            case 'W':
            case 'w':
                CCOUT << strName " = _osh.deserialInt16( _bbuf );"
                break;
            case 'f':
                CCOUT << strName " = _osh.deserialFloat( _bbuf );"
                break;
            case 'F':
                CCOUT << strName " = _osh.deserialDouble( _bbuf );"
                break;
            case 'b':
                CCOUT << strName " = _osh.deserialBool( _bbuf );"
                break;
            case 'B':
                CCOUT << strName " = _osh.deserialInt8( _bbuf );"
                break;
            case 'h':
                CCOUT << strName " = _osh.deserialHStream( _bbuf );"
                break;
            case 's':
                CCOUT << strName " = _osh.deserialString( _bbuf );"
                break;
            case 'a':
                CCOUT << strName " = _osh.deserialBuf( _bbuf );"
                break;
            case 'o':
                CCOUT << strName " = _osh.deserialObjPtr( _bbuf );"
                break;
            case 'O':
                CCOUT << strName " = ("<<strTypeText<<") _osh.deserialStruct( _bbuf );"
                break;
            case '(':
                CCOUT << strName " = ("<<strTypeText<<") _osh.deserialArray( _bbuf, \""<< strSig << "\" );"
                break;
            case '[':
                CCOUT << strName " = ("<<strTypeText<<") _osh.deserialMap( _bbuf, \""<< strSig << "\" );"
                break;
            default
                ret = -EINVAL;
                break;
            }
            if( ERROR( ret ) )
                break;
            NEW_LINE;
        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitDeclArgs(
    ObjPtr& pArgs, bool bInit )
{
    if( pArgs.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CArgList* pal = pArgs;
        CArgListUtils oau;
        gint32 iCount = oau.GetArgCount( pal );
        if( iCount == 0 )
            break;

        for( gint32 i = 0; i < iCount; i++ )
        {
            ObjPtr pChild = pal->GetChild( i );
            CFormalArg* pfa = pChild;
            if( pfa == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strType;
            ObjPtr pType = pfa->GetType();
            ret = CJTypeHelper::GetTypeText(
                pType, strType );
            if( ERROR( ret ) )
                break;

            stdstr strName = pfa->GetName();
            if( !bInit )
            {
                CCOUT << "public " << strType << " "
                    << strName << ";";
            }
            else
            {
                stdstr strVal =
                    GetDefValOfType( pType );
                CCOUT << "public " << strType << " "
                    << strName << " = " 
                    << strVal << ";";
            }
            NEW_LINE;
        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitCastArgFromObject(
    ObjPtr& pArg,
    const stdstr& strVar,
    stdstr& strCast )
{
    if( pArg.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CFormalArg* pfa = pArg;
        if( pfa == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        stdstr strType;
        ObjPtr pType = pfa->GetType();
        ret = CJTypeHelper::GetTypeText(
            pType, strType );
        if( ERROR( ret ) )
            break;

        stdstr strNewType;
        CJTypeHelper::GetObjectPrimitive(
            strType, strNewType );
        strCast = "( ";
        strCast += strNewType + " )" + strVar;

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitCastArgsFromObject(
    ObjPtr& pMethod, bool bIn,
    const stdstr strObjArr )
{
    if( pMethod.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        CMethodDecl* pmd = pMethod;
        if( pmd == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ObjPtr pArgs;
        if( bIn )
            pArgs = pmd->GetInArgs();
        else
            pArgs = pmd->GetOutArgs();
        
        if( pArgs.IsEmpty() )
            break;
            
        gint32 iCount = oau.GetArgCount( pArgs );
        if( iCount == 0 )
            break;

        CArgList* pal = pArgs;
        for( gint32 i = 0; i < iCount; ++i )
        {
            ObjPtr pArg = pal->GetChild( i );
            CFormalArg pfa = pArg;

            stdstr strName = strObjArr + "[" +
                std::to_string( i ) + "]";

            stdstr strCast;
            ret = EmitCastArgFromObject(
                pArg, strName, strCast );
            if( ERROR( ret ) )
                break;
            CCOUT << strCast;
            if( i < iCount - 1 )
            {
                CCOUT << ",";
                NEW_LINE;
            }
            else
            {
                CCOUT << " ";
            }
        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitCatchExcept(
    stdstr strExcept, bool bSetRet )
{
    CCOUT << "catch( " << strExcept << " e )";
    NEW_LINE;
    if( !bSetRet )
        Wa( "{}" );
    else
    {
        Wa( "{" );
        if( strExcept == "NullPointerException" )
            Wa( "    ret = -RC.EFAULT;");
        else if( strExcept == "IllegalArgumentException" )
            Wa( "    ret = -RC.EINVAL;");
        else if( strExcept == "IndexOutOfBoundsException" )
            Wa( "    ret = -RC.ERANGE;");
        else if( strExcept == "Exception" )
            Wa( "    ret = RC.ERROR_FAIL;");
        Wa( "}");
    }
    return 0;
}

gint32 CJavaSnippet::EmitBanner()
{
    Wa( "GENERATED BY RIDLC. MAKE SURE TO BACKUP BEFORE RE-COMPILING." );
    CCOUT << "package " << g_strPrefix << "." << g_strAppName << ";";
    NEW_LINE;
    Wa( "import org.rpcf.rpcbase.*;" );
    Wa( "import java.util.Map;" );
    Wa( "import java.util.HashMap;" );
    Wa( "import java.lang.String;" );
    NEW_LINE;
}

gint32 CJavaSnippet::EmitArgClassObj(
    ObjPtr& pArgs )
{
    std::vector< STRPAIR > vecArgs;
    CJTypeHelper::GetFormalArgList( pArgs, vecArgs );
    for( gint32 i = 0; i < vecArgs.size(); i++ )
    {
        CCOUT << vecArgs[ i ].first << ".class";
        if( i < vecArgs.size() - 1 )
        {
            CCOUT << ",";
            NEW_LINE;
        }
    }
    return 0;
}

gint32 GenJavaProj(
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

        CJavaWriter oWriter(
            strOutPath, strAppName, pRoot );

        ret = GenStructsFile( &oWriter, pRoot );
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

CImplJavaMthdSvr::CImplJavaMthdSvr(
    CJavaWriter* pWriter,
    ObjPtr& pNode,
    ObjPtr& pSvc )
    : super( pWriter )
{
    m_pNode = pNode;
    m_pSvc = pSvc;
    if( m_pNode == nullptr || m_pSvc == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'method/service' node" );
        throw std::runtime_error( strMsg );
    }
    CAstNodeBase* pMdl = m_pNode->GetParent();
    m_pIf = ObjPtr( pMdl->GetParent() );
}

gint32 CImplJavaMthdSvr::DeclAbstractFuncs()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iCount = GetArgCount( pInArgs );
    CInterfaceDecl* pIf = m_pIf;
    stdstr strIfName = pIf->GetName();

    CCOUT << "// " << strIfName << "::" << strName;
    NEW_LINE;
    for( gint32 i = 0; i < 2; i++ )
    {
        if( i == 0 )
        {
            CCOUT << "public abstract int "
                << strName << "(";
        }
        else
        {
            if( !pNode->IsAsyncs() ||
                pNode->IsNoReply() )
                break;
            CCOUT << "public abstract void on"
                << strName << "Canceled(";
        }
        INDENT_UPL;
        CCOUT << "JavaReqContext oReqCtx";
        if( iCount == 0 )
        {
            CCOUT << " );";
        }
        else
        {
            Wa( "," );
            ret = CJavaSnippet::EmitFormalArgList(
                pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT " );";
        }
        INDENT_DOWNL;
        NEW_LINE;
    }
    return ret;
}

gint32 CImplJavaMthdSvr::ImplNewCancelNotify()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    CServiceDecl* pSvc = m_pSvc;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    stdstr strSvcName = pSvc->GetName() + "svrbase";
    if( !pNode->IsAsyncs() ||
        pNode->IsNoReply() )
        return 0;

    do{
        Wa( "private ICancelNotify newCancelNotify(" );
        CCOUT << "    " << strSvcName << " oHost,";
        NEW_LINE;
        CCOUT << "JavaReqContext oContext )";
        BLOCK_OPEN;
        Wa( "return new ICancelNotify()" );
        BLOCK_OPEN;
        CCOUT << strSvcName << " m_oHost = oHost;";
        NEW_LINE;
        Wa( "JavaReqContext m_oReqCtx = oContext;" );
        Wa( "public int getArgCount()" );
        CCOUT << "{ return " << iInCount << " };";
        NEW_LINE;
        Wa( "public Class<?>[] getArgTypes()" );
        BLOCK_OPEN
        CCOUT "return new Class[] {";
        INDENT_UPL;
        CJavaSnippet::EmitArgClassObj( pInArgs );
        INDENT_DOWN;
        CCOUT << "};";
        BLOCK_CLOSE;

        Wa( "public void onAsyncResp( Object oContext," );
        Wa( "    int iRet, Object[] oParams )" );
        BLOCK_OPEN;
        if( iInCount == 0 )
        {
            CCOUT "m_oHost.onEchoCanceled( m_oReqCtx, iRet )";
        }
        else
        {
            CCOUT << "if( oParams.length != " << iInCount << " )";
            NEW_LINE;
            Wa( "    return;" );
            Wa( "m_oHost.onEchoCanceled(" );
            CCOUT << "    m_oReqCtx, iRet,";
            INDENT_UPL;
            CJavaSnippet::EmitCastArgsFromObject(
                pNode, true, "oParams" );
            CCOUT << ");";
            INDENT_DOWN;
        }
        BLOCK_CLOSE;

        BLOCK_CLOSE; // return new ICancelNotify();
        CCOUT << ";";
        BLOCK_CLOSE; // newCancelNotify

    }while( 0 );
    return ret;
}

gint32 CImplJavaMthdSvr::OutputReqHandler()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    CInterfaceDecl* pIf = m_pIf;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    gint32 iOutCount = GetArgCount( pInArgs );
    stdstr strIfName = pIf->GetName();

    do{
        CCOUT << "// Wrapper for " << strIfName << "::" << strName;
        NEW_LINE;
        Wa( "val = new IReqHandler() {" );
        INDENT_UPL;
        Wa( "public int getArgCount()" );
        CCOUT << "{ return" << iInCount << "; }";
        Wa( "public Class<?>[] getArgTypes()" );
        if( iInCount == 0 )
        {
            Wa( "{ return new Class[ 0 ]; }" );
            break;
        }
        BLOCK_OPEN;
        Wa( "Class[] o ={" );
        INDENT_UPL;
        CJavaSnippet::EmitArgClassObj( pInArgs );
        INDENT_DOWNL;
        Wa( "};" );
        Wa( "return o;" );
        BLOCK_CLOSE;

        ret = ImplNewCancelNotify();
        if( ERROR( ret ) )
            break;

        ret = ImplInvoke();
        if( ERROR( ret ) )
            break;

        INDENT_DOWNL;
        Wa( "};" );
        stdstr strIfName = pIf->GetName();
        CCOUT << "o.put( \"" << strIfName << "::"
            << strName << "\", val );";

    }while( 0 );

    return ret;
}

gint32 CImplJavaMthdSvr::ImplReqContext()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    CServiceDecl* pSvc = m_pSvc;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    ObjPtr pOutArgs = pNode->GetOutArgs();
    gint32 iOutCount = GetArgCount( pOutArgs );
    do{
        Wa( "JavaReqContext _oReqCtx =" );
        Wa( "    new JavaReqContext( oOuterObj, callback )" );
        BLOCK_OPEN;
        Wa( "public int iRet;" );
        CJavaSnippet::EmitDeclArgs( pOutArgs, false );
        if( !pNode->IsNoReply() )
        {
            Wa( "// call this method when the handler" );
            Wa( "// returns immediately and successfully" );
            Wa( "public void setResponse(" );
            Wa( "    int ret, Object ...args )" );
            BLOCK_OPEN;
            Wa( "iRet = ret;" );
            if( iOutCount > 0 )
            {
                Wa( "if( RC.SUCCEEDED( ret ) )" );
                BLOCK_OPEN;
                std::vector< STRPAIR > vecArgs;
                ret = CJTypeHelper::GetFormalArgList(
                    pOutArgs, vecArgs );
                if( ERROR( ret ) )
                    break;

                for( gint32 i = 0; i < vecArgs.size(); i++ )
                {
                    auto& elem = vecArgs[ i ];
                    CCOUT << elem.second " = ( "
                        << elem.first << " )"
                        << "args[ " << i << " ];";
                }
                BLOCK_CLOSE;
            }
            Wa( "m_bSet = true;" );
            BLOCK_CLOSE;

            Wa( "public Object[] getResponse()" )
            BLOCK_OPEN;
            if( iOutCount == 0 )
            {
                Wa( "return new Object[ 0 ];" );
            }
            else
            {
                Wa( "if( !m_bSet )" );
                Wa( "    return new Object[ 0 ];" );
                Wa( "return new Object[]{" );
                CJavaSnippet::EmitActArgList( pOutArgs );
                Wa( "};" );
            }
            BLOCK_CLOSE;

            ret = ImplSvcComplete();
            if( ERROR( ret ) )
                break;
        }

        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

gint32 CImplJavaMthdSvr::ImplSvcComplete()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    CServiceDecl* pSvc = m_pSvc;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    ObjPtr pOutArgs = pNode->GetOutArgs();
    gint32 iOutCount = GetArgCount( pOutArgs );

    if( !pNode->IsAsyncs() )
        return 0;

    if( pNode->IsNoReply() )
        return 0;

    do{
        Wa( "// call this method when the"
        Wa( "// request has completed
        Wa( "// asynchronously
        Wa( "public int onServiceComplete(" );
        Wa( "    int iRet, Object ...args )" );
        BLOCK_OPEN;
        Wa( "int ret = 0;" );
        Wa( "BufPtr _pBuf = null" );
        Wa( "JavaRpcServer oHost =" );
        Wa( "    ( JavaRpcServer )m_oHost;" );
        if( iOutCount > 0 )
        {
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "if( RC.ERROR( iRet ) )" );
            Wa( "    break;" );
            CCOUT << "if( args.length != " << iOutCount << " )";
            BLOCK_OPEN;
            Wa( "iRet = -RC.EINVAL;" );
            Wa( "    break;" );
            BLOCK_CLOSE;
            Wa( "JavaSerialHelperS _osh =" );
            Wa( "    new JavaSerialHelperS( oHost.getInst() );" );

            Wa( "_pBuf = new BufPtr( true );" );
            Wa( "ret = _pBuf.Resize( 1024 );" );
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "Integer _offset = 0;" );

            ret = CJavaSnippet::EmitSerialArgs(
                pOutArgs, "args", true );
            if( ERROR( ret ) )
                break;

            BLOCK_CLOSE;
            CCOUT << "while( false );";
            NEW_LINE;
            Wa( "if( RC.ERROR( ret ) &&" );
            Wa( "    RC.SUCCEEDED( iRet ) )" );
            Wa( "    iRet = ret;" );
        }
        Wa( "return oHost.ridlOnServiceComplete(" );
        Wa( "    getCallback(), iRet, _pBuf );" );
        BLOCK_CLOSE;
        Wa( ";" );

    }while( 0 );

    return ret;
}

gint32 CImplJavaMthdSvr::ImplInvoke()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    CServiceDecl* pSvc = m_pSvc;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    ObjPtr pOutArgs = pNode->GetOutArgs();
    gint32 iOutCount = GetArgCount( pOutArgs );
    stdstr strSvc = pSvc->GetName();
    do{
        Wa( "public JRetVal invoke( Object oOuterObj," );
        Wa( "    ObjPtr callback, Object[] oParams )" );
        NEW_LINE;
        ret = ImplReqContext();
        if( ERROR( ret ) )
            break;
        Wa( "int ret = 0" );
        Wa( "JRetVal jret = new JRetVal();" );
        Wa( "do" );
        BLOCK_OPEN;
        CCOUT << strSvc << "svrbase oHost =" );
        NEW_LINE;
        CCOUT << "    ( " << strSvc << "svrbase )oOuterObj;" );
        NEW_LINE;
        if( iInCount == 0 )
        {
            CCOUT "ret = oHost." << strName << "( _oReqCtx );";
            NEW_LINE;
        }
        else
        {
           Wa( "JavaSerialHelperS _osh =" );
           Wa( "    new JavaSerialHelperS( oHost.getInst() );" );
           Wa( "if( oParams.length != 1 )" );
           Wa( "{ ret = -RC.EINVAL; break; }" );
           Wa( "byte[] _buf = ( byte[] )oParams[ 0 ];" );
           ret = EmitDeserialArgs( pInArgs, true );
           if( ERROR( ret ) )
               break;
           CCOUT "ret = oHost." << strName << "( _oReqCtx,";
           NEW_LINE;
           ret = CJavaSnippet::EmitActArgList( pInArgs );
           if( ERROR( ret ) )
               break;
           Wa( ");" );
           if( pNode->IsNoReply() )
           {
               Wa( "ret = 0" );
           }
           else
           {
                if( !pNode->IsAsyncs() )
                {
                    Wa( "if( ret == RC.STATUS_PENDING )" );
                    Wa( "    ret = RC.ERROR_STATE;" );
                }
                else
                {
                    Wa( "if( ret == RC.STATUS_PENDING )" );
                    BLOCK_OPEN;
                    guint32 dwTimeout = pNode->GetTimeoutSec();
                    Wa( "oHost.getInst().SetInvTimeout(" );
                    CCOUT << "    _oReqCtx.getCallback(), " <<
                        std::to_string( dwTimeout ) << " );";

                    Wa( "ICancelNotify o =" );
                    Wa( "    newCancelNotify( oHost, _oReqCtx );" );
                    Wa( "int iRet = oHost.installCancelNotify(" );
                    Wa( "    _oReqCtx.getCallback(), o," );
                    Wa( "    new Object[]" );
                    BLOCK_OPEN;
                    ret = CJavaSnippet.EmitActArgList( pInArgs );
                    BLOCK_CLOSE;
                    wa( " );" );
                    Wa( "if( RC.ERROR( iRet ) )" );
                    Wa( "    ret = iRet;" );
                    Wa( "break;" );
                    BLOCK_CLOSE;
                }

                Wa( "if( RC.ERROR( ret ) )" );
                Wa( "    break;" );
                if( iOutCount > 0 )
                {
                    Wa( "if( !_oReqCtx.hasResponse() )" );
                    Wa( "{ ret = -RC.ENODATA; break; }" );
                    Wa( "do" );
                    BLOCK_OPEN;
                    Wa( "BufPtr _pBuf = new BufPtr( true );" );
                    Wa( "ret = _pBuf.Resize( 1024 );" );
                    Wa( "if( RC.ERROR( ret ) )" );
                    Wa( "    break;" );
                    Wa( "Integer _offset = 0;" );
                    Wa( "Object[] oResp = _oReqCtx.getResponse();" );
                    ret = CJavaSnippet::EmitSerialArgs(
                        pOutArgs, oResp, true );
                    if( ERROR( ret ) )
                        break;
                    Wa( "ret = _pBuf.Resize( _offset );");
                    Wa( "if( RC.ERROR( ret ) ) break;" );
                    Wa( "jret.addElem( _pBuf );" );
                    BLOCK_CLOSE;
                    Wa( "while( false );" );
                }
           }
        }
        BLOCK_CLOSE;
        Wa( "while( false );" );
        Wa( "jret.setError( ret );" );

    }while( 0 );

    return ret;
}

gint32 CImplJavaMthdSvr::ImplEvent()
{
    gint32 ret = 0;
    stdstr strName = m_pNode->GetName();
    ObjPtr pInArgs = m_pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    stdstr strIfName = m_pIf->GetName();

    do{
        if( iInCount == 0 )
        {
            CCOUT << "public int " << strName << "( Object callback )";
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "return ridlSendEvent( callback," );
            CCOUT << "    \"" << strIfName << "\","
            NEW_LINE;
            CCOUT << "    \"" << strName << "\","
            NEW_LINE;
            CCOUT << "    \"\", null );"
            NEW_LINE;
            BLOCK_CLOSE;
        }
        else
        {
            CCOUT << "public int " << strName << "( Object callback,";
            INDENT_UPL;
            ret = CJavaSnippet::EmitFormalArgList( pInArgs );
            if( ERROR( ret ) )
                break;
            Wa( ")" );
            INDENT_DOWNL;
            BLOCK_OPEN;
            Wa( "int ret = 0;" );
            Wa( "do" );
            BLOCK_OPEN;

            Wa( "JavaSerialHelperS _osh =" );
            Wa( "    new JavaSerialHelperS( getInst() );" );
            Wa( "BufPtr _pBuf = new BufPtr( true );" );
            Wa( "ret = _pBuf.Resize( 1024 );" );
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "Integer _offset = 0;" );
            ret = CJavaSnippet::EmitSerialArgs(
                pInArgs, "", false );

            Wa( "_pBuf.Resize( _offset );" );
            Wa( "ret = ridlSendEvent( callback," );
            CCOUT << "    \"" << strIfName << "\","
            NEW_LINE;
            CCOUT << "    \"" << strName << "\","
            NEW_LINE;
            CCOUT << "    \"\", _pBuf );"
            NEW_LINE;
            BLOCK_CLOSE;
            Wa( "while( false );" );
            Wa( "return ret;" );
            BLOCK_CLOSE;
        }

    }while( 0 );

    return ret;
}

CImplJavaSvcsvrbase::CImplJavaSvcsvrbase(
    CJavaWriter* pWriter,
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

int CImplJavaSvcsvrbase::Output()
{
    CJavaSnippet::EmitBanner();
    gint32 ret = 0;
    do{
        CServiceDecl* pSvc = m_pNode;
        std::vector< ObjPtr > vecIfs;
        ret = pSvc->GetIfRefs( vecIfs );
        if( ERROR( ret ) )
            break;

        std::vector< ObjPtr > vecifds;
        for( auto elem& : vecIfs )
        {
            CInterfRef* pifr = elem;
            ObjPtr pifd;
            ret = pifr->GetIfDecl( pifd );
            if( ERROR( ret ) )
                continue;
            vecifds.push_back( pifd );
        }

        if( vecIfds.empty() )
            break;

        stdstr strName = pSvc->GetName();
        CCOUT << "abstract public class " << strName
            << "svrbase extends JavaRpcServer";
        NEW_LINE;
        BLOCK_OPEN;

        CCOUT << "public " << strName << "svrbase( ObjPtr pIoMgr,";
        NEW_LINE;
        Wa( "    String strDesc, String strSvrObj )" );
        Wa( "{ super( pIoMgr, strDesc, strSvrObj ); }" );

        std::vector< ObjPtr > vecMethods;
        std::vector< ObjPtr > vecEvents;
        for( auto& elem : vecIfds )
        {
            CInterfaceDecl* pifd = elem;
            if( pifd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ObjPtr pObj = pifd->GetMethodList();
            CMethodDecls* pmdls = pObj;
            if( pmdls == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            gint32 iCount = pmdls->GetCount();
            if( iCount == 0 )
                continue;

            for( gint32 i = 0; i < iCount; i++ )
            {
                ObjPtr pmd = pmdls->GetChild( i );
                if( pmd.IsEmpty() ) 
                {
                    ret = -EFAULT;
                    break;
                }
                CMethodDecl* pmethod = pmd;
                if( pmethod->IsEvent() )
                    vecEvents.push_back( pmd );
                else
                    vecMethods.push_back( pmd );
            }
        }

        for( auto& elem : vecMethods )
        {
            CImplJavaMthdSvr ojms(
                m_pWriter, elem,
                ObjPtr( m_pNode ) );

            ojms.DeclAbstractFuncs();
        }
        
        Wa( "public Map< String, IReqHandler > initMaps()" );
        BLOCK_OPEN;

        Wa( "Map< String, IReqHandler > o =" );
        Wa( "    new HashMap< String, IReqHandler >();" );
        Wa( "IReqHandler val;" );

        for( auto& elem : vecMethods )
        {
            CImplJavaMthdSvr ojms(
                m_pWriter, elem,
                ObjPtr( m_pNode ) );

            ojms.OutputReqHandler();
        }

        Wa( "return o;" );
        BLOCK_CLOSE;

        for( auto& elem : vecEvents )
        {
            CImplJavaMthdSvr ojms(
                m_pWriter, elem,
                ObjPtr( m_pNode ) );
            ojms.OutputEvent();
        }

        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}
