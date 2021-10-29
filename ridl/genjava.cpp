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

gint32 CJavaSnippet::EmitFormalArgListJava(
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
                NEWLINE;
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
        NEWLINE;
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
        NEWLINE;
        Wa( "if( iSize > rpcbaseConstants.MAX_BUF_SIZE )" );
        Wa( "{ ret = -ERANGE; break;}" );
        Wa(
            "ByteBuffer _bbuf = new ByteBuffer();" );
        CCOUT << "_bbuf.allocate( " << iSize << ");";
        NEWLINE;
        CCOUT << "_bbuf.put( " << strBuf << " );";
        NEWLINE;
        Wa( "_bbuf.flip();");

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitSerialArgsJava(
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
                CCOUT << "_osh.serialInt64( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'D':
            case 'd':
                CCOUT << "_osh.serialInt32( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'W':
            case 'w':
                CCOUT << "_osh.serialInt16( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'f':
                CCOUT << "_osh.serialFloat( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'F':
                CCOUT << "_osh.serialDouble( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'b':
                CCOUT << "_osh.serialBool( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'B':
                CCOUT << "_osh.serialInt8( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'h':
                CCOUT << "_osh.serialHStream( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 's':
                CCOUT << "_osh.serialString( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'a':
                CCOUT << "_osh.serialBuf( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'o':
                CCOUT << "_osh.serialObjPtr( _pBuf, _offset, "
                    << strName << " );";
                break;
            case 'O':
                CCOUT << "_osh.serialStruct( _pBuf, _offset, "
                    << strName << " );";
                break;
            case '(':
                CCOUT << "_osh.serialArray( _pBuf, _offset, "
                    << strName << ", \"" << strSig << "\");";
                break;
            case '[':
                CCOUT << "_osh.serialMap( _pBuf, _offset, "
                    << strName << ", \"" << strSig << "\");";
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

gint32 CJavaSnippet::EmitDeserialArgsJava(
    ObjPtr& pArgs )
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
                CCOUT << strName " = ("<<strTypeText<<") _osh.deserialStruct( _bbuf, \""<< strSig << "\" );"
                break;
            case '[':
                CCOUT << strName " = ("<<strTypeText<<") _osh.deserialStruct( _bbuf, \""<< strSig << "\" );"
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
                CCOUT << strType << " "
                    << strName << ";";
            }
            else
            {

            }
            NEWLINE;
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
