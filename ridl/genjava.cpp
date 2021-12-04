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
#include "genpy.h"
#include <fcntl.h>
#include "filetran.h"

extern CDeclMap g_mapDecls;
extern ObjPtr g_pRootNode;
extern CAliasMap g_mapAliases;
extern stdstr g_strPrefix;

std::map<stdstr, ObjPtr> g_mapMapTypeDecl;
std::map<stdstr, ObjPtr> g_mapArrayTypeDecl;

extern std::string g_strAppName;
extern gint32 SetStructRefs( ObjPtr& pRoot );
extern guint32 GenClsid( const std::string& strName );
extern gint32 SyncCfg( const stdstr& strPath );

std::map< stdstr, stdstr > CJTypeHelper::m_mapTypeCvt {
    { "long", "Long" },
    { "int", "Integer" },
    { "short", "Short" },
    { "boolean", "Boolean" },
    { "byte", "Byte" },
    { "float", "Float" },
    { "double", "Double" }
};

std::map< char, stdstr > CJTypeHelper::m_mapSig2JTp {
    { '(' , "[]" },
    { '[' , "Map" },
    { 'O' ,"Object" },
    { 'Q', "long" },
    { 'q', "long" },
    { 'D', "int" },
    { 'd', "int" },
    { 'W', "short" },
    { 'w', "short" },
    { 'b', "boolean" },
    { 'B', "byte" },
    { 'f', "float" },
    { 'F', "double" },
    { 's', "String" },
    { 'a', "byte[]" },
    { 'o', "ObjPtr" },
    { 'h', "long" }
};

std::map< char, stdstr > CJTypeHelper::m_mapSig2DefVal
{
    { '(' , "null" },
    { '[' , "null" },
    { 'O' ,"null" },
    { 'Q', "0" },
    { 'q', "0" },
    { 'D', "0" },
    { 'd', "0" },
    { 'W', "0" },
    { 'w', "0" },
    { 'b', "false" },
    { 'B', "0" },
    { 'f', "0.0" },
    { 'F', "0.0" },
    { 's', "\"\"" },
    { 'a', "null" },
    { 'o', "null" },
    { 'h', "0" }
};

std::string GetTypeSigJava( ObjPtr& pObj )
{
    std::string strSig;
    gint32 ret = 0;
    if( pObj.IsEmpty() )
        return strSig;
    do{
        CAstNodeBase* pType = pObj;
        if( pType->GetClsid() !=
            clsid( CStructRef ) )
        {
            strSig = pType->GetSignature();
        }
        else
        {
            ObjPtr pRefType;
            CStructRef* pRef = pObj;
            stdstr strName = pRef->GetName();
            ret = g_mapDecls.GetDeclNode(
                strName, pRefType );
            if( SUCCEEDED( ret ) )
            {
                stdstr strMsgId = g_strAppName;
                strMsgId += "::" + strName;
                guint32 dwClsid =
                    GenClsid( strMsgId );
                char szHex[ 16 ] = {0};
                sprintf( szHex, "%08X*", dwClsid );
                strSig = strSig + "O" + szHex + "";
                break;
            }
            ret = g_mapAliases.GetAliasType(
                strName, pRefType );
            if( ERROR( ret ) )
                break;
            CAstNodeBase* pNode = pRefType;
            strSig = pNode->GetSignature();
        }
    }while( 0 );

    return strSig;
}
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

        if( m_mapTypeCvt.find( strKeyText ) !=
            m_mapTypeCvt.end() )
        {
            strKeyText =
                m_mapTypeCvt[ strKeyText ];
        }

        ObjPtr pValType = pmt->GetElemType();
        if( pValType.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }
        stdstr strValText;
        ret = GetTypeText( pValType, strValText );
        if( ERROR( ret ) )
            break;

        if( m_mapTypeCvt.find( strValText ) !=
            m_mapTypeCvt.end() )
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
    ObjPtr& pObj, stdstr& strTypeText )
{
    stdstr strSig;
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
    std::vector< stdstr >& vecArgs )
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

            stdstr strArgName = pfa->GetName();
            vecArgs.push_back( strArgName );
        }

    }while( 0 );

    return ret;
}

gint32 CJTypeHelper::GetMethodsOfSvc(
    ObjPtr& pNode,
    std::vector< ObjPtr >& vecm )
{
    gint32 ret = 0;
    do{
        CServiceDecl* pSvc = pNode;
        if( pSvc == nullptr ) 
        {
            ret = -EINVAL;
            break;
        }

        std::vector< ObjPtr > vecIfrs;
        ret = pSvc->GetIfRefs( vecIfrs );
        if( ERROR( ret ) )
            break;

        if( vecIfrs.empty() )
            break;

        for( auto& elem : vecIfrs )
        {
            ObjPtr pObj;
            CInterfRef* pifr = elem;
            ret = pifr->GetIfDecl( pObj );
            if( ERROR( ret ) )
                break;
            CInterfaceDecl* pifd = pObj;
            if( pifd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pObj = pifd->GetMethodList();
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
                vecm.push_back( pmd );
            }
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitFormalArgList(
    ObjPtr& pArgs )
{
    if( pArgs.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    std::vector< STRPAIR > vecArgs;
    do{
        ret = CJTypeHelper::GetFormalArgList(
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
    if( pArgs.IsEmpty() )
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
        Wa( "int[] _offset = new int[]{0};" );

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
        Wa( "ByteBuffer _bbuf = null;" );
        Wa( "if( iSize > RC.MAX_BUF_SIZE || iSize <= 0 )" );
        Wa( "{" );
        Wa( "    ret = -RC.ERANGE;" );
        Wa( "    break;" );
        Wa( "}" );
        Wa( "else" );
        BLOCK_OPEN;
        Wa( "_bbuf = ByteBuffer.allocate( iSize );" );
        CCOUT << "_bbuf.put( " << strBuf << " );";
        NEW_LINE;
        CCOUT << "_bbuf.flip();";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitSerialArgs(
    ObjPtr& pArgs,
    const stdstr& strArrName,
    bool bCast )
{
    if( pArgs.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CArgList* pal = pArgs;
        CArgListUtils oau;
        gint32 iCount = oau.GetArgCount( pArgs );
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
            stdstr strSig = GetTypeSigJava( pType );

            stdstr strName = strArrName;
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
                    pChild, strName, strCast );
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
            default:
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
        gint32 iCount = oau.GetArgCount( pArgs );
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
            stdstr strSig = GetTypeSigJava( pType );

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
                CCOUT << strName << " = _osh.deserialInt64( _bbuf );";
                break;
            case 'D':
            case 'd':
                CCOUT << strName << " = _osh.deserialInt32( _bbuf );";
                break;
            case 'W':
            case 'w':
                CCOUT << strName << " = _osh.deserialInt16( _bbuf );";
                break;
            case 'f':
                CCOUT << strName << " = _osh.deserialFloat( _bbuf );";
                break;
            case 'F':
                CCOUT << strName << " = _osh.deserialDouble( _bbuf );";
                break;
            case 'b':
                CCOUT << strName << " = _osh.deserialBool( _bbuf );";
                break;
            case 'B':
                CCOUT << strName << " = _osh.deserialInt8( _bbuf );";
                break;
            case 'h':
                CCOUT << strName << " = _osh.deserialHStream( _bbuf );";
                break;
            case 's':
                CCOUT << strName << " = _osh.deserialString( _bbuf );";
                break;
            case 'a':
                CCOUT << strName << " = _osh.deserialBuf( _bbuf );";
                break;
            case 'o':
                CCOUT << strName << " = _osh.deserialObjPtr( _bbuf );";
                break;
            case 'O':
                CCOUT << strName << " = ("<<strTypeText<<") _osh.deserialStruct( _bbuf );";
                break;
            case '(':
                CCOUT << strName << " = ("<<strTypeText<<") _osh.deserialArray( _bbuf, \""<< strSig << "\" );";
                break;
            case '[':
                CCOUT << strName << " = ("<<strTypeText<<") _osh.deserialMap( _bbuf, \""<< strSig << "\" );";
                break;
            default:
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
    ObjPtr& pArgs, bool bInit, bool bLocal )
{
    if( pArgs.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CArgList* pal = pArgs;
        CArgListUtils oau;
        gint32 iCount = oau.GetArgCount( pArgs );
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
            stdstr strPublic = "";
            if( !bLocal )
                strPublic = "public ";
            if( !bInit )
            {
                CCOUT << strPublic << strType << " "
                    << strName << ";";
            }
            else
            {
                stdstr strVal =
                    CJTypeHelper::GetDefValOfType( pType );
                CCOUT << strPublic << strType << " "
                    << strName << " = " 
                    << strVal << ";";
            }
            NEW_LINE;
        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitDeclFields(
    ObjPtr& pFields )
{
    if( pFields.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CFieldList* pfl = pFields;
        gint32 iCount = pfl->GetCount();
        if( iCount == 0 )
            break;

        for( gint32 i = 0; i < iCount; i++ )
        {
            ObjPtr pChild = pfl->GetChild( i );
            CFieldDecl* pfd = pChild;
            if( pfd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            stdstr strType;
            ObjPtr pType = pfd->GetType();
            ret = CJTypeHelper::GetTypeText(
                pType, strType );
            if( ERROR( ret ) )
                break;

            stdstr strName = pfd->GetName();
            BufPtr pVal = pfd->GetVal();
            CCOUT << "public " << strType << " "
                << strName;
            if( pVal.IsEmpty() )
            {
                CCOUT << ";";
            }
            else
            {
                CCOUT << " = ";
                stdstr strSig = GetTypeSigJava( pType );
                switch( strSig[ 0 ] )
                {
                case 'Q':
                case 'q':
                    {
                        gint64 qwVal = *pVal;
                        CCOUT << qwVal << ";";
                        break;
                    }
                case 'D':
                case 'd':
                    {
                        gint32 iVal = *pVal;
                        CCOUT << iVal << ";";
                        break;
                    }
                case 'W':
                case 'w':
                    {
                        gint16 sVal = *pVal;
                        CCOUT << sVal << ";";
                        break;
                    }
                case 'b':
                    {
                        bool bVal = *pVal;
                        if( bVal )
                            CCOUT << "true;";
                        else
                            CCOUT << "false;";
                        break;
                    }
                case 'B':
                    {
                        gint8 byVal = *pVal;
                        CCOUT << byVal << ";";
                        break;
                    }
                case 'f':
                    {
                        float fVal = *pVal;
                        CCOUT << fVal << ";";
                        break;
                    }
                case 'F':
                    {
                        double dblVal = *pVal;
                        CCOUT << dblVal << ";";
                        break;
                    }
                case 's':
                    {
                        const char* szVal = pVal->ptr();
                        CCOUT <<  "\"" << szVal << "\";";
                        break;
                    }
                default:
                    {
                        break;
                    }
                }
            }
            NEW_LINE;
        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitSerialFields(
    ObjPtr& pFields )
{
    if( pFields.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CFieldList* pfl = pFields;
        CArgListUtils oau;
        gint32 iCount = pfl->GetCount();
        if( iCount == 0 )
            break;

        for( gint32 i = 0; i < iCount; i++ )
        {
            ObjPtr pChild = pfl->GetChild( i );
            CFieldDecl* pfd = pChild;
            if( pfd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ObjPtr pType = pfd->GetType();
            stdstr strSig = GetTypeSigJava( pType );
            stdstr strName = pfd->GetName();

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
            default :
                ret = -EINVAL;
                break;
            }
            if( ERROR( ret ) )
                break;
            NEW_LINE;
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );
            NEW_LINE;

        }

    }while( 0 );

    return ret;
}

gint32 CJavaSnippet::EmitDeserialFields(
    ObjPtr& pFields )
{
    if( pFields.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CFieldList* pfl = pFields;
        gint32 iCount = pfl->GetCount();
        if( iCount == 0 )
            break;

        for( gint32 i = 0; i < iCount; i++ )
        {
            ObjPtr pChild = pfl->GetChild( i );
            CFieldDecl* pfd = pChild;
            if( pfd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ObjPtr pType = pfd->GetType();
            stdstr strSig = GetTypeSigJava( pType );
            stdstr strTypeText;
            ret = CJTypeHelper::GetTypeText(
                    pType, strTypeText );
            if( ERROR( ret ) )
                break;

            stdstr strName = pfd->GetName();
            switch( strSig[ 0 ] )
            {
            case 'Q':
            case 'q':
                CCOUT << strName << " = _osh.deserialInt64( _bbuf );";
                break;
            case 'D':
            case 'd':
                CCOUT << strName << " = _osh.deserialInt32( _bbuf );";
                break;
            case 'W':
            case 'w':
                CCOUT << strName << " = _osh.deserialInt16( _bbuf );";
                break;
            case 'f':
                CCOUT << strName << " = _osh.deserialFloat( _bbuf );";
                break;
            case 'F':
                CCOUT << strName << " = _osh.deserialDouble( _bbuf );";
                break;
            case 'b':
                CCOUT << strName << " = _osh.deserialBool( _bbuf );";
                break;
            case 'B':
                CCOUT << strName << " = _osh.deserialInt8( _bbuf );";
                break;
            case 'h':
                CCOUT << strName << " = _osh.deserialHStream( _bbuf );";
                break;
            case 's':
                CCOUT << strName << " = _osh.deserialString( _bbuf );";
                break;
            case 'a':
                CCOUT << strName << " = _osh.deserialBuf( _bbuf );";
                break;
            case 'o':
                CCOUT << strName << " = _osh.deserialObjPtr( _bbuf );";
                break;
            case 'O':
                CCOUT << strName << " = ("<<strTypeText<<") _osh.deserialStruct( _bbuf );";
                break;
            case '(':
                CCOUT << strName << " = ("<<strTypeText<<") _osh.deserialArray( _bbuf, \""<< strSig << "\" );";
                break;
            case '[':
                CCOUT << strName << " = ("<<strTypeText<<") _osh.deserialMap( _bbuf, \""<< strSig << "\" );";
                break;
            default :
                ret = -EINVAL;
                break;
            }
            if( ERROR( ret ) )
                break;
            if( i < iCount - 1 )
                NEW_LINES(2);
            else
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
            
        CArgListUtils oau;
        gint32 iCount = oau.GetArgCount( pArgs );
        if( iCount == 0 )
            break;

        CArgList* pal = pArgs;
        for( gint32 i = 0; i < iCount; ++i )
        {
            ObjPtr pArg = pal->GetChild( i );
            CFormalArg* pfa = pArg;

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

void CJavaSnippet::EmitCatchExcepts(
    bool bSetRet )
{
    EmitCatchExcept(
        "IllegalArgumentException", bSetRet );
    EmitCatchExcept(
        "NullPointerException", bSetRet );
    EmitCatchExcept(
        "IndexOutOfBoundsException", bSetRet );
    EmitCatchExcept(
        "Exception", bSetRet );
}

gint32 CJavaSnippet::EmitBanner()
{
    Wa( "// GENERATED BY RIDLC. MAKE SURE TO BACKUP BEFORE RE-COMPILING." );
    CCOUT << "package " << g_strPrefix << g_strAppName << ";";
    NEW_LINE;
    Wa( "import org.rpcf.rpcbase.*;" );
    Wa( "import java.util.Map;" );
    Wa( "import java.util.HashMap;" );
    Wa( "import java.lang.String;" );
    Wa( "import java.nio.ByteBuffer;" );
    NEW_LINE;
    return 0;
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
        else
        {
            CCOUT << " ";
        }
    }
    return 0;
}

gint32 CJavaSnippet::EmitGetArgTypes(
    ObjPtr& pArgs )
{
    gint32 ret = 0;
    CArgListUtils oau;
    gint32 iCount = oau.GetArgCount( pArgs );
    do{
        Wa( "public int getArgCount()" );
        CCOUT << "{ return " << iCount << "; }";
        NEW_LINES( 2 );
        Wa( "public Class<?>[] getArgTypes()" );
        if( iCount == 0 )
        {
            Wa( "{ return new Class[0]; }" );
        }
        else
        {
            BLOCK_OPEN;
            CCOUT << "return new Class<?>[] {";
            INDENT_UPL;
            EmitArgClassObj( pArgs );
            CCOUT << "};";
            INDENT_DOWN;
            BLOCK_CLOSE;
            NEW_LINE;
        }

    }while( 0 );

    return ret;
}

CJavaFileSet::CJavaFileSet(
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


    GEN_FILEPATH( m_strFactory,
        strOutPath, "StructFactory.java",
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
        strOutPath, "maincli.java",
        true );

    GEN_FILEPATH( m_strMainSvr, 
        strOutPath, "mainsvr.java",
        true );

    GEN_FILEPATH( m_strReadme, 
        strOutPath, "README.md",
        false );

    GEN_FILEPATH( m_strDeserialMap, 
        strOutPath, "DeserialMaps.java",
        false );

    GEN_FILEPATH( m_strDeserialArray, 
        strOutPath, "DeserialArrays.java",
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
gint32 CJavaFileSet::AddSvcImpl(
    const std::string& strSvcName )
{
    if( strSvcName.empty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        gint32 idx = m_vecFiles.size();
        std::string strExt = ".java";
        std::string strSvrJava = m_strPath +
            "/" + strSvcName + "svr.java";
        std::string strCliJava = m_strPath +
            "/" + strSvcName + "cli.java";

        std::string strSvrJavaBase = m_strPath +
            "/" + strSvcName + "svrbase.java";

        std::string strCliJavaBase = m_strPath +
            "/" + strSvcName + "clibase.java";

        ret = access( strSvrJava.c_str(), F_OK );
        if( ret == 0 )
        {
            strExt = ".java.new";
            strSvrJava = m_strPath + "/" +
                strSvcName + "svr.java.new";
        }

        ret = access( strCliJava.c_str(), F_OK );
        if( ret == 0 )
        {
            strExt = ".java.new";
            strCliJava = m_strPath + "/" +
                strSvcName + "cli.java.new";
        }

        STMPTR pstm( new std::ofstream(
            strSvrJava,
            std::ofstream::out |
            std::ofstream::trunc) );

        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strSvrJava ] = idx;

        pstm = STMPTR( new std::ofstream(
            strCliJava,
            std::ofstream::out |
            std::ofstream::trunc) );

        idx += 1;
        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strCliJava ] = idx;

        pstm = STMPTR( new std::ofstream(
            strCliJavaBase,
            std::ofstream::out |
            std::ofstream::trunc) );

        idx += 1;
        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strCliJavaBase ] = idx;

        pstm = STMPTR( new std::ofstream(
            strSvrJavaBase,
            std::ofstream::out |
            std::ofstream::trunc) );

        idx += 1;
        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strSvrJavaBase ] = idx;

    }while( 0 );

    return ret;
}

gint32 CJavaFileSet::AddStructImpl(
    const std::string& strStructName )
{
    if( strStructName.empty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        gint32 idx = m_vecFiles.size();
        std::string strStructJava = m_strPath +
            "/" + strStructName + ".java";

        STMPTR pstm( new std::ofstream(
            strStructJava,
            std::ofstream::out |
            std::ofstream::trunc) );

        m_vecFiles.push_back( std::move( pstm ) );
        m_mapSvcImp[ strStructJava ] = idx;

    }while( 0 );

    return ret;
}

void CJavaFileSet::OpenFile(
    const stdstr strName )
{
    STMPTR pstm( new std::ofstream(
        strName,
        std::ofstream::out |
        std::ofstream::trunc ) );

    m_vecFiles.push_back( std::move( pstm ) );
}

gint32 CJavaFileSet::OpenFiles()
{
    this->OpenFile( m_strFactory );
    this->OpenFile( m_strObjDesc );
    this->OpenFile( m_strDriver );
    this->OpenFile( m_strMakefile );
    this->OpenFile( m_strMainCli );
    this->OpenFile( m_strMainSvr );
    this->OpenFile( m_strReadme );
    this->OpenFile( m_strDeserialMap );
    this->OpenFile( m_strDeserialArray );

    return STATUS_SUCCESS;
}

CJavaFileSet::~CJavaFileSet()
{
    for( auto& elem : m_vecFiles )
    {
        if( elem != nullptr )
            elem->close();
    }
    m_vecFiles.clear();
}

gint32 FindFullPath(
    const stdstr& strFile,
    stdstr& strFullPath )
{
    stdstr strSrcFile = "./";
    strSrcFile += basename( strFile.c_str() );
    gint32 ret = access(
        strSrcFile.c_str(), F_OK );
    if( ret == -1 )
    {
        stdstr strPath;
        ret = FindInstCfg( strSrcFile, strPath );
        if( ERROR( ret ) )
            return ret;
        strSrcFile = strPath;
    }
    strFullPath = strSrcFile;
    return ret;
}

#include <sys/types.h>
#include <sys/wait.h>

gint32 Execve(
    const char* cmd,
    char* const args[],
    char* const env[] )
{
    int ret = 0;
    pid_t pid = fork();
    int fd = -1;
    if (pid == -1)
        return ERROR_FAIL;

    else if (pid != 0)
    {
        int status = 0;
        while( ( ret = waitpid( pid, &status, 0 ) ) == -1 )
        {
            if (errno != EINTR) {
                /* Handle error */
                ret = -errno;
                break;
            }
        }
        if ((ret == 0) ||
            !(WIFEXITED(status) && !WEXITSTATUS(status)))
        {
            /* Report unexpected child status */
            ret = -ECHILD;
        }
    }
    else
    {
        /* ... Initialize env as a sanitized copy of
         * environ ... */
        if( execve(cmd, args, env) == -1 )
        {
            /* Handle error */
            printf( "error running /usr/bin/cpp %d", errno );
            ret = -errno;
        }
    }
    return ret;
}

gint32 GenSerialHelper(
    const stdstr& strInput, 
    const stdstr& strOutPath,
    bool bProxy )
{
    gint32 ret = 0;
    stdstr strCmd;
    const char* args[10];
    args[ 0 ] = "/usr/bin/cpp";
    stdstr strArg5, strArg8;
    strArg5 = "-DXXXXX=";
    strArg5 += g_strPrefix + g_strAppName;

    strArg8 = strOutPath;
    char* env[ 1 ] = { nullptr };
    if( bProxy )
        strArg8 += "/JavaSerialHelperP.java";
    else
        strArg8 += "/JavaSerialHelperS.java";

    if( bProxy )
    {
         args[1] = "-P";
         args[2] = "-DJavaSerialImpl=JavaSerialHelperP";
         args[3] = "-DGetIdHash=GetPeerIdHash";
         args[4] = "-DInstType=CJavaProxyImpl" ;
         args[5] = strArg5.c_str();
         args[6] = strInput.c_str();
         args[7] = "-o";
         args[8] = strArg8.c_str();
    }
    else
    {
         args[1] = "-P";
         args[2] = "-DJavaSerialImpl=JavaSerialHelperS";
         args[3] = "-DGetIdHash=GetIdHashByChan";
         args[4] = "-DInstType=CJavaServerImpl" ;
         args[5] = strArg5.c_str();
         args[6] = strInput.c_str();
         args[7] = "-o";
         args[8] = strArg8.c_str();
    }

    const char* const args2[ 10 ] = {
        args[0], args[1], args[2], args[3],
        args[4], args[5], args[6], 
        args[7], args[8], nullptr
        };

    return Execve( "/usr/bin/cpp",
        const_cast< char* const*>( args2 ),
        env );    
}

gint32 GenSerialBase(
    const stdstr& strInput, 
    const stdstr& strOutPath )
{
    gint32 ret = 0;
    stdstr strCmd;
    const char* args[7];
    args[ 0 ] = "/usr/bin/cpp";
    stdstr strArg5, strArg8;
    strArg5 = "-DXXXXX=";
    strArg5 += g_strPrefix + g_strAppName;

    strArg8 = strOutPath;
    char* env[ 1 ] = { nullptr };

     args[1] = "-P";
     args[2] = strArg5.c_str();
     args[3] = strInput.c_str();
     args[4] = "-o";
     args[5] = strArg8.c_str();

    const char* const args2[7] = {
        args[0], args[1], args[2], args[3],
        args[4], args[5], nullptr 
        };

    return Execve( "/usr/bin/cpp",
        const_cast< char* const*>( args2 ),
        env );    
}

gint32 GenSerialBaseFiles(
    CJavaWriter* pWriter )
{
    if( pWriter == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        stdstr strSeriBase;
        ret = FindFullPath(
            "JavaSerialBase.java", strSeriBase );
        if( ERROR( ret ) )
            break;

        stdstr strDstSeriBase =
            pWriter->GetOutPath();
        strDstSeriBase += "/JavaSerialBase.java";

        ret = GenSerialBase(
            strSeriBase,
            strDstSeriBase );
        if( ERROR( ret ) )
            break;

        stdstr strSeriImpl;
        ret = FindFullPath(
            "JavaSerialImpl.java", strSeriImpl );
        if( ERROR( ret ) )
            break;

        ret = access(
            strSeriImpl.c_str(), F_OK | R_OK );
        if( ret == -1 )
        {
            ret = -errno;
            break;
        }

        ret = access(
            strSeriImpl.c_str(), X_OK );
        if( ret == 0 )
        {
            ret = -EBADF;
            break;
        }

        ret = GenSerialHelper(
            strSeriImpl,
            pWriter->GetOutPath(),
            true );

        if( ERROR( ret ) )
            break;

        ret = GenSerialHelper(
            strSeriImpl,
            pWriter->GetOutPath(),
            false );

    }while( 0 );
    
    return ret;
}

gint32 GenStructFilesJava(
    CJavaWriter* pWriter, ObjPtr& pRoot )
{
    if( pWriter == nullptr ||
        pRoot.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        GenSerialBaseFiles( pWriter );
        CStatements* pStmts = pRoot;
        if( unlikely( pStmts == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }
        std::vector< ObjPtr > vecAllStructs;
        pStmts->GetStructDecls( vecAllStructs );

        std::vector< std::pair< std::string, ObjPtr > > vecStructs;
        for( auto& elem : vecAllStructs )
        {
            CStructDecl* psd = elem;
            if( psd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            if( psd->RefCount() == 0 )
                continue;

            vecStructs.push_back(
                { psd->GetName(), elem } );
            stdstr strName = psd->GetName();
            pWriter->AddStructImpl( strName );
        }
        
        for( auto& elem : vecStructs )
        {
            stdstr strFile =
                pWriter->GetOutPath() +
                "/" + elem.first + ".java"; 

            ret = pWriter->SelectImplFile(
                strFile );
            if( ERROR( ret ) )
                break;

            CDeclareStructJava odsj(
                pWriter, elem.second );
            ret = odsj.Output();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

gint32 GenSvcFiles(
    CJavaWriter* pWriter, ObjPtr& pRoot )
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
                strCommon + "clibase.java" ); 

            CImplJavaSvcclibase ojspb(
                pWriter, elem.second );
            ret = ojspb.Output();
            if( ERROR( ret ) )
                break;

            // server base imlementation
            pWriter->SelectImplFile(
                strCommon + "svrbase.java" );

            CImplJavaSvcsvrbase ojssb(
                pWriter, elem.second );
            ret = ojssb.Output();
            if( ERROR( ret ) )
                break;

            // server imlementation
            ret = pWriter->SelectImplFile(
                strCommon + "svr.java" );
            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    strCommon + "svr.java.new" );
                if( ERROR( ret ) )
                    break;
            }

            CImplJavaSvcSvr opss(
                pWriter, elem.second );
            ret = opss.Output();
            if( ERROR( ret ) )
                break;

            // client imlementation
            ret = pWriter->SelectImplFile(
                strCommon + "cli.java" );
            if( ERROR( ret ) )
            {
                ret = pWriter->SelectImplFile(
                    strCommon + "cli.java.new" );
                if( ERROR( ret ) )
                    break;
            }

            CImplJavaSvcCli opsc(
                pWriter, elem.second );
            ret = opsc.Output();
            if( ERROR( ret ) )
                break;
        }

    }while( 0 );

    return ret;
}

gint32 GenJavaProj(
    const std::string& szOutPath,
    const std::string& strAppName,
    ObjPtr pRoot )
{
    if( szOutPath.empty() ||
        strAppName.empty() ||
        pRoot.IsEmpty() )
        return -EINVAL;

    gint32 ret = 0;
    stdstr strOutPath;

    do{
        struct stat sb;
        stdstr strPkgPath = g_strPrefix;
        std::replace( strPkgPath.begin(),
            strPkgPath.end(), '.', '/' );
        strPkgPath += "/";
        strPkgPath += g_strAppName;

        if( szOutPath == "output" )
            strOutPath = strPkgPath;
        else
            strOutPath =
                szOutPath + "/" + strPkgPath;

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

        oWriter.SelectFactoryFile();
        CImplStructFactory oisf( &oWriter, pRoot );
        ret = oisf.Output();
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "error generating Factory file" );
            break;
        }

        oWriter.SelectDrvFile();
        CExportDrivers oedrv( &oWriter, pRoot );
        ret = oedrv.Output();
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "error generating driver.json file" );
            break;
        }

        oWriter.SelectDescFile();
        CExportObjDesc oedesc( &oWriter, pRoot );
        ret = oedesc.Output();
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "error generating description file" );
            break;
        }

        ret = GenSvcFiles( &oWriter, pRoot );
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "error generating java file" );
            break;
        }

        oWriter.SelectMakefile();
        CJavaExportMakefile opemk( &oWriter, pRoot );
        ret = opemk.Output();
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "error generating Makefile" );
            break;
        }

        oWriter.SelectReadme();
        CJavaExportReadme ordme( &oWriter, pRoot );
        ret = ordme.Output();
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "error generating README.md" );
            break;
        }

        oWriter.SelectDeserialMap();
        CImplDeserialMap oidm( &oWriter );
        ret = oidm.Output();
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "error generating DeserialMaps.java" );
            break;
        }

        oWriter.SelectDeserialArray();
        CImplDeserialArray oida( &oWriter );
        ret = oida.Output();
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "error generating DeserialArrays.java" );
            break;
        }

        oWriter.SelectMainCli();
        CImplJavaMainCli ojmc( &oWriter, pRoot );
        ret = ojmc.Output();
        if( ERROR( ret ) )
        {
            stdstr strName = "maincli.java";
            OutputMsg( ret,
                "error generating %s.",
                strName.c_str() );
            break;
        }

        oWriter.SelectMainSvr();
        CImplJavaMainSvr ojms( &oWriter, pRoot );
        ret = ojms.Output();
        if( ERROR( ret ) )
        {
            stdstr strName = "mainsvr.java";
            OutputMsg( ret,
                "error generating %s.",
                strName.c_str() );
            break;
        }

        ret = GenStructFilesJava( &oWriter, pRoot );
        if( ERROR( ret ) )
        {
            OutputMsg( ret,
                "error generating Struct files." );
            break;
        }
    }while( 0 );

    if( SUCCEEDED( ret ) )
        SyncCfg( strOutPath );


    return ret;
}

CImplJavaMethodSvrBase::CImplJavaMethodSvrBase(
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

gint32 CImplJavaMethodSvrBase::DeclAbstractFuncs()
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
    CJavaSnippet os( m_pWriter );
    for( gint32 i = 0; i < 2; i++ )
    {
        if( i == 0 )
        {
            CCOUT << "public abstract int "
                << strName << "(";
        }
        else
        {
            if( !pNode->IsAsyncs() )
                break;
            CCOUT << "public abstract void on"
                << strName << "Canceled(";
        }
        INDENT_UPL;
        CCOUT << "JavaReqContext oReqCtx";
        if( iCount == 0 )
        {
            if( i == 0 )
                CCOUT << " );";
            else
                CCOUT << ", int iRet );";
        }
        else
        {
            if( i == 0 )
                Wa( "," );
            else
                Wa( ", int iRet," );
            ret = os.EmitFormalArgList( pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << " );";
        }
        INDENT_DOWNL;
        NEW_LINE;
    }
    return ret;
}

gint32 CImplJavaMethodSvrBase::ImplNewCancelNotify()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    CServiceDecl* pSvc = m_pSvc;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    stdstr strSvcName = pSvc->GetName() + "svrbase";
    if( !pNode->IsAsyncs() )
        return 0;

    CJavaSnippet os( m_pWriter );
    do{
        Wa( "private ICancelNotify newCancelNotify(" );
        CCOUT << "    " << strSvcName << " oHost,";
        NEW_LINE;
        CCOUT << "    JavaReqContext oContext )";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "return new ICancelNotify()" );
        BLOCK_OPEN;
        CCOUT << "final " << strSvcName << " m_oHost = oHost;";
        NEW_LINE;
        Wa( "final JavaReqContext m_oReqCtx = oContext;" );
        os.EmitGetArgTypes( pInArgs );

        Wa( "public void onAsyncResp( Object oContext," );
        Wa( "    int iRet, Object[] oParams )" );
        BLOCK_OPEN;
        if( iInCount == 0 )
        {
            CCOUT << "m_oHost.on"<< strName
                << "Canceled( m_oReqCtx, iRet );";
        }
        else
        {
            CCOUT << "if( oParams.length != " << iInCount << " )";
            NEW_LINE;
            Wa( "    return;" );
            CCOUT << "m_oHost.on" << strName <<"Canceled(";
            NEW_LINE;
            CCOUT << "    m_oReqCtx, iRet,";
            INDENT_UPL;
            ObjPtr pMethod = pNode;
            os.EmitCastArgsFromObject(
                pMethod, true, "oParams" );
            CCOUT << ");";
            INDENT_DOWN;
        }
        BLOCK_CLOSE;

        BLOCK_CLOSE; // return new ICancelNotify();
        CCOUT << ";";
        BLOCK_CLOSE; // newCancelNotify
        NEW_LINE;

    }while( 0 );
    return ret;
}

gint32 CImplJavaMethodSvrBase::OutputReqHandler()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    CInterfaceDecl* pIf = m_pIf;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    gint32 iOutCount = GetArgCount( pInArgs );
    stdstr strIfName = pIf->GetName();

    CJavaSnippet os( m_pWriter );
    do{
        CCOUT << "// Invoke Wrapper for " << strIfName << "::" << strName;
        NEW_LINE;
        Wa( "val = new IReqHandler() {" );
        INDENT_UPL;
        os.EmitGetArgTypes( pInArgs );

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
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplJavaMethodSvrBase::ImplReqContext()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    CServiceDecl* pSvc = m_pSvc;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    ObjPtr pOutArgs = pNode->GetOutArgs();
    gint32 iOutCount = GetArgCount( pOutArgs );
    CJavaSnippet os( m_pWriter );
    do{
        Wa( "JavaReqContext _oReqCtx =" );
        Wa( "    new JavaReqContext( oOuterObj, callback )" );
        BLOCK_OPEN;
        Wa( "public int iRet;" );
        os.EmitDeclArgs( pOutArgs, false );
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
                    CCOUT << elem.second << " = ( "
                        << elem.first << " )"
                        << "args[ " << i << " ];";
                    if( i < vecArgs.size() - 1 )
                        NEW_LINE;
                }
                BLOCK_CLOSE;
                NEW_LINE;
            }
            CCOUT << "m_bSet = true;";
            BLOCK_CLOSE;

            if( iOutCount > 0 )
            {
                NEW_LINE;
                Wa( "public Object[] getResponse()" );
                BLOCK_OPEN;
                {
                    Wa( "if( !m_bSet )" );
                    Wa( "    return new Object[ 0 ];" );
                    CCOUT << "return new Object[]{";
                    INDENT_UPL;
                    os.EmitActArgList( pOutArgs );
                    CCOUT << "};";
                    INDENT_DOWN;
                }
                BLOCK_CLOSE;
            }
        }

        if( pNode->IsAsyncs() )
            NEW_LINE;

        ret = ImplSvcComplete();
        if( ERROR( ret ) )
            break;

        BLOCK_CLOSE;
        Wa( ";" );

    }while( 0 );

    return ret;
}

gint32 CImplJavaMethodSvrBase::ImplSvcComplete()
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

    CJavaSnippet os( m_pWriter );
    do{
        Wa( "// call this method when the" );
        Wa( "// request has completed" );
        Wa( "// asynchronously" );
        Wa( "public int onServiceComplete(" );
        Wa( "    int iRet, Object ...args )" );
        BLOCK_OPEN;
        Wa( "int ret = 0;" );
        if( iOutCount > 0 )
            Wa( "BufPtr _pBuf = null;" );
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
            CCOUT << "    break;";
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "JavaSerialHelperS _osh =" );
            Wa( "    new JavaSerialHelperS( oHost.getInst() );" );

            Wa( "_pBuf = new BufPtr( true );" );
            Wa( "ret = _pBuf.Resize( 1024 );" );
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "int[] _offset = new int[]{0};" );

            ret = os.EmitSerialArgs(
                pOutArgs, "args", true );
            if( ERROR( ret ) )
                break;

            Wa( "ret = _pBuf.Resize( _offset[0] );");
            Wa( "if( RC.ERROR( ret ) ) break;" );
            BLOCK_CLOSE;
            CCOUT << "while( false );";
            NEW_LINE;
            Wa( "if( RC.ERROR( ret ) &&" );
            Wa( "    RC.SUCCEEDED( iRet ) )" );
            Wa( "    iRet = ret;" );
            Wa( "ObjPtr oCallback_ = getCallback();" );
            Wa( "if( oCallback_ != null )" );
            Wa( "{" );
            Wa( "    ret = oHost.ridlOnServiceComplete(" );
            Wa( "        oCallback_, iRet, _pBuf );" );
            Wa( "    oCallback_.Clear();" );
            Wa( "}" );
            Wa( "if( _pBuf != null )" );
            Wa( "    _pBuf.Clear();" );
        }
        else
        {
            Wa( "ObjPtr oCallback_ = getCallback();" );
            Wa( "if( oCallback_ != null )" );
            Wa( "{" );
            Wa( "    ret = oHost.ridlOnServiceComplete(" );
            Wa( "        oCallback_, iRet, null );" );
            Wa( "    oCallback_.Clear();" );
            Wa( "}" );
        }
        CCOUT << "return ret;";
        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

gint32 CImplJavaMethodSvrBase::ImplInvoke()
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
    CJavaSnippet os( m_pWriter );
    do{
        Wa( "public JRetVal invoke( Object oOuterObj," );
        Wa( "    ObjPtr callback, Object[] oParams )" );
        BLOCK_OPEN;
        ret = ImplReqContext();
        if( ERROR( ret ) )
            break;
        NEW_LINE;
        Wa( "int ret = 0;" );
        Wa( "JRetVal jret = new JRetVal();" );
        CCOUT << "do";
        BLOCK_OPEN;
        CCOUT << strSvc << "svrbase oHost =";
        NEW_LINE;
        CCOUT << "    ( " << strSvc << "svrbase )oOuterObj;";
        NEW_LINE;
        if( iInCount == 0 )
        {
            CCOUT << "ret = oHost." << strName << "( _oReqCtx );";
            NEW_LINE;
        }
        else
        {
            Wa( "JavaSerialHelperS _osh =" );
            Wa( "    new JavaSerialHelperS( oHost.getInst() );" );
            Wa( "if( oParams.length != 1 )" );
            Wa( "{ ret = -RC.EINVAL; break; }" );
            Wa( "byte[] _buf = ( byte[] )oParams[ 0 ];" );
            ret = os.EmitByteBufferForDeserial( "_buf" );
            if( ERROR( ret ) )
                break;
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );
            ret = os.EmitDeserialArgs(
                pInArgs, true );
            if( ERROR( ret ) )
               break;
            CCOUT << "ret = oHost." << strName << "( _oReqCtx,";
            INDENT_UPL;
            ret = os.EmitActArgList( pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << ");";
            INDENT_DOWNL;
        }

        if( !pNode->IsAsyncs() )
        {
            Wa( "if( ret == RC.STATUS_PENDING )" );
            Wa( "    ret = RC.ERROR_STATE;" );
        }
        else
        {
            // we allow no-reply request to be able to
            // return pending too
            Wa( "if( ret == RC.STATUS_PENDING )" );
            BLOCK_OPEN;
            Wa( "oHost.getInst().SetInvTimeout(" );
            guint32 dwTimeoutSec = pNode->GetTimeoutSec();
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
                CCOUT << "    _oReqCtx.getCallback(), " <<
                    dwTimeoutSec << ", " << dwKeepAliveSec << " );";
            }
            else
            {
                CCOUT << "    _oReqCtx.getCallback(), 0 );";
            }
            NEW_LINE;
            Wa( "ICancelNotify o =" );
            Wa( "    newCancelNotify( oHost, _oReqCtx );" );
            Wa( "int iRet = oHost.installCancelNotify(" );
            Wa( "    _oReqCtx.getCallback(), o," );
            if( iInCount > 0 )
            {
                CCOUT << "    new Object[]"; 
                INDENT_UPL;
                BLOCK_OPEN;
                ret = os.EmitActArgList( pInArgs );
                BLOCK_CLOSE;
                CCOUT << ");";
                INDENT_DOWNL;
            }
            else
            {
                CCOUT << "    new Object[0] );";
                NEW_LINE;
            }
            Wa( "if( RC.ERROR( iRet ) )" );
            Wa( "    ret = iRet;" );
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
        }

        Wa( "if( RC.ERROR( ret ) )" );
        if( iOutCount > 0 )
        {
            Wa( "    break;" );
            Wa( "if( !_oReqCtx.hasResponse() )" );
            Wa( "{ ret = -RC.ENODATA; break; }" );
            CCOUT << "do";
            BLOCK_OPEN;
            Wa( "BufPtr _pBuf = new BufPtr( true );" );
            Wa( "ret = _pBuf.Resize( 1024 );" );
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "int[] _offset = new int[]{0};" );
            Wa( "Object[] oResp = _oReqCtx.getResponse();" );
            ret = os.EmitSerialArgs(
                pOutArgs, "oResp", true );
            if( ERROR( ret ) )
                break;
            Wa( "ret = _pBuf.Resize( _offset[0] );");
            Wa( "if( RC.ERROR( ret ) ) break;" );
            CCOUT << "jret.addElem( _pBuf );";
            BLOCK_CLOSE;
            Wa( "while( false );" );
        }
        else
        {
            CCOUT << "    break;";
        }
        BLOCK_CLOSE;
        Wa( "while( false );" );
        Wa( "jret.setError( ret );" );
        CCOUT << "return jret;";
        BLOCK_CLOSE;
    }while( 0 );

    return ret;
}

gint32 CImplJavaMethodSvrBase::OutputEvent()
{
    gint32 ret = 0;
    stdstr strName = m_pNode->GetName();
    ObjPtr pInArgs = m_pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    stdstr strIfName = m_pIf->GetName();

    CJavaSnippet os( m_pWriter );
    do{
        CCOUT << "// " << strIfName << "::" << strName << " event sender";
        NEW_LINE;
        if( iInCount == 0 )
        {
            CCOUT << "public int " << strName << "( Object callback )";
            NEW_LINE;
            BLOCK_OPEN;
            Wa( "return ridlSendEvent( callback," );
            CCOUT << "    \"" << strIfName << "\",";
            NEW_LINE;
            CCOUT << "    \"" << strName << "\",";
            NEW_LINE;
            CCOUT << "    \"\", null );";
            NEW_LINE;
            BLOCK_CLOSE;
        }
        else
        {
            CCOUT << "public int " << strName << "( Object callback,";
            INDENT_UPL;
            ret = os.EmitFormalArgList( pInArgs );
            if( ERROR( ret ) )
                break;
            Wa( ")" );
            INDENT_DOWNL;
            BLOCK_OPEN;
            Wa( "int ret = 0;" );
            CCOUT << "do";
            BLOCK_OPEN;

            Wa( "JavaSerialHelperS _osh =" );
            Wa( "    new JavaSerialHelperS( getInst() );" );
            Wa( "BufPtr _pBuf = new BufPtr( true );" );
            Wa( "ret = _pBuf.Resize( 1024 );" );
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "int[] _offset = new int[]{0};" );
            ret = os.EmitSerialArgs(
                pInArgs, "", false );

            Wa( "_pBuf.Resize( _offset[0] );" );
            Wa( "ret = ridlSendEvent( callback," );
            CCOUT << "    \"" << strIfName << "\",";
            NEW_LINE;
            CCOUT << "    \"" << strName << "\",";
            NEW_LINE;
            CCOUT << "    \"\", _pBuf );";
            NEW_LINE;
            CCOUT << "_pBuf.Clear();";
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
    CJavaSnippet os( m_pWriter );
    os.EmitBanner();
    gint32 ret = 0;
    do{
        CServiceDecl* pSvc = m_pNode;

        stdstr strName = pSvc->GetName();
        CCOUT << "abstract public class " << strName
            << "svrbase extends JavaRpcServer";
        NEW_LINE;
        BLOCK_OPEN;

        CCOUT << "public " << strName << "svrbase( ObjPtr pIoMgr,";
        NEW_LINE;
        Wa( "    String strDesc, String strSvrObj )" );
        Wa( "{ super( pIoMgr, strDesc, strSvrObj ); }" );
        NEW_LINE;

        std::vector< ObjPtr > vecAllMethods;
        ObjPtr pNode = m_pNode;
        ret = CJTypeHelper::GetMethodsOfSvc(
            pNode, vecAllMethods );
        if( ERROR( ret ) )
            break;
        std::vector< ObjPtr > vecMethods;
        std::vector< ObjPtr > vecEvents;
        for( auto& elem : vecAllMethods )
        {
            CMethodDecl* pmd = elem;
            if( pmd == nullptr )
                continue;

            if( pmd->IsEvent() )
            {
                vecEvents.push_back( elem );
                continue;
            }
            vecMethods.push_back( elem );
        }

        for( auto& elem : vecMethods )
        {
            CImplJavaMethodSvrBase ojms(
                m_pWriter, elem, pNode );

            ojms.DeclAbstractFuncs();
        }
        
        Wa( "public Map< String, IReqHandler > initMaps()" );
        BLOCK_OPEN;

        Wa( "Map< String, IReqHandler > o =" );
        Wa( "    new HashMap< String, IReqHandler >();" );
        Wa( "IReqHandler val;" );

        for( auto& elem : vecMethods )
        {
            CImplJavaMethodSvrBase ojms(
                m_pWriter, elem, pNode );

            ojms.OutputReqHandler();
            NEW_LINE;
        }

        CCOUT << "return o;";
        BLOCK_CLOSE;
        NEW_LINE;
        for( auto& elem : vecEvents )
        {
            CImplJavaMethodSvrBase ojms(
                m_pWriter, elem, pNode );
            ojms.OutputEvent();
            NEW_LINE;
        }

        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

CImplJavaMethodSvr::CImplJavaMethodSvr(
    CJavaWriter* pWriter,
    ObjPtr& pNode )
    : super( pWriter )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    CAstNodeBase* pMdl = m_pNode->GetParent();
    m_pIf = ObjPtr( pMdl->GetParent() );
    if( m_pNode == nullptr || m_pIf == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'service' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplJavaMethodSvr::Output()
{
    gint32 ret = 0;
    stdstr strName = m_pNode->GetName();
    stdstr strIfName = m_pIf->GetName();
    ObjPtr pInArgs = m_pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    CJavaSnippet os( m_pWriter );

    do{
        if( m_pNode->IsEvent() )
        {
            ret = ERROR_NOT_HANDLED;
            break;
        }

        bool bAsync = m_pNode->IsAsyncs();
        CCOUT << "// " << strIfName << "::" << strName;
        stdstr strHint;
        if( m_pNode->IsAsyncs() )
            strHint = "async-handler";
        else
            strHint = "sync-handler";
        if( m_pNode->IsNoReply() )
            if( strHint.empty() )
                strHint = "no-reply";
            else
                strHint += " no-reply";
        CCOUT << " " << strHint;
        NEW_LINE;
        CCOUT << "public int " << strName << "(";
        NEW_LINE;
        CCOUT << "    JavaReqContext oReqCtx";
        if( iInCount == 0 )
        {
            CCOUT << " )";
        }
        else
        {
            CCOUT <<",";
            INDENT_UPL;
            ret = os.EmitFormalArgList(
                pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << ")";
            INDENT_DOWN;
        }
        NEW_LINE;
        BLOCK_OPEN;
        if( !bAsync )
        {
            if( !m_pNode->IsNoReply() )
            {
                Wa( "// Synchronous handler. Make sure to call" );
                Wa( "// oReqCtx.setResponse before return" );
            }
            else
            {
                Wa( "// Synchronous handler with no-reply" );
                Wa( "// return value and response is ignored." );
            }
        }
        else
        {
            if( !m_pNode->IsNoReply() )
            {
                Wa( "// Asynchronous handler. Make sure to call" );
                Wa( "// oReqCtx.setResponse with response" );
                Wa( "// parameters if return immediately or call" );
            }
            else
            {
                Wa( "// Asynchronous handler with no-reply. " );
                Wa( "// Make sure to call" );
            }
            Wa( "// oReqCtx.onServiceComplete in the future" );
            Wa( "// if returning RC.STATUS_PENDING." );
        }
        CCOUT << "return RC.ERROR_NOT_IMPL;";
        BLOCK_CLOSE;

        if( !m_pNode->IsAsyncs() )
        {
            NEW_LINE;
            break;
        }

        NEW_LINE;
        CCOUT << "public void on" << strName << "Canceled(";
        NEW_LINE;
        CCOUT << "    JavaReqContext oReqCtx, int iRet";
        if( iInCount == 0 )
        {
            CCOUT << " )";
        }
        else
        {
            CCOUT <<",";
            INDENT_UPL;
            ret = os.EmitFormalArgList(
                pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << ")";
            INDENT_DOWN;
        }
        NEW_LINE;
        BLOCK_OPEN;
        CCOUT << "// " << strIfName << "::" << strName << " is canceled.";;
        NEW_LINE;
        Wa( "// Optionally make change here to do the cleanup" );
        Wa( "// work if the request was timed-out or canceled" );
        CCOUT << "return;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

CImplJavaMethodCliBase::CImplJavaMethodCliBase(
    CJavaWriter* pWriter,
    ObjPtr& pNode,
    ObjPtr& pSvc )
    : super( pWriter )
{
    gint32 ret = 0;
    do{
        m_pNode = pNode;
        m_pSvc = pSvc;
        if( m_pNode == nullptr ||
            m_pSvc == nullptr )
        {
            ret = -EINVAL;
            break;
        }
        CAstNodeBase* pMdl = m_pNode->GetParent();
        if( pMdl == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        m_pIf = ObjPtr( pMdl->GetParent() );

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error found "
            "empty node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplJavaMethodCliBase::DeclAbstractFuncs()
{
    gint32 ret = 0;
    CMethodDecl* pNode = m_pNode;
    stdstr strName = pNode->GetName();
    ObjPtr pInArgs = pNode->GetInArgs();
    gint32 iCount = GetArgCount( pInArgs );
    ObjPtr pOutArgs = pNode->GetOutArgs();
    gint32 iOutCount = GetArgCount( pOutArgs );
    CInterfaceDecl* pIf = m_pIf;
    stdstr strIfName = pIf->GetName();
    CJavaSnippet os( m_pWriter );

    if( !pNode->IsAsyncp() &&
        !pNode->IsEvent() )
        return 0;

    do{
        if( pNode->IsEvent() )
        {
            CCOUT << "// Event Handler "
                << strIfName << "::" << strName;
            NEW_LINE;
            CCOUT << "public abstract void "<< strName << "(";
            INDENT_UPL;
            CCOUT << "JavaReqContext oContext";
        }
        else if( pNode->IsAsyncp() )
        {
            CCOUT << "// Asynchronous callback for "
                << strIfName << "::" << strName;
            NEW_LINE;
            CCOUT << "public abstract void on"
                << strName << "Complete(";
            INDENT_UPL;
            CCOUT << "Object oContext, int iRet";
            iCount = iOutCount;
            pInArgs = pOutArgs;
        }
        if( iCount == 0 )
        {
            CCOUT << " );";
        }
        else
        {
            Wa( "," );
            ret = os.EmitFormalArgList(
                pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << ");";
        }
        INDENT_DOWNL;
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplJavaMethodCliBase::OutputReqSender()
{
    gint32 ret = 0;
    stdstr strName = m_pNode->GetName();
    ObjPtr pInArgs = m_pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    CInterfaceDecl* pIf = m_pIf;
    stdstr strIfName = pIf->GetName();
    CJavaSnippet os( m_pWriter );

    CCOUT << "// " << strIfName << "::" << strName;

    stdstr strHint;
    if( m_pNode->IsAsyncp() )
        strHint = "async-req";
    else
        strHint = "sync-req";
    if( m_pNode->IsNoReply() )
        if( strHint.empty() )
            strHint = "no-reply";
        else
            strHint += " no-reply";
    CCOUT << " " << strHint;
    NEW_LINE;
    do{
        CCOUT << "public JRetVal " << strName << "(";
        if( iInCount == 0 )
        {
            if( m_pNode->IsAsyncp() )
            {
                NEW_LINE;
                CCOUT << "    Object oContext )";
            }
            else
            {
                CCOUT << ")";
            }
            NEW_LINE;
        }
        else
        {
            if( m_pNode->IsAsyncp() )
            {
                NEW_LINE;
                CCOUT << "    Object oContext,";
            }
            INDENT_UPL;
            ret = os.EmitFormalArgList(
                pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << ")";
            INDENT_DOWNL;
        }
        BLOCK_OPEN;
        Wa( "int ret = 0;" );
        Wa( "JRetVal jret = new JRetVal();" );
        Wa( "try{" );
        CCOUT << "do";
        BLOCK_OPEN;
        if( iInCount > 0 )
        {
            Wa( "JavaSerialHelperP _osh =" );
            Wa( "    new JavaSerialHelperP( getInst() );" );
            Wa( "BufPtr _pBuf = new BufPtr( true );" );
            Wa( "ret = _pBuf.Resize( 1024 );" );
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );
            Wa( "int[] _offset = new int[]{0};" );

            ret = os.EmitSerialArgs(
                pInArgs, "", false );
            if( ERROR( ret ) )
                break;

            Wa( "ret = _pBuf.Resize( _offset[0] );" );
            Wa( "if( RC.ERROR( ret ) )" );
            Wa( "    break;" );
        }
        Wa( "CParamList oParams = new CParamList();" );
        Wa( "oParams.SetStrProp(" );
        CCOUT << "    RC.propIfName, \"" << strIfName << "\" );";
        NEW_LINE;

        Wa( "oParams.SetStrProp(" );
        CCOUT << "    RC.propMethodName, \"" << strName << "\" );";
        NEW_LINE;

        Wa( "oParams.SetIntProp(" );
        CCOUT << "    RC.propSeriProto, " << "RC.seriRidl );";
        NEW_LINE;

        if( m_pNode->IsNoReply() )
        {
            Wa( "oParams.SetBoolProp(" );
            CCOUT << "    RC.propNoReply, " << "true );";
            NEW_LINE;
        }

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
            Wa( "oParams.SetIntProp(" );
            CCOUT << "    RC.propTimeoutSec, " <<
                dwTimeoutSec << " );";
            NEW_LINE;

            Wa( "oParams.SetIntProp(" );
            CCOUT << "    RC.propKeepAliveSec, " <<
                dwKeepAliveSec << " );";
            NEW_LINE;
        }

        Wa( "jret = ( JRetVal )oParams.GetCfg();" );
        Wa( "if( jret.ERROR() )" );
        Wa( "{ ret = jret.getError();break;}" );
        Wa( "ObjPtr pObj_1 = ( ObjPtr )jret.getAt(0);" );
        Wa( "CfgPtr pCfg = rpcbase.CastToCfg( pObj_1 );" );
        ret = ImplAsyncCb();
        if( ERROR( ret ) )
            break;

        ret = ImplMakeCall();
        if( ERROR( ret ) )
            break;

        BLOCK_CLOSE;
        Wa( "while( false );" );
        Wa( "}" );
        os.EmitCatchExcepts( true );

        Wa( "jret.setError( ret );" );
        CCOUT<< "return jret;";
        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

gint32 CImplJavaMethodCliBase::ImplAsyncCb()
{
    gint32 ret = 0;
    stdstr strName = m_pNode->GetName();
    ObjPtr pOutArgs = m_pNode->GetOutArgs();
    gint32 iOutCount = GetArgCount( pOutArgs );
    CServiceDecl* pSvc = m_pSvc;
    stdstr strSvc = pSvc->GetName();
    CJavaSnippet os( m_pWriter );
    if( !m_pNode->IsAsyncp() )
        return 0;
    do{
       
        Wa( "IAsyncRespCb callback = new IAsyncRespCb()" );
        BLOCK_OPEN;
        os.EmitGetArgTypes( pOutArgs );

        Wa( "public void onAsyncResp( Object oContext," );
        Wa( "    int iRet, Object[] oParams )" );
        BLOCK_OPEN; 
        CCOUT << strSvc << "clibase oInst =";
        NEW_LINE;
        CCOUT << "    " << strSvc << "clibase.this;";
        NEW_LINE;
        CCOUT << "do";
        BLOCK_OPEN;
        if( iOutCount == 0 )
        {
            CCOUT << "oInst.on" << strName
                << "Complete( oContext, iRet );";
            NEW_LINE;
        }
        else
        {
            ret = os.EmitDeclArgs(
                pOutArgs, true, true );

            if( ERROR( ret ) )
                break;
            Wa( "if( RC.ERROR( iRet ) )" );
            BLOCK_OPEN;
            CCOUT << "oInst.on" << strName
                << "Complete( oContext, iRet,";
            INDENT_UPL;
            ret = os.EmitActArgList(
                pOutArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << ");";
            INDENT_DOWNL;
            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;
            Wa( "int ret = iRet;" );
            Wa( "byte[] _buf = ( byte[] )oParams[ 0 ];" );
            Wa( "if( _buf == null )" );
            Wa( "    ret = -RC.ENODATA;" );
            Wa( "else" );
            Wa( "{" );
            INDENT_UPL;
            ret = os.EmitByteBufferForDeserial( "_buf" );
            if( ERROR( ret ) )
                break;
            Wa( "if( RC.ERROR( ret ) )" );
            BLOCK_OPEN;

                CCOUT << "oInst.on" << strName
                    << "Complete( oContext, ret,";
                INDENT_UPL;
                ret = os.EmitActArgList(
                    pOutArgs );
                if( ERROR( ret ) )
                    break;
                CCOUT << ");";
                INDENT_DOWNL;

            CCOUT << "break;";
            BLOCK_CLOSE;
            NEW_LINE;

            ret = os.EmitDeserialArgs(
                pOutArgs, false );
            if( ERROR( ret ) )
                break;
            INDENT_DOWNL;
            Wa( "}" );
            CCOUT << "oInst.on" << strName
                << "Complete( oContext, ret,";
            INDENT_UPL;
            ret = os.EmitActArgList(
                pOutArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << ");";
            INDENT_DOWN;
        }
        BLOCK_CLOSE;
        Wa( "while( false );" );
        CCOUT << "return;";
        BLOCK_CLOSE;
        BLOCK_CLOSE;
        CCOUT << ";";
        NEW_LINE;

    }while( 0 );

    return ret;
}

gint32 CImplJavaMethodCliBase::ImplMakeCall()
{
    gint32 ret = 0;
    stdstr strName = m_pNode->GetName();
    ObjPtr pInArgs = m_pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    ObjPtr pOutArgs = m_pNode->GetOutArgs();
    gint32 iOutCount = GetArgCount( pOutArgs );
    CServiceDecl* pSvc = m_pSvc;
    stdstr strSvc = pSvc->GetName();
    CJavaSnippet os( m_pWriter );
    do{
        if( iInCount > 0 )
            Wa( "Object[] args = new Object[]{ _pBuf };" );
        else
            Wa( "Object[] args = new Object[ 0 ];" );
        if( m_pNode->IsAsyncp() )
        {
            Wa( "jret = makeCallWithOptAsync(" );
            Wa( "    callback, oContext, pCfg, args );" );
            if( iInCount > 0 )
                Wa( "_pBuf.Clear();" );
            Wa( "if( jret.isPending() )" );
            BLOCK_OPEN;
            Wa( "ret = RC.STATUS_PENDING;" );
            CCOUT << "break;";
            BLOCK_CLOSE;
        }
        else
        {
            Wa( "jret = makeCallWithOpt( pCfg, args );" );
            if( iInCount > 0 )
                Wa( "_pBuf.Clear();" );
            Wa( "if( jret.isPending() )" );
            Wa( "{ jret.setError( RC.ERROR_STATE ); break; }" );
        }
        if( !m_pNode->IsNoReply() )
        {
            Wa( "if( jret.ERROR() )" );
            Wa( "{ ret = jret.getError(); break; }" );
            if( iOutCount > 0 )
            {
                Wa( "byte[] _buf = ( byte[] )jret.getAt( 0 );" );
                ret = os.EmitByteBufferForDeserial(
                    "_buf" );
                if( ERROR( ret ) )
                    break;
                ret = os.EmitDeserialArgs(
                    pOutArgs, true );
                if( ERROR( ret ) )
                    break;
                std::vector< stdstr > vecArgs;
                ret = CJTypeHelper::GetActArgList(
                    pOutArgs, vecArgs );
                if( ERROR( ret ) )
                    break;
                Wa( "if( RC.ERROR( ret ) )" );
                Wa( "    break;" );
                Wa( "jret.clear();" );
                for( auto& elem : vecArgs )
                {
                    CCOUT << "jret.addElem( " << elem << " );";
                    NEW_LINE;
                }
            }
        }

    }while( 0 );

    return ret;
}

gint32 CImplJavaMethodCliBase::OutputEvent()
{
    gint32 ret = 0;
    stdstr strName = m_pNode->GetName();
    ObjPtr pInArgs = m_pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    stdstr strIfName = m_pIf->GetName();
    stdstr strSvc = m_pSvc->GetName();
    CJavaSnippet os( m_pWriter );
    if( !m_pNode->IsEvent() )
        return 0;
    do{
        Wa( "val = new IEvtHandler()" );
        BLOCK_OPEN;
        os.EmitGetArgTypes( pInArgs );
        Wa( "public JRetVal invoke(" );
        Wa( "    Object oOuterObj, ObjPtr callback, Object[] oParams )" );
        BLOCK_OPEN;
        Wa( "JavaReqContext _oReqCtx = new JavaReqContext(" );
        Wa( "    oOuterObj, callback ) {};" );

        Wa( "int ret = 0;" );
        Wa( "JRetVal jret = new JRetVal();" );
        CCOUT << "do";
        BLOCK_OPEN;
        CCOUT << strSvc << "clibase oHost =";
        NEW_LINE;
        CCOUT << "    ( " << strSvc << "clibase )oOuterObj;";
        NEW_LINE;
        if( iInCount == 0 )
        {
            CCOUT << "oHost." << strName << "( _oReqCtx );";
            NEW_LINE;
        }
        else
        {
            Wa( "JavaSerialHelperP _osh =" );
            Wa( "    new JavaSerialHelperP( oHost.getInst() );" );
            Wa( "byte[] _buf = ( byte[] )oParams[ 0 ];" );
            ret = os.EmitByteBufferForDeserial(
                "_buf" );
            if( ERROR( ret ) )
                break;
            Wa( "if( RC.ERROR( ret ) ) break;" );
            ret = os.EmitDeserialArgs(
                pInArgs, true );
            if( ERROR( ret ) )
                break;
            NEW_LINE; 
            CCOUT << "oHost." << strName << "( _oReqCtx,";
            INDENT_UPL
            ret = os.EmitActArgList( pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << ");";
            INDENT_DOWNL;
        }
        BLOCK_CLOSE;
        Wa( "while( false );" );
        Wa( "jret.setError( ret );" );
        Wa( "return jret;" );
        BLOCK_CLOSE; // invoke

        BLOCK_CLOSE; // IEvtHandler
        Wa( ";" );
        CCOUT << "o.put( \"" << strIfName
            << "::" << strName << "\", val );";
        NEW_LINE;

    }while( 0 );

    return ret;
}

CImplJavaMethodCli::CImplJavaMethodCli(
        CJavaWriter* pWriter,
        ObjPtr& pNode )
    : super( pWriter )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    CAstNodeBase* pMdl = m_pNode->GetParent();
    m_pIf = ObjPtr( pMdl->GetParent() );
    if( m_pNode == nullptr || m_pIf == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'service' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplJavaMethodCli::Output()
{
    gint32 ret = 0;
    stdstr strName = m_pNode->GetName();
    stdstr strIfName = m_pIf->GetName();
    ObjPtr pInArgs = m_pNode->GetInArgs();
    gint32 iInCount = GetArgCount( pInArgs );
    ObjPtr pOutArgs = m_pNode->GetOutArgs();
    gint32 iOutCount = GetArgCount( pOutArgs );
    CJavaSnippet os( m_pWriter );

    do{
        if( !m_pNode->IsEvent() )
        {
            bool bAsync = m_pNode->IsAsyncp();
            if( !bAsync )
            {
                ret = ERROR_NOT_HANDLED;
                break;
            }

            CCOUT << "// " << strIfName << "::" << strName;
            CCOUT << " async callback";
            NEW_LINE;
            CCOUT << "public void on" << strName << "Complete(";
            NEW_LINE;
            CCOUT << "    Object oContext, int iRet";
            if( iOutCount == 0 || m_pNode->IsNoReply() )
            {
                CCOUT << " )";
            }
            else
            {
                CCOUT <<",";
                INDENT_UPL;
                ret = os.EmitFormalArgList(
                    pOutArgs );
                if( ERROR( ret ) )
                    break;
                CCOUT << " )";
                INDENT_DOWN;
            }
            NEW_LINE;
            BLOCK_OPEN;
            CCOUT << "// " << strIfName << "::" << strName
                << " has completed";
            NEW_LINE;

            Wa( "// oContext is what has passed when the" );
            Wa( "// request was sent. iRet is the status" );
            Wa( "// code of the request. if RC.ERROR(iRet)," );
            Wa( "// the request is failed, and the following" );
            Wa( "// response parameters should be ignored." );

            CCOUT << "return;";
            BLOCK_CLOSE;
            NEW_LINE;
            break;
        }

        CCOUT << "// " << strIfName << "::" << strName;
        CCOUT << " the event handler";
        NEW_LINE;
        CCOUT << "public void " << strName << "(";
        NEW_LINE;
        CCOUT << "    JavaReqContext oReqCtx";
        if( iInCount == 0 )
        {
            CCOUT << " )";
        }
        else
        {
            CCOUT <<",";
            INDENT_UPL;
            ret = os.EmitFormalArgList(
                pInArgs );
            if( ERROR( ret ) )
                break;
            CCOUT << " )";
            INDENT_DOWN;
        }
        NEW_LINE;
        BLOCK_OPEN;
        CCOUT << "// event " << strIfName << "::" << strName << " comes";
        NEW_LINE;
        Wa( "// make change here to handle the event" );
        CCOUT << "return;";
        BLOCK_CLOSE;
        NEW_LINE;

    }while( 0 );

    return ret;
}

CImplJavaSvcclibase::CImplJavaSvcclibase(
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

int CImplJavaSvcclibase::Output()
{
    CJavaSnippet os( m_pWriter );
    os.EmitBanner();
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        CCOUT << "abstract public class " << strName
            << "clibase extends JavaRpcProxy";
        NEW_LINE;
        BLOCK_OPEN;

        CCOUT << "public " << strName << "clibase( ObjPtr pIoMgr,";
        NEW_LINE;
        Wa( "    String strDesc, String strSvrObj )" );
        Wa( "{ super( pIoMgr, strDesc, strSvrObj ); }" );
        NEW_LINE;

        std::vector< ObjPtr > vecAllMethods;
        ObjPtr pNode = m_pNode;
        ret = CJTypeHelper::GetMethodsOfSvc(
            pNode, vecAllMethods );
        if( ERROR( ret ) )
            break;
        std::vector< ObjPtr > vecMethods;
        std::vector< ObjPtr > vecEvents;
        for( auto& elem : vecAllMethods )
        {
            CMethodDecl* pmd = elem;
            if( pmd == nullptr )
                continue;

            if( pmd->IsEvent() )
            {
                vecEvents.push_back( elem );
                continue;
            }
            vecMethods.push_back( elem );
        }

        for( auto& elem : vecAllMethods )
        {
            CImplJavaMethodCliBase ojmp(
                m_pWriter, elem, pNode );

            ojmp.DeclAbstractFuncs();
        }

        for( auto& elem : vecMethods )
        {
            CImplJavaMethodCliBase ojmp(
                m_pWriter, elem, pNode );

            ojmp.OutputReqSender();
            NEW_LINE;
        }
        
        Wa( "public Map< String, IReqHandler > initMaps()" );
        BLOCK_OPEN;

        Wa( "Map< String, IReqHandler > o =" );
        Wa( "    new HashMap< String, IReqHandler >();" );
        Wa( "IReqHandler val;" );

        for( auto& elem : vecEvents )
        {
            CImplJavaMethodCliBase ojmp(
                m_pWriter, elem, pNode );
            ojmp.OutputEvent();
            NEW_LINE;
        }

        CCOUT << "return o;";
        BLOCK_CLOSE; // initMaps

        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

CImplJavaSvcSvr::CImplJavaSvcSvr(
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

int CImplJavaSvcSvr::Output()
{
    CJavaSnippet os( m_pWriter );
    os.EmitBanner();
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        CCOUT << "public class " << strName
            << "svr extends " << strName << "svrbase";
        NEW_LINE;
        BLOCK_OPEN;

        CCOUT << "public " << strName << "svr( ObjPtr pIoMgr,";
        NEW_LINE;
        Wa( "    String strDesc, String strSvrObj )" );
        Wa( "{ super( pIoMgr, strDesc, strSvrObj ); }" );
        NEW_LINE;

        ObjPtr pNode = m_pNode;
        std::vector< ObjPtr > vecAllMethods;
        ret = CJTypeHelper::GetMethodsOfSvc(
            pNode, vecAllMethods );
        if( ERROR( ret ) )
            break;

        for( auto& elem : vecAllMethods )
        {
            CImplJavaMethodSvr ojms(
                m_pWriter, elem );
            ret = ojms.Output();
            if( SUCCEEDED( ret ) )
                NEW_LINE;
            else
                ret = 0;
        }

        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

CImplJavaSvcCli::CImplJavaSvcCli(
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

int CImplJavaSvcCli::Output()
{
    CJavaSnippet os( m_pWriter );
    os.EmitBanner();
    gint32 ret = 0;
    do{
        stdstr strName = m_pNode->GetName();
        CCOUT << "public class " << strName
            << "cli extends " << strName << "clibase";
        NEW_LINE;
        BLOCK_OPEN;

        CCOUT << "public " << strName << "cli( ObjPtr pIoMgr,";
        NEW_LINE;
        Wa( "    String strDesc, String strSvrObj )" );
        Wa( "{ super( pIoMgr, strDesc, strSvrObj ); }" );
        NEW_LINE;

        ObjPtr pNode = m_pNode;
        std::vector< ObjPtr > vecAllMethods;
        ret = CJTypeHelper::GetMethodsOfSvc(
            pNode, vecAllMethods );
        if( ERROR( ret ) )
            break;

        for( auto& elem : vecAllMethods )
        {
            CImplJavaMethodCli ojmc(
                m_pWriter, elem );
            ret = ojmc.Output();
            if( SUCCEEDED( ret ) )
                NEW_LINE;
            else
                ret = 0;
        }

        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

CDeclareStructJava::CDeclareStructJava(
    CJavaWriter* pWriter,
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

gint32 CDeclareStructJava::Output()
{
    CStructDecl* pStruct = m_pNode;
    stdstr strName = pStruct->GetName();
    stdstr strMsgName = g_strAppName + "::" + strName;
    gint32 iMsgId = GenClsid( strMsgName );
    CJavaSnippet os( m_pWriter );
    os.EmitBanner();
    Wa( "import java.nio.ByteBuffer;" );
    gint32 ret = 0;
    do{
        CCOUT << "public class " << strName << " extends JavaSerialBase.ISerializable";
        NEW_LINE;
        BLOCK_OPEN;
        char szHexId[ 32 ];
        snprintf( szHexId,
            sizeof( szHexId ) - 1, "0x%08X", iMsgId );
        Wa( "public static int getStructId()" );
        CCOUT << "{ return "<< szHexId <<"; };";
        NEW_LINES( 2 );

        Wa( "public static String getStructName()" );
        CCOUT << "{ return \""<< strMsgName <<"\"; };";
        NEW_LINES( 2 );

        CCOUT << "public " << strName << "(){}";
        NEW_LINES( 2 );

        Wa( "Object m_oInst;" );
        Wa( "public void setInst( Object oInst )" );
        Wa( "{ m_oInst = oInst; }" );
        Wa( "public Object getInst()" );
        Wa( "{ return m_oInst; }" );
        NEW_LINE;
        ObjPtr pFields = pStruct->GetFieldList();
        ret = os.EmitDeclFields(
            pFields );        
        if( ERROR( ret ) )
            break;

        NEW_LINE;
        Wa( "public int serialize(" );
        Wa( "    BufPtr _pBuf, int[] _offset )" );
        BLOCK_OPEN;
        Wa( "int ret = 0;" );
        Wa( "JavaSerialBase _osh = getSerialBase();" );
        Wa( "try{" );
        CCOUT << "do";
        BLOCK_OPEN;
        CCOUT << "ret = _osh.serialInt32( _pBuf,"
            << " _offset, " << "getStructId() );";
        NEW_LINE;
        Wa( "if( RC.ERROR( ret ) )" );
        Wa( "    break;" );
        NEW_LINE;
        ret = os.EmitSerialFields( pFields );
        if( ERROR( ret ) )
            break;
        Wa( "ret = _pBuf.Resize( _offset[0] );");
        Wa( "if( RC.ERROR( ret ) ) break;" );
        BLOCK_CLOSE;
        Wa( "while( false );" );
        Wa( "}" );
        os.EmitCatchExcepts( true );
        Wa( "return ret;" );
        BLOCK_CLOSE;

        NEW_LINE;
        Wa( "public int deserialize( ByteBuffer _bbuf )" );
        BLOCK_OPEN;
        Wa( "int ret = 0;" );
        Wa( "JavaSerialBase _osh = getSerialBase();" );
        Wa( "try{" );
        CCOUT << "do";
        BLOCK_OPEN;
        Wa( "int _structId = _osh.deserialInt32( _bbuf );" );
        Wa( "if( _structId != getStructId() )" );
        Wa( "    break;" );
        ret = os.EmitDeserialFields( pFields );
        if( ERROR( ret ) )
            break;
        BLOCK_CLOSE;
        Wa( "while( false );" );
        Wa( "}" );
        os.EmitCatchExcepts( true );
        Wa( "return ret;" );
        BLOCK_CLOSE;
        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

CImplStructFactory::CImplStructFactory(
     CJavaWriter* pWriter,
     ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'statement' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplStructFactory::Output()
{
    CJavaSnippet os( m_pWriter );
    gint32 ret = 0;
    do{
        os.EmitBanner();
        Wa( "import java.nio.ByteBuffer;" );
        std::vector< ObjPtr > vecStructs;
        m_pNode->GetStructDecls( vecStructs );

        std::vector< CStructDecl* > vecActStructs;
        for( auto& elem : vecStructs )
        {
            CStructDecl* psd = elem;
            if( psd == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            if( psd->RefCount() == 0 )
                continue;
            vecActStructs.push_back( psd ); 
        }
        if( ERROR( ret ) )
            break;
        
        Wa( "public class StructFactory" );
        BLOCK_OPEN;

        Wa( "public interface IFactory " );
        Wa( "{ public abstract JavaSerialBase.ISerializable create(); }" );
        Wa( "protected static Map< Integer, IFactory > m_mapStructs = initMaps();" );

        Wa( "public static JavaSerialBase.ISerializable create( int id )" );
        BLOCK_OPEN; 
        Wa( "if( !m_mapStructs.containsKey( id ) )" );
        Wa( "    return null;" );

        Wa( "IFactory oFactory = m_mapStructs.get( id );" );
        Wa( "if( oFactory == null )" );
        Wa( "    return null;" );
        CCOUT << "return oFactory.create();";
        BLOCK_CLOSE;
        NEW_LINE;

        Wa( "public static Map< Integer, IFactory > initMaps()" );
        BLOCK_OPEN;
        Wa( "Map< Integer, IFactory > mapFactories =" );
        Wa( "    new HashMap< Integer, IFactory >();" );
        if( !vecActStructs.empty() )
        {
            Wa( "IFactory o;" );
            for( auto& elem : vecActStructs )
            {
                stdstr strName = elem->GetName();
                Wa( "o = new IFactory() {" );
                Wa( "    public JavaSerialBase.ISerializable create()" );
                CCOUT << "    " << "{ return new " << strName << "();}";
                NEW_LINE;
                Wa( "};" );
                stdstr strMsgName = g_strAppName + "::" + strName;
                gint32 iMsgId = GenClsid( strMsgName );
                char szHexId[ 16 ] = {0};
                snprintf( szHexId,
                    sizeof( szHexId ) - 1, "0x%08X", iMsgId );
                CCOUT << "mapFactories.put( " << szHexId << ", o );";
                NEW_LINE;
            }
        }
        CCOUT << "return mapFactories;";
        BLOCK_CLOSE;    
        BLOCK_CLOSE;

    }while( 0 );

    return ret;
}

gint32 CJavaExportReadme::Output()
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

        CCOUT << "* **maincli.java**, **mainsvr.java**: "
            << "Containing defintion of `main()` method for client, as the main "
            << "entry for client program "
            << "and definition of `main()` function server program respectively. ";
        NEW_LINE;
        CCOUT << "And you can make changes to the files to customize the program. "
            << "The `ridlc` will not touch them if they exist in the project directory. "
            << "When it runs again, it puts the newly "
            << "generated code to `mainxxx.java.new` files instead.";
        NEW_LINES( 2 );

        for( auto& elem : vecSvcNames )
        {
            CCOUT << "* **" << elem << "svr.java**, **" << elem << "cli.java**: "
                << "Containing the declarations and definitions of all the server/client side "
                << "methods that need to be implemented by you, mainly the request/event handlers, "
                << "for service `" << elem << "`.";
            NEW_LINE;
            CCOUT << "And you need to make changes to the files to implement the "
                << "functionality for server/client. "
                << "The `ridlc` will not touch them if they exist in the project directory. "
                << "When it runs again, it puts the newly "
                << "generated code to `"<<elem  <<"xxx.java.new` files instead.";
            NEW_LINES( 2 );
        }

        for( auto& elem : vecSvcNames )
        {
            CCOUT << "* *" << elem << "svrbase.java*, *"<< elem << "clibase.java* : "
                << "Containing the declarations and definitions of all the server/client side "
                << "utilities and helpers for the interfaces of service `" << elem << "`.";
            NEW_LINE;
            CCOUT << "And please don't edit them, since they will be "
                << "overwritten by next run of `ridlc` without backup.";
            NEW_LINES( 2 );
        }

        CCOUT<< "* *" << g_strAppName << "Factory.java*: "
            << "Containing the definition of struct factory "
            << "declared and referenced in the ridl file.";
        NEW_LINE;
        CCOUT << "And please don't edit it, since they will be "
            << "overwritten by next run of `ridlc` without backup.";
        NEW_LINES( 2 );

        CCOUT<< "* *" << g_strAppName << "desc.json*: "
            << "Containing the configuration parameters for all "
            << "the services declared in the ridl file";
        NEW_LINE;
        CCOUT << "And please don't edit it, since they will be "
            << "overwritten by next run of `ridlc` or synccfg.py without backup.";
        NEW_LINES( 2 );

        CCOUT << "* *driver.json*: "
            << "Containing the configuration parameters for all "
            << "the ports and drivers";
        NEW_LINE;
        CCOUT << "And please don't edit it, since they will be "
            << "overwritten by next run of `ridlc` or synccfg.py "
            << "without backup.";
        NEW_LINES( 2 );

        CCOUT << "* *Makefile*: "
            << "The Makefile will just synchronize the configurations "
            << "with the local system settings. And it does nothing else.";
        NEW_LINE;
        CCOUT << "And please don't edit it, since it will be "
            << "overwritten by next run of `ridlc` and synccfg.py without backup.";
        NEW_LINES( 2 );

        CCOUT << "* *DeserialMaps*, *DeserialArrays*, *JavaSerialBase.java*, "
            << "*JavaSerialHelperS.java*, *JavaSerialHelperP.java*: "
            << "Containing the utility classes for serializations.";
        NEW_LINE;
        CCOUT << "And please don't edit it, since they will be "
            << "overwritten by next run of `ridlc`.";
        NEW_LINES( 2 );

        CCOUT << "* *synccfg.py*: "
            << "a small python script to synchronous settings "
            << "with the system settings, just ignore it.";
        NEW_LINES(2);
        CCOUT <<"* **run:** you can run `java org.rpcf."<<g_strAppName<<".mainsvr`"
            << " and `java org.rpcf."<<g_strAppName<<".maincli` to start the server "
            << "and client. Also make sure to run `make` to update the configuration"
            << " files, that is, the `"<< g_strAppName <<"desc.json` and `driver.json`.";
        NEW_LINES(2);

        CCOUT << "**Note**: the files in bold text need your further implementation. "
            << "And files in italic text do not. And of course, "
            << "you can still customized the italic files, but be aware they "
            << "will be rewritten after running RIDLC again.";
        NEW_LINE;

   }while( 0 );

   return ret;
}
    
CJavaExportMakefile::CJavaExportMakefile(
    CWriterBase* pWriter,
    ObjPtr& pNode ):
    super( pWriter, pNode )
{
    m_strFile = "./pymktpl";
}

gint32 CJavaExportMakefile::Output()
{
    gint32 ret = super::Output();
    if( ERROR( ret ) )
        return ret;

    m_pWriter->m_curFp->flush();
    m_pWriter->m_curFp->close();

    stdstr strPath = m_pWriter->GetOutPath();
    stdstr strFile = strPath + "/Makefile";
    stdstr strCmdLine = "sed -i \"s:XXXXX:";
    strCmdLine += strPath + ":g\" " +  strFile;

    printf( "%s\n", strCmdLine.c_str() );
    system( strCmdLine.c_str() );
    return ret;
}

CImplDeserialMap::CImplDeserialMap(
    CJavaWriter* pWriter )
{
    m_pWriter = pWriter;
}

gint32 CImplDeserialMap::Output()
{
    CJavaSnippet os( m_pWriter );
    os.EmitBanner();
    gint32 ret = 0;

    do{
        Wa( "public class DeserialMaps" );
        BLOCK_OPEN;
        Wa( "public static Map<?,?> deserialMap(" );
        Wa( "    JavaSerialBase osb, ByteBuffer bbuf, String sig )" );
        BLOCK_OPEN;
        Wa( "IDeserialMap o = null;" );
        Wa( "if( m_oDeserialMaps.containsKey( sig ) )" );
        Wa( "    o = m_oDeserialMaps.get( sig );" );
        Wa( "if( o == null )" );
        Wa( "    return null;" );
        CCOUT << "return o.deserialMap( osb, bbuf, sig );";
        BLOCK_CLOSE;
        NEW_LINES( 2 );
        Wa( "static public interface IDeserialMap" );
        BLOCK_OPEN;    
        Wa( "public abstract Map<?,?> deserialMap(" );
        CCOUT << "    JavaSerialBase osb, ByteBuffer bbuf, String sig );";
        BLOCK_CLOSE; 
        NEW_LINES( 2 );
        Wa( "static Map< String, IDeserialMap >" );
        Wa( "    m_oDeserialMaps = initMaps();" );
        NEW_LINE;
        Wa( "static Map< String, IDeserialMap > initMaps()" );
        BLOCK_OPEN; 
        Wa( "Map< String, IDeserialMap > o =" );
        Wa( "    new HashMap< String, IDeserialMap >();" );
        Wa( "IDeserialMap val;" );
        for( auto& elem : g_mapMapTypeDecl )
        {
            CMapType* pmt = elem.second;
            stdstr strSig = elem.first;
            stdstr strTypeText;
            ret = CJTypeHelper::GetTypeText(
                elem.second, strTypeText );
            if( ERROR( ret ) )
                break;
            stdstr strKeyType1, strValType1;
            ObjPtr pKeyType = pmt->GetKeyType();
            ObjPtr pValType = pmt->GetElemType();

            ret = CJTypeHelper::GetTypeText(
                pKeyType, strKeyType1 );
            if( ERROR( ret ) )
                break;

            stdstr strKeyType, strValType;
            CJTypeHelper::GetObjectPrimitive(
                strKeyType1, strKeyType );
            ret = CJTypeHelper::GetTypeText(
                pValType, strValType1 );
            if( ERROR( ret ) )
                break;
            CJTypeHelper::GetObjectPrimitive(
                strValType1, strValType );

            Wa( "val = new IDeserialMap() " );
            BLOCK_OPEN;

            Wa( "public Map<?, ?>  deserialMap(" );
            Wa( "    JavaSerialBase osb, ByteBuffer bbuf, String sig )" );
            BLOCK_OPEN;
            CCOUT << "if( !sig.equals(  \""<< strSig << "\" ) )";
            NEW_LINE;
            Wa( "    throw new IllegalArgumentException(" );
            CCOUT << "        \"deserialMap[" << strTypeText << "]\" + ";
            NEW_LINE;
            CCOUT << "        " << "\" error signature\" );";
            NEW_LINE;
            Wa( "return osb.deserialMapInternal(" );
            CCOUT << "    bbuf, sig, "
                << strKeyType << ".class, "
                << strValType << ".class );";

            BLOCK_CLOSE;
            BLOCK_CLOSE;
            CCOUT << ";";
            NEW_LINE;
            CCOUT << "o.put( \"" << strSig << "\", val );";
            NEW_LINES( 2 );
        }
        CCOUT << "return o;";
        BLOCK_CLOSE; // initMaps
        BLOCK_CLOSE;// DeserialMaps

    }while( 0 );
    return ret;
}

CImplJavaMainCli::CImplJavaMainCli(
    CJavaWriter* pWriter,
    ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'statement' node" );
        throw std::runtime_error( strMsg );
    }
}
gint32 CImplJavaMainCli::Output()
{
    CJavaSnippet os( m_pWriter );
    os.EmitBanner();
    Wa( "import java.util.concurrent.TimeUnit;" );
    gint32 ret = 0;
    do{
        CCOUT << "public class maincli";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "public static JavaRpcContext m_oCtx;" );
        Wa( "public static void main( String[] args )" );
        BLOCK_OPEN;
        Wa( "m_oCtx = JavaRpcContext.createProxy(); " );
        Wa( "if( m_oCtx == null )" );
        Wa( "    System.exit( -RC.EFAULT );" );
        NEW_LINE;

        std::vector< ObjPtr > vecSvcs;
        ret = m_pNode->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) || vecSvcs.empty() )
            break;
        CServiceDecl* pSvc = vecSvcs[ 0 ];
        stdstr strSvc = pSvc->GetName();

        Wa( "// create the service object" );
        CCOUT << strSvc << "cli oSvcCli = new " << strSvc << "cli(";
        NEW_LINE;
            Wa( "    m_oCtx.getIoMgr(), " );
            CCOUT << "    \"./"<< g_strAppName<<"desc.json\",";
            NEW_LINE;
            CCOUT << "    \""<< strSvc << "\" );";

        NEW_LINES( 2 );
        Wa( "// check if there are errors" );
        Wa( "if( RC.ERROR( oSvcCli.getError() ) )" );
        Wa( "    System.exit( oSvcCli.getError() );" );

        NEW_LINE;
        Wa( "// start the proxy" );
        Wa( "int ret = oSvcCli.start();" );
        Wa( "if( RC.ERROR( ret ) )" );
        Wa( "    System.exit( ret );" );
        
        NEW_LINE;
        CCOUT << "do{";
        INDENT_UPL;
        Wa( "// test remote server is not online" );
        Wa( "while( oSvcCli.getState() == RC.stateRecovery )" );
        Wa( "try{" );
        Wa( "    TimeUnit.SECONDS.sleep(1);" );
        Wa( "}" );
        Wa( "catch( InterruptedException e ){};" );
        NEW_LINE;
        Wa( "if( oSvcCli.getState() != RC.stateConnected )" );
        Wa( "{ ret = RC.ERROR_STATE;break;}" );
        NEW_LINE;
        std::vector< ObjPtr > vecm;
        ret = CJTypeHelper::GetMethodsOfSvc(
            vecSvcs[ 0 ], vecm );
        if( ERROR( ret ) || vecm.empty() )
            break;

        ObjPtr pMethod;
        CArgListUtils oau;
        bool bHasEvent = false;
        for( auto& elem : vecm )
        {
            pMethod = elem;
            CMethodDecl* pmd = elem;
            if( pmd->IsEvent() )
            {
                bHasEvent = true;
                pMethod.Clear();
                continue;
            }
            ObjPtr pInArgs = pmd->GetInArgs();
            if( oau.GetArgCount( pInArgs ) > 1 )
                break;
        }

        CMethodDecl* pmd = pMethod;
        if( pmd != nullptr )
        {
            Wa( "/*// request something from the server" );
            CCOUT << "JRetVal jret = oSvcCli."
                << pmd->GetName();
            ObjPtr pInArgs = pmd->GetInArgs();
            gint32 iInCount =
                oau.GetArgCount( pInArgs );

            if( iInCount == 0 && pmd->IsAsyncp() )
            {
                CCOUT << "( oYourCtx );";
                NEW_LINE;
            }
            else if( iInCount == 0 )
            {
                CCOUT << "();";
                NEW_LINE;
            }
            else if( iInCount > 0 )
            {
                if( pmd->IsAsyncp() )
                    CCOUT << "( oYourCtx,";
                else
                    CCOUT << "(";
                INDENT_UPL;
                ret = os.EmitActArgList( pInArgs );
                if( ERROR( ret ) )
                    break;
                CCOUT << ");";
                INDENT_DOWNL;
            }
            Wa( "if( jret.ERROR() )" );
            Wa( "{ ret = jret.getError();break; }" );
            CCOUT << "*/";
        }
        else if( bHasEvent )
        {
            Wa( "/*" );
            Wa( " *Just waiting and events will " );
            Wa( " *be handled in the background" );
            NEW_LINE;
            Wa( " while( oSvcCli.getState() == RC.stateConnected )" );
            Wa( " try{" );
            Wa( "     TimeUnit.SECONDS.sleep(1);" );
            Wa( " } catch (InterruptedException e) {" );
            Wa( " }" );
            CCOUT << " */";
        }
        INDENT_DOWNL;
        CCOUT << "}while( false );";
        NEW_LINES(2);
        Wa( "oSvcCli.stop();" );
        Wa( "m_oCtx.stop();" );
        CCOUT << "System.exit( ret );";
        BLOCK_CLOSE; 
        BLOCK_CLOSE; 

    }while( 0 );

    return ret;
}

CImplJavaMainSvr::CImplJavaMainSvr(
    CJavaWriter* pWriter,
    ObjPtr& pNode )
{
    m_pWriter = pWriter;
    m_pNode = pNode;
    if( m_pNode == nullptr )
    {
        std::string strMsg = DebugMsg(
            -EFAULT, "internal error empty "
            "'statement' node" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CImplJavaMainSvr::Output()
{
    CJavaSnippet os( m_pWriter );
    os.EmitBanner();
    Wa( "import java.util.concurrent.TimeUnit;" );
    gint32 ret = 0;
    do{
        CCOUT << "public class mainsvr";
        NEW_LINE;
        BLOCK_OPEN;
        Wa( "public static JavaRpcContext m_oCtx;" );
        Wa( "public static void main( String[] args )" );
        BLOCK_OPEN;
        Wa( "m_oCtx = JavaRpcContext.createServer(); " );
        Wa( "if( m_oCtx == null )" );
        Wa( "    System.exit( -RC.EFAULT );" );
        NEW_LINE;

        std::vector< ObjPtr > vecSvcs;
        ret = m_pNode->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) || vecSvcs.empty() )
            break;
        CServiceDecl* pSvc = vecSvcs[ 0 ];
        stdstr strSvc = pSvc->GetName();

        Wa( "// create the service object" );
        CCOUT << strSvc << "svr oSvcSvr = new " << strSvc << "svr(";
        NEW_LINE;
            Wa( "    m_oCtx.getIoMgr(), " );
            CCOUT << "    \"./"<< g_strAppName<<"desc.json\",";
            NEW_LINE;
            CCOUT << "    \""<< strSvc << "\" );";
        NEW_LINES( 2 );
        Wa( "// check if there are errors" );
        Wa( "if( RC.ERROR( oSvcSvr.getError() ) )" );
        Wa( "    System.exit( oSvcSvr.getError() );" );
        NEW_LINE;
        Wa( "int ret = oSvcSvr.start();" );
        Wa( "if( RC.ERROR( ret ) )" );
        Wa( "    System.exit( ret );" );
        
        NEW_LINE;
        Wa( "do{" );
        CCOUT << "try{";
        INDENT_UPL;
        Wa( "// replace 'sleep()' with your favorite" );
        Wa( "// waiting logic if necessary. The requests" );
        Wa( "// handling is going on in the background" );
        Wa( "TimeUnit.SECONDS.sleep(1);" );
        Wa( "if( oSvcSvr.getState() != RC.stateConnected )" );
        CCOUT << "    break;";
        INDENT_DOWNL;
        Wa( "}" );
        Wa( "catch( InterruptedException e ){}" );
        CCOUT << "}while( true );";
        NEW_LINES(2);
        Wa( "oSvcSvr.stop();" );
        Wa( "m_oCtx.stop();" );
        CCOUT << "System.exit( ret );";
        BLOCK_CLOSE; 
        BLOCK_CLOSE; 

    }while( 0 );

    return ret;
}

CImplDeserialArray::CImplDeserialArray(
    CJavaWriter* pWriter )
{
    m_pWriter = pWriter;
}

gint32 CImplDeserialArray::Output()
{
    CJavaSnippet os( m_pWriter );
    os.EmitBanner();
    gint32 ret = 0;

    do{
        Wa( "public class DeserialArrays" );
        BLOCK_OPEN;
        Wa( "public static Object deserialArray(" );
        Wa( "    JavaSerialBase osb, ByteBuffer bbuf, String sig )" );
        BLOCK_OPEN;
        Wa( "IDeserialArray o = null;" );
        Wa( "if( m_oDeserialArrays.containsKey( sig ) )" );
        Wa( "    o = m_oDeserialArrays.get( sig );" );
        Wa( "if( o == null )" );
        Wa( "    return null;" );
        CCOUT << "return o.deserialArray( osb, bbuf, sig );";
        BLOCK_CLOSE;
        NEW_LINES( 2 );
        Wa( "static public interface IDeserialArray" );
        BLOCK_OPEN;    
        Wa( "public abstract Object deserialArray(" );
        CCOUT << "    JavaSerialBase osb, ByteBuffer bbuf, String sig );";
        BLOCK_CLOSE; 
        NEW_LINES( 2 );
        Wa( "static Map< String, IDeserialArray >" );
        Wa( "    m_oDeserialArrays = initMaps();" );
        NEW_LINE;
        Wa( "static Map< String, IDeserialArray > initMaps()" );
        BLOCK_OPEN; 
        Wa( "Map< String, IDeserialArray > o =" );
        Wa( "    new HashMap< String, IDeserialArray >();" );
        Wa( "IDeserialArray val;" );
        for( auto& elem : g_mapArrayTypeDecl )
        {
            CArrayType* pat = elem.second;
            stdstr strSig = elem.first;
            stdstr strTypeText;
            ret = CJTypeHelper::GetTypeText(
                elem.second, strTypeText );
            if( ERROR( ret ) )
                break;

            stdstr strValType1;
            ObjPtr pValType = pat->GetElemType();

            ret = CJTypeHelper::GetTypeText(
                pValType, strValType1 );
            if( ERROR( ret ) )
                break;

            stdstr strValType;
            CJTypeHelper::GetObjectPrimitive(
                strValType1, strValType );

            stdstr sigElem;
            CAstNodeBase* pNode = pValType;
            if( pNode == nullptr )
                continue;

            sigElem = pNode->GetSignature();
            if( sigElem.size() == 1 )
            {
                bool bPrimitive = false;
                switch( strValType[ 0 ] )
                {
                case 'Q':
                case 'q':
                case 'h':
                case 'D':
                case 'd':
                case 'W':
                case 'w':
                case 'f':
                case 'F':
                case 'b':
                case 'B':
                    bPrimitive = true;
                    break;
                default:
                    break;
                }
                if( bPrimitive )
                    continue;
            }

            Wa( "val = new IDeserialArray() " );
            BLOCK_OPEN;

            Wa( "public Object  deserialArray(" );
            Wa( "    JavaSerialBase osb, ByteBuffer bbuf, String sig )" );
            BLOCK_OPEN;
            CCOUT << "if( !sig.equals( \""<< strSig << "\" ) )";
            NEW_LINE;
            Wa( "    throw new IllegalArgumentException(" );
            CCOUT << "        \"deserialArray[" << strTypeText << "]\" + ";
            NEW_LINE;
            CCOUT << "        " << "\" error signature\" );";
            NEW_LINE;
            Wa( "return osb.deserialArrayInternal(" );
            CCOUT << "    bbuf, sig, "
                << strValType << ".class );";

            BLOCK_CLOSE;
            BLOCK_CLOSE;
            CCOUT << ";";
            NEW_LINE;
            CCOUT << "o.put( \"" << strSig << "\", val );";
            NEW_LINES( 2 );
        }
        CCOUT << "return o;";
        BLOCK_CLOSE; // initMaps
        BLOCK_CLOSE;// DeserialArrays

    }while( 0 );
    return ret;
}
