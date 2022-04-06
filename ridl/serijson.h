/*
 * =====================================================================================
 *
 *       Filename:  serijson.h
 *
 *    Description:  Declarations of Json classes serializing to or deserializing
 *                  from ridl buffer
 *
 *        Version:  1.0
 *        Created:  02/07/2022 12:49:54 PM
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
#pragma once
#include "seribase.h"
#include "base64.h"
#include <json/json.h>
#include <set>
#include "fusedefs.h"

using namespace Json;
extern std::set< guint32 > g_setMsgIds;

class CJsonSerialBase : public CSerialBase
{
    protected:
    gint32 SerializeFromStr(
        BufPtr& pBuf,
        const Value& val,
        const char* szSig );

    public:
    typedef CSerialBase super;
    CJsonSerialBase() : super()
    {}
    CJsonSerialBase( ObjPtr pIf ) : super(pIf)
    {}

    gint32 SerializeBool(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeByte(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeUShort(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeUInt(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeUInt64(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeFloat(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeDouble(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeString(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeHStream(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeObjPtr(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeByteArray(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeStruct(
        BufPtr& pBuf, const Value& val );

    gint32 SerializeArray(
        BufPtr& pBuf, const Value& val,
        const char* szSignature );

    gint32 SerializeMap(
        BufPtr& pBuf,
        const Value& val,
        const char* szSignature );

    gint32 SerializeElem(
        BufPtr& pBuf, const Value& val,
        const char* szSignature,
        bool bKey = false );

    gint32 DeserializeBool(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeByte(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeUShort(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeUInt(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeUInt64(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeFloat(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeDouble(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeString(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeHStream(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeObjPtr(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeByteArray(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeStruct(
        BufPtr& pBuf, Value& val );

    gint32 DeserializeArray(
        BufPtr& pBuf, Value& val,
        const char* szSignature );

    gint32 DeserializeMap(
        BufPtr& pBuf,
        Value& val,
        const char* szSignature );

    gint32 DeserializeElem(
        BufPtr& pBuf, Value& val,
        const char* szSignature,
        bool bKey = false );
};

class CJsonStructBase :
    public CJsonSerialBase,
    public CObjBase
{
    public:
    typedef CJsonSerialBase super;
    CJsonStructBase() :
        super(), CObjBase()
    {}

    CJsonStructBase( ObjPtr pIf ) :
        super( pIf ), CObjBase()
    {}

    virtual gint32 Serialize(
        BufPtr& pBuf ) = 0;

    virtual gint32 Deserialize(
        BufPtr& pBuf ) = 0;

    virtual guint32
        GetMsgId() const = 0;

    virtual const std::string&
        GetMsgName() const = 0;
    virtual gint32 JsonSerialize(
        BufPtr& pBuf, const Value& val ) = 0;

    virtual gint32 JsonDeserialize(
        BufPtr& pBuf, Value& val ) = 0;
};

