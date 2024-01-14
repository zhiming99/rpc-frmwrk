/*
 * =====================================================================================
 *
 *       Filename:  configdb.h
 *
 *    Description:  declaration of the IConfigDb and CConfigDb
 *
 *        Version:  1.0
 *        Created:  07/21/2016 09:46:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#include <arpa/inet.h>

#include "autoptr.h"
#include "buffer.h"
#include <stdexcept>
#include <unordered_map>
#include "variant.h"

// NOTE:
// changed back to std::map. Because usually the
// map element is less than 20, it should be ok to
// use std::map without performance loss. and
// keeping the order is important for generating
// the same hash across different platform.
#define hashmap unordered_map

namespace rpcf
{

class CIoManager;

class IConfigDb : public CObjBase
{

    public:
    typedef CObjBase    super;

    virtual gint32 RemoveProperty( gint32 iProp ) = 0;
    virtual void RemoveAll() = 0;

    virtual gint32 size() const = 0;
    virtual bool exist( gint32 iProp ) const = 0;
    virtual const IConfigDb& operator=( const IConfigDb& oCfg ) = 0;

    virtual gint32 SetProperty( gint32 iProp, const BufPtr& pBuf ) = 0;
    virtual gint32 GetProperty( gint32 iProp, BufPtr& pBuf ) const = 0;

    virtual gint32 SetProperty( gint32 iProp, const Variant& oVar ) = 0;
    virtual gint32 GetProperty( gint32 iProp, Variant& oVar ) const = 0;

    // defined in CObjBase
    virtual gint32 GetProperty( gint32 iProp, CBuffer& oBuf ) const = 0;
    virtual gint32 SetProperty( gint32 iProp, const CBuffer& oProp ) = 0;
    virtual gint32 GetPropertyType( gint32 iProp, gint32& iType ) const = 0;

    // more helpers
    virtual gint32 Clone( const IConfigDb& oCfg ) = 0;

};

// typedef CAutoPtr< clsid( CConfigDb2 ), IConfigDb > CfgPtr;
class CConfigDb2 : public IConfigDb
{
    protected:
    std::hashmap<gint32, Variant> m_mapProps;

    public:
    typedef IConfigDb   super;

    struct SERI_HEADER : public SERI_HEADER_BASE
    {
        typedef SERI_HEADER_BASE super;
        guint32 dwCount;

        SERI_HEADER() : SERI_HEADER_BASE()
        {
            dwClsid = clsid( CConfigDb2 );
            dwCount = 0;
        }

        SERI_HEADER( const SERI_HEADER& rhs )
            :SERI_HEADER_BASE( rhs )
        {
            dwCount = rhs.dwCount;
        }

        SERI_HEADER& operator=( const SERI_HEADER& rhs )
        {
            super::operator=( rhs );
            dwCount = rhs.dwCount;
            return *this;
        }

        void ntoh()
        {
            super::ntoh();
            dwCount = ntohl( dwCount );
        }

        void hton()
        {
            super::hton();
            dwCount = htonl( dwCount );
        }
    };

    CConfigDb2( const IConfigDb* pCfg = nullptr );
    ~CConfigDb2();

    // get a reference to pBuf content from the config db
    gint32 GetProperty( gint32 iProp, BufPtr& pBuf ) const;
    // add a reference to pBuf content to the config db
    gint32 SetProperty( gint32 iProp, const BufPtr& pBuf );

    // make a copy of oBuf content from the config db 
    gint32 GetProperty( gint32, CBuffer& oBuf ) const;
    // add a copy of oBuf content to the config db 
    gint32 SetProperty( gint32, const CBuffer& oBuf );

    gint32 GetPropertyType( gint32 iProp, gint32& iType ) const;
    gint32 RemoveProperty( gint32 iProp );
    void RemoveAll();

    // get a reference to variant from the config db
    gint32 GetProperty(
        gint32 iProp, Variant& oVar ) const override;
    // add a reference to variant to the config db
    gint32 SetProperty(
        gint32 iProp, const Variant& oVar ) override;

    // get a reference to variant from the config db
    const Variant& GetProperty( gint32 iProp ) const;
    Variant& GetProperty( gint32 iProp );
    const Variant* GetPropertyPtr( gint32 iProp ) const;
    Variant* GetPropertyPtr( gint32 iProp );

    gint32 GetPropIds( std::vector<gint32>& vecIds ) const;

    virtual gint32 Deserialize( const CBuffer& oBuf );
    gint32 Serialize( CBuffer& oBuf ) const override;
    gint32 Deserialize( const char* oBuf, guint32 dwSize ) override;

    gint32 size() const;
    bool exist( gint32 iProp ) const;

    const IConfigDb& operator=( const IConfigDb& oCfg );
    gint32 Clone( const IConfigDb& oCfg );

    virtual gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const; 
};

template< class T1, typename T=
    typename std::enable_if< std::is_base_of< CObjBase, T1 >::value, T1 >::type >
class CCfgDbOpener
{
    private:
    ObjPtr      m_pObjGuard;

    protected:

    T*          m_pCfg;
    const T*    m_pConstCfg;

    public:
    CCfgDbOpener( const T* pObj )
    {
        m_pCfg = nullptr;
        m_pConstCfg = nullptr;

        if( pObj == nullptr )
        {
            return;
        }

        m_pConstCfg = pObj;
        m_pCfg = nullptr;
        m_pObjGuard = const_cast< T* >( pObj );
    }

    CCfgDbOpener( T* pObj ) :
        CCfgDbOpener( ( const T* )pObj )
    {
        m_pCfg = pObj;
    }

    inline gint32 GetProperty(
        gint32 iProp, CBuffer& oBuf ) const
    {
        BufPtr pBuf;
        gint32 ret = GetProperty( iProp, pBuf ); 
        if( ERROR( ret ) )
            return ret;
        oBuf = *pBuf;
        return 0;
    }

    inline gint32 SetProperty(
        gint32 iProp, const CBuffer& oBuf )
    {
        Variant oVar( oBuf );
        return m_pCfg->SetProperty( iProp, oVar );
    }

    inline gint32 GetProperty(
        gint32 iProp, BufPtr& pBuf ) const
    {
        Variant oVar;
        gint32 ret = GetProperty( iProp, oVar );
        if( ERROR( ret ) )
            return ret;
        pBuf = oVar.ToBuf();
        return 0;
    }

    inline gint32 SetProperty(
        gint32 iProp, const BufPtr& pBuf )
    {
        Variant oVar( *pBuf );
        return m_pCfg->SetProperty( iProp, oVar );
    }

    inline gint32 GetProperty(
        gint32 iProp, Variant& oBuf ) const
    {
        if( m_pConstCfg == nullptr )
            return -EFAULT;
        return m_pConstCfg->GetProperty( iProp, oBuf );
    }

    inline gint32 SetProperty(
        gint32 iProp, const Variant& oBuf )
    {
        if( m_pCfg == nullptr )
            return -EFAULT;

        return m_pCfg->SetProperty( iProp, oBuf );
    }
    
    template< class U, typename U1=
        class std::enable_if< std::is_base_of< CObjBase, U >::value, U >::type  >
    gint32 GetPointer( gint32 iProp, U*& pObj ) const
    {
        gint32 ret = 0;

        do{
            ObjPtr objPtr;

            ret = GetObjPtr( iProp, objPtr );

            if( ERROR( ret ) )
                break;

            pObj = objPtr;

        }while( 0 );

        return ret;
    }

    template< class U, typename U1=
        class std::enable_if< std::is_base_of< CObjBase, U >::value, U >::type  >
    gint32 SetPointer( gint32 iProp, U* pObj )
    {
        gint32 ret = 0;

        do{
            ObjPtr objPtr( pObj );
            ret = SetObjPtr( iProp, objPtr );

        }while( 0 );

        return ret;
    }

    gint32 GetObjPtr( gint32 iProp, ObjPtr& pObj ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            Variant oVar;
            ret = m_pConstCfg->GetProperty(
                iProp, oVar );

            if( ERROR( ret ) )
                break;

            try{
                pObj = ( ObjPtr& )oVar;
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }
        
        }while( 0 );

        return ret;
    }

    gint32 SetObjPtr( gint32 iProp, const ObjPtr& pObj )
    {
        gint32 ret = 0;

        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            Variant oVar( pObj );
            if( pObj.IsEmpty() )
                oVar.Clear();

            ret = m_pCfg->SetProperty( iProp, oVar );
            if( ERROR( ret ) )
                break;
        
        }while( 0 );

        return ret;
    }

    gint32 GetStrProp( gint32 iProp, std::string& strVal ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            Variant oVar;
            ret = m_pConstCfg->GetProperty( iProp, oVar );
            if( ERROR( ret ) )
                break;
            strVal = ( stdstr& )oVar;
        }while( 0 );

        return ret;
    }

    gint32 SetStrProp( gint32 iProp, const std::string& strVal )
    {

        gint32 ret = 0;
        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            Variant oVar( strVal );
            ret = m_pCfg->SetProperty( iProp, oVar );

        }while( 0 );

        return ret;
    }

    gint32 GetStringProp( gint32 iProp, const char*& pStr ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            Variant oVar;
            ret = m_pConstCfg->GetProperty( iProp, oVar );
            if( ERROR( ret ) )
                break;
            pStr = ( const char* )oVar;
        }while( 0 );

        return ret;
    }

    gint32 GetMsgPtr( gint32 iProp, DMsgPtr& pMsg ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            Variant oVar;
            ret = m_pConstCfg->GetProperty(
                iProp, oVar );

            if( ERROR( ret ) )
                break;

            try{
                pMsg = ( DMsgPtr& )oVar;
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }
        
        }while( 0 );

        return ret;
    }

    gint32 SetMsgPtr( gint32 iProp, const DMsgPtr& pMsg )
    {
        gint32 ret = 0;

        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            Variant oVar( pMsg );
            ret = m_pCfg->SetProperty( iProp, oVar );
            if( ERROR( ret ) )
                break;
        
        }while( 0 );

        return ret;
    }

    gint32 CopyProp( gint32 iProp, gint32 iSrcProp,
        const CObjBase* pSrcCfg )
    {
        if( pSrcCfg == nullptr )
            return -EINVAL;

        CCfgDbOpener< CObjBase > a( pSrcCfg );

        Variant oVar;
        gint32 ret = a.GetProperty( iSrcProp, oVar );
        if( ERROR( ret ) )
            return ret;

        return SetProperty( iProp, oVar );
    }

    gint32 CopyProp( gint32 iProp, const CObjBase* pSrcCfg )
    { return CopyProp( iProp, iProp, pSrcCfg ); }


    gint32 MoveProp( gint32 iProp, CObjBase* pSrcCfg )
    {
        if( pSrcCfg == nullptr )
            return -EINVAL;

        gint32 ret = CopyProp( iProp, pSrcCfg );
        if( ERROR( ret ) )
            return ret;

        ret = pSrcCfg->RemoveProperty( iProp );
        return ret;
    }

    gint32 SwapProp( gint32 iProp1, gint32 iProp2 )
    {
        Variant oVar1, oVar2;
        gint32 ret = GetProperty( iProp1, oVar1 );
        if( ERROR( ret ) )
            return ret;

        ret = GetProperty( iProp2, oVar2 );
        if( ERROR( ret ) )
            return ret;

        SetProperty( iProp1, oVar2 );
        SetProperty( iProp2, oVar1 );
        return ret;
    }

    template< typename PrimType >
    gint32 GetPrimProp( gint32 iProp, PrimType& val ) const 
    {
        gint32 ret = 0;

        if( unlikely( m_pConstCfg == nullptr ) )
            return -EFAULT;

        do{
            Variant oVar;
            ret = m_pConstCfg->GetProperty( iProp, oVar );
            if( ERROR( ret ) )
                break;

            val = ( PrimType& )oVar;

        }while( 0 );

        return ret;
    }

    template< typename PrimType >
    gint32 SetPrimProp( gint32 iProp, PrimType val )
    {
        gint32 ret = 0;
        do{
            if( unlikely( m_pCfg == nullptr ) )
            {
                ret = -EFAULT;
                break;
            }

            Variant oVar;
            oVar = val;

            ret = m_pCfg->SetProperty( iProp, oVar );
            if( ERROR( ret ) )
                break;

        }while( 0 );

        return ret;
    }

    gint32 GetIntPtr( gint32 iProp, guint32*& val ) const
    {
        intptr_t val1;
        Variant oVar;
        gint32 ret = GetProperty( iProp, oVar );

        if( ERROR( ret ) )
            return ret;

        val1 = oVar;
        val = ( guint32* )val1;
        return 0;
    }

    gint32 SetIntPtr( gint32 iProp, guint32* val )
    {
        Variant oVar;
        oVar = ( intptr_t )val;
        return SetProperty( iProp, oVar );
    }

    gint32 GetShortProp( gint32 iProp, guint16& val ) const
    {
        return GetPrimProp( iProp, val );
    }

    gint32 SetShortProp( gint32 iProp, guint16 val )
    {
        return SetPrimProp( iProp, val );
    }

    gint32 GetIntProp( gint32 iProp, guint32& val ) const
    {
        return GetPrimProp( iProp, val );
    }

    gint32 SetIntProp( gint32 iProp, guint32 val )
    {
        return SetPrimProp( iProp, val );
    }
    gint32 GetBoolProp( gint32 iProp, bool& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    gint32 SetBoolProp( gint32 iProp, bool val )
    {
        return SetPrimProp( iProp, val );
    }

    gint32 GetByteProp( gint32 iProp, guint8& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    gint32 SetByteProp( gint32 iProp, guint8 val )
    {
        return SetPrimProp( iProp, val );
    }
    gint32 GetFloatProp( gint32 iProp, float& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    gint32 SetFloatProp( gint32 iProp, float val )
    {
        return SetIntProp( iProp, val );
    }

    gint32 GetQwordProp( gint32 iProp, guint64& val ) const
    {
        return GetPrimProp( iProp, val );
    }

    gint32 SetQwordProp( gint32 iProp, guint64 val )
    {
        return SetPrimProp( iProp, val );
    }

    gint32 GetDoubleProp( gint32 iProp, double& val ) const
    {
        return GetPrimProp( iProp, val );
    }

    gint32 SetDoubleProp( gint32 iProp, double val )
    {
        return SetPrimProp( iProp, val );
    }

    template< typename PrimType >
    gint32 IsEqual( gint32 iProp, const PrimType& val )
    {
        // c++11 required
        // static_assert( !std::is_base_of< CObjBase, PrimType >::value );
        gint32 ret = 0;
        do{
            if( m_pConstCfg== nullptr )
            {
                ret = -EFAULT;
                break;
            }

            Variant oVar1, oVar2;
            ret = m_pConstCfg->GetProperty( iProp, oVar1 );
            if( ERROR( ret ) )
                break;

            oVar2 = val;

            if( oVar2 == oVar1 )
                break;
            ret = ERROR_FALSE;
        }while( 0 );

        return ret;
    }

    gint32 IsEqualProp( gint32 iProp, const CObjBase* pObj )
    {
        gint32 ret = 0;

        do{
            if( pObj == nullptr )
            {
                ret = -EINVAL;
                break;
            }
            if( m_pConstCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            Variant oVar1, oVar2;
            ret = pObj->GetProperty( iProp, oVar1 );
            if( ERROR( ret ) )
                break;

            ret = this->GetProperty( iProp, oVar2 );
            if( ERROR( ret ) )
                break;

            if( oVar1 == oVar2 )
                break;

            ret = ERROR_FALSE;

        }while( 0 );
        return ret;
    }
};

// template<> class CCfgDbOpener< CObjBase >;
#define CFGDB2( _pCfg ) \
    static_cast< CConfigDb2* >( _pCfg ) 

#define CCFGDB2( _pCfg ) \
    static_cast< const CConfigDb2* const >( _pCfg )

template<> 
class CCfgDbOpener< IConfigDb >
{
    protected:

    IConfigDb*          m_pCfg;
    const IConfigDb*    m_pConstCfg;

    template< typename PrimType, typename T2=
        typename std::enable_if< !std::is_same< PrimType, stdstr >::value, PrimType >::type >
    gint32 GetPrimProp( gint32 iProp, PrimType& val ) const 
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        auto pdb2 = CCFGDB2( m_pConstCfg );
        do{
            const Variant* p =
                pdb2->GetPropertyPtr( iProp );
            if( p == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            val = *p;

        }while( 0 );

        return ret;
    }

    template< typename PrimType, typename T2=
        typename std::enable_if< std::is_same<
            PrimType, stdstr >::value, PrimType >::type,
        typename T3=T2 >
    gint32 GetPrimProp(
        gint32 iProp, PrimType& val ) const
    {
        gint32 ret = 0;
        if( m_pConstCfg == nullptr )
            return -EFAULT;

        auto pdb2 = CCFGDB2( m_pConstCfg );
        do{
            const Variant* p =
                pdb2->GetPropertyPtr( iProp );
            if( p == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            val = *p;

        }while( 0 );

        return ret;
    }

    template< typename PrimType >
    gint32 SetPrimProp( gint32 iProp, PrimType val )
    {
        gint32 ret = 0;

        auto pdb2 = CFGDB2( m_pCfg );
        do{
            Variant& o =
                pdb2->GetProperty( iProp );
            o = val;

        }while( 0 );

        return ret;
    }

    public:
    CCfgDbOpener( const IConfigDb* pObj )
        : m_pCfg( nullptr ),
        m_pConstCfg( nullptr )
    {
        if( pObj == nullptr )
            return;

        m_pConstCfg = pObj;
    }

    CCfgDbOpener( IConfigDb* pObj ) :
        CCfgDbOpener( ( const IConfigDb* )pObj )
    {
        if( pObj == nullptr )
            return;

        m_pCfg = pObj;
    }

    inline gint32 GetProperty(
        gint32 iProp, BufPtr& pBuf ) const
    {
        if( m_pConstCfg == nullptr )
            return -EFAULT;
        return m_pConstCfg->GetProperty( iProp, pBuf );
    }

    inline gint32 SetProperty(
        gint32 iProp, const BufPtr& pBuf )
    {
        if( m_pCfg == nullptr )
            return -EFAULT;

        return m_pCfg->SetProperty( iProp, pBuf );
    }

    inline gint32 GetProperty(
        gint32 iProp, CBuffer& oBuf ) const
    {
        if( m_pConstCfg == nullptr )
            return -EFAULT;
        return m_pConstCfg->GetProperty( iProp, oBuf );
    }

    inline gint32 SetProperty(
        gint32 iProp, const CBuffer& oBuf )
    {
        if( m_pCfg == nullptr )
            return -EFAULT;

        return m_pCfg->SetProperty( iProp, oBuf );
    }

    inline gint32 GetProperty(
        gint32 iProp, Variant& oVar ) const
    {
        auto pdb2 = CCFGDB2( m_pConstCfg );
        return pdb2->GetProperty( iProp, oVar );
    }

    inline gint32 SetProperty(
        gint32 iProp, const Variant& oVar )
    {
        auto pdb2 = CFGDB2( m_pCfg );
        return pdb2->SetProperty( iProp, oVar );
    }

    inline const Variant* GetPropPtr( gint32 iProp ) const
    {
        auto pdb2 = CCFGDB2( m_pConstCfg );
        return pdb2->GetPropertyPtr( iProp );
    }

    inline Variant* GetPropPtr( gint32 iProp )
    {
        auto pdb2 = CFGDB2( m_pCfg );
        return pdb2->GetPropertyPtr( iProp );
    }

    inline const Variant& operator[]( gint32 iProp ) const
    {
        auto pdb2 = CCFGDB2( m_pConstCfg );
        return pdb2->GetProperty( iProp );
    }

    inline Variant& operator[]( gint32 iProp )
    {
        auto pdb2 = CFGDB2( m_pCfg );
        return pdb2->GetProperty( iProp );
    }

    gint32 GetObjPtr( gint32 iProp, ObjPtr& pObj ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        auto pdb2 = CCFGDB2( m_pConstCfg );
        do{
            const Variant* p;
            p = pdb2->GetPropertyPtr( iProp );
            if( p == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            if( p->GetTypeId() != typeObj )
            {
                ret = -EFAULT;
                break;
            }
            pObj = ( const ObjPtr& )*p;
        
        }while( 0 );

        return ret;
    }

    gint32 SetObjPtr( gint32 iProp, const ObjPtr& pObj )
    {
        gint32 ret = 0;

        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            auto pdb2 = CFGDB2( m_pCfg );
            Variant& o = pdb2->GetProperty( iProp );
            o = pObj;

        }while( 0 );

        return ret;
    }

    gint32 GetBufPtr( gint32 iProp, BufPtr& pBuf ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        auto pdb2 = CCFGDB2( m_pConstCfg );
        do{
            const Variant* p;
            p = pdb2->GetPropertyPtr( iProp );
            if( p == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            if( p->GetTypeId() != typeByteArr )
            {
                ret = -EFAULT;
                break;
            }
            pBuf = ( const BufPtr& )*p;
        
        }while( 0 );

        return ret;
    }

    inline gint32 SetBufPtr( gint32 iProp, const BufPtr& pBuf )
    {
        gint32 ret = 0;

        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            auto pdb2 = CFGDB2( m_pCfg );
            Variant& o = pdb2->GetProperty( iProp );
            o = pBuf;

        }while( 0 );

        return ret;
    }

    gint32 GetStrProp( gint32 iProp, std::string& strVal ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            auto pdb2 = CCFGDB2( m_pConstCfg );
            const Variant* p =
                pdb2->GetPropertyPtr( iProp );
            if( p == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            if( p->GetTypeId() != typeString )
            {
                ret = -EFAULT;
                break;
            }
            strVal = ( const stdstr& )*p;

        }while( 0 );

        return ret;
    }

    gint32 SetStrProp( gint32 iProp, const std::string& strVal )
    {

        gint32 ret = 0;
        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            auto pdb2 = CFGDB2( m_pCfg );
            Variant& o = pdb2->GetProperty( iProp );
            o = strVal;

        }while( 0 );

        return ret;
    }

    gint32 GetStringProp( gint32 iProp, const char*& pStr ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            auto pdb2 = CCFGDB2( m_pConstCfg );
            const Variant* p =
                pdb2->GetPropertyPtr( iProp );
            if( p == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            if( p->GetTypeId() != typeString )
                break;
            pStr = p->m_strVal.c_str();

        }while( 0 );

        return ret;
    }

    gint32 GetMsgPtr( gint32 iProp, DMsgPtr& pMsg ) const
    {
        gint32 ret = 0;

        if( m_pConstCfg == nullptr )
            return -EFAULT;

        do{
            auto pdb2 = CCFGDB2( m_pConstCfg );
            const Variant* p =
                pdb2->GetPropertyPtr( iProp );

            if( p == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            try{
                if( p->GetTypeId() != typeDMsg )
                {
                    ret = -EFAULT;
                    break;
                }
                pMsg = ( const DMsgPtr& )*p;
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }
        
        }while( 0 );

        return ret;
    }

    gint32 SetMsgPtr( gint32 iProp, const DMsgPtr& pMsg )
    {
        gint32 ret = 0;

        do{
            if( m_pCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            auto pdb2 = CFGDB2( m_pCfg );
            Variant& o = pdb2->GetProperty( iProp );
            o = pMsg;

        }while( 0 );

        return ret;
    }

    gint32 CopyProp( gint32 iProp, gint32 iSrcProp )
    {
        gint32 ret = 0;
        do{
            auto pCfg = CFGDB2( m_pCfg );
            const Variant* p =
                pCfg->GetPropertyPtr( iSrcProp );
            if( p == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            Variant& odst =
                pCfg->GetProperty( iProp );
            odst = *p;

        }while( 0 );

        return ret;
    }

    gint32 CopyProp(
        gint32 iProp, gint32 iSrcProp, const CObjBase* pSrcCfg )
    {
        if( pSrcCfg == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        if( pSrcCfg->GetClsid() !=
            clsid( CConfigDb2 ) )
        {
            CCfgDbOpener< CObjBase > a( pSrcCfg );

            BufPtr pBuf( true );
            ret = a.GetProperty( iSrcProp, *pBuf);

            if( ERROR( ret ) )
                return ret;

            return SetProperty( iProp, pBuf );
        }
        do{
            auto psrc = CCFGDB2( pSrcCfg );
            Variant p;
            ret = psrc->GetProperty( iSrcProp, p );
            if( ERROR( ret ) )
                break;

            auto pdst = CFGDB2( m_pCfg );
            ret = pdst->SetProperty( iProp, p );

        }while( 0 );
        return ret;
    }

    inline gint32 CopyProp(
        gint32 iProp, const CObjBase* pSrcCfg )
    {
        return CopyProp( iProp, iProp, pSrcCfg );
    }

    gint32 MoveProp( gint32 iProp, CObjBase* pSrcCfg )
    {
        gint32 ret = CopyProp( iProp, pSrcCfg );
        if( ERROR( ret ) )
            return ret;

        ret = pSrcCfg->RemoveProperty( iProp );
        return ret;
    }

    gint32 SwapProp( gint32 iProp1, gint32 iProp2 )
    {
        auto pdb2 = CFGDB2( m_pCfg );
        Variant* p1 =
            pdb2->GetPropertyPtr( iProp1 );
        if( p1 == nullptr )
            return -ENOENT;;

        Variant* p2 =
            pdb2->GetPropertyPtr( iProp2 );
        if( p2 == nullptr )
            return -ENOENT;

        Variant o( *p1 );
        pdb2->SetProperty( iProp1, *p2 );
        pdb2->SetProperty( iProp2, o );
        return STATUS_SUCCESS;
    }


    inline gint32 GetIntPtr( gint32 iProp, guint32*& val ) const
    {
        intptr_t val1;
        auto pdb2 = CCFGDB2( m_pConstCfg );
        const Variant* p =
            pdb2->GetPropertyPtr( iProp );

        if( p == nullptr )
            return -ENOENT;

        val1 = *p;
        val = ( guint32* )val1;
        return 0;
    }

    inline gint32 SetIntPtr( gint32 iProp, guint32* val )
    {
        auto pdb2 = CFGDB2( m_pCfg );
        Variant& o = pdb2->GetProperty( iProp );
        o = ( intptr_t )val;
        return STATUS_SUCCESS;
    }
    inline gint32 GetShortProp( gint32 iProp, guint16& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    inline gint32 SetShortProp( gint32 iProp, guint16 val )
    {
        return SetPrimProp( iProp, val );
    }
    inline gint32 GetIntProp( gint32 iProp, guint32& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    inline gint32 SetIntProp( gint32 iProp, guint32 val )
    {
        return SetPrimProp( iProp, val );
    }
    inline gint32 GetBoolProp( gint32 iProp, bool& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    inline gint32 SetBoolProp( gint32 iProp, bool val )
    {
        return SetPrimProp( iProp, val );
    }
    inline gint32 GetByteProp( gint32 iProp, guint8& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    inline gint32 SetByteProp( gint32 iProp, guint8 val )
    {
        return SetPrimProp( iProp, val );
    }
    inline gint32 GetFloatProp( gint32 iProp, float& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    inline gint32 SetFloatProp( gint32 iProp, float val )
    {
        return SetPrimProp( iProp, val );
    }
    inline gint32 GetQwordProp( gint32 iProp, guint64& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    inline gint32 SetQwordProp( gint32 iProp, guint64 val )
    {
        return SetPrimProp( iProp, val );
    }
    inline gint32 GetDoubleProp( gint32 iProp, double& val ) const
    {
        return GetPrimProp( iProp, val );
    }
    inline gint32 SetDoubleProp( gint32 iProp, double val )
    {
        return SetPrimProp( iProp, val );
    }

    gint32 IsEqualProp( gint32 iProp, const CObjBase* pObj )
    {
        gint32 ret = 0;

        do{
            if( pObj == nullptr )
            {
                ret = -EINVAL;
                break;
            }
            if( m_pConstCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            Variant  oVar1, oVar2;
            ret = pObj->GetProperty( iProp, oVar1 );
            if( ERROR( ret ) )
                break;

            ret = this->GetProperty( iProp, oVar2 );
            if( ERROR( ret ) )
                break;

            if( oVar1 == oVar2 )
                break;

            ret = ERROR_FALSE;

        }while( 0 );
        return ret;
    }

    template< class U, typename U1=
        class std::enable_if< std::is_base_of< CObjBase, U >::value, U >::type  >
    gint32 GetPointer( gint32 iProp, U*& pObj ) const
    {
        gint32 ret = 0;

        do{
            ObjPtr objPtr;

            ret = GetObjPtr( iProp, objPtr );

            if( ERROR( ret ) )
                break;

            pObj = objPtr;

        }while( 0 );

        return ret;
    }

    template< class U, typename U1=
        class std::enable_if< std::is_base_of< CObjBase, U >::value, U >::type  >
    gint32 SetPointer( gint32 iProp, U* pObj )
    {
        gint32 ret = 0;

        do{
            ObjPtr objPtr( pObj );
            ret = SetObjPtr( iProp, objPtr );

        }while( 0 );

        return ret;
    }

    template< typename PrimType >
    gint32 IsEqual( gint32 iProp, const PrimType& val )
    {
        gint32 ret = 0;
        do{
            if( m_pConstCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            auto pdb2 = CCFGDB2( m_pConstCfg );
            const Variant* p =
                pdb2->GetPropertyPtr( iProp );
            if( p == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            if( val == ( const PrimType& )*p )
                break;

            ret = ERROR_FALSE;

        }while( 0 );
        return ret;
    }
};

template< typename T >
class CCfgOpenerT : public CCfgDbOpener< T >
{
    protected:
    CfgPtr m_pNewCfg;

    public:

    typedef CCfgDbOpener< T > super;
    using super::m_pCfg;
    using super:: m_pConstCfg;

    CCfgOpenerT( T* pCfg )
        :CCfgDbOpener< T >( pCfg )
    {

    }

    CCfgOpenerT( const T* pCfg )
        :CCfgDbOpener< T >( pCfg )
    {

    }

    CCfgOpenerT() :
        CCfgDbOpener< T >( ( T* )nullptr ),
        m_pNewCfg( true )
    {
        m_pCfg = m_pNewCfg;
        m_pConstCfg = m_pNewCfg;
    }

    CfgPtr& GetCfg()
    {
        return m_pNewCfg;
    }

    const CfgPtr& GetCfg() const
    {
        return m_pNewCfg;
    }

    void Clear()
    {
        if( !m_pNewCfg.IsEmpty() )
        {
            m_pNewCfg->RemoveAll();
        }
    }

    gint32 RemoveProperty( gint32 iPropId )
    {
        if( m_pCfg == nullptr )
            return -EFAULT;

        return m_pCfg->RemoveProperty( iPropId );
    }

    bool exist( gint32 iPropId ) const
    {
        if( m_pConstCfg != nullptr &&
            m_pConstCfg->GetClsid() == clsid( CConfigDb2 ) )
        {
            return static_cast< const IConfigDb* >
                ( m_pConstCfg )->exist( iPropId );
        }

        std::vector<gint32> vecProps;
        gint32 ret = m_pConstCfg->EnumProperties( vecProps );
        if( ERROR( ret ) )
            return false;

        for( auto i : vecProps )
        {
            if( i == iPropId )
                return true;
        }
        return false;
    }

};

typedef CCfgOpenerT< IConfigDb > CCfgOpenerBase;
typedef CCfgOpenerT< CObjBase > CCfgOpenerObj;

class CCfgOpener : public CCfgOpenerBase
{
    public:
    typedef CCfgOpenerBase super;

    CCfgOpener( IConfigDb* pCfg )
        :CCfgOpenerBase( pCfg )
    {}

    CCfgOpener( const IConfigDb* pCfg )
        :CCfgOpenerBase( pCfg )
    {}

    CCfgOpener() :
        CCfgOpenerBase()
    {}

    CCfgOpener( CCfgOpener& rhs ):
        CCfgOpenerBase(
            ( IConfigDb* )rhs.m_pNewCfg )
    { m_pNewCfg = rhs.m_pNewCfg; }

    gint32 Serialize( CBuffer& oBuf ) const
    {
        if( m_pNewCfg.IsEmpty() )
            return -EFAULT;
        return m_pNewCfg->Serialize( oBuf );
    }

    gint32 Serialize( BufPtr& pBuf ) const
    {
        if( pBuf.IsEmpty() ||
            m_pNewCfg.IsEmpty() )
            return -EFAULT;
        return m_pNewCfg->Serialize( *pBuf );
    }

    gint32 Deserialize( BufPtr& pBuf )
    {
        if( pBuf.IsEmpty() ||
            m_pNewCfg.IsEmpty() )
            return -EFAULT;
        return m_pNewCfg->Deserialize( *pBuf );
    }

    using CCfgOpenerBase::IsEqualProp;
    gint32 IsEqualProp( gint32 iProp, const IConfigDb* pObj )
    {
        gint32 ret = 0;

        do{
            if( pObj == nullptr )
            {
                ret = -EINVAL;
                break;
            }
            if( m_pConstCfg == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            auto psrc = CCFGDB2( pObj );
            auto pdst = CCFGDB2( m_pConstCfg );
            ret = STATUS_SUCCESS;

            const Variant* p1 =
                psrc->GetPropertyPtr( iProp );
            if( p1 == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            const Variant* p2 =
                pdst->GetPropertyPtr( iProp );
            if( p2 == nullptr )
            {
                ret = -ENOENT;
                break;
            }
            if( *p1 != *p2 )
                ret = ERROR_FALSE;

        }while( 0 );
        return ret;
    }
};

class CParamList : public CCfgOpener
{
    void InitParamList()
    {
        if( !GetCfg()->exist( propParamCount ) )
            SetIntProp( propParamCount, 0 );
    }

    protected:
    gint32 SetCount( gint32 iPos )
    {
        if( iPos < 0 )
            return -EINVAL;

        if( GetCfg().IsEmpty() )
            return -EFAULT;

        gint32 ret = SetIntProp( propParamCount,
            ( guint32& )iPos );

        if( ERROR( ret ) )
        {
            return ret;
        }
        return 0;
    }

    public:
    typedef CCfgOpener  super;

    gint32 GetCount()
    {
        gint32 ret = 0;
        gint32 iPos = 0;

        if( GetCfg().IsEmpty() )
            return -EFAULT;

        ret = GetIntProp(
            propParamCount, ( guint32& )iPos );

        if( ERROR( ret ) )
        {
            return ret;
        }
        return iPos;
    }

    CParamList()
        :CCfgOpener()
    {
        // this is for push/pop to a new cfg
        if( GetCfg().IsEmpty() )
            return;

        SetIntProp( propParamCount, 0 );
    }

    ~CParamList()
    {
        // we cannot remove the propParamCount
        // because the m_pCfg will be passed on to
        // other users
    }

    CParamList( IConfigDb* pCfg )
    {
        // this is for pop an existing cfg you
        // should make sure the properties are
        // strictly numbered from 0~(size-1) or an
        // earlier cfg generated from this class
        Reset( pCfg );
    }

    CParamList( CfgPtr& pCfg )
        : CParamList( ( IConfigDb* )pCfg )
    {}

    CParamList( const CParamList& rhs )
        : CParamList()
    {
        // copy constructor, added for sip
        if( rhs.IsEmpty() )
            return;
        CfgPtr pCfg = rhs.GetCfg();                                     
        Append( pCfg );
    } 

    gint32 Reset( IConfigDb* pCfg = nullptr )
    {
        if( pCfg != nullptr )
        {
            m_pNewCfg = pCfg;
        }
        else
        {
            gint32 ret = m_pNewCfg.NewObj();
            if( ERROR( ret ) )
                return ret;
        }

        m_pCfg = m_pNewCfg;
        m_pConstCfg = m_pNewCfg;
        if( !GetCfg().IsEmpty() )
            InitParamList();

        return 0;
    }

    gint32 CopyParams(
        const IConfigDb* pCfg )
    {
        CCfgOpener oCfg( pCfg );
        gint32 ret = 0;

        do{
            guint32 dwSize = 0;
            ret = oCfg.GetIntProp(
                propParamCount, dwSize );

            if( ERROR( ret ) )
                break;

            if( dwSize > 1024 * 16 )
            {
                ret = -ERANGE;
                break;
            }
            for( guint32 i = 0; i < dwSize; ++i )
            {
                ret = CopyProp( i, pCfg );
                if( ERROR( ret ) )
                    break;
            }
            SetIntProp( propParamCount, dwSize );

        }while ( 0 );
        
        return ret;
    }

    gint32 Append( IConfigDb* pCfg )
    {
        if( pCfg == nullptr )
            return -EINVAL;

        std::vector< gint32 > vecProps;
        gint32 ret = pCfg->EnumProperties( vecProps );
        if( ERROR( ret ) )
            return ret;

        for( auto iProp : vecProps )
        {
            ret = CopyProp( iProp, pCfg );
            if( ERROR( ret ) )
                break;
        }
        return ret;
    }

    gint32 ClearParams()
    {
        guint32 dwSize = 0;

        gint32 ret = GetSize( dwSize );
        if( ERROR( ret ) )
            return ret;

        for( guint32 i = 0; i < dwSize; ++i )
        {
            RemoveProperty( i );
        }
        SetIntProp( propParamCount, 0 );

        return 0;
    }

    inline gint32 GetSize(
        guint32 &dwSize ) const
    {
        return GetIntProp(
            propParamCount, dwSize );
    }

    gint32 GetType(
        gint32 iPos, gint32& iType ) const
    {
        gint32 ret = GetCfg()->
            GetPropertyType( iPos, iType );

        return ret;
    }

    gint32 IncDecPos( bool bInc )
    {
        gint32 ret = 0;
        gint32 iPos = 0;

        if( GetCfg().IsEmpty() )
            return -EFAULT;

        if( !GetCfg()->exist( propParamCount ) )
        {
            ret = SetIntProp( propParamCount, 0 );
            if( ERROR( ret ) )
                return ret;
        }

        ret = GetIntProp(
            propParamCount, ( guint32& )iPos );

        if( ERROR( ret ) )
        {
            return ret;
        }
        if( bInc )
        {
            ++iPos;
        }
        else 
        {
            if( iPos > 0 )
                --iPos;
        }
        SetIntProp( propParamCount, iPos );
        return iPos;
    }

    template< typename T,
        typename filter=typename std::enable_if<
            !std::is_pointer<T>::value, T>::type >
    gint32 Push( const T& val  )
    {
        gint32 ret = 0;
        do{
            gint32 iPos = GetCount();
            if( ERROR( iPos ) )
            {
                ret = iPos;
                break;
            }

            auto pdb2 = CFGDB2(
                ( IConfigDb* )GetCfg() );
            Variant& o = pdb2->GetProperty( iPos );
            o = val;
            SetCount( iPos + 1 );

        }while( 0 );

        return ret;
    }

    template< typename T,
        typename filter=typename std::enable_if<
            std::is_base_of< CObjBase, T >::value,
            T>::type >
    gint32 Push( T* val  )
    {
        ObjPtr pObj( val );
        return Push( pObj );
    }

    gint32 Push( const char* val )
    {
        Variant o = val;
        return Push( o );
    }

    template< typename T >
    gint32 Pop( T& val )
    {
        gint32 ret = 0;
        do{
            gint32 iPos = ( gint32 )GetCount();
            if( ERROR( iPos ) )
            {
                ret = iPos;
                break;
            }

            if( iPos <= 0 )
            {
                ret = -ENOENT;
                break;
            }

            try{
                auto pdb2 = CFGDB2( GetCfg() );
                gint32 idx = iPos - 1;
                Variant* p = pdb2->GetPropertyPtr( idx );
                if( p == nullptr )
                    break;

                gint32 iType =
                    rpcf::GetTypeId((T*)nullptr );
                if( iType != p->GetTypeId() )
                {
                    ret = -EINVAL;
                    break;
                }
                val = ( T& )*p;
                pdb2->RemoveProperty( idx );
                SetCount( idx );
            }
            catch( std::invalid_argument& e )
            {
                ret = -EINVAL;
            }

        }while( 0 );

        return ret;
    }

    bool IsEmpty() const
    {
        guint32 dwSize = 0;
        gint32 ret = GetSize( dwSize );

        if( ERROR( ret ) )
            return true;

        return ( dwSize == 0 );
    }
};

#define CFGDB_MAX_SIZE  ( 16 * 1024 * 1024 )
#define CFGDB_MAX_ITEM  1024

}
