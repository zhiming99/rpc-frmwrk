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

// Json request/response must have a string attribute
// 'interface' and a string attributes 'method' to
// specify which interface and which method the
// parameters are for.
#define JSON_ATTR_IFNAME        "rpcf_Interface"
#define JSON_ATTR_METHOD        "rpcf_Method"

// A json message must have a string attribute MsgType
// and the value wiill be one of 'req', 'resp' or
// 'evt'
#define JSON_ATTR_MSGTYPE       "rpcf_MsgType"

// A json message can have an object attribute 'params'
#define JSON_ATTR_PARAMS        "rpcf_Params"

// Json representation of a struct must have a UInt
// attribute 'structId', as mapped to the internal
// msgid
#define JSON_ATTR_STRUCTID      "rpcf_StructId"

// Each json request/response/event message has an
// UInt64 attribute reqctxid, as a unique identifier of
// the request. Particularly, the response's identifer
// must be the same as the one from the associated
// request.
#define JSON_ATTR_REQCTXID      "rpcf_ReqCtxId"

// Each json response msg must have an attribute
// ReturnCode with a negative int value defined in
// 'errno.h'. If it is < 0, attribute PARAMS is
// ignored. if its value is STATUS_PENDING on proxy
// side, a uint64 attribute 'taskid' is also returned
// as for canceling purpose. And the true response
// message will be returned in the future.
#define JSON_ATTR_RETCODE       "rpcf_ReturnCode"

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

