#pragma once
#include "rpc.h"

#define DATATYPE_COMP_MASK 0x300UL
typedef EnumContType
{
    typeArray = 0x100UL,
    typeMap = 0x200UL,
};

struct CContCtx
{
    EnumContType m_iContType = typeArray;
    EnumTypeId m_iType = typeNone;
    guint32* m_pdwBytes = nullptr;
    guint32* m_pdwCount = nullptr;
    guint32 m_dwCurCount = 0;
    guint32 m_dwMaxSize = 0;

    CContCtx()
    {}

    CContCtx( const CContCtx& rhs )
    {
        m_iContType = rhs.m_iContType;
        m_iType = rhs.m_iType;
        m_pdwBytes = rhs.m_pdwBytes;
        m_pdwCount = rhs.m_pdwCount;
        m_dwCurCount = rhs.m_dwCurCount;
        m_dwMaxSize = rhs.m_dwMaxSize;
    }
};

class CSerialBase : public CObjBase
{
    public:
    typedef CObjBase super;
    CSerialBase() : super()
    {}

    std::vector< CContCtx > m_vecContStack;

    template< typename T >
    gint32 Serialize( BufPtr& pBuf,
        EnumTypeId iType, T& ptr );

    template< typename T >
    gint32 Deserialize( BufPtr& pBuf,
        EnumTypeId iType, T& pResult );

    gint32 BeginSerialArray(
        CContCtx& oCtx,
        BufPtr& pBuf );

    gint32 EndSerialArray(
        CContCtx& oCtx );

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
        char* pBuf, guint32 dwSize, T& val )
    {
        return val.Deserialize( pBuf, dwSize );
    }
};

class CArgsTuple : public CSerialBase
{
    public CSerialBase super;
    std::vector< BufPtr& > m_vecArgs;
    CArgsTuple() : super() 
    { SetClassId( clsid( CArgsTuple() ); };
};

