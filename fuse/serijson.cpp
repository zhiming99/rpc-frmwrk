/*
 * =====================================================================================
 *
 *       Filename:  serijson.cpp
 *
 *    Description:  Implementations of Json classes serializing to or deserializing
 *                  from ridl buffer
 *
 *        Version:  1.0
 *        Created:  02/07/2022 08:04:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#include "rpc.h"
using namespace rpcf;
#include "serijson.h"
#include "streamex.h"
#include "fuseif.h"

gint32 CJsonSerialBase::SerializeBool(
    BufPtr& pBuf, const Value& val )
{
    if( val.empty() || !val.isBool() )
        return -EINVAL;
    bool bVal = val.asBool();
    return Serialize( pBuf, bVal );
}

gint32 CJsonSerialBase::SerializeByte(
    BufPtr& pBuf, const Value& val )
{
    if( val.empty() ||
        !( val.isInt() && val.isUInt() ) )
        return -EINVAL;
    guint8 bVal = ( guint8 )val.asInt();
    return Serialize( pBuf, bVal );
}

gint32 CJsonSerialBase::SerializeUShort(
    BufPtr& pBuf, const Value& val )
{
    if( val.empty() ||
        !( val.isInt() && val.isUInt() ) )
        return -EINVAL;
    guint16 wVal = ( guint16 )val.asUInt();
    return Serialize( pBuf, wVal );
}

gint32 CJsonSerialBase::SerializeUInt(
    BufPtr& pBuf, const Value& val )
{
    if( val.empty() ||
        !( val.isInt() && val.isUInt() ) )
        return -EINVAL;
    guint32 dwVal = val.asUInt();
    return Serialize( pBuf, dwVal );
}

gint32 CJsonSerialBase::SerializeUInt64(
    BufPtr& pBuf, const Value& val )
{
    if( val.empty() ||
        !( val.isInt64() && val.isUInt64() ) )
        return -EINVAL;
    guint64 qwVal = val.asUInt64();
    return Serialize( pBuf, qwVal );
}

gint32 CJsonSerialBase::SerializeFloat(
    BufPtr& pBuf, const Value& val )
{
    if( val.empty() || !val.isDouble() )
        return -EINVAL;
    float fVal = val.asFloat();
    return Serialize( pBuf, fVal );
}
gint32 CJsonSerialBase::SerializeDouble(
    BufPtr& pBuf, const Value& val )
{
    if( val.empty() || !val.isDouble() )
        return -EINVAL;
    double dblVal = val.asDouble();
    return Serialize( pBuf, dblVal );
}
gint32 CJsonSerialBase::SerializeString(
    BufPtr& pBuf, const Value& val )
{
    if( !val.isString() )
        return -EINVAL;
    stdstr strVal = val.asString();
    return Serialize( pBuf, strVal );
}
gint32 CJsonSerialBase::SerializeHStream(
    BufPtr& pBuf, const Value& val )
{
    if( val.empty() || !val.isString() )
        return -EINVAL;
    gint32 ret = 0;
    ObjPtr pIf = this->GetIf();
    CFuseSvcServer* pSvr = pIf;
    CFuseSvcProxy* pProxy = pIf;
    HANDLE hstm = INVALID_HANDLE;
    do{
        // we are within the RLOCK/WLOCK
        if( IsLocked() )
        {
            if( pSvr != nullptr )
            {
                ret = pSvr->NameToStream(
                    val.asString(), hstm );
            }
            else if( pProxy != nullptr )
            {
                ret = pProxy->NameToStream(
                    val.asString(), hstm );
            }
            else
            {
                ret = -EFAULT;
            }
        }
        else
        {
            RLOCK_TESTMNT2( this->GetIf() );
            if( pSvr != nullptr )
            {
                ret = pSvr->NameToStream(
                    val.asString(), hstm );
            }
            else if( pProxy != nullptr )
            {
                ret = pProxy->NameToStream(
                    val.asString(), hstm );
            }
            else
            {
                ret = -EFAULT;
            }
        }

    }while( 0 );

    if( ERROR( ret ) )
        return ret;

    HSTREAM oStm;
    oStm.m_hStream = hstm;
    oStm.m_pIf = this->GetIf();
    return oStm.Serialize( pBuf );
}

gint32 CJsonSerialBase::SerializeObjPtr(
    BufPtr& pBuf, const Value& val )
{
    if( val.empty() || !val.isString() )
        return -EINVAL;
    const char* szVal = val.asCString();
    size_t len = strlen( szVal );
    return base64_decode( szVal, len, pBuf );
}

gint32 CJsonSerialBase::SerializeByteArray(
    BufPtr& pBuf, const Value& val )
{
    if( !val.isString() )
        return -EINVAL;
    const char* szVal = val.asCString();
    size_t len = strlen( szVal );
    BufPtr pByteArray( true );
    gint32 ret = base64_decode(
        szVal, len, pByteArray );
    if( ERROR( ret ) )
        return ret;

    return Serialize( pBuf, pByteArray );
}

gint32 CJsonSerialBase::SerializeStruct(
    BufPtr& pBuf, const Value& val )
{
    gint32 ret = 0;
    do{
        if( !val.isObject() )
        {
            ret = -EINVAL;
            break;
        }

        if( !( val.isMember( JSON_ATTR_STRUCTID ) &&
            val[ JSON_ATTR_STRUCTID ].isUInt() ) )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwMsgId =
            val[ JSON_ATTR_STRUCTID ].asUInt(); 

        if( g_setMsgIds.find( dwMsgId ) ==
            g_setMsgIds.end() )
        {
            ret = -ENOENT;
            break;
        }
            
        ObjPtr pObj;
        ret = pObj.NewObj( ( EnumClsid )dwMsgId );
        if( ERROR( ret ) )
            break;

        CJsonStructBase* pStruct = pObj;
        pStruct->SetIf( this->GetIf() );
        pStruct->SetLocked( IsLocked() );
        ret = pStruct->JsonSerialize( pBuf, val );
    }while( 0 );

    return ret;
}

gint32 CJsonSerialBase::SerializeVariant(
    BufPtr& pBuf, const Value& val )
{
    gint32 ret = 0;
    do{
        if( !val.isObject() )
        {
            ret = -EINVAL;
            break;
        }

        if( !( val.isMember( "t" ) &&
            val[ "t" ].isUInt() ) )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwTypeId = val[ "t" ].asUInt(); 
        if( dwTypeId != typeNone ||
            !val.isMember( "v" ) )
        {
            ret = -EINVAL;
            break;
        }

        const Value& subVal = val[ "v" ];
        ret = this->SerializeByte(
            pBuf, dwTypeId );
        if( ERROR( ret ) )
            break;

        switch( dwTypeId )
        {
        case typeByte: 
            {
                ret = SerializeByte( pBuf, subVal );
                break;
            }
        case typeUInt16:
            {
                ret = SerializeUShort( pBuf, subVal );
                break;
            }
        case typeUInt32:
            {
                ret = SerializeUInt( pBuf, subVal );
                break;
            }
        case typeUInt64:
            {
                ret = SerializeUInt64( pBuf, subVal );
                break;
            } 
        case typeFloat:
            {
                ret = SerializeUInt64( pBuf, subVal );
                break;
            } 
        case typeDouble:
            {
                ret = SerializeUInt64( pBuf, subVal );
                break;
            } 
        case typeString:
            {
                ret = SerializeUInt64( pBuf, subVal );
                break;
            } 
        case typeObj:
            {
                if( subVal.isObject() )
                    ret = SerializeStruct( pBuf, subVal );
                else
                    ret = SerializeObjPtr( pBuf, subVal );
                break;
            } 
        case typeByteArr:
            {
                ret = SerializeByteArray( pBuf, subVal );
                break;
            } 
        case typeNone:
            { break; }
        case typeDMsg:
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }
    }while( 0 );

    return ret;
}

gint32 CJsonSerialBase::SerializeArray(
    BufPtr& pBuf, const Value& val,
    const char* szSignature )
{
    // don't test pBuf->empty, since it is true
    // all need to do is to append
    if( pBuf.IsEmpty() ||
        szSignature == nullptr ||
        *szSignature != '(' )
        return -EINVAL;

    if( !val.isArray() )
        return -EINVAL;

    gint32 ret = STATUS_SUCCESS;

    do{
        std::string strElemSig = szSignature + 1;
        if( strElemSig.empty() ||
            strElemSig.back() != ')' )
        {
            ret = -EINVAL;
            break;
        }
        std::string::iterator itr =
            strElemSig.begin() +
            ( strElemSig.size() - 1 );
        strElemSig.erase( itr );

        guint32 dwHdrSize =
            2 * sizeof( guint32 );

        guint32 dwBytesOff = pBuf->offset();
        Serialize( pBuf, ( guint32 )0 );

        guint32 dwCount = val.size();
        if( dwCount == 0 )
        {
            Serialize( pBuf, ( guint32 )0 );
            break;
        }

        if( dwCount >= MAX_ELEM_COUNT )
        {
            ret = -ERANGE;
            break;
        }

        Serialize( pBuf,
            ( guint32 )val.size() );

        dwHdrSize = pBuf->offset() -
            dwBytesOff;

        gint32 ret = 0;
        for( size_t i = 0; i < dwCount; ++i )
        {
            ret = SerializeElem( pBuf,
                val[ ( gint32 )i ],
                strElemSig.c_str() );

            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            break;

        guint32 dwBytes = pBuf->offset() -
            dwBytesOff - dwHdrSize;

        guint32* pdwBytes = ( guint32* )(
            pBuf->ptr() - pBuf->offset() +
            dwBytesOff );

        dwBytes = htonl( dwBytes );
        memcpy( pdwBytes,
            &dwBytes, sizeof( dwBytes ) );

    }while( 0 );

    return ret;
}

gint32 CJsonSerialBase::SerializeFromStr(
    BufPtr& pBuf,
    const Value& val,
    const char* szSig )
{
    if( szSig == nullptr ||
        val.isNull() ||
        !val.isString() )
        return -EINVAL;

    stdstr strKey = val.asString();
    char ch = szSig[ 0 ];
    gint32 ret = 0;

    Json::CharReaderBuilder oBuilder;
    std::shared_ptr< Json::CharReader >
        pReader( oBuilder.newCharReader() );
    if( pReader == nullptr )
        return -ENOMEM;

    switch( ch )
    {
    case '[':
        {
            Value valKey;
            const char* pszKey = strKey.c_str();
            if( !pReader->parse( pszKey,
                pszKey + strKey.size(),
                &valKey, nullptr ) )
            {
                ret = -EBADMSG;
                break;
            }

            Json::Value keyObj = 
            ret = SerializeMap(
                pBuf, valKey, szSig );
            break;
        }
    case '(':
        {
            Value valKey;
            const char* pszKey = strKey.c_str();
            if( !pReader->parse( pszKey,
                pszKey + strKey.size(),
                &valKey, nullptr ) )
            {
                ret = -EBADMSG;
                break;
            }
            ret = SerializeArray(
                pBuf, valKey, szSig );
            break;
        }
    case 'Q':
    case 'q':
        {
            guint64 qwVal = ( guint64 )strtoull(
                strKey.c_str(), nullptr, 0 );
            ret = Serialize( pBuf, qwVal );
            break;
        }
    case 'D':
    case 'd':
        {
            guint32 dwVal = ( guint32 )strtoul(
                strKey.c_str(), nullptr, 0 );
            ret = Serialize( pBuf, dwVal );
            break;
        }
    case 'W':
    case 'w':
        {
            guint16 wVal = ( guint16 )strtoul(
                strKey.c_str(), nullptr, 0 );
            ret = Serialize( pBuf, wVal );
            break;
        }
    case 'f':
        {
            float fVal = strtof(
                strKey.c_str(), nullptr );
            ret = Serialize( pBuf, fVal );
            break;
        }
    case 'F':
        {
            double fVal = strtod(
                strKey.c_str(), nullptr );
            ret = Serialize( pBuf, fVal );
            break;
        }
    case 'b':
        {
            bool bVal;
            if( strKey == "true" )
                bVal = true;
            else if( strKey == "false" )
                bVal = false;
            else
            {
                ret = -EINVAL;
                break;
            }

            ret = Serialize( pBuf, bVal );
            break;
        }
    case 'B':
        {
            guint8 bVal = strtoul(
                strKey.c_str(), nullptr, 0 );
            ret = Serialize( pBuf, bVal );
            break;
        }
    case 'h':
        {
            guint64 qwVal = ( guint64 )strtoull(
                strKey.c_str(), nullptr, 0 );
            Value valStm( ( Json::UInt64& )qwVal );
            ret = SerializeHStream( pBuf, valStm );
            break;
        }
    case 's':
        {
            ret = SerializeString( pBuf, val );
            break;
        }
    case 'a':
        {
            ret = SerializeByteArray(
                pBuf, val );
            break;
        }
    case 'o':
        {
            ret = SerializeObjPtr( pBuf, val );
            break;
        }
    case 'O':
        {
            Value valKey;
            const char* pszKey = strKey.c_str();
            if( !pReader->parse( pszKey,
                pszKey + strKey.size(),
                &valKey, nullptr ) )
            {
                ret = -EBADMSG;
                break;
            }
            ret = SerializeStruct( pBuf, valKey );
            break;
        }
    case 'v':
        {
            ret = SerializeVariant( pBuf, val );
            break;
        }
    default:
        ret = -EINVAL;
    }
    return ret;
}

gint32 CJsonSerialBase::SerializeMap(
    BufPtr& pBuf,
    const Value& val,
    const char* szSignature )
{
    if( pBuf.IsEmpty() ||
        szSignature == nullptr ||
        *szSignature != '[' )
        return -EINVAL;

    if( !val.isObject() )
        return -EINVAL;

    gint32 ret = 0;
    do{
        std::string strElemSig =
            szSignature + 1;

        if( strElemSig.empty() ||
            strElemSig.back() != ']' )
        {
            ret = -EINVAL;
            break;
        }

        std::string::iterator itr =
            strElemSig.begin() +
            ( strElemSig.size() - 1 );

        strElemSig.erase( itr );
        std::string strKeySig =
            strElemSig.substr( 0, 1 );

        strElemSig.erase(
            strElemSig.begin() );

        guint32 dwBytesOff = pBuf->offset();

        Serialize( pBuf, ( guint32 )0 );

        guint32 dwCount = val.size();
        if( dwCount == 0 )
        {
            Serialize( pBuf, ( guint32 )0 );
            break;
        }

        if( dwCount >= MAX_ELEM_COUNT )
        {
            ret = -ERANGE;
            break;
        }

        Serialize( pBuf,
            ( guint32 )dwCount );

        guint32 dwHdrSize =
            pBuf->offset() - dwBytesOff;

        gint32 ret = 0;
        Value::Members vecNames =
            val.getMemberNames();
        for( size_t i = 0; i < dwCount; ++i )
        {
            std::string&
                strKey = vecNames[ i ];

            Value oKey( strKey );
            ret = SerializeElem(
                pBuf, oKey ,
                strKeySig.c_str(),
                true );
            if( ERROR( ret ) )
                break;

            ret = SerializeElem(
                pBuf, val[ strKey ] ,
                strElemSig.c_str() );
            if( ERROR( ret ) )
                break;
        }

        if( ERROR( ret ) )
            break;

        guint32 dwBytes = pBuf->offset() -
            dwBytesOff - dwHdrSize;

        guint32* pdwBytes = ( guint32* )(
            pBuf->ptr() - pBuf->offset() +
            dwBytesOff );

        dwBytes = htonl( dwBytes );
        memcpy( pdwBytes,
            &dwBytes, sizeof( dwBytes ) );

    }while( 0 );
    return ret;
}

gint32 CJsonSerialBase::SerializeElem(
    BufPtr& pBuf, const Value& val,
    const char* szSignature, bool bKey )
{
    char ch = szSignature[ 0 ];
    gint32 ret = 0;
    if( bKey )
    {
        return SerializeFromStr(
            pBuf, val, szSignature );
    }
    switch( ch )
    {
    case '[':
        {
            ret = SerializeMap(
                pBuf, val, szSignature );
            break;
        }
    case '(':
        {
            ret = SerializeArray(
                pBuf, val, szSignature );
            break;
        }
    case 'Q':
    case 'q':
        {
            ret = SerializeUInt64( pBuf, val );
            break;
        }
    case 'D':
    case 'd':
        {
            ret = SerializeUInt( pBuf, val );
            break;
        }
    case 'W':
    case 'w':
        {
            ret = SerializeUShort( pBuf, val );
            break;
        }
    case 'f':
        {
            ret = SerializeFloat( pBuf, val );
            break;
        }
    case 'F':
        {
            ret = SerializeDouble( pBuf, val );
            break;
        }
    case 'b':
        {
            ret = SerializeBool( pBuf, val );
            break;
        }
    case 'B':
        {
            ret = SerializeByte( pBuf, val );
            break;
        }
    case 'h':
        {
            ret = SerializeHStream( pBuf, val );
            break;
        }
    case 's':
        {
            ret = SerializeString( pBuf, val );
            break;
        }
    case 'a':
        {
            ret = SerializeByteArray( pBuf, val );
            break;
        }
    case 'o':
        {
            ret = SerializeObjPtr( pBuf, val );
            break;
        }
    case 'O':
        {
            ret = SerializeStruct( pBuf, val );
            break;
        }
    case 'v':
        {
            ret = SerializeVariant( pBuf, val );
            break;
        }
    default:
        ret = -EINVAL;
    }
    return ret;
}

gint32 CJsonSerialBase::DeserializeBool(
    BufPtr& pBuf, Value& val )
{
    bool bVal;
    gint32 ret = Deserialize( pBuf, bVal );
    if( ERROR( ret ) )
        return ret;
    val = bVal;
    return STATUS_SUCCESS;
}

gint32 CJsonSerialBase::DeserializeByte(
    BufPtr& pBuf, Value& val )
{
    guint8 bVal;
    gint32 ret = Deserialize( pBuf, bVal );
    if( ERROR( ret ) )
        return ret;
    val = ( guint32 )bVal;
    return STATUS_SUCCESS;
}
gint32 CJsonSerialBase::DeserializeUShort(
    BufPtr& pBuf, Value& val )
{
    guint16 wVal;
    gint32 ret = Deserialize( pBuf, wVal );
    if( ERROR( ret ) )
        return ret;
    val = ( guint32 )wVal;
    return STATUS_SUCCESS;
}
gint32 CJsonSerialBase::DeserializeUInt(
    BufPtr& pBuf, Value& val )
{
    guint32 dwVal;
    gint32 ret = Deserialize( pBuf, dwVal );
    if( ERROR( ret ) )
        return ret;
    val = dwVal;
    return STATUS_SUCCESS;
}

gint32 CJsonSerialBase::DeserializeUInt64(
    BufPtr& pBuf, Value& val )
{
    guint64 qwVal;
    gint32 ret = Deserialize( pBuf, qwVal );
    if( ERROR( ret ) )
        return ret;
    val = ( Json::UInt64& )qwVal;
    return STATUS_SUCCESS;
}

gint32 CJsonSerialBase::DeserializeFloat(
    BufPtr& pBuf, Value& val )
{
    float fVal;
    gint32 ret = Deserialize( pBuf, fVal );
    if( ERROR( ret ) )
        return ret;
    val = fVal;
    return STATUS_SUCCESS;

}
gint32 CJsonSerialBase::DeserializeDouble(
    BufPtr& pBuf, Value& val )
{
    double fVal;
    gint32 ret = Deserialize( pBuf, fVal );
    if( ERROR( ret ) )
        return ret;
    val = fVal;
    return STATUS_SUCCESS;
}

gint32 CJsonSerialBase::DeserializeString(
    BufPtr& pBuf, Value& val )
{
    stdstr strVal;
    gint32 ret = Deserialize( pBuf, strVal );
    if( ERROR( ret ) )
        return ret;
    val = strVal;
    return STATUS_SUCCESS;
}
gint32 CJsonSerialBase::DeserializeHStream(
    BufPtr& pBuf, Value& val )
{
    HSTREAM oStm;
    oStm.m_pIf = this->GetIf();
    gint32 ret = oStm.Deserialize( pBuf );
    if( ERROR( ret ) )
        return ret;
    HANDLE hstm = oStm.m_hStream;
    ObjPtr pIf = oStm.m_pIf;
    CFuseSvcServer* pSvr = pIf;
    CFuseSvcProxy* pProxy = pIf;
    stdstr strFile;
    do{
        if( IsLocked() )
        {
            if( pSvr != nullptr )
            {
                ret = pSvr->StreamToName(
                    hstm, strFile );
            }
            else
            {
                ret = pProxy->StreamToName(
                    hstm, strFile );
            }
        }
        else
        {
            RLOCK_TESTMNT2( this->GetIf() );
            if( pSvr != nullptr )
            {
                ret = pSvr->StreamToName(
                    hstm, strFile );
            }
            else
            {
                ret = pProxy->StreamToName(
                    hstm, strFile );
            }
        }

    }while( 0 );

    if( ERROR( ret ) )
        return ret;

    val = strFile;
    return STATUS_SUCCESS;
}

gint32 CJsonSerialBase::DeserializeObjPtr(
    BufPtr& pBuf, Value& val )
{
    gint32 ret = 0;
    guint32 dwSize = 0;
    guint8* pHead = ( guint8* )pBuf->ptr();

    guint32 dwHdrSize =
        sizeof( SERI_HEADER_BASE );

    memcpy( &dwSize,
        pHead + sizeof( dwSize ),
        sizeof( dwSize ) );

    dwSize = ntohl( dwSize );
    dwSize += dwHdrSize;

    if( dwSize > pBuf->size() )
        return -ERANGE;

    BufPtr pNewBuf( true ); 
    ret = base64_encode(
        ( guint8* )pBuf->ptr(),
        dwSize, pNewBuf );

    if( ERROR( ret ) )
        return ret;

    val = pNewBuf->ptr();
    pBuf->IncOffset( dwSize );
    return STATUS_SUCCESS;
}

gint32 CJsonSerialBase::DeserializeByteArray(
    BufPtr& pBuf, Value& val )
{
    BufPtr pVal;
    gint32 ret = Deserialize( pBuf, pVal );
    if( ERROR( ret ) )
        return ret;
    BufPtr pNewBuf( true );
    ret = base64_encode(
        ( guint8* )pVal->ptr(),
        pVal->size(), pNewBuf );

    if( ERROR( ret ) )
        return ret;

    val = Value( pNewBuf->ptr(),
        pNewBuf->ptr() + pNewBuf->size() );
    return STATUS_SUCCESS;
}

gint32 CJsonSerialBase::DeserializeStruct(
    BufPtr& pBuf, Value& val )
{
    gint32 ret = 0;
    do{

        guint32 dwMagicId = 0;
        guint32 dwMsgId = 0;
        if( pBuf->size() < sizeof( guint32 ) * 2)
        {
            ret = -ERANGE;
            break;
        }
        memcpy( &dwMagicId, pBuf->ptr(),
            sizeof( dwMagicId ) );
        dwMagicId = ntohl( dwMagicId );
        if( dwMagicId != SERIAL_STRUCT_MAGIC )
        {
            ret = -EBADMSG;
            break;
        }

        memcpy( &dwMsgId,
            pBuf->ptr() + sizeof( guint32 ),
            sizeof( dwMsgId ) );
        dwMsgId = ntohl( dwMsgId );
        if( g_setMsgIds.find( dwMsgId ) ==
            g_setMsgIds.end() )
        {
            ret = -ENOENT;
            break;
        }

        ObjPtr pObj;
        ret = pObj.NewObj( ( EnumClsid )dwMsgId );
        if( ERROR( ret ) )
            break;

        CJsonStructBase* pStruct = pObj;
        pStruct->SetLocked( IsLocked() );
        ret = pStruct->JsonDeserialize(
            pBuf, val );

    }while( 0 );

    return ret;
}

gint32 CJsonSerialBase::DeserializeVariant(
    BufPtr& pBuf, Value& val )
{
    gint32 ret = 0;
    do{
        guint8 iType;
        ret = Deserialize( pBuf, iType );
        if( ERROR( ret ) )
            break;

        Json::Value oValue;
        switch( iType )
        {
        case typeByte: 
            {
                ret = DeserializeByte( pBuf, oValue );
                if( ERROR( ret ) )
                    break;
                val[ "v" ] = oValue;
                break;
            }
        case typeUInt16:
            {
                ret = DeserializeUShort( pBuf, oValue );
                if( ERROR( ret ) )
                    break;
                val[ "v" ] = oValue;
                break;
            }
        case typeUInt32:
            {
                ret = DeserializeUInt( pBuf, oValue );
                if( ERROR( ret ) )
                    break;
                val[ "v" ] = oValue;
                break;
            }
        case typeUInt64:
            {
                ret = DeserializeUInt64( pBuf, oValue );
                if( ERROR( ret ) )
                    break;
                val[ "v" ] = oValue;
                break;
            } 
        case typeFloat:
            {
                ret = Deserialize( pBuf, oValue );
                if( ERROR( ret ) )
                    break;
                val[ "v" ] = oValue;
                break;
            } 
        case typeDouble:
            {
                ret = DeserializeDouble(
                    pBuf, oValue );
                if( ERROR( ret ) )
                    break;
                val[ "v" ] = oValue;
                break;
            }
        case typeString:
            {
                ret = DeserializeString( pBuf, oValue );
                if( ERROR( ret ) )
                    break;
                val[ "v" ] = oValue;
                break;
            } 
        case typeObj:
            {
                guint32 dwMagicId = 0;
                guint32 dwMsgId = 0;
                if( pBuf->size() < sizeof( guint32 ) * 2)
                {
                    ret = -ERANGE;
                    break;
                }

                memcpy( &dwMagicId, pBuf->ptr(),
                    sizeof( dwMagicId ) );
                dwMagicId = ntohl( dwMagicId );
                if( dwMagicId != SERIAL_STRUCT_MAGIC )
                {
                    ret = DeserializeObjPtr(
                        pBuf, oValue );
                    if( ERROR( ret ) )
                        break;
                    val[ "v" ] = oValue;
                    break;
                }
                else
                {
                    ret = DeserializeStruct(
                        pBuf, oValue );
                    if( ERROR( ret ) )
                        break;
                    val[ "v" ] = oValue;
                    break;

                }
                break;
            } 
        case typeByteArr:
            {
                ret = DeserializeByteArray(
                    pBuf, oValue );
                if( ERROR( ret ) )
                    break;
                val[ "v" ] = oValue;
                break;
            } 
        case typeNone:
            { break; }
        case typeDMsg:
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }
        if( ERROR( ret ) )
            break;
        val[ "t" ] = iType;

    }while( 0 );
    return ret;
}

gint32 CJsonSerialBase::DeserializeArray(
    BufPtr& pBuf, Value& val,
    const char* szSignature )
{
    if( pBuf.IsEmpty() ||
        pBuf->empty() ||
        szSignature == nullptr ||
        *szSignature != '(' )
        return -EINVAL;

    gint32 ret = 0;
    do{
        guint32 dwHdrSize =
            sizeof( guint32 ) * 2;

        if( pBuf->size() < dwHdrSize ) 
        {
            ret = -EINVAL;
            break;
        }

        std::string strElemSig =
            szSignature + 1;

        if( strElemSig.empty() ||
            strElemSig.back() != ')' )
        {
            ret = -EINVAL;
            break;
        }

        std::string::iterator itr =
            strElemSig.begin() +
            ( strElemSig.size() - 1 );

        strElemSig.erase( itr );

        guint32 dwBytes = 0;

        char* pdwBytes = pBuf->ptr();
        ret = Deserialize( pBuf, dwBytes );
        if( ERROR( ret ) )
            break;

        if( dwBytes > MAX_BYTES_PER_BUFFER )
        {
            ret = -ERANGE;
            break;
        }

        guint32 dwCount = 0;
        ret = Deserialize( pBuf, dwCount );
        if( ERROR( ret ) )
            break;

        val = Json::Value( arrayValue );
        if( dwBytes == 0 || dwCount == 0 )
            break;

        gint32 ret = 0;
        if( dwCount >= MAX_ELEM_COUNT )
        {
            ret = -ERANGE;
            break;
        }

        dwHdrSize = pBuf->ptr() - pdwBytes;

        val.resize( dwCount );
        for( size_t i = 0; i < dwCount; ++i )
        {
            Value elem;
            ret = DeserializeElem( pBuf, elem,
                strElemSig.c_str() );
            if( ERROR( ret ) )
                break;
            val[ ( gint32 )i ] = elem;
        }

        if( ERROR( ret ) )
            break;

        guint32 dwActBytes = pBuf->ptr() -
            pdwBytes - dwHdrSize;

        if( dwActBytes != dwBytes )
        {
            ret = -EBADMSG;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CJsonSerialBase::DeserializeMap(
    BufPtr& pBuf,
    Value& val,
    const char* szSignature )
{
    if( pBuf.IsEmpty() ||
        pBuf->empty() ||
        szSignature == nullptr ||
        *szSignature != '[' )
        return -EINVAL;

    gint32 ret = 0;
    guint32 dwHdrSize =
        sizeof( guint32 ) * 2;
    do{
        if( pBuf->size() < dwHdrSize ) 
        {
            ret = -EINVAL;
            break;
        }

        std::string strElemSig =
            szSignature + 1;

        if( strElemSig.empty() ||
            strElemSig.back() != ']' )
        {
            ret = -EINVAL;
            break;
        }

        std::string::iterator itr =
            strElemSig.begin() +
            ( strElemSig.size() - 1 );

        strElemSig.erase( itr );
        std::string strKeySig =
            strElemSig.substr( 0, 1 );

        strElemSig.erase(
            strElemSig.begin() );

        guint32 dwBytes = 0;
        char* pdwBytes = pBuf->ptr();
        ret = Deserialize( pBuf, dwBytes );
        if( ERROR( ret ) )
            break;

        if( dwBytes > MAX_BYTES_PER_BUFFER )
        {
            ret = -ERANGE;
            break;
        }

        guint32 dwCount = 0;
        ret = Deserialize( pBuf, dwCount );
        if( ERROR( ret ) )
            break;

        val = Json::Value( objectValue );
        if( dwCount == 0 )
            break;

        if( dwCount >= MAX_ELEM_COUNT )
        {
            ret = -ERANGE;
            break;
        }

        dwHdrSize = pBuf->ptr() - pdwBytes;

        for( guint32 i = 0; i < dwCount; ++i )
        {
            Value key, elem;
            ret = DeserializeElem(
                pBuf, key,
                strKeySig.c_str(),
                true );

            if( ERROR( ret ) )
                break;

            ret = DeserializeElem(
                pBuf, elem,
                strElemSig.c_str() );

            if( ERROR( ret ) )
                break;

            val[ key.asString() ] = elem;
        }

        if( ERROR( ret ) )
            break;

        guint32 dwActBytes = pBuf->ptr() -
            pdwBytes - dwHdrSize;

        if( dwActBytes != dwBytes )
        {
            ret = -EBADMSG;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CJsonSerialBase::DeserializeElem(
    BufPtr& pBuf, Value& val,
    const char* szSignature, bool bKey )
{
    char ch = szSignature[ 0 ];
    gint32 ret = 0;
    switch( ch )
    {
    case '[':
        {
            if( bKey )
            {
                ret = -EINVAL;
                break;
            }
            ret = DeserializeMap(
                pBuf, val, szSignature );
            break;
        }
    case '(':
        {
            if( bKey )
            {
                ret = -EINVAL;
                break;
            }
            ret = DeserializeArray(
                pBuf, val, szSignature );
            break;
        }
    case 'Q':
    case 'q':
        {
            ret = DeserializeUInt64( pBuf, val );
            break;
        }
    case 'D':
    case 'd':
        {
            ret = DeserializeUInt( pBuf, val );
            break;
        }
    case 'W':
    case 'w':
        {
            ret = DeserializeUShort( pBuf, val );
            break;
        }
    case 'f':
        {
            ret = DeserializeFloat( pBuf, val );
            break;
        }
    case 'F':
        {
            ret = DeserializeDouble( pBuf, val );
            break;
        }
    case 'b':
        {
            ret = DeserializeBool( pBuf, val );
            break;
        }
    case 'B':
        {
            ret = DeserializeByte( pBuf, val );
            break;
        }
    case 'h':
        {
            ret = DeserializeHStream( pBuf, val );
            break;
        }
    case 's':
        {
            ret = DeserializeString( pBuf, val );
            break;
        }
    case 'a':
        {
            ret = DeserializeByteArray( pBuf, val );
            break;
        }
    case 'o':
        {
            ret = DeserializeObjPtr( pBuf, val );
            break;
        }
    case 'O':
        {
            if( bKey )
            {
                ret = -EINVAL;
                break;
            }
            ret = DeserializeStruct( pBuf, val );
            break;
        }
    case 'v':
        {
            ret = DeserializeVariant( pBuf, val );
            break;
        }
    default:
        ret = -EINVAL;
    }
    return ret;
}
