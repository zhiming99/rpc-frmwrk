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

#include "serijson.h"
#include "streamex.h"
using namespace rpcf;

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
    if( val.empty() || !val.isUInt64() )
        return -EINVAL;
    HANDLE hStream = val.asUInt64();
    HSTREAM oStm;
    oStm.m_hStream = hStream;
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

        if( !( val.isMember( JSON_ATTR_MSGID ) &&
            val[ JSON_ATTR_MSGID ].isUInt() ) )
        {
            ret = -EINVAL;
            break;
        }

        guint32 dwMsgId =
            val[ JSON_ATTR_MSGID ].asUInt(); 

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
        ret = pStruct->JsonSerialize(
            pBuf, val );
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
            ret = SerialElem( pBuf, val[ ( gint32 )i ],
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
                strKey.c_str(), nullptr, 10 );
            ret = Serialize( pBuf, qwVal );
            break;
        }
    case 'D':
    case 'd':
        {
            guint32 dwVal = ( guint32 )strtoul(
                strKey.c_str(), nullptr, 10 );
            ret = Serialize( pBuf, dwVal );
            break;
        }
    case 'W':
    case 'w':
        {
            guint16 wVal = ( guint16 )strtoul(
                strKey.c_str(), nullptr, 10 );
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
                strKey.c_str(), nullptr, 10 );
            ret = Serialize( pBuf, bVal );
            break;
        }
    case 'h':
        {
            guint64 qwVal = ( guint64 )strtoull(
                strKey.c_str(), nullptr, 10 );
            Value valStm( qwVal );
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
            ret = SerializeObjPtr(
                pBuf, val );
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
            ret = SerialElem(
                pBuf, oKey ,
                strKeySig.c_str(),
                true );
            if( ERROR( ret ) )
                break;

            ret = SerialElem(
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

gint32 CJsonSerialBase::SerialElem(
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
            ret = SerializeUInt64(
                pBuf, val );
            break;
        }
    case 'D':
    case 'd':
        {
            ret = SerializeUInt(
                pBuf, val );
            break;
        }
    case 'W':
    case 'w':
        {
            ret = SerializeUShort(
                pBuf, val );
            break;
        }
    case 'f':
        {
            ret = SerializeFloat(
                pBuf, val );
            break;
        }
    case 'F':
        {
            ret = SerializeDouble(
                pBuf, val );
            break;
        }
    case 'b':
        {
            ret = SerializeBool(
                pBuf, val );
            break;
        }
    case 'B':
        {
            ret = SerializeByte(
                pBuf, val );
            break;
        }
    case 'h':
        {
            ret = SerializeHStream(
                pBuf, val );
            break;
        }
    case 's':
        {
            ret = SerializeString(
                pBuf, val );
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
            ret = SerializeObjPtr(
                pBuf, val );
            break;
        }
    case 'O':
        {
            ret = SerializeStruct(
                pBuf, val );
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
    val = qwVal;
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
    val = ( HANDLE )oStm.m_hStream;
    return STATUS_SUCCESS;
}

gint32 CJsonSerialBase::DeserializeObjPtr(
    BufPtr& pBuf, Value& val )
{
    gint32 ret = 0;
    SERI_HEADER_BASE* pHeader =
        ( SERI_HEADER_BASE* )pBuf->ptr();
    SERI_HEADER_BASE oHeader;
    memcpy( &oHeader, pHeader,
        sizeof( SERI_HEADER_BASE ) );
    oHeader.ntoh();
    guint32 dwSize = oHeader.dwSize +
        sizeof( SERI_HEADER_BASE );
    if( oHeader.dwSize == 0 )
    {
        val = "";
        pBuf->IncOffset(
            sizeof( SERI_HEADER_BASE ) );
        return STATUS_SUCCESS;
    }
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
        ( guint8* )pBuf->ptr(),
        pBuf->size(), pNewBuf );

    if( ERROR( ret ) )
        return ret;

    val = pNewBuf->ptr();
    return STATUS_SUCCESS;
}

gint32 CJsonSerialBase::DeserializeStruct(
    BufPtr& pBuf, Value& val )
{
    gint32 ret = 0;
    do{
        SERI_HEADER_BASE* pHeader =
            ( SERI_HEADER_BASE* )pBuf->ptr();
        SERI_HEADER_BASE oHeader;
        memcpy( &oHeader, pHeader,
            sizeof( SERI_HEADER_BASE ) );
        oHeader.ntoh();

        guint32 dwMsgId = oHeader.dwClsid;
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
        ret = pStruct->JsonDeserialize(
            pBuf, val );

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
            ret = DeserialElem( pBuf, elem,
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
            ret = DeserialElem(
                pBuf, key,
                strKeySig.c_str(),
                true );

            if( ERROR( ret ) )
                break;

            ret = DeserialElem(
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

gint32 CJsonSerialBase::DeserialElem(
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
            ret = DeserializeUInt64(
                pBuf, val );
            break;
        }
    case 'D':
    case 'd':
        {
            ret = DeserializeUInt(
                pBuf, val );
            break;
        }
    case 'W':
    case 'w':
        {
            ret = DeserializeUShort(
                pBuf, val );
            break;
        }
    case 'f':
        {
            ret = DeserializeFloat(
                pBuf, val );
            break;
        }
    case 'F':
        {
            ret = DeserializeDouble(
                pBuf, val );
            break;
        }
    case 'b':
        {
            ret = DeserializeBool(
                pBuf, val );
            break;
        }
    case 'B':
        {
            ret = DeserializeByte(
                pBuf, val );
            break;
        }
    case 'h':
        {
            ret = DeserializeHStream(
                pBuf, val );
            break;
        }
    case 's':
        {
            ret = DeserializeString(
                pBuf, val );
            break;
        }
    case 'a':
        {
            ret = DeserializeByteArray(
                pBuf, val );
            break;
        }
    case 'o':
        {
            ret = DeserializeObjPtr(
                pBuf, val );
            break;
        }
    case 'O':
        {
            if( bKey )
            {
                ret = -EINVAL;
                break;
            }
            ret = DeserializeStruct(
                pBuf, val );
            break;
        }
    default:
        ret = -EINVAL;
    }
    return ret;
}
