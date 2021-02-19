#pragma once
#include "rpc.h"

#define DATATYPE_COMP_MASK 0x300UL
typedef EnumContType
{
    typeArray = 0x100UL,
    typeMap = 0x200UL,
};

struct CSerialCtx
{
    EnumContType m_iContType = typeArray;
    std::vector< guint32 > m_vecElemOff;
    EnumTypeId m_iType = typeNone;
    guint32* m_pdwBytes;
    guint32* m_pdwCount;
};

class CSerialBase : public CObjBase
{
    public:
    typedef CObjBase super;
    CSerialBase() : super()
    {}

    template< typename T >
    gint32 Serialize( BufPtr& pBuf,
        EnumTypeId iType, T& ptr );

    template< typename T >
    gint32 Deserialize( BufPtr& pBuf,
        EnumTypeId iType, T& pResult );

    gint32 BeginSerialArray(
        CSerialCtx& oCtx,
        BufPtr& pBuf );

    gint32 EndSerialArray(
        CSerialCtx& oCtx );

    gint32 BeginDeseriArr( BufPtr& pBuf );

    gint32 EndDeseriArr( BufPtr& pBuf,
        std::vector< T >& vecVals );

    template< typename key, typename val >
    gint32 SerialMap(
        const std::map< key, val >& mapVals);

    template< typename key, typename val >
    gint32 DeserialMap(
        std::map< key, val >& mapVals);

    template< typename T >
    gint32 SerialStruct(
        BufPtr& pBuf, const T& val )
    {
        return val.Serialize( *pBuf );
    }

    template< typename T >
    gint32 DeserialStruct(
        BufPtr& pBuf, guint32 dwSize, T& val )
    {
        return val.Deserialize(
            pBuf->ptr(), dwSize );
    }
};

class CArgsTuple : public CSerialBase
{
    public CSerialBase super;
    std::vector< BufPtr& > m_vecArgs;
    CArgsTuple() : super() 
    { SetClassId( clsid( CArgsTuple() ); };
};

