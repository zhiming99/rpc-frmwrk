/*
 * =====================================================================================
 *
 *       Filename:  ifhelper.h
 *
 *    Description:  declarations and implementations
 *                  template classes or utilities for building
 *                  proxy/server interface
 *
 *        Version:  1.0
 *        Created:  08/25/2018 06:01:25 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
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
#include "proxy.h"

namespace rpcf
{

// remove the reference or const modifiers from
// the type list
#define ValType( _T ) typename std::remove_cv< typename std::remove_pointer< typename std::decay< _T >::type >::type>::type

template< typename T >
inline guint32 GetTypeSize( T* )
{ return sizeof( T ); }

// cast to stdstr temporary
template< typename T,
    typename Allowed = typename std::enable_if<
        std::is_same< stdstr, T >::value, T >::type >
stdstr CastTo( BufPtr& pBuf )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        return stdstr("");
    if( pBuf->GetExDataType() != typeString )
        throw std::invalid_argument(
            "error cast to string" );
    return stdstr( pBuf->ptr() );
}

// cast to prime type reference except char* or const
// char* or stdstr
template< typename T,
    typename T1= typename std::enable_if<
        !std::is_same< DBusMessage*, T >::value, T>::type,
    typename Allowed = typename std::enable_if<
        !std::is_base_of< IAutoPtr, ValType( T ) >::value && 
        !std::is_base_of< CObjBase, ValType( T ) >::value && 
        !std::is_same< stdstr, DecType( T ) >::value &&
        !std::is_same< char*, DecType( T ) >::value &&
        !std::is_same< const char*, DecType( T ) >::value, T >::type >
T& CastTo( BufPtr& pBuf )
{
    if( pBuf.IsEmpty() || pBuf->empty() )
        throw std::invalid_argument(
            "error cast to prime types" );

    guint32 dwSize = GetTypeSize( ( T* )0 );
    if( dwSize > pBuf->size() )
        throw std::invalid_argument(
            "error cast to prime types" );

    return ( T& )*pBuf;
}

// cast to CObjBase reference
template< typename T,
    typename T1= typename std::enable_if<
        !std::is_same< DBusMessage*, T >::value, T>::type,
    typename T2= typename std::enable_if<
        std::is_base_of< CObjBase, T >::value, T >::type,
    typename T3=T >
T& CastTo( BufPtr& pBuf )
{
    if( pBuf.IsEmpty() || pBuf->empty() ||
        pBuf->GetExDataType() != typeObj )
        throw std::invalid_argument(
            "error cast to ObjPtr" );
    CObjBase* p = (ObjPtr&)*pBuf;
    T* pT = static_cast< T* >( p );
    return *pT;
}

// cast to ObjPtr or DMsgPtr reference
template< typename T,
    typename Allowed= typename std::enable_if<
        std::is_same< DMsgPtr, T >::value ||
        std::is_same< ObjPtr, T >::value, T >::type,
    typename T2 = typename std::enable_if<
        !std::is_same< BufPtr, T >::value, T >::type,
        typename T3=T,
        typename T4=T
        >
T& CastTo( BufPtr& pBuf )
{
    if( pBuf.IsEmpty() || pBuf->empty() ||
        ( pBuf->GetExDataType() != typeObj &&
        pBuf->GetExDataType() != typeDMsg ) )
        throw std::invalid_argument(
            "error cast to ObjPtr or DMsgPtr" );
    return ( T& )*pBuf;
}

// cast to IfPtr, CfgPtr, IrpPtr or other ptrs
template<typename T,
    typename T1= typename std::enable_if<
        std::is_base_of< IAutoPtr, T >::value, T >::type,
    typename T2= typename std::enable_if<
        !std::is_same< DMsgPtr, T >::value &&
        !std::is_same< BufPtr, T >::value &&
        !std::is_same< ObjPtr, T >::value, T >::type >
T CastTo( BufPtr& pBuf )
{
    if( pBuf->empty() )
    { T1 o; return o; }
    ObjPtr* ppObj = ( ObjPtr* )pBuf->ptr();
    T1 oT( *ppObj );
    return oT;
}

#define DFirst DecType( First )
#define DSecond DecType( Second )
#define DTypes DecType( Types )

#define ValTypeFirst ValType( First )

// cast to plain object*
template< typename First,
    typename T2 = typename std::enable_if<
        !std::is_same< DBusMessage*, First >::value,
         First >::type,
    typename Allowed = typename std::enable_if<
        !std::is_base_of< CObjBase, ValTypeFirst >::value &&
        !std::is_base_of< IAutoPtr, ValTypeFirst >::value &&
        !std::is_same< stdstr, ValTypeFirst >::value &&
        !std::is_same< char, ValTypeFirst >::value, First >
        ::type >
ValTypeFirst* CastTo( const BufPtr& pCBuf )
{
    BufPtr& pBuf = const_cast<BufPtr&>( pCBuf );
    if( pBuf->empty() )
    { return nullptr; }
    if( pBuf->GetExDataType() == typeNone )
        throw std::invalid_argument(
            "error invalid type cast" );
    ValTypeFirst& val = *pBuf;
    return &val;
}

// cast to char* or const char*
template< typename First,
    typename Allowed = typename std::enable_if<
        std::is_same< char*, DFirst >::value ||
        std::is_same< const char*, DFirst >::value, First >::type,
    typename T3=Allowed >
DFirst CastTo( BufPtr& pCBuf )
{
    BufPtr& pBuf = const_cast<BufPtr&>( pCBuf );
    if( pBuf->empty() )
    { return nullptr; }
    DFirst val = pBuf->ptr();
    return val;
}

// cast to DBusMessage*
template< typename First,
    typename Allowed = typename std::enable_if<
        std::is_same< DBusMessage*, First >::value,
        First >::type >
DBusMessage* CastTo( BufPtr& pCBuf )
{
    BufPtr& pBuf = const_cast<BufPtr&>( pCBuf );
    if( pBuf->empty() )
    { return nullptr; }
    DMsgPtr& pMsg = *pBuf;
    return ( First )pMsg;
}

// cast to CObjBase*
template< typename First,
    typename T2 = typename std::enable_if<
        !std::is_same< DBusMessage*, First >::value,
         First >::type,
    typename Allowed = typename std::enable_if<
        std::is_base_of< CObjBase, ValTypeFirst >::value &&
        std::is_pointer< First >::value, First >::type,
    typename T4 = typename std::enable_if<
        !std::is_same< CBuffer*, First >::value,
         First >::type,
    typename T5=Allowed >
DFirst CastTo( BufPtr& pCBuf )
{
    BufPtr& pBuf = const_cast<BufPtr&>( pCBuf );
    if( pBuf->empty() )
    { return nullptr; }
    ObjPtr& pObj = *pBuf;
    DFirst val = pObj;
    return val;
}

// cast to CBuffer*
template< typename First,
    typename Allowed = typename std::enable_if<
        std::is_same< CBuffer, ValTypeFirst >::value &&
        std::is_pointer< First >::value, First >::type  >
CBuffer* CastTo( BufPtr& pCBuf )
{
    BufPtr& pBuf = const_cast<BufPtr&>( pCBuf );
    if( pBuf.IsEmpty() )
        return nullptr;

    return ( CBuffer* )pBuf;
}

// cast to BufPtr reference
template< typename T,
    typename Allowed= typename std::enable_if<
        std::is_same< BufPtr, T >::value , T >::type  >
BufPtr& CastTo( BufPtr& pBuf )
{ return pBuf; }

template< typename First >
auto VecToTupleHelper( std::vector< BufPtr >& vec ) -> std::tuple<DFirst>
{
    BufPtr& i = vec.back();
    DFirst oVal( CastTo< DFirst >( i ) );
    return std::tuple< DFirst >( oVal );
}

// generate a tuple from the vector with the
// proper type set for each element
template< typename First, typename Second, typename ...Types >
auto VecToTupleHelper( std::vector< BufPtr >& vec ) -> std::tuple< DFirst, DSecond, DTypes... >
{
    BufPtr& i = vec[ vec.size() - sizeof...( Types ) - 2 ];
    DFirst oVal( CastTo< DFirst >( i ) );

    // to assign right type to the element in the
    // tuple
    return std::tuple_cat(
        std::tuple< DFirst >( oVal ), 
        VecToTupleHelper< Second, Types... >( vec ) );
}

template< typename ...Types >
auto VecToTuple( std::vector< BufPtr >& vec ) -> std::tuple< DTypes... >
{
    return VecToTupleHelper< Types...>( vec );
}

template<>
auto VecToTuple<>( std::vector< BufPtr >& vec ) -> std::tuple<>; 

// parameter pack generator
template< int ... >
struct NumberSequence
{};

template<>
struct NumberSequence<>
{};

template< int N, int ...S >
struct GenSequence : GenSequence< N - 1, N - 1, S... > {};

template< int ...S >
struct GenSequence< 0, S... >
{
    typedef NumberSequence< S... > type;
};

// callback definitions for sync/async interface
// calls
template< class T >
class CGenericCallback :
    public T
{

    public:
    typedef T super; 

    CGenericCallback( const IConfigDb* pCfg = nullptr )
        : T( pCfg )
    {;}

    gint32 NormalizeParams( bool bResp, IConfigDb* pResp,
        std::vector< BufPtr >& oRes )
    {
        CParamList oResp( pResp );
        gint32 ret = 0;
        do{
            if( bResp )
            {
                gint32 iRet = 0;
                ret = oResp.GetIntProp( propReturnValue,
                    ( guint32& )iRet );

                if( ERROR( ret ) )
                    break;

                BufPtr pBuf( true );

                *pBuf = ( guint32 )iRet;
                oRes.push_back( pBuf );
            }

            guint32 dwSize = 0;
            ret = oResp.GetSize( dwSize );
            if( ERROR( ret )  )
                break;

            // NOTE: if iRet is an error code, the
            // dwSize could be zero, and we need
            // to handle this
            for( guint32 i = 0; i < dwSize; i++ )
            {
                BufPtr pBuf;
                ret = oResp.GetProperty( i, pBuf );
                if( ERROR( ret ) )
                    break;

                oRes.push_back( pBuf );
            }
        }while( 0 );

        if( ERROR( ret ) )
            oRes.clear();

        return ret;
    }
};

class CTaskWrapper :
    public CGenericCallback< CTasklet >
{
    TaskletPtr m_pTask;
    public:
    typedef CGenericCallback< CTasklet > super;

    CTaskWrapper()
    {
        SetClassId( clsid( CTaskWrapper ) );
    }

    gint32 SetTask( IEventSink* pTask )
    {
        if( pTask == nullptr )
            return 0;

        m_pTask = ObjPtr( pTask );
        return 0;
    }

    void TransferParams()
    {
        CCfgOpener oCfg( ( IConfigDb* )
            m_pTask->GetConfig() );

        oCfg.MoveProp( propRespPtr, this );
        oCfg.MoveProp( propMsgPtr, this );
        oCfg.MoveProp( propReqPtr, this );
    }

    virtual gint32 OnEvent( EnumEventId iEvent,
        LONGWORD dwParam1 = 0,
        LONGWORD dwParam2 = 0,
        LONGWORD* pData = NULL  )
    {
        if( m_pTask.IsEmpty() )
            return 0;

        TransferParams();
        return m_pTask->OnEvent( iEvent,
            dwParam1, dwParam2, pData );
    }

    virtual gint32 operator()(
        guint32 dwContext )
    {
        if( m_pTask.IsEmpty() )
            return 0;

        TransferParams();
        return m_pTask->operator()( dwContext );
    }
};

template<typename T,
    typename T2 = typename std::enable_if<
        std::is_base_of< IAutoPtr, T >::value, T >::type,
    typename T3=T2 >
BufPtr PackageTo( const T& pObj )
{
    BufPtr pBuf( true );
    *pBuf = ObjPtr( pObj );
    return pBuf;
}

template< typename T,
    typename T2 = typename std::enable_if<
        !std::is_base_of< IAutoPtr, T >::value, T >::type >
BufPtr PackageTo( const T& i )
{
    BufPtr pBuf( true );
    *pBuf = i;
    return pBuf;
}

template<typename T,
    typename T2 = typename std::enable_if<
        !std::is_same<DBusMessage, T>::value, T >::type,
    typename T1 = typename std::enable_if<
        std::is_base_of<CObjBase, T>::value, T >::type >
BufPtr PackageTo( T* pObj )
{
    BufPtr pBuf( true );
    *pBuf = ObjPtr( pObj );
    return pBuf;
}

template<typename T,
    typename T2 = typename std::enable_if<
        std::is_same<DBusMessage, T>::value, T >::type,
    typename T1 = T2,
    typename T3 = T2 >
BufPtr PackageTo( T* pObj )
{
    BufPtr pBuf( true );
    *pBuf = DMsgPtr( pObj );
    return pBuf;
}

template< typename T >
BufPtr PackageTo( const T* pObj )
{
    BufPtr pBuf( true );
    *pBuf = *pObj;
    return pBuf;
}


template<>
CObjBase& CastTo< CObjBase >( BufPtr& i );

template<>
BufPtr& CastTo< BufPtr >( BufPtr& i );

template<>
BufPtr PackageTo< DMsgPtr >( const DMsgPtr& pMsg );

template<>
BufPtr PackageTo< stdstr >( const stdstr& str );

template<>
BufPtr PackageTo< BufPtr >( const BufPtr& pBuf );

template<>
BufPtr PackageTo< ObjPtr >( const ObjPtr& pObj );

template<>
BufPtr PackageTo< CBuffer >( CBuffer* pObj );

}

#include <stdarg.h>

namespace rpcf
{

template< int N >
struct _DummyClass_
{
    static void PackExp( std::vector< BufPtr >& vec, ... )
    { 
        va_list va;
        CBuffer* pBuf = nullptr;
        // NOTE: this is an approach to make both
        // X86 and ARM happy, since the evaluation
        // of parameters of the parameter pack is
        // done in the opposite order on X86 to
        // that on the ARM.
        va_start( va, vec );
        for( int i = 0; i < N; ++i )
        {
            pBuf = va_arg( va, CBuffer* );
            vec.push_back( BufPtr( pBuf ) );
        }
        va_end( va );
        return;
    }
};

template< typename...Types >
inline void PackParams(
    std::vector< BufPtr >& vec, Types&&...args )
{
    // note that the last arg is inserted first
    _DummyClass_< sizeof...( args )>::PackExp(
        vec, ( ( CBuffer* ) PackageTo( args ) ) ... );
}

// proxy related classes
template< typename ...Args>
class CMethodProxy :
    public CObjBase
{
    //---------
    std::string m_strMethod;
    bool m_bNonDBus;
    public:
    CMethodProxy( bool bNonDBus,
        const std::string& strMethod )
        : m_strMethod( strMethod ),
        m_bNonDBus( bNonDBus )
    { SetClassId( clsid( CMethodProxy ) ); }

    gint32 operator()(
        CInterfaceProxy* pIf,
        IEventSink* pCallback,
        guint64& qwIoTaskId,
        Args... args )
    {
        if( pIf == nullptr )
            return -EINVAL;

        std::vector< BufPtr > vec;
        if( sizeof...( Args ) )
            PackParams( vec, args ... );

        return pIf->SendProxyReq( pCallback,
            m_bNonDBus, m_strMethod, vec, qwIoTaskId );
    }
};

#define BEGIN_PROXY_MAP_COMMON( bNonDBus, _iIfId_, _pIf ) \
do{ \
    CCfgOpenerObj oCfg( _pIf ); \
    std::string strIfName; \
    oCfg.GetStrProp( propIfName, strIfName ); \
    strIfName = IF_NAME_FROM_DBUS( strIfName ); \
    EnumClsid tempIid = CoGetIidFromIfName( strIfName, "p" ); \
    if( tempIid == clsid( Invalid ) ) \
        CoAddIidName( strIfName, _iIfId_, "p" ); \
    PROXY_MAP _oAnon_; \
    PROXY_MAP* _pMapProxies_ = &_oAnon_; \
    bool _bNonBus_ = bNonDBus; \
    UNREFERENCED( _bNonBus_ ); \
    PROXY_MAP* _pMap_ = nullptr; \
    do{ \
        _pMap_ = _pIf->GetProxyMap( _iIfId_ ); \
        if( _pMap_ != nullptr ) \
            _pMapProxies_ = _pMap_; \
    }while( 0 )

// construction of the major interface proxy map 
#define BEGIN_PROXY_MAP( bNonDBus ) \
do{ EnumClsid _iIfId_ = GetClsid();\
    CInterfaceProxy* _pIf = this; \
    BEGIN_PROXY_MAP_COMMON( bNonDBus, _iIfId_, _pIf );\

template< typename...Args >
inline ObjPtr NewMethodProxy( bool bNonDBus,
    const std::string strName )
{
    ObjPtr pObj = new CMethodProxy< DecType( Args )...>
        ( bNonDBus, strName );

    pObj->DecRef();
    return pObj;
}

// the macro is to accomodate the case of zero
// argument
#define VA_ARGS(...) , ##__VA_ARGS__
#define ADD_PROXY_METHOD( MethodName, ... ) \
do{ \
    std::string strName = MethodName; \
    if( _pMapProxies_->size() > 0 && \
        _pMapProxies_->find( strName ) != \
            _pMapProxies_->end() ) \
        break; \
    ObjPtr pObj = NewMethodProxy< __VA_ARGS__ >( \
        _bNonBus_, strName );\
    ( *_pMapProxies_ )[ strName ] = pObj; \
}while( 0 )
    
#define ADD_USER_PROXY_METHOD( MethodName, ... ) \
do{ \
    std::string strMethod = USER_METHOD( MethodName );\
    ADD_PROXY_METHOD( strMethod, __VA_ARGS__ ); \
}while( 0 )

#define END_PROXY_MAP \
        do{ \
            if( _pMap_ == nullptr && \
                _pMapProxies_->size() > 0 ) \
                _pIf->SetProxyMap( *_pMapProxies_, _iIfId_ ); \
        }while( 0 ); \
    }while( 0 ); \
}while( 0 )

// construction of the normal interface proxy map 
#define BEGIN_IFPROXY_MAP_COMMON( _InterfName_, bNonDBus, _pIf ) \
do{ \
    EnumClsid _iIfId_ = iid( _InterfName_ ); \
    std::string strIfName = #_InterfName_; \
    EnumClsid tempIid = CoGetIidFromIfName( strIfName, "p" ); \
    if( tempIid == clsid( Invalid ) ) \
        CoAddIidName( strIfName, _iIfId_, "p" ); \
    PROXY_MAP _oAnon_; \
    PROXY_MAP* _pMapProxies_ = &_oAnon_; \
    bool _bNonBus_ = bNonDBus; \
    UNREFERENCED( _bNonBus_ ); \
    PROXY_MAP* _pMap_ = nullptr; \
    do{ \
        _pMap_ = _pIf->GetProxyMap( _iIfId_ ); \
        if( _pMap_ != nullptr ) \
            _pMapProxies_ = _pMap_; \
    }while( 0 )

#define BEGIN_IFPROXY_MAP( _InterfName_, bNonDBus ) \
do{ CInterfaceProxy* _pIf = this;\
    BEGIN_IFPROXY_MAP_COMMON( _InterfName_, bNonDBus, _pIf );

#define END_IFPROXY_MAP END_PROXY_MAP

// proxy's side helper
template< int N, typename ...S >
struct InputParamTypes;

template< typename T0, typename ...S >
struct InputParamTypes< 1, T0, S... >
{
    typedef std::tuple<T0> InputTypes;
};

template< typename T0, typename ...S >
struct InputParamTypes< 0, T0, S... >
{
    typedef std::tuple<> InputTypes;
};

template< typename ...S >
struct InputParamTypes< 0, S... >
{
    typedef std::tuple<> InputTypes;
};

template< int N, typename T0, typename...S>
struct InputParamTypes< N, T0, S...>
{
    private:

    typedef std::tuple< T0 > T1;
    typedef typename InputParamTypes< N-1, S...>::InputTypes T2;
    static T1 t1f()
    { return *( T1* )nullptr; }
    static T2 t2f()
    { return *( T2* )nullptr; }

    public:
    // a tuple type with merged parameter types
    typedef decltype( std::tuple_cat< T1, T2 >( t1f(), t2f() ) ) InputTypes;
};

template< typename ...InArgs >
struct GetProxyType;

template< typename ...InArgs >
struct GetProxyType< std::tuple< InArgs... > >
{
    typedef CMethodProxy< InArgs... > type;
};

template< int iNumInput, typename ClassName, typename ...Args>
class CMethodProxyEx;

template< int iNumInput, typename ClassName, typename ...Args>
struct CMethodProxyEx< iNumInput, gint32 (ClassName::*)( Args ...) >
{
    public:
    typedef gint32 ( ClassName::* FuncType)( Args... ) ;
    CMethodProxyEx( bool bNonDBus,
        const std::string& strMethod, FuncType pFunc, ObjPtr& pProxy )
    {
        using InTypes = typename InputParamTypes< iNumInput, DecType( Args )... >::InputTypes;
        using ProxyType = typename GetProxyType<InTypes>::type;
        pProxy = new ProxyType( bNonDBus, strMethod );
        pProxy->DecRef();
    }
};

template < int iNumInput, typename C, typename ...Args>
inline gint32 NewMethodProxyEx(
    ObjPtr& pProxy, bool bNonDBus,
    const std::string& strMethod,
    gint32(C::*f)(Args ...),
    InputCount< iNumInput >* b )
{
    CMethodProxyEx< iNumInput, gint32 (C::*)(Args...)> a( bNonDBus, strMethod, f, pProxy );  
    if( pProxy.IsEmpty() )
        return -ENOMEM;
    return 0;
}

// NOTE: use this macro if you are ok if the method to
// have a fixed layout of the parameter list, that is,
// Input parameters come first and output parameters
// follow. No other parameters are allowed
//
// if you want to have different layout of parameter
// list, please use ADD_PROXY_METHOD and
// ADD_USER_PROXY_METHOD instead.
#define ADD_PROXY_METHOD_EX( iNumInput, _f, MethodName ) \
do{ \
    std::string strName = MethodName; \
    if( _pMapProxies_->size() > 0 && \
        _pMapProxies_->find( strName ) != \
            _pMapProxies_->end() ) \
        break; \
    ObjPtr pObj;\
    InputCount< iNumInput > *p = nullptr; \
    NewMethodProxyEx( \
          pObj, _bNonBus_, strName, &_f, p );\
    ( *_pMapProxies_ )[ strName ] = pObj; \
}while( 0 )
    
#define ADD_USER_PROXY_METHOD_EX( iNumInput, _f, MethodName ) \
do{ \
    std::string strMethod = USER_METHOD( MethodName );\
    ADD_PROXY_METHOD_EX( iNumInput, _f, strMethod ); \
}while( 0 )

// handler related classes
class CDelegateBase :
    public CGenericCallback< CTasklet >
{

    public:
    typedef CGenericCallback< CTasklet > super;

    CDelegateBase( const IConfigDb* pCfg = nullptr)
        : CGenericCallback< CTasklet >( pCfg ) 
    {;}

    virtual gint32 Delegate(
        CObjBase* pObj,
        IEventSink* pCallback,
        std::vector< BufPtr >& vecParams ) = 0;

    gint32 operator()( guint32 dwContext )
    {
        return 0;
    }

    gint32 operator()(
        CObjBase* pObj,
        IEventSink* pCallback,
        IConfigDb* pParams )
    {
        if( pCallback == nullptr ||
            pParams == nullptr ||
            pObj == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            CfgPtr pReq = pParams;
            if( pReq.IsEmpty()  )
            {
                ret = -EFAULT;
                break;
            }
            std::vector< BufPtr > vecRespParams;

            ret = NormalizeParams(
                false, pReq, vecRespParams );

            if( ERROR( ret ) )
                break;

            ret = Delegate( pObj, pCallback,
                vecRespParams );

        }while( 0 );

        return ret;
    }
};

template<typename ClassName, typename ...Args>
class CMethodServer :
    public CDelegateBase
{
    public:
    typedef gint32 ( ClassName::* FuncType)(
        IEventSink* pCallback, Args... ) ;

    protected:
    template< int ...S>
    gint32 CallUserFunc( ClassName* pObj,
        IEventSink* pCallback,
        std::tuple< typename std::decay<Args>::type... >& oTuple,
        NumberSequence< S... > )
    {
        if( pObj == nullptr || m_pUserFunc == nullptr )
            return -EINVAL;

        gint32 ret = ( pObj->*m_pUserFunc )( pCallback,
            std::get< S >( oTuple )... );

        return ret;
    }

    //---------

    FuncType m_pUserFunc;

    public:
    typedef CDelegateBase super;
    CMethodServer( FuncType pFunc )
        :super()
    {
        m_pUserFunc = pFunc;
        SetClassId( clsid( CMethodServer ) );
    }

    gint32 Delegate( 
        CObjBase* pObj,
        IEventSink* pCallback,
        std::vector< BufPtr >& vecParams )
    {
        if( pObj == nullptr ||
            pCallback == nullptr )
            return -EINVAL;

        ClassName* pClass = dynamic_cast< ClassName* >( pObj );
        if( vecParams.size() != sizeof...( Args ) )
        {
            return -EINVAL;
        }

        std::vector< BufPtr > vecResp;
        for( auto& elem : vecParams )
            vecResp.push_back( elem );

        gint32 ret = 0;

        try{
            // std::tuple<typename std::decay<Args>::type...> oTuple =
            auto oTuple = VecToTuple< Args... >( vecResp );

            ret = CallUserFunc( pClass, pCallback, oTuple,
                typename GenSequence< sizeof...( Args ) >::type() );

        }
        catch( const std::invalid_argument& e )
        {
            ret = -EINVAL;
        }

        return ret;
    }
};

template < typename C, typename ...Types>
inline gint32 NewMethodServer( TaskletPtr& pDelegate,
    gint32(C::*f)(IEventSink*, Types ...) )
{
    if( f == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    try{
        // we cannot use NewObj for this
        // template object
        pDelegate = new CMethodServer< C, Types... >( f );
        pDelegate->DecRef();
    }
    catch( const std::bad_alloc& e )
    {
        ret = -ENOMEM;
    }
    catch( const std::runtime_error& e )
    {
        ret = -EFAULT;
    }
    catch( const std::invalid_argument& e )
    {
        ret = -EINVAL;
    }
    return ret;
}

template< typename ...Args >
struct InitTupleDefault;

template< typename T0, typename ...Args >
struct InitTupleDefault< std::tuple< T0, Args... > > :
    InitTupleDefault< std::tuple< Args... > >
{
    typedef InitTupleDefault< std::tuple< Args... > > super;
    InitTupleDefault( std::vector< BufPtr >& vecDefault )
        : super( vecDefault )
    {
        vecDefault.insert(
            vecDefault.begin(),
            GetDefault( ( T0* )nullptr ) );
    }
};
template<typename T0>
struct InitTupleDefault< std::tuple< T0 > >
{
    InitTupleDefault( std::vector< BufPtr >& vecDefault )
    {
        vecDefault.insert(
            vecDefault.begin(),
            GetDefault( ( T0* )nullptr ) );
    }
};
template<>
struct InitTupleDefault< std::tuple<> >
{
    InitTupleDefault( std::vector< BufPtr >& vecDefault )
    {}
};

template< int N, typename...S >
struct OutputParamTypes;

template< int N, typename T0, typename ...S>
struct OutputParamTypes< N, T0, S...> 
{
    using OutputTypes = typename OutputParamTypes< N - 1, S... >::OutputTypes;
};

template< typename T0, typename ...S >
struct OutputParamTypes< 1, T0, S... >
{
    typedef std::tuple< S...> OutputTypes;
};

template< typename T0, typename ... S>
struct OutputParamTypes< 0, T0, S...>
{
    typedef std::tuple< T0, S... > OutputTypes;
};

template< typename ... S>
struct OutputParamTypes< 0, S...>
{
    typedef std::tuple< S... > OutputTypes;
};

template< int iNumInput, typename ClassName, typename ...Args>
class CMethodServerEx; 

template< int iNumInput, typename ClassName, typename ...Args>
class CMethodServerEx< iNumInput, gint32 (ClassName::*)(IEventSink*, Args ...) > :
    public CMethodServer< ClassName, Args... >
{
    typedef gint32 ( ClassName::* FuncType)(
        IEventSink* pCallback, Args... ) ;

    public:
    typedef CMethodServer< ClassName, Args... > super;
    CMethodServerEx( FuncType pFunc )
        :super( pFunc )
    { this->SetClassId( clsid( CMethodServerEx ) ); }

    void TupleToVec2( std::tuple<>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence<> )
    {}

    template < int N >
    void TupleToVec2( std::tuple< DecType( Args )...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N > )
    {
        if( N < iNumInput )
            return;

        vec[ N - iNumInput ] = PackageTo< ValType( decltype( std::get< N >( oTuple ) ) ) >
            ( std::get< N >( oTuple ) );
    }

    template < int N, int M, int...S >
    void TupleToVec2( std::tuple< DecType( Args )...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N, M, S... > )
    {
        if( N >= iNumInput )
        {
            vec[ N - iNumInput ] = PackageTo< ValType( decltype( std::get< N >( oTuple ) ) ) >
                ( std::get< N >( oTuple ) );
        }
        TupleToVec2< M, S... >( oTuple, vec, NumberSequence<M, S...>() );
    }

    gint32 Delegate( 
        CObjBase* pObj,
        IEventSink* pCallback,
        std::vector< BufPtr >& vecParams )
    {
        if( pObj == nullptr ||
            pCallback == nullptr )
            return -EINVAL;

        ClassName* pClass = dynamic_cast< ClassName* >( pObj );
        // vecParams contains only the input parameters.
        // Args contains both the input and output parameters
        if( vecParams.size() > sizeof...( Args ) )
            return -EINVAL;

        if( vecParams.size() != iNumInput )
            return -EINVAL;

        using OutTupType = typename OutputParamTypes<
            iNumInput, DecType( Args )... >::OutputTypes;

        std::vector< BufPtr > vecResp;
        for( auto& elem : vecParams )
            vecResp.push_back( elem );

        std::vector< BufPtr > vecDefOut;

        gint32 ret = 0;

        try{
            // fill the output parameters with fake values
            InitTupleDefault< OutTupType > a( vecDefOut );
            vecResp.insert( vecResp.end(),
                vecDefOut.begin(), vecDefOut.end() );

            // the parameters are ready for the call
            std::tuple< DecType( Args )...> oTuple =
                VecToTuple< DecType( Args )... >( vecResp );

            ret = this->CallUserFunc( pClass, pCallback, oTuple,
                typename GenSequence< sizeof...( Args ) >::type() );

            if( ret == STATUS_PENDING )
                return ret;

            do{
                CParamList oParams;
                oParams[ propReturnValue ] = ret;

                // error, just return the error code
                // the caller will setup the response
                // package
                if( ERROR( ret ) )
                    break;

                if( sizeof...( Args ) > iNumInput )
                {
                    // fill the output values to the vector
                    vecResp.resize( sizeof...( Args ) - iNumInput );
                    TupleToVec2( oTuple, vecResp,
                        typename GenSequence< sizeof...( Args ) >::type() );

                    for( auto elem : vecResp )
                        oParams.Push( elem );
                }
                else
                {
                    // fine, no parameters for response
                }
                
                CTasklet* pTask =
                    static_cast< CTasklet* >( pCallback );

                // we are outside the task's context, let's
                // complete it 
                ObjPtr pObj;
                CCfgOpenerObj oTaskCfg( pTask );

                ret = oTaskCfg.GetObjPtr( propIfPtr, pObj );
                if( ERROR( ret ) )
                    break;

                CRpcServices* pIf = pObj;
                if( pIf == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                if( !pIf->IsServer() )
                    break;

                CInterfaceServer* pSvr = pObj;
                if( pTask->IsInProcess() )
                {
                    pSvr->SetResponse(
                        pCallback, oParams.GetCfg() );
                }
                else
                {
                    pSvr->OnServiceComplete(
                        oParams.GetCfg(), pCallback );
                }
            }while( 0 );
        }
        catch( const std::invalid_argument& e )
        {
            ret = -EINVAL;
        }


        return ret;
    }
};

template < int iNumInput, typename C, typename ...Types>
struct NewMethodServerEx;

template < int iNumInput, typename C, typename ...Types>
struct NewMethodServerEx< iNumInput, gint32(C::*)(IEventSink*, Types ...) >
{
    typedef gint32(C::*FuncType)(IEventSink*, Types ...);
    TaskletPtr pDelegate;
    NewMethodServerEx( FuncType f )
    {
        if( f == nullptr )
            return;

        gint32 ret = 0;
        try{
            pDelegate = new CMethodServerEx< iNumInput, FuncType >( f );
            pDelegate->DecRef();
        }
        catch( const std::bad_alloc& e )
        {
            ret = -ENOMEM;
        }
        catch( const std::runtime_error& e )
        {
            ret = -EFAULT;
        }
        catch( const std::invalid_argument& e )
        {
            ret = -EINVAL;
        }
        if( ERROR( ret ) )
        {
            std::string strMsg = DebugMsg( ret, \
                "Error alloc CMethodServerEx" ); \
            throw std::runtime_error( strMsg ); \
        }
    }
};

// construction of the major interface handler map 

#define BEGIN_HANDLER_MAP_IID( _iid_ ) \
do{\
    EnumClsid _iIfId_ = ( _iid_ );\
    std::string strIfName; \
    CCfgOpenerObj oCfg( this ); \
    oCfg.GetStrProp( propIfName, strIfName ); \
    strIfName = IF_NAME_FROM_DBUS( strIfName ); \
    std::string strSuffix = \
        IsServer() ? "s" : "p";\
    EnumClsid tempIid = CoGetIidFromIfName( \
        strIfName, strSuffix ); \
    if( tempIid == clsid( Invalid ) ) \
        CoAddIidName( strIfName, _iIfId_, strSuffix );\
    FUNC_MAP _oAnon_; \
    FUNC_MAP* _pCurMap_ = &_oAnon_; \
    FUNC_MAP* _pMap_ = nullptr; \
    do{ \
        _pMap_ = GetFuncMap( _iIfId_ ); \
        if( _pMap_ != nullptr ) \
            _pCurMap_ = _pMap_; \
    }while( 0 )

#define BEGIN_HANDLER_MAP \
    BEGIN_HANDLER_MAP_IID( GetClsid() )

#define END_HANDLER_MAP \
    do{ \
        if( _pMap_ == nullptr && \
            _pCurMap_->size() > 0 ) \
            SetFuncMap( *_pCurMap_, _iIfId_ ); \
    }while( 0 ); \
}while( 0 )

// we have some limitation on the service handler,
// that is, output parameter is not supported so
// far, because no one care about it. for the
// service handler's output, better attach it to
// the propResp property of the incoming
// "pCallback"
#define ADD_SERVICE_HANDLER( f, fname ) \
do{ \
    if( _pCurMap_->size() > 0 && \
        _pCurMap_->find( fname ) != _pCurMap_->end() ) \
        break; \
    TaskletPtr pTask; \
    gint32 ret = NewMethodServer( pTask,  &f ); \
    if( ERROR( ret ) ) \
    { \
        std::string strMsg = DebugMsg( ret, \
            "Error allocaate delegate" ); \
        throw std::runtime_error( strMsg ); \
    } \
    _pCurMap_->insert( \
        std::pair< std::string, TaskletPtr >( fname, pTask ) ); \
    if( ERROR( ret ) ) \
    { \
        std::string strMsg = DebugMsg( ret, \
            "Error allocaate delegate" ); \
        throw std::runtime_error( strMsg ); \
    } \
}while( 0 )

#define ADD_USER_SERVICE_HANDLER( f, fname ) \
do{ \
    std::string strHandler = USER_METHOD( fname ); \
    ADD_SERVICE_HANDLER( f, strHandler ); \
}while( 0 )

#define ADD_EVENT_HANDLER( f, fname ) \
do{ \
    std::string strHandler = SYS_EVENT( fname ); \
    ADD_SERVICE_HANDLER( f, strHandler ); \
}while( 0 )

#define ADD_USER_EVENT_HANDLER( f, fname ) \
do{ \
    std::string strHandler = USER_EVENT( fname ); \
    ADD_SERVICE_HANDLER( f, strHandler ); \
}while( 0 )

// construction of the normal interface handler map 
#define BEGIN_IFHANDLER_MAP( _iInterfName_ ) \
do{\
    EnumClsid _iIfId_ = iid( _iInterfName_ );\
    std::string strIfName = #_iInterfName_; \
    std::string strSuffix = \
        IsServer() ? "s" : "p";\
    EnumClsid tempIid = CoGetIidFromIfName( \
        strIfName, strSuffix ); \
    if( tempIid == clsid( Invalid ) ) \
        CoAddIidName( strIfName, _iIfId_, strSuffix ); \
    FUNC_MAP _oAnon_; \
    FUNC_MAP* _pCurMap_ = &_oAnon_; \
    FUNC_MAP* _pMap_ = nullptr; \
    do{ \
        _pMap_ = GetFuncMap( _iIfId_ ); \
        if( _pMap_ != nullptr ) \
            _pCurMap_ = _pMap_; \
    }while( 0 )

#define END_IFHANDLER_MAP END_HANDLER_MAP

// A more high-level handler macro 
#define ADD_SERVICE_HANDLER_EX_BASE( numArgs, f, fname ) \
do{ \
    if( _pCurMap_->size() > 0 && \
        _pCurMap_->find( fname ) != _pCurMap_->end() ) \
        break; \
    TaskletPtr pTask; \
    NewMethodServerEx< numArgs, decltype( &f ) > a( &f ); \
    pTask = a.pDelegate;\
    if( pTask.IsEmpty() ) \
    { \
        std::string strMsg = DebugMsg( -ENOMEM, \
            "Error allocaate delegate" ); \
        throw std::runtime_error( strMsg ); \
    } \
    _pCurMap_->insert( \
        std::pair< std::string, TaskletPtr >( fname, pTask ) ); \
}while( 0 )

#define ADD_USER_SERVICE_HANDLER_EX( numArgs, f, fname ) \
do{ \
    std::string strHandler = USER_METHOD( fname ); \
    ADD_SERVICE_HANDLER_EX_BASE( numArgs, f, strHandler ); \
}while( 0 )

#define ADD_SERVICE_HANDLER_EX( numArgs, f, fname ) \
do{ \
    std::string strHandler = SYS_METHOD( fname ); \
    ADD_SERVICE_HANDLER_EX_BASE( numArgs, f, strHandler ); \
}while( 0 )

template< class T >
class CDeferredCallBase :
    public CGenericCallback< T >
{

    public:
    typedef CGenericCallback< T > super;

    CDeferredCallBase( const IConfigDb* pCfg = nullptr)
        : CGenericCallback< T >( pCfg )
    {;}

    virtual gint32 Delegate( CObjBase* pObj,
        std::vector< BufPtr >& vecParams ) = 0;

    ObjPtr m_pObj;
    std::vector< BufPtr > m_vecArgs;

    virtual gint32 operator()( guint32 dwContext )
    {
#ifdef DEBUG
        this->SetError( -1 );
#endif
        if( m_pObj.IsEmpty() )
            return this->SetError( -EINVAL );

        gint32 ret = this->Delegate( m_pObj, m_vecArgs );
        return this->SetError( ret );
    }

    gint32 UpdateParamAt( guint32 i, BufPtr& pBuf )
    {
        if( i >= m_vecArgs.size() || i < 0 )
            return -EINVAL;
        m_vecArgs[ i ] = pBuf;
        return 0;
    }

    gint32 GetParamAt( guint32 i, BufPtr& pBuf ) const
    {
        if( i >= m_vecArgs.size() || i < 0 )
            return -EINVAL;
        pBuf = m_vecArgs[ i ];
        return 0;
    }
};

template<typename TaskType, typename ClassName, typename ...Args>
class CDeferredCall :
    public CDeferredCallBase< TaskType >
{
    typedef gint32 ( ClassName::* FuncType)( Args... ) ;

    template< int ...S>
    gint32 CallUserFunc( ClassName* pObj, 
        std::vector< BufPtr >& vecParams, NumberSequence< S... > )
    {
        if( pObj == nullptr || m_pUserFunc == nullptr )
            return -EINVAL;

        std::tuple< DecType( Args )...> oTuple( 
            VecToTuple< DecType( Args )... >(
                vecParams ) );

        return ( pObj->*m_pUserFunc )(
            std::get< S >( oTuple )... );
    }

    //---------

    FuncType m_pUserFunc;

    static CfgPtr InitCfg( ObjPtr& pObj )
    {
        CParamList oParams;
        // the pObj must be a interface object
        // so that the CIfParallelTask can be
        // constructed
        CRpcServices* pIf = pObj;
        if( pIf != nullptr )
        {
            oParams[ propIfPtr ] = ObjPtr( pIf );
        }
        return oParams.GetCfg();
    }

    public:
    typedef CDeferredCallBase< TaskType > super;
    CDeferredCall( FuncType pFunc, ObjPtr& pObj, Args... args )
        :super( InitCfg( pObj ) )
    {
        this->m_pObj = pObj;
        m_pUserFunc = pFunc;
        this->SetClassId( clsid( CDeferredCall ) );
        if( sizeof...( args ) )
            PackParams( this->m_vecArgs, args... );
    }

    gint32 Delegate( CObjBase* pObj,
        std::vector< BufPtr >& vecParams )
    {
        if( unlikely( pObj == nullptr ) )
            return -EINVAL;

        ClassName* pClass = dynamic_cast< ClassName* >( pObj );
        if( unlikely( pClass == nullptr ) )
            return -EFAULT;

        if( unlikely( vecParams.size() != sizeof...( Args ) ) )
            return -EINVAL;

        gint32 ret = CallUserFunc( pClass, vecParams,
            typename GenSequence< sizeof...( Args ) >::type() );

        return ret;
    }
};

template < typename C, typename ... Types, typename ...Args>
inline gint32 NewDeferredCall( TaskletPtr& pCallback,
    ObjPtr pObj, gint32(C::*f)(Types ...), Args&&... args )
{
    CTasklet* pDeferredCall =
        new CDeferredCall< CTasklet, C, Types... >( f, pObj, args... );

    if( unlikely( pDeferredCall == nullptr ) )
        return -EFAULT;

    pCallback = pDeferredCall;
    pCallback->DecRef();
    return 0;
}

#define DEFER_CALL( pMgr, pObj, func, ... ) \
({ \
    TaskletPtr __pTask; \
    gint32 ret_ = NewDeferredCall( __pTask, pObj, func, ##__VA_ARGS__  ); \
    if( SUCCEEDED( ret_ ) ) \
        ret_ = pMgr->RescheduleTask( __pTask ); \
    ret_; \
})

#define DEFER_CALL_NOSCHED( __pTask, pObj, func, ... ) \
    NewDeferredCall( __pTask, pObj, func , ##__VA_ARGS__ )

#define DEFER_CALL_DELAY( pMgr, iSec, pObj, func, ... ) \
({ \
    TaskletPtr __pTask; \
    gint32 ret_ = NewDeferredCall( __pTask, pObj, func, ##__VA_ARGS__  ); \
    if( SUCCEEDED( ret_ ) ) \
    { \
        CTimerService& oTimerSvc = pMgr->GetUtils().GetTimerSvc();\
        IEventSink* pEvent = __pTask;\
        if( pEvent != nullptr )\
            ret_ = oTimerSvc.AddTimer( iSec, pEvent, 0 );\
        else\
            ret_ = -EFAULT;\
    }\
    ret_; \
})

template< class T >
class CIfDeferCallTaskBase :
    public T
{
    TaskletPtr m_pDeferCall;
    EnumIfState m_iTaskState = stateStarting;
    public:
    typedef T super;
    CIfDeferCallTaskBase( const IConfigDb* pCfg )
        :super( pCfg )
    { this->SetClassId( clsid( CIfDeferCallTask ) );}

    void SetDeferCall( TaskletPtr& pTask )
    { m_pDeferCall = pTask; }

    // just as a place holder
    virtual gint32 RunTask()
    {
        m_iTaskState = stateStarted;
        if( m_pDeferCall.IsEmpty() )
            return 0;

        return ( *m_pDeferCall )( 0 );
    }

    gint32 OnTaskComplete( gint32 iRet )
    { return iRet; }

    gint32 UpdateParamAt(
        guint32 i, BufPtr pBuf )
    {
        CDeferredCallBase< CTasklet >*
            pTask = m_pDeferCall;
        if( pTask == nullptr )
            return -EINVAL;

        return pTask->UpdateParamAt( i, pBuf );
    }

    gint32 GetParamAt(
        guint32 i, BufPtr& pBuf ) const
    {
        CDeferredCallBase< CTasklet >*
            pTask = m_pDeferCall;
        if( pTask == nullptr )
            return -EINVAL;

        return pTask->GetParamAt( i, pBuf );
    }

    gint32 OnComplete( gint32 iRetVal )
    {
        gint32 ret = super::OnComplete( iRetVal );
        m_pDeferCall.Clear();
        return ret;
    }

    gint32 OnCancel( guint32 dwContext )
    {
        gint32 ret = super::OnCancel( dwContext );
        if( m_iTaskState == stateStarting )
            m_pDeferCall.Clear();
        return ret;
    }
};

typedef CIfDeferCallTaskBase< CIfRetryTask > CIfDeferCallTask;
class CIfDeferCallTaskEx :
    public CIfDeferCallTaskBase< CIfInterceptTaskProxy >
{

    public:
    typedef CIfDeferCallTaskBase< CIfInterceptTaskProxy > super;
    CIfDeferCallTaskEx ( const IConfigDb* pCfg )
        :super( pCfg )
    { SetClassId( clsid( CIfDeferCallTaskEx ) );}
};

template < typename C, typename ... Types, typename ...Args>
inline gint32 NewIfDeferredCall( EnumClsid iTaskClsid,
    TaskletPtr& pCallback, ObjPtr pIf, gint32(C::*f)(Types ...),
    Args&&... args )
{
    TaskletPtr pWrapper;
    gint32 ret = NewDeferredCall( pWrapper, pIf, f, args... );
    if( ERROR( ret ) )
        return ret;

    TaskletPtr pIfTask;
    CParamList oParams;
    oParams[ propIfPtr ] = pIf;
    ret = pIfTask.NewObj(
        iTaskClsid,
        oParams.GetCfg() );
    if( ERROR( ret ) )
        return ret;

    if( iTaskClsid == clsid( CIfDeferCallTask ) )
    {
        CIfDeferCallTask* pDeferTask = pIfTask;
        pDeferTask->SetDeferCall( pWrapper );
        pCallback = pDeferTask;
    }
    else if( iTaskClsid == clsid( CIfDeferCallTaskEx ) )
    {
        CIfDeferCallTaskEx* pDeferTask = pIfTask;
        pDeferTask->SetDeferCall( pWrapper );
        pCallback = pDeferTask;
    }
    else
    {
        return -ENOTSUP;
    }
    return 0;
}

#define DEFER_IFCALL_NOSCHED( __pTask, pObj, func, ... ) \
    NewIfDeferredCall( clsid( CIfDeferCallTask ), \
        __pTask, pObj, func , ##__VA_ARGS__ )

// use this macro when the _pTask will be added
// to a task group.
#define DEFER_IFCALL_NOSCHED2( _pos, _pTask, pObj, func, ... ) \
({ \
    gint32 _ret = NewIfDeferredCall( clsid( CIfDeferCallTask ), \
        _pTask, pObj, func , ##__VA_ARGS__ ); \
    if( SUCCEEDED( _ret ) ) \
    { \
        CIfDeferCallTask* pDefer = _pTask; \
        BufPtr pBuf( true ); \
        *pBuf = ObjPtr( pDefer ); \
        pDefer->UpdateParamAt( _pos, pBuf );  \
    } \
    _ret; \
})

#define DEFER_IFCALLEX_NOSCHED( _pTask, pObj, func, ... ) \
    NewIfDeferredCall( clsid( CIfDeferCallTaskEx ), \
        _pTask, pObj, func , ##__VA_ARGS__ )

// use this macro when the _pTask will be added
// to a task group.
#define DEFER_IFCALLEX_NOSCHED2( _pos, _pTask, pObj, func, ... ) \
({ \
    gint32 _ret = NewIfDeferredCall( clsid( CIfDeferCallTaskEx ), \
        _pTask, pObj, func , ##__VA_ARGS__ ); \
    if( SUCCEEDED( _ret ) ) \
    { \
        CIfDeferCallTaskEx* pDefer = _pTask; \
        BufPtr pBuf( true ); \
        *pBuf = ObjPtr( pDefer ); \
        pDefer->UpdateParamAt( _pos, pBuf );  \
    } \
    _ret; \
})

// to insert the task pInterceptor to the head of
// completion chain of the task pTarget 
// please make sure both tasks inherit from the
// CIfRetryTask
inline gint32 InterceptCallback(
    IEventSink* pInterceptor, IEventSink* pTarget )
{
    if( pInterceptor == nullptr ||
        pTarget == nullptr )
        return -EINVAL;

    CIfRetryTask* pInterceptTask =
        ObjPtr( pInterceptor );

    if( pInterceptTask == nullptr )
        return -EINVAL;

    CIfRetryTask* pTargetTask =
        ObjPtr( pTarget );

    if( pTargetTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    EventPtr pParent;
    ret = pTargetTask->GetClientNotify(
        pParent );
    if( SUCCEEDED( ret ) )
    {
        pInterceptTask->SetClientNotify(
            pParent);
    }

    pTargetTask->SetClientNotify(
        pInterceptTask );
    return 0;
}

inline gint32 RemoveInterceptCallback(
    IEventSink* pInterceptor, IEventSink* pTarget )
{
    if( pInterceptor == nullptr ||
        pTarget == nullptr )
        return -EINVAL;

    CIfRetryTask* pInterceptTask =
        ObjPtr( pInterceptor );

    if( pInterceptTask == nullptr )
        return -EINVAL;

    CIfRetryTask* pTargetTask =
        ObjPtr( pTarget );

    if( pTargetTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    EventPtr pParent;
    ret = pInterceptTask->GetClientNotify(
        pParent );
    if( SUCCEEDED( ret ) )
    {
        pTargetTask->SetClientNotify(
            pParent);
    }

    pInterceptTask->ClearClientNotify();
    return 0;
}

class CIfDeferredHandler :
    public CIfInterceptTaskProxy
{
    protected:
    TaskletPtr m_pDeferCall;

    public:
    typedef CIfInterceptTaskProxy super;
    CIfDeferredHandler( const IConfigDb* pCfg )
        :super( pCfg )
    { SetClassId( clsid( CIfDeferredHandler ) );}

    void SetDeferCall( TaskletPtr& pTask )
    { m_pDeferCall = pTask; }

    // just as a place holder
    gint32 RunTask();
    gint32 UpdateParamAt( guint32 i, BufPtr pBuf );
    gint32 OnTaskComplete( gint32 iRet );
    gint32 OnComplete( gint32 iRetVal );

    gint32 OnCancel( guint32 dwContext )
    {
        gint32 ret = super::OnCancel( dwContext );
        if( m_iTaskState == stateStarting )
            m_pDeferCall.Clear();
        return ret;
    }
};

template < typename C, typename ... Types, typename ...Args>
inline gint32 NewDeferredHandler(
    bool bInsertBefore, // put the new task to complete before/after pTaskToIntercept
    EnumClsid iClsid,
    gint32 iPos,
    TaskletPtr& pCallback,
    ObjPtr pIf, gint32(C::*f)(Types ...),
    IEventSink* pTaskToIntercept, Args&&... args )
{
    TaskletPtr pWrapper;
    if( pTaskToIntercept == nullptr ||
        pIf.IsEmpty() )
        return -EINVAL;

    gint32 ret = NewDeferredCall(
        pWrapper, pIf, f, pTaskToIntercept, args... );

    if( ERROR( ret ) )
        return ret;

    TaskletPtr pIfTask;
    CParamList oParams;
    oParams[ propIfPtr ] = pIf;

    if( bInsertBefore )
    {
        oParams[ propEventSink ] =
            ObjPtr( pTaskToIntercept );
    }

    ret = pIfTask.NewObj( 
        iClsid, oParams.GetCfg() );
    if( ERROR( ret ) )
        return ret;

    if( !bInsertBefore )
    {
        ret = rpcf::InterceptCallback(
            pIfTask, pTaskToIntercept );
        if( ERROR( ret ) )
            return ret;
    }
    CIfDeferredHandler* pDeferTask = pIfTask;
    pDeferTask->SetDeferCall( pWrapper );

    BufPtr pBuf( true );
    *pBuf = ObjPtr( pDeferTask );

    pCallback = pDeferTask;
    if( iPos < 0 )
        return 0;

    // for the handler method, pass the pDeferTask
    // as the callback
    ret = pDeferTask->UpdateParamAt( iPos, pBuf );
    if( ERROR( ret ) )
        return ret;

    return 0;
}

#define DEFER_HANDLER_NOSCHED( __pTask, pObj, func, pCallback, ... ) \
    NewDeferredHandler( true, clsid( CIfDeferredHandler ),\
        0, __pTask, pObj, func , pCallback, ##__VA_ARGS__ )

#define DEFER_HANDLER_NOSCHED2( _pos, __pTask, pObj, func, pCallback, ... ) \
    NewDeferredHandler( true, clsid( CIfDeferredHandler ),\
        _pos, __pTask, pObj, func , pCallback, ##__VA_ARGS__ )

class CIfAsyncCancelHandler :
    public CIfDeferredHandler
{
    bool m_bSelfCleanup = false;
    public:
    typedef CIfDeferredHandler super;
    CIfAsyncCancelHandler( const IConfigDb* pCfg )
        :super( pCfg )
    { SetClassId( clsid( CIfAsyncCancelHandler ) );}

    // just as a place holder
    gint32 RunTask()
    { return STATUS_PENDING; }

    void SetSelfCleanup()
    { m_bSelfCleanup = true; }
    gint32 OnTaskComplete( gint32 iRet );
};

#define DEFER_CANCEL_HANDLER2( _pos, __pTask, pObj, func, pCallback, ... ) \
    ( { gint32 ret_ = NewDeferredHandler( false, clsid( CIfAsyncCancelHandler ),\
        _pos, __pTask, pObj, func , pCallback, ##__VA_ARGS__ ); \
    if( SUCCEEDED( ret_ ) ) \
        ( *__pTask )( eventZero ); \
    ret_;} )

class CIfResponseHandler :
    public CIfDeferredHandler
{
    public:
    typedef CIfDeferredHandler super;
    CIfResponseHandler( const IConfigDb* pCfg )
        :super( pCfg )
    { SetClassId( clsid( CIfResponseHandler ) );}

    // just as a place holder
    gint32 RunTask()
    { return STATUS_PENDING; }

    gint32 OnTaskComplete( gint32 iRet );
    gint32 OnCancel( guint32 dwContext );
    gint32 OnIrpComplete( PIRP pIrp );
};

class CIfIoCallTask :
    public CIfResponseHandler
{
    TaskletPtr m_pMajorCall;

    public:
    typedef CIfResponseHandler super;
    CIfIoCallTask( const IConfigDb* pCfg )
        :super( pCfg )
    { SetClassId( clsid( CIfIoCallTask ) );}

    void SetMajorCall( TaskletPtr& pCall )
    { m_pMajorCall = pCall; }

    // just as a place holder
    gint32 RunTask();
    gint32 OnTaskComplete( gint32 iRet );
    gint32 OnCancel( guint32 dwContext );
};

template< class C >
inline gint32 NewResponseHandler(
    EnumClsid iClsid,
    TaskletPtr& pRespHandler,
    ObjPtr pIf, gint32(C::* f)( IEventSink*, IEventSink*, IConfigDb* ),
    IEventSink* pCallback, IConfigDb* pContext )
{
    TaskletPtr pWrapper;
    if( pIf.IsEmpty() )
        return -EINVAL;

    gint32 ret = NewDeferredCall(
        pWrapper, pIf, f, pCallback,
        ( IEventSink* )nullptr, pContext );

    if( ERROR( ret ) )
        return ret;

    TaskletPtr pIfTask;

    CParamList oParams;
    oParams[ propIfPtr ] = pIf;

    ret = pIfTask.NewObj( 
        iClsid,
        oParams.GetCfg() );
    if( ERROR( ret ) )
        return ret;

    CIfResponseHandler* pNewTask = pIfTask;
    pNewTask->SetDeferCall( pWrapper );
    pRespHandler = pNewTask;

    return 0;
}

#define NEW_PROXY_RESP_HANDLER( __pTask, pObj, func, pCallback, pContext ) \
    NewResponseHandler( clsid( CIfResponseHandler ), __pTask, pObj, func , pCallback, pContext )

#define NEW_PROXY_RESP_HANDLER2( __pTask, pObj, func, pCallback, pContext ) \
( { gint32 ret_ = NewResponseHandler( clsid( CIfResponseHandler ), __pTask, pObj, func , pCallback, pContext ); \
    if( SUCCEEDED( ret_ ) ) \
        ( *__pTask )( eventZero ); \
    ret_;} )
/**
* @name NEW_PROXY_IOTASK
* @brief the macro to combine the async call and callback in the task __pTask.
* @param _iPos the position of the callback parameter in the major call _pMajorCall
* @param __pTask the pointer contains the task created for the io call and callback
* @param _pMajorCall the pointer to the major call task, which should be created by DEFER_CALL_NOSCHED
* @param _pObj the object whose _func will be called as the callback
* @param _func the function pointer as the callback.
* @param _pCallback the first parameter for the _func
* @param _pContext the third parameter for the _func
*
* @note The macro restricts the response callback to have three parameters, a
* pCallback, a pointer to the ioreq task, and a pointer to the context, and it
* also needs the major call to be a simple DEFER_CALL_NOSCHED taskr; the _func
* has the responsibility to pass on _pCallback if it returns pending or send
* eventTaskComp to make the final completion, and __pTask's life cycle ends after
* the _func gets called, whether _func return pending or not.
* 
* @{ */
/**  @} */

#define NEW_PROXY_IOTASK( _iPos, __pTask, _pMajorCall, _pObj, _func, _pCallback, _pContext ) \
({ ret = NewResponseHandler( clsid( CIfIoCallTask ), __pTask, _pObj, _func , _pCallback, _pContext ); \
    if( SUCCEEDED( ret ) ) {\
        CIfIoCallTask* _pIoCall = __pTask;\
        _pIoCall->SetMajorCall( _pMajorCall );\
        CDeferredCallBase< CTasklet >* \
            pDeferCall = _pMajorCall; \
        BufPtr pBuf( true );\
        *pBuf = ObjPtr( _pIoCall );\
        pDeferCall->UpdateParamAt( _iPos, pBuf );\
    }\
    ret;\
})

template < typename T, typename C, typename ... Types, typename ...Args>
inline gint32 NewObjDeferredCall( EnumClsid iTaskClsid,
    TaskletPtr& pCallback, T* pIf, gint32(C::*f)(Types ...),
    Args&&... args )
{
    TaskletPtr pWrapper;
    gint32 ret = NewDeferredCall( pWrapper, pIf, f, args... );
    if( ERROR( ret ) )
        return ret;

    if( pIf == nullptr )
        return -EINVAL;

    CParamList oParams;
    oParams.SetPointer( propIoMgr, pIf->GetIoMgr() );

    TaskletPtr pIfTask;
    ret = pIfTask.NewObj( iTaskClsid, oParams.GetCfg() );
    if( ERROR( ret ) )
        return ret;

    if( iTaskClsid == clsid( CIfDeferCallTask ) )
    {
        CIfDeferCallTask* pDeferTask = pIfTask;
        pDeferTask->SetDeferCall( pWrapper );
        pCallback = pDeferTask;
    }
    else if( iTaskClsid == clsid( CIfDeferCallTaskEx ) )
    {
        CIfDeferCallTaskEx* pDeferTask = pIfTask;
        pDeferTask->SetDeferCall( pWrapper );
        pCallback = pDeferTask;
    }
    else
    {
        return -ENOTSUP;
    }
    return 0;
}

#define DEFER_OBJCALL_NOSCHED( __pTask, pObj, func, ... ) \
    NewObjDeferredCall( clsid( CIfDeferCallTask ), \
        __pTask, pObj, func , ##__VA_ARGS__ )

// use this macro when the _pTask will be added
// to a task group.
#define DEFER_OBJCALL_NOSCHED2( _pos, _pTask, pObj, func, ... ) \
({ \
    gint32 _ret = NewObjDeferredCall( clsid( CIfDeferCallTask ), \
        _pTask, pObj, func , ##__VA_ARGS__ ); \
    if( SUCCEEDED( _ret ) ) \
    { \
        CIfDeferCallTask* pDefer = _pTask; \
        BufPtr pBuf( true ); \
        *pBuf = ObjPtr( pDefer ); \
        pDefer->UpdateParamAt( _pos, pBuf );  \
    } \
    _ret; \
})

#define DEFER_OBJCALLEX_NOSCHED( _pTask, pObj, func, ... ) \
    NewObjDeferredCall( clsid( CIfDeferCallTaskEx ), \
        _pTask, pObj, func , ##__VA_ARGS__ )

// use this macro when the _pTask will be added
// to a task group.
#define DEFER_OBJCALLEX_NOSCHED2( _pos, _pTask, pObj, func, ... ) \
({ \
    gint32 _ret = NewObjDeferredCall( clsid( CIfDeferCallTaskEx ), \
        _pTask, pObj, func , ##__VA_ARGS__ ); \
    if( SUCCEEDED( _ret ) ) \
    { \
        CIfDeferCallTaskEx* pDefer = _pTask; \
        BufPtr pBuf( true ); \
        *pBuf = ObjPtr( pDefer ); \
        pDefer->UpdateParamAt( _pos, pBuf );  \
    } \
    _ret; \
})

template< class T, class C >
inline gint32 NewResponseHandler2(
    EnumClsid iClsid,
    TaskletPtr& pRespHandler,
    T* pIf, gint32(C::* f)( IEventSink*, IEventSink*, IConfigDb* ),
    IEventSink* pCallback, IConfigDb* pContext )
{
    TaskletPtr pWrapper;
    if( pIf == nullptr )
        return -EINVAL;

    gint32 ret = NewDeferredCall(
        pWrapper, pIf, f, pCallback,
        ( IEventSink* )nullptr, pContext );

    if( ERROR( ret ) )
        return ret;

    CParamList oParams;
    oParams.SetPointer(
        propIoMgr, pIf->GetIoMgr() );

    TaskletPtr pIfTask;
    ret = pIfTask.NewObj( 
        iClsid, oParams.GetCfg() );

    if( ERROR( ret ) )
        return ret;

    CIfResponseHandler* pNewTask = pIfTask;
    pNewTask->SetDeferCall( pWrapper );
    pRespHandler = pNewTask;

    return 0;
}

#define NEW_OBJ_RESP_HANDLER( __pTask, pObj, func, pCallback, pContext ) \
    NewResponseHandler2( clsid( CIfResponseHandler ), __pTask, pObj, func , pCallback, pContext )

#define NEW_OBJ_RESP_HANDLER2( __pTask, pObj, func, pCallback, pContext ) \
( { ret = NewResponseHandler2( clsid( CIfResponseHandler ), __pTask, pObj, func , pCallback, pContext ); \
    if( SUCCEEDED( ret ) ) \
        ( *__pTask )( eventZero ); \
    ret;} )
/**
* @name NEW_PROXY_IOTASK
* @brief the macro to combine the async call and callback in the task __pTask.
* @param _iPos the position of the callback parameter in the major call _pMajorCall
* @param __pTask the pointer contains the task created for the io call and callback
* @param _pMajorCall the pointer to the major call task, which should be created by DEFER_CALL_NOSCHED
* @param _pObj the object whose _func will be called as the callback
* @param _func the function pointer as the callback.
* @param _pCallback the first parameter for the _func
* @param _pContext the third parameter for the _func
*
* @note The macro restricts the response callback to have three parameters, a
* pCallback, a pointer to the ioreq task, and a pointer to the context, and it
* also needs the major call to be a simple DEFER_CALL_NOSCHED taskr; the _func
* has the responsibility to pass on _pCallback if it returns pending or send
* eventTaskComp to make the final completion, and __pTask's life cycle ends after
* the _func gets called, whether _func return pending or not.
* 
* @{ */
/**  @} */

#define NEW_OBJ_IOTASK( _iPos, __pTask, _pMajorCall, _pObj, _func, _pCallback, _pContext ) \
({ ret = NewResponseHandler2( clsid( CIfIoCallTask ), __pTask, _pObj, _func , _pCallback, _pContext ); \
    if( SUCCEEDED( ret ) ) {\
        CIfIoCallTask* _pIoCall = __pTask;\
        _pIoCall->SetMajorCall( _pMajorCall );\
        CDeferredCallBase< CTasklet >* \
            pDeferCall = _pMajorCall; \
        BufPtr pBuf( true );\
        *pBuf = ObjPtr( _pIoCall );\
        pDeferCall->UpdateParamAt( _iPos, pBuf );\
    }\
    ret;\
})
template<typename ClassName, typename ...Args>
class CDeferredCallOneshot :
    public CDeferredCall< CIfParallelTask, ClassName, Args... >
{
    typedef gint32 ( ClassName::* FuncType)( Args... ) ;

    public:
    typedef CDeferredCall< CIfParallelTask, ClassName, Args... > super;

    CDeferredCallOneshot( IEventSink* pCallback,
        FuncType pFunc, ObjPtr& pObj, Args... args )
        : super( pFunc, pObj, args... )
    {
        if( pCallback == nullptr )
            return;
        InterceptCallback( this, pCallback );
    }

    virtual gint32 operator()( guint32 dwContext = 0 )
    {
        return CIfParallelTask::operator()( dwContext );
    }

    // just to initiate the task state
    virtual gint32 RunTask()
    {
        return STATUS_PENDING;
    }

    virtual gint32 OnTaskComplete( gint32 iRet ) 
    {
        if( this->m_pObj.IsEmpty() )
            return -EFAULT;

        this->Delegate(
            this->m_pObj, this->m_vecArgs );

        return iRet;
    }
};

template < typename C, typename ... Types, typename ...Args>
inline gint32 NewDeferredCallOneshot(
    IEventSink* pTaskToIntercept,
    TaskletPtr& pCallback,
    ObjPtr pObj,
    gint32(C::*f)(Types ...),
    Args&&... args )
{
    if( pTaskToIntercept == nullptr )
        return -EINVAL;

    CTasklet* pDeferredCall =
        new CDeferredCallOneshot< C, Types... >(
            pTaskToIntercept, f, pObj, args... );

    if( pDeferredCall == nullptr )
        return -EFAULT;

    pCallback = pDeferredCall;
    pCallback->DecRef();
    return 0;
}

#define NEW_COMPLETE_CALLBACK( _pTask, pEvent, _pObj, _func, ... ) \
({ \
    gint32 _ret = NewDeferredCallOneshot( \
        pEvent, _pTask, _pObj, _func, ##__VA_ARGS__ ); \
    if( !_pTask.IsEmpty() ) \
        ( *_pTask )( eventZero ); \
    _ret; \
})

// NOTE: it is required to provide at least one
// argument in the variadic parameter pack
#define INSTALL_COMPLETE_HANDLER( pEvent, pObj, func, ... ) \
({ \
    TaskletPtr pTask; \
    NEW_COMPLETE_CALLBACK( pTask, pEvent, pObj, func VA_ARGS(__VA_ARGS__) ); \
})

template< class T >
class CAsyncCallbackBase :
    public CGenericCallback< T >
{
    public:
    typedef CGenericCallback< T > super;

    gint32 DoCallback( CObjBase* pCfg )
    {
        if( pCfg == nullptr )
            return -EINVAL;

        gint32 ret = 0;
        do{
            // for timeout, this task carries the
            // response
            CCfgOpenerObj oCfg( pCfg );

            ObjPtr pObj;

            ret = oCfg.GetObjPtr( propRespPtr, pObj );
            if( ERROR( ret ) )
                break;

            CfgPtr pResp = pObj;
            if( pResp.IsEmpty()  )
            {
                ret = -EFAULT;
                break;
            }
            std::vector< BufPtr > vecRespParams;
            ret = this->NormalizeParams( true, pResp, vecRespParams );
            if( ERROR( ret ) )
                break;

            ret = Callback( vecRespParams );

        }while( 0 );

        return ret;
    }

    public:

    CAsyncCallbackBase( const IConfigDb* pCfg = nullptr)
        : CGenericCallback< T >( pCfg ) 
    {;}

    virtual gint32 Callback(
        std::vector< BufPtr >& vecParams ) = 0;

    gint32 operator()( guint32 dwContext = 0 )
    {
        gint32 ret = 0;

        switch( dwContext )
        {
        case eventTimeout:
            {
                // for timeout, this task carries the
                // response
                ret = DoCallback( this->GetConfig() );
                break;

            }
        case eventTaskComp:
            {
                std::vector< LONGWORD > vecParams;

                gint32 ret = this->GetParamList( vecParams );

                if( ERROR( ret ) )
                    break;

                if( vecParams.size() < 4 )
                {
                    ret = -EINVAL;
                    break;
                }

                CObjBase* pObjBase =
                    reinterpret_cast< CObjBase* >( vecParams[ 3 ] );

                if( pObjBase == nullptr )
                    break;

                ret = DoCallback( pObjBase );
                break;

            }
        default:
            {
                ret = -ENOTSUP;
                break;
            }
        }

        return ret;
    }
};

template<typename TaskType, typename ClassName, typename ...Args>
class CAsyncCallback :
    public CAsyncCallbackBase< TaskType >
{
    protected:
    typedef gint32 ( ClassName::* FuncType)( IEventSink* pTask, Args... ) ;

    template< int ...S>
    gint32 CallUserFunc( std::tuple< DecType( Args )... >& oTuple, NumberSequence< S... > )
    {
        if( m_pObj == nullptr || m_pUserFunc == nullptr )
            return -EINVAL;

        gint32 ret = ( m_pObj->*m_pUserFunc )( this,
            std::get< S >( oTuple ) ... );

        return ret;
    }
    //---------

    FuncType m_pUserFunc;
    ClassName* m_pObj;

    static CfgPtr InitCfg( ClassName* pObj )
    {
        CParamList oParams;
        // the pObj must be a interface object
        // so that the CIfParallelTask can be
        // constructed
        CRpcServices* pIf = pObj;
        oParams[ propIfPtr ] = ObjPtr( pIf );
        return oParams.GetCfg();
    }

    public:
    typedef CAsyncCallbackBase< TaskType > super;
    CAsyncCallback( ClassName* pObj, FuncType pFunc )
       : super( InitCfg( pObj ) )
    {
        m_pUserFunc = pFunc;
        m_pObj = pObj;
        // note: `this' is necessary to get compiler
        // happy because it cannot directly find
        // SetClassId as the base class is a template
        // to resolve
        this->SetClassId( clsid( CAsyncCallback ) );
    }

    template< typename T, typename S, typename ...Rest >
    gint32 GetDefaults( std::vector< BufPtr >& vecBufs  )
    {
        BufPtr pBuf = GetDefault(
            ( ValType( T )* )nullptr );

       vecBufs.push_back(  pBuf );
       return GetDefaults< S, Rest...>( vecBufs );
    }

    template< typename T >
    gint32 GetDefaults( std::vector< BufPtr >& vecBufs )
    {
        BufPtr pBuf = GetDefault(
            ( ValType( T )* )nullptr );

       vecBufs.push_back(  pBuf );
       return 0;
    }

    // well, this is a virtual function in the template
    // class. 
    gint32 Callback( std::vector< BufPtr >& vecParams )
    {
        if( vecParams.empty() )
            return -EINVAL;

        gint32 ret = 0;
        try{
            if( vecParams.size() < sizeof...( Args ) )
            {
                guint32 iRet = *vecParams[ 0 ];
                if( ERROR( iRet ) )
                {
                    // error returned, let's make some fake
                    // values to get the callback be called
                    // with the error
                    std::vector< BufPtr > vecDefaults;

                    // at least there will be one argument
                    GetDefaults<Args...>( vecDefaults );

                    if( vecDefaults.size() <= vecParams.size() )
                        return -ENOENT;

                    for( size_t i = vecParams.size(); i < vecDefaults.size(); ++i )
                    {
                        // replace with significant values
                        // when the call returns
                        vecParams.push_back( vecDefaults[ i ] );
                    }
                }
                else
                {
                    return -ENOENT;
                }
            }

            std::vector< BufPtr > vecResp;

            vecResp.insert( vecResp.begin(),
                 vecParams.begin(), vecParams.end() );

            std::tuple< DecType( Args )...>
                oTupleStore(  VecToTuple< Args... >( vecResp ) );

            ret = CallUserFunc( oTupleStore,
                typename GenSequence< sizeof...( Args ) >::type() );

        }
        catch( const std::invalid_argument& e )
        {
            return -EINVAL;
        }

        return ret;
    }
};

/**
* @name CAsyncCallbackOneshot
* @{ */
/**
 * Description: This class uses CIfParallelTask as its
 * base class, because CIfParallelTask has a few useful
 * attributes:
 *  1. thread-safe
 *  2. has state, that is, once the task is
 *  completed, it cannot be reentered.
 *
 * the main purpose of this class is to intercept the
 * eventsink ptr as contained in  parameter `pCallback'
 * of ctor,  which will be called back in the task
 * OnComplete routine. Between the caller and the
 * eventSink, the bound function `pFunc' in the ctor
 * parameter list,  will be called.
 *
 * @} */

template<typename ClassName, typename ...Args>
class CAsyncCallbackOneshot :
    public CAsyncCallback< CIfParallelTask, ClassName, Args... >
{
    typedef gint32 ( ClassName::* FuncType)( IEventSink* pTask, Args... ) ;

    gint32 CallOrigCallback(
        CObjBase* pCallback, gint32 iRet )
    {
        if( pCallback == nullptr )
            return -EINVAL;

        CCfgOpener oCfg(
            ( IConfigDb* ) this->GetConfig() );

        ObjPtr pObj;
        gint32 ret = oCfg.GetObjPtr(
            propEventSink, pObj );

        if( ERROR( ret ) )
            return ret;

        EventPtr pEvt( pObj );
        if( pEvt.IsEmpty() )
            return -EFAULT;

        ret = pEvt->OnEvent( eventTaskComp,
            iRet, 0, ( LONGWORD* )pCallback );

        oCfg.RemoveProperty( propEventSink );

        return ret;
    }

    gint32 InterceptCallback( IEventSink* pCallback )
    {
        return rpcf::InterceptCallback(
            this, pCallback );
    }

    public:

    typedef CAsyncCallback< CIfParallelTask, ClassName, Args... > super;
    CAsyncCallbackOneshot( IEventSink* pCallback,
        ClassName* pObj, FuncType pFunc )
        : super( pObj, pFunc )
    {
        InterceptCallback( pCallback );
    }

    virtual gint32 operator()( guint32 dwContext = 0 )
    {
        return CIfParallelTask::operator()( dwContext );
    }

    // just to initiate the task state
    virtual gint32 RunTask()
    {
        return STATUS_PENDING;
    }

    virtual gint32 OnTaskComplete( gint32 iRet ) 
    {
        gint32 ret = 0;
        CObjBase* pObjBase = nullptr;
        do{
            std::vector< LONGWORD > vecParams;

            ret = this->GetParamList( vecParams );

            if( ERROR( ret ) )
                break;

            if( vecParams.size() < 4 )
            {
                ret = -EINVAL;
                break;
            }

            pObjBase = reinterpret_cast< CObjBase* >
                ( vecParams[ 3 ] );

            if( pObjBase == nullptr )
                break;

            ret = this->DoCallback( pObjBase );

            break;

        }while( 0 );

        if( ret == STATUS_PENDING )
            return ret;

        if( pObjBase != nullptr )
            CallOrigCallback( pObjBase, iRet );

        return ret;
    }
};

/**
* @name NewAsyncCallback
* @{ */
/**
  * helper function to ease the developer's effort
  * on instanciation of the the CAsyncCallback via
  * implicit instanciation.
  *@} */

template < typename C, typename ... Types>
inline gint32 NewAsyncCallback(
    TaskletPtr& pCallback, C* pObj,
    gint32(C::*f)( IEventSink* pTask, Types ...) )
{
    if( pObj == nullptr || f == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    try{
        // we cannot use NewObj for this
        // template object
        pCallback = new CAsyncCallback< CTasklet, C, Types... >( pObj, f );
        pCallback->DecRef();
    }
    catch( const std::bad_alloc& e )
    {
        ret = -ENOMEM;
    }
    catch( const std::runtime_error& e )
    {
        ret = -EFAULT;
    }
    catch( const std::invalid_argument& e )
    {
        ret = -EINVAL;
    }

    return ret;
}

/**
* @name BIND_CALLBACK 
* @{ */
/**
  * Helper macro for further effort saving note the
  * `this' pointer, which indicated you should use
  * the macro only in the context of the  member
  * method of the same class as pFunc.
  *
  * So the pTask is a TaskletPtr to receive the new
  * callback object, and the pFunc is the member method
  * as the callback for a specific request
  *
  * NOTE: please make sure the first parameter of the
  * pFunc must be the return value of the call
 * @} */

#define BIND_CALLBACK( pTask, pFunc ) \
    NewAsyncCallback( pTask, this, pFunc ) 

template < typename C, typename ... Types>
inline gint32 NewAsyncCallbackOneshot(
    TaskletPtr& pCallback, C* pObj,
    gint32(C::*f)( IEventSink* pTask, Types ...), IEventSink* pEvent )
{
    if( pObj == nullptr || f == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    try{
        // we cannot use NewObj for this
        // template object
        pCallback = new CAsyncCallbackOneshot< C, Types... >( pEvent, pObj, f );
        pCallback->DecRef();
    }
    catch( const std::bad_alloc& e )
    {
        ret = -ENOMEM;
    }
    catch( const std::runtime_error& e )
    {
        ret = -EFAULT;
    }
    catch( const std::invalid_argument& e )
    {
        ret = -EINVAL;
    }

    return ret;
}
#define BIND_CALLBACK_ONESHOT( pTask, pFunc, pEvtPtr ) \
do{ \
    NewAsyncCallbackOneshot( pTask, this, pFunc, pEvtPtr ); \
    if( !pTask.IsEmpty() ) \
        ( *pTask )( eventZero ); \
}while( 0 )


class CIoReqSyncCallback : public CSyncCallback
{
    public:
    typedef CSyncCallback super;

    CIoReqSyncCallback( const IConfigDb* pCfg )
        : super( pCfg ) 
    {
        SetClassId( clsid( CIoReqSyncCallback ) );
    }
    gint32 operator()( guint32 dwContext = 0 );
};

template< typename ...Args >
gint32 CInterfaceProxy::SyncCallEx(
    CfgPtr& pOptions,
    CfgPtr& pResp,
    const std::string& strcMethod,
    Args&&... args )
{
    gint32 ret = 0;

    do{ 
        guint64 qwIoTaskId = 0; 
        TaskletPtr pTask; 
        ret = pTask.NewObj( clsid( CIoReqSyncCallback ) ); 
        if( ERROR( ret ) ) 
            break; 

        if( pResp.IsEmpty() )
            pResp.NewObj();

        CopyUserOptions( pTask, pOptions );
        CParamList oResp( ( IConfigDb* )pResp );

        std::string strMethod( strcMethod );
        if( !pOptions.IsEmpty() )
        {
            bool bSysMethod = false;
            CCfgOpener oOptions(
                ( IConfigDb* )pOptions );

            ret = oOptions.GetBoolProp(
                propSysMethod, bSysMethod );

            if( SUCCEEDED( ret ) && bSysMethod )
                strMethod = SYS_METHOD( strMethod );
            else
                strMethod = USER_METHOD( strMethod );
        }

        ret = CallProxyFunc( pTask, 
            qwIoTaskId, strMethod, args... ); 

        CIoReqSyncCallback* pSyncCall = pTask;
        if( ret == STATUS_PENDING ) 
        { 
            // return the task id for cancel purpose
            oResp[ propTaskId ] = qwIoTaskId;
            ret = pSyncCall->WaitForComplete(); 
            if( SUCCEEDED( ret ) )
                ret = pSyncCall->GetError();
        } 
        if( ERROR( ret ) ) 
            break; 

        if( !pOptions.IsEmpty() )
        {
            CCfgOpener oOptions(
                ( IConfigDb* )pOptions );
            bool bNoReply = false;
            gint32 iRet = oOptions.GetBoolProp(
                propNoReply, bNoReply );
            if( SUCCEEDED( iRet ) && bNoReply )
                break;
        }

        ObjPtr pObj; 
        CStdRTMutex oLock( pSyncCall->GetLock() );
        // CCfgOpener is faster than CCfgOpenerObj
        CCfgOpener oTask(
            ( IConfigDb* )pTask->GetConfig() ); 
        ret = oTask.GetObjPtr( propRespPtr, pObj ); 
        oLock.Unlock();
        if( ERROR( ret ) )
        {
            DebugPrint( ret, "fatal error, about to quit, tid=%d\n",
                rpcf::GetTid() );
            break;
        }
        pResp = pObj; 
        CParamList oNewResp( ( IConfigDb* )pResp );
        if( !oNewResp.exist( propReturnValue ) )
        {
            ret = ERROR_FAIL;
            break;
        }
        ret = oNewResp[ propReturnValue ];

    }while( 0 );

    return ret;
}

template< typename ...Args >
gint32 CInterfaceServer::SendEventEx(
    IEventSink* pCallback, // optional
    IConfigDb* pOptions,
    EnumClsid iid,
    const std::string& strMethod,
    const std::string& strDest, // optional
    Args&&... args )
{
    guint32 ret = 0;
    do{
        if( strMethod.empty() ||
            iid == clsid( Invalid ) )
        {
            ret = -EINVAL;
            break;
        }

        CReqBuilder oReq( this );
        if( true )
        {
            if( pOptions != nullptr )
            {
                oReq.CopyProp(
                    propSeriProto, pOptions );
            }
            CStdRMutex oIfLock( GetLock() );
            MatchPtr pIfMatch;
            for( auto pMatch : m_vecMatches )
            {
                guint32 iidMatch = clsid( Invalid );
                CCfgOpenerObj oMatch( ( CObjBase* )pMatch );

                ret = oMatch.GetIntProp(
                    propIid, iidMatch );

                if( ERROR( ret ) )
                    continue;

                if( iidMatch == iid )
                {
                    pIfMatch = pMatch;
                    break;
                }
            }

            if( pIfMatch.IsEmpty() )
            {
                ret = -ENOENT;
                break;
            }
            
            std::string strIfName;
            CMessageMatch* pMsgMatch = pIfMatch;
            if( pMsgMatch == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            pMsgMatch->GetIfName( strIfName );
            oReq.SetIfName( strIfName );
        }

        if( !strDest.empty() )
            oReq.SetDestination( strDest );
        
        // we don't expect a response
        oReq.SetCallFlags( 
           DBUS_MESSAGE_TYPE_SIGNAL
           | CF_ASYNC_CALL );

        oReq.SetMethodName( strMethod );

        std::vector< BufPtr > vecArgs;
        PackParams( vecArgs, args... );

        for( auto pBuf : vecArgs )
            oReq.Push<BufPtr&>( pBuf );

        if( pCallback == nullptr )
        {
            TaskletPtr pTask;
            pTask.NewObj( clsid( CIfDummyTask ) );
            BroadcastEvent( oReq.GetCfg(), pTask );
        }
        
    }while( 0 );

    return ret;
}

template< typename ...Args >
gint32 CInterfaceServer::SendEvent(
    IEventSink* pCallback, // optional
    EnumClsid iid,
    const std::string& strMethod,
    const std::string& strDest, // optional
    Args&&... args )
{
    return SendEventEx( pCallback,
        nullptr, iid, strMethod, strDest, args... );
}

#define BROADCAST_USER_EVENT( _iid_, ... ) \
    SendEvent( nullptr, _iid_, USER_EVENT( __func__ ), "" VA_ARGS( __VA_ARGS__ )  );

#define BROADCAST_SYS_EVENT( _iid_, ... ) \
    SendEvent( nullptr, _iid_, SYS_EVENT( __func__ ), "" VA_ARGS( __VA_ARGS__ )  );

template <typename...>
struct Parameters;
 
template < typename...Types, typename ...Types2 >
struct Parameters< std::tuple< Types... >, std::tuple< Types2... > >
{
    CInterfaceProxy* m_pIf = nullptr;
    std::string m_strMethod;
    CfgPtr m_pCfg;

    Parameters( CInterfaceProxy* pProxy,
        const std::string strMehtod,
        IConfigDb* pCfg = nullptr )
    {
        m_pIf = pProxy;
        m_strMethod = strMehtod;
        if( pCfg != nullptr )
            m_pCfg = pCfg;
    }

    gint32 SendReceive(
        Types&&...inArgs, Types2&&... outArgs )
    {
        gint32 ret = 0;

        do{
            CfgPtr pResp;
            CParamList oCfg( ( IConfigDb* )m_pCfg );
            EnumClsid iid = clsid( Invalid );
            if( m_pCfg.IsEmpty() )
            {
                iid = m_pIf->GetClsid();
            }
            else
            {
                guint32* pIid = ( guint32* )&iid;
                ret = oCfg.GetIntProp(
                    propIid, *pIid );
                if( ERROR( ret ) )
                    iid = m_pIf->GetClsid();
            }

            CParamList oOptions;
            std::string strIfName =
                CoGetIfNameFromIid( iid, "p" );

            oOptions[ propIfName ] =
                DBUS_IF_NAME( strIfName );

            if( !m_pCfg.IsEmpty() )
            {
                m_pIf->CopyUserOptions(
                    oOptions.GetCfg(), m_pCfg );
                oOptions.CopyProp( propSysMethod,
                    ( IConfigDb* )m_pCfg );
            }

            // make the call
            ret = m_pIf->SyncCallEx( oOptions.GetCfg(),
                pResp, m_strMethod, inArgs... );

            if( ERROR( ret ) )
                break;

            // fill the output parameter
            gint32 iRet = 0;
            try{
                ret = m_pIf->FillArgs(
                    pResp, iRet, outArgs... );
            }
            catch( const std::bad_alloc& e )
            {
                ret = -ENOMEM;
            }
            catch( const std::runtime_error& e )
            {
                ret = -EFAULT;
            }
            catch( const std::invalid_argument& e )
            {
                ret = -EINVAL;
            }

            if( SUCCEEDED( ret ) )
                ret = iRet;

        }while( 0 );

        return ret;
    }
};

/**
* @name ProxyCall: the entry point as sync request senders.
* @{ */
/**
 * Template paramaters:
 *  iNumInput: number of input arguments
 *
 *  Args: The types of each individual input & output
 *      parameters If a parameter serves as input/output,
 *      its type should appear twice in the Args. the
 *      iNumInput of input parameters comes first and the
 *      output parameters follows in the order the
 *      parameters sent over the network.
 *
 * Arguments:
 *  p: a placeholder for template deduction. nullptr is
 *  fine.
 *
 *  strMethod: the name of the method, as you have put
 *      in the InitUserFuncs
 *
 *  args : the formal parameter list, corresponding to
 *      the types in the template parameters.
 * @} */

template< int iNumInput, typename...Args >
gint32 CInterfaceProxy::ProxyCall(
    InputCount< iNumInput > *p,
    const std::string& strMethod,
    Args&&... args )
{ 
    using OutTypes = typename OutputParamTypes< iNumInput, Args... >::OutputTypes;
    using InTypes = typename InputParamTypes< iNumInput, Args... >::InputTypes;
    if( strMethod == "EmptyFunc" )
        return -EINVAL;

    IConfigDb* pCfg = nullptr;
    if( p != nullptr && !p->m_pOptions.IsEmpty() )
        pCfg = p->m_pOptions;

    Parameters< InTypes, OutTypes > a( this, strMethod, pCfg );
    return a.SendReceive( args... );
}

#define FORWARD_CALL( iNumInput, strMethod, ... ) \
    ProxyCall( _N( iNumInput ), strMethod, ##__VA_ARGS__ )

#define FORWARD_SYSIF_CALL( _iid_, iNumInput, strMethod, ... ) \
({  CParamList oParams; \
    oParams[ propIid ] = ( _iid_ ); \
    oParams[ propSysMethod ] = true; \
    InputCount< iNumInput > a( ( oParams.GetCfg() ) ); \
    ProxyCall( &a, strMethod, ##__VA_ARGS__ );} )

#define FORWARD_IF_CALL( _iid_, iNumInput, strMethod, ... ) \
({  CParamList oParams; \
    oParams[ propIid ] = ( _iid_ ); \
    oParams[ propSysMethod ] = false; \
    InputCount< iNumInput > a( ( oParams.GetCfg() ) ); \
    ProxyCall( &a, strMethod, ##__VA_ARGS__ );} )

#define DEFINE_HAS_METHOD( MethodName, _rettype, ... ) \
template< typename T > \
struct has_##MethodName\
{\
    private:\
    template<typename U, _rettype (U::*)( __VA_ARGS__ ) > struct SFINAE {};\
    template<typename U > static char Test(SFINAE<U, &U::MethodName>*);\
    template<typename U> static int Test(...);\
    template< typename U, typename V = typename std::enable_if<std::is_base_of< virtbase, U >::value, U >::type > \
    static constexpr bool InitValue() {\
        return ( ( sizeof(Test<T>(0) ) == sizeof(char) ) || \
            sizeof( char ) == has_##MethodName< typename T::super >::value  ); \
    }\
    template< typename U, typename V = typename std::enable_if<!std::is_base_of< virtbase, U >::value, U >::type, typename W=U > \
    static constexpr bool InitValue() { return false; }\
    public:\
    static bool const value = InitValue< T >();\
}; \


#define VA_LIST(...) __VA_ARGS__
// this macro serves to define the master virtual
// method to all the overridden virtual methods
// from the interfaces on this object which, in
// turn, overrides the base class(
// CInterfaceServer or CInterfaceProxy ) if
// defined.
#define ITERATE_IF_VIRT_METHODS_IMPL( _MethodName, rettype, PARAMS, ARGS ) \
    private: \
    DEFINE_HAS_METHOD( _MethodName, rettype, PARAMS ); \
    gint32 Interf##_MethodName( NumberSequence<>  ) \
    { return 0; } \
    template < int N > \
    gint32 Interf##_MethodName( \
         NumberSequence< N >, PARAMS ) \
    { \
        using ClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        if( has_##_MethodName< ClassName >::value ) \
            return this->ClassName::_MethodName( ARGS ); \
        return 0; \
    } \
    template < int N, int M, int...S > \
    gint32 Interf##_MethodName( \
        NumberSequence< N, M, S... >, PARAMS ) \
    { \
        using ClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        if( has_##_MethodName< ClassName >::value ) \
        { \
            gint32 ret = this->ClassName::_MethodName( ARGS ); \
            if( ERROR( ret ) ) \
                return ret; \
        } \
        return Interf##_MethodName( NumberSequence<M, S...>(), ARGS ); \
    } \
    public: \
    virtual rettype _MethodName( PARAMS ) \
    { \
        gint32 ret = virtbase::_MethodName( ARGS ); \
        if( ERROR( ret ) ) \
            return ret; \
        if( sizeof...( Types ) ) \
        { \
            using seq = typename GenSequence< sizeof...( Types ) >::type; \
            ret = Interf##_MethodName( seq(), ARGS ); \
        } \
        return ret; \
    }

#define ITERATE_IF_VIRT_METHODS_IMPL_NOARG( _MethodName, rettype ) \
    private: \
    DEFINE_HAS_METHOD( _MethodName, rettype ); \
    gint32 Interf##_MethodName( \
        NumberSequence<>  ) \
    { return 0; } \
    template < int N > \
    gint32 Interf##_MethodName( \
         NumberSequence< N > ) \
    { \
        using ClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        if( has_##_MethodName< ClassName >::value ) \
            return this->ClassName::_MethodName(); \
        return 0; \
    } \
    template < int N, int M, int...S > \
    gint32 Interf##_MethodName( \
        NumberSequence< N, M, S... > ) \
    { \
        using ClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        if( has_##_MethodName< ClassName >::value ) \
        { \
            gint32 ret = this->ClassName::_MethodName(); \
            if( ERROR( ret ) ) \
                return ret; \
        } \
        return Interf##_MethodName( NumberSequence<M, S...>() ); \
    } \
    public: \
    virtual rettype _MethodName() \
    { \
        gint32 ret = virtbase::_MethodName(); \
        if( ERROR( ret ) ) \
            return ret; \
        if( sizeof...( Types ) ) \
        { \
            using seq = typename GenSequence< sizeof...( Types ) >::type; \
            ret = Interf##_MethodName( seq() ); \
        } \
        return ret; \
    }

// the master handler for overrides of the
// CInterfaceServer, note that there must be a
// parameter `IEventSink* pCallback' in the PARAMS list
#define DEFINE_UNIQUE_HANDLER_IMPL( _MethodName, rettype, PARAMS, ARGS ) \
    private: \
    gint32 Handler##_MethodName( NumberSequence<>  ) \
    { return ERROR_NOT_HANDLED; } \
    template < int N > \
    gint32 Handler##_MethodName( \
         NumberSequence< N >, PARAMS ) \
    { \
        EnumClsid iid = _GETIID_INV( pCallback ); \
        using ClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        if( ClassName::GetIid() == iid || ClassName::SupportIid( iid ) ) \
            return this->ClassName::_MethodName( ARGS ); \
        return ERROR_NOT_HANDLED; \
    } \
    template < int N, int M, int...S > \
    gint32 Handler##_MethodName( \
        NumberSequence< N, M, S... >, PARAMS ) \
    { \
        using ClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        EnumClsid iid = _GETIID_INV( pCallback ); \
        if( ClassName::GetIid() == iid || ClassName::SupportIid( iid ) ) \
            return this->ClassName::_MethodName( ARGS ); \
        return Handler##_MethodName( NumberSequence<M, S...>(), ARGS ); \
    } \
    public: \
    virtual rettype _MethodName( PARAMS ) \
    { \
        gint32 ret = 0;\
        if( sizeof...( Types ) ) \
        { \
            using seq = typename GenSequence< sizeof...( Types ) >::type; \
            ret = Handler##_MethodName( seq(), ARGS ); \
        } \
        if( ret == ERROR_NOT_HANDLED ) \
            ret = virtbase::_MethodName( ARGS ); \
        return ret; \
    }

#define DEFINE_UNIQUE_FETCH_IMPL( _MethodName, rettype, PARAMS, ARGS ) \
    private: \
    gint32 Handler##_MethodName( NumberSequence<>  ) \
    { return ERROR_NOT_HANDLED; } \
    template < int N > \
    gint32 Handler##_MethodName( \
         NumberSequence< N >, PARAMS ) \
    { \
        EnumClsid iid = _GETIID( pDataDesc ); \
        using ClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        if( ClassName::GetIid() == iid || ClassName::SupportIid( iid ) ) \
            return this->ClassName::_MethodName( ARGS ); \
        return ERROR_NOT_HANDLED; \
    } \
    template < int N, int M, int...S > \
    gint32 Handler##_MethodName( \
        NumberSequence< N, M, S... >, PARAMS ) \
    { \
        using ClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        EnumClsid iid = _GETIID( pDataDesc ); \
        if( ClassName::GetIid() == iid || ClassName::SupportIid( iid ) ) \
            return this->ClassName::_MethodName( ARGS ); \
        return Handler##_MethodName( NumberSequence<M, S...>(), ARGS ); \
    } \
    public: \
    virtual rettype _MethodName( PARAMS ) \
    { \
        gint32 ret = ERROR_NOT_HANDLED;\
        if( sizeof...( Types ) ) \
        { \
            using seq = typename GenSequence< sizeof...( Types ) >::type; \
            ret = Handler##_MethodName( seq(), ARGS ); \
        } \
        if( ret == ERROR_NOT_HANDLED ) \
            ret = virtbase::_MethodName( ARGS ); \
        return ret; \
    }

template< typename DefaultType, typename DesiredType, typename ... Types >
struct TypeContained;

template< typename DefaultType, typename DesiredType, typename M, typename...Types >
struct TypeContained< DefaultType, DesiredType, M, Types... >
{
    template < typename, bool >
    struct BaseType;

    template < typename U >
    struct BaseType< U, true >
    {
        typedef U type;
    };

    template < typename U >
    struct BaseType< U, false >
    {
        typedef typename TypeContained< DefaultType, U, Types...>::mybasetype::type type;
    };

    typedef struct BaseType< DesiredType, std::is_same< DesiredType, M >::value ||
        std::is_base_of< DesiredType, M >::value > mybasetype;
};

template< typename DefaultType, typename DesiredType >
struct TypeContained< DefaultType, DesiredType >
{
    typedef struct BaseType{
        typedef DefaultType type;
    } mybasetype;
};

// Start point for aggregatable interface
struct CAggInterfaceServer :
    public CInterfaceServer
{
    typedef CInterfaceServer super;
    CAggInterfaceServer( const IConfigDb* pCfg )
        : CInterfaceServer( pCfg )
    {}
    virtual const EnumClsid GetIid() const = 0;
    // for those interface who inherits other
    // interfaces
    virtual bool SupportIid( EnumClsid ) const
    { return false; }
};

class CStreamServer;
class CFileTransferServer;

template< typename virtbase, typename...Types >
struct CAggregatedObject
    : Types...
{
    public:
    CAggregatedObject( const IConfigDb* pCfg )
    : virtbase( pCfg ), Types( pCfg )...
    {
    }

};

template< typename Type >
gint32 GetIidOfType( std::vector< guint32 >& vecIids, Type* pType )
{
    vecIids.push_back( pType->Type::GetIid() );
    return 0;
}

}

#include "frmwrk.h"


namespace rpcf
{

// _pos is the position for the callback 'pCallback', which must be present in the
// ARGS, otherwise the macro does not compile.
#define ITERATE_IF_VIRT_METHODS_ASYNC_IMPL( _pos, _MethodName, rettype, PARAMS, ARGS ) \
    private: \
    DEFINE_HAS_METHOD( _MethodName, rettype, PARAMS ); \
    gint32 AsyncInterf##_MethodName( NumberSequence<>  ) \
    { return 0; } \
    template < int N > \
    gint32 AsyncInterf##_MethodName( \
         std::vector< gint32 (*)( ThisType*, PARAMS )>& _vec, NumberSequence< N >, PARAMS ) \
    { \
        using IfClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        if( has_##_MethodName< IfClassName >::value ) \
            _vec.push_back( []( ThisType* p, PARAMS ){ return p->IfClassName::_MethodName( ARGS ); } ); \
        return 0; \
    } \
    template < int N, int M, int...S > \
    gint32 AsyncInterf##_MethodName( \
        std::vector< gint32 (*)( ThisType*, PARAMS )>& _vec, NumberSequence< N, M, S... >, PARAMS ) \
    { \
        using IfClassName = typename std::tuple_element< N, std::tuple<Types...>>::type; \
        if( has_##_MethodName< IfClassName >::value ) \
            _vec.push_back( []( ThisType* p, PARAMS ){ return p->IfClassName::_MethodName( ARGS ); } ); \
        return AsyncInterf##_MethodName( _vec, NumberSequence<M, S...>(), ARGS ); \
    } \
    rettype _MethodName##Hidden( \
         LONGWORD ptr, PARAMS ){ \
            gint32 (*p)( ThisType*, PARAMS ) = ( gint32 (*)(ThisType*, PARAMS ) )ptr;\
            return p( this, ARGS );\
        } \
    public: \
    virtual rettype _MethodName( PARAMS ) \
    { \
        /* main entrance of _MethodName */ \
        std::vector< gint32 (*)( ThisType*, PARAMS )> _vec;\
        TaskGrpPtr pTaskGrp;\
        CParamList oParams;\
        oParams[ propIfPtr ] = ObjPtr( this );\
        gint32 ret = pTaskGrp.NewObj(\
            clsid( CIfTaskGroup ),oParams.GetCfg() );\
        _vec.push_back( []( ThisType* p, PARAMS ){ return p->virtbase::_MethodName( ARGS ); } ); \
        if( ERROR( ret ) ) \
            return ret; \
        if( sizeof...( Types ) ) \
        { \
            using seq = typename GenSequence< sizeof...( Types ) >::type; \
            AsyncInterf##_MethodName( _vec, seq(), ARGS ); \
            if( _vec.empty() )\
                return 0;\
            for( auto& elem : _vec ) { \
                TaskletPtr pTask;\
                ret = DEFER_IFCALLEX_NOSCHED2( \
                    _pos + 1, pTask, \
                    ObjPtr( this ), \
                    &ThisType::_MethodName##Hidden, \
                    ( LONGWORD )elem, ARGS ); \
                if( ERROR( ret ) ) break; \
                pTaskGrp->AppendTask( pTask ); \
            } \
            if( ERROR( ret ) ) \
            { \
                ( *pTaskGrp )( eventCancelTask ); \
                return ret; \
            } \
            CIfRetryTask* pTask = ObjPtr( pTaskGrp ); \
            auto tup = std::make_tuple( ARGS ); \
            pTask->SetClientNotify( std::get< _pos >( tup ) ); \
            TaskletPtr pGrp = ObjPtr( pTask ); \
            ( *pTask )( eventZero ); \
            ret = pTask->GetError(); \
        } \
        return ret; \
    }

template< typename...Types >
struct CAggregatedObject< CAggInterfaceServer, Types... >
    : Types...
{
    using virtbase = CAggInterfaceServer;
    using ThisType = CAggregatedObject< CAggInterfaceServer, Types... >;

    public:
    typedef virtbase super;
    CAggregatedObject( const IConfigDb* pCfg )
    : virtbase( pCfg ), Types( pCfg )...
    {
    }

    ITERATE_IF_VIRT_METHODS_IMPL_NOARG( InitUserFuncs, gint32 )

    ITERATE_IF_VIRT_METHODS_IMPL( OnPreStart, gint32,
        VA_LIST( IEventSink* pCallback ), VA_LIST( pCallback ) )

    ITERATE_IF_VIRT_METHODS_ASYNC_IMPL( 0, OnPostStart, gint32,
        VA_LIST( IEventSink* pCallback ), VA_LIST( pCallback ) )

    ITERATE_IF_VIRT_METHODS_IMPL( OnPostStop, gint32,
        VA_LIST( IEventSink* pCallback ), VA_LIST( pCallback ) )

    ITERATE_IF_VIRT_METHODS_IMPL( AddStartTasks, gint32,
        VA_LIST( IEventSink* pTaskGrp ), VA_LIST( pTaskGrp ) )

    ITERATE_IF_VIRT_METHODS_IMPL( AddStopTasks, gint32,
        VA_LIST( IEventSink* pTaskGrp ), VA_LIST( pTaskGrp ) )

    ITERATE_IF_VIRT_METHODS_ASYNC_IMPL( 0, OnPreStop, gint32,
        VA_LIST( IEventSink* pCallback ), VA_LIST( pCallback ) )

    const EnumClsid GetIid() const
    { return this->GetClsid(); }

    const gint32 GetIids( std::vector< guint32 >& vecIids ) const
    {
        std::make_tuple( GetIidOfType( vecIids, static_cast< const Types* >( this ) )... );
        if( vecIids.empty() )
            return -ENOENT;
        return 0;
    }

    private:
    inline EnumClsid _GETIID_INV( IEventSink* pCallback )
    {
        if( pCallback == nullptr )
            return clsid( Invalid );

        TaskletPtr pTask;
        pTask = ObjPtr( pCallback );
        if( pTask.IsEmpty() )
            return clsid( Invalid );

        CIfInvokeMethodTask* pInvTask = pTask;
        if( pInvTask == nullptr )
            return clsid( Invalid );

        EnumClsid iid = clsid( Invalid );
        gint32 ret = pInvTask->GetIid( iid );
        if( ERROR( ret ) )
            return clsid( Invalid );

        return iid;
    }

    inline EnumClsid _GETIID( IConfigDb* pDataDesc )
    {
        CCfgOpener oDataDesc( pDataDesc );
        EnumClsid iid = clsid( Invalid );
        guint32* piid = ( guint32* )&iid;
        oDataDesc.GetIntProp( propIid, *piid );
        return iid;
    }

    protected:
    DEFINE_UNIQUE_FETCH_IMPL( FetchData_Server, gint32,
        VA_LIST( IConfigDb* pDataDesc, gint32& fd, guint32& dwOffset, guint32& dwSize, IEventSink* pCallback ),
        VA_LIST( pDataDesc, fd, dwOffset, dwSize, pCallback ) )
};

// a interface as a start point for interface query
struct IUnknown
{
    virtual gint32 EnumInterfaces(
        IEventSink* pCallback,
        IntVecPtr& pClsids ) = 0;
};

#define METHOD_EnumInterfaces "EnumInterfaces"

#define DECLARE_AGGREGATED_SERVER( ClassName, ... )\
struct ClassName : CAggregatedObject< CAggInterfaceServer, ##__VA_ARGS__ >, IUnknown\
{\
    using virtbase = CAggInterfaceServer; \
    typedef CAggregatedObject< virtbase, ##__VA_ARGS__ > super;\
    ClassName( const IConfigDb* pCfg )\
        : virtbase( pCfg ), super( pCfg )\
    { this->SetClassId( clsid( ClassName ) ); }\
    gint32 InitUserFuncs()\
    {\
        gint32 ret = super::InitUserFuncs(); \
        if( ERROR( ret ) ) return ret; \
        BEGIN_HANDLER_MAP;\
        END_HANDLER_MAP;\
        \
        BEGIN_IFHANDLER_MAP( IUnknown );\
        ADD_USER_SERVICE_HANDLER_EX( 0,\
            ClassName::EnumInterfaces,\
            METHOD_EnumInterfaces );\
        END_IFHANDLER_MAP; \
        return 0; \
    }\
    gint32 EnumInterfaces(\
        IEventSink* pCallback,\
        IntVecPtr& pClsids )\
    {\
        if( pClsids.IsEmpty() )\
            pClsids.NewObj();\
        ( *pClsids )().push_back( GetClsid() );\
        ( *pClsids )().push_back( iid( IInterfaceServer ) );\
        ( *pClsids )().push_back( iid( IUnknown ) );\
        GetIids( ( *pClsids)() );\
        return 0;\
    }\
}

// Start point for aggregatable interface
struct CAggInterfaceProxy :
    public CInterfaceProxy
{
    typedef CInterfaceProxy super;
    CAggInterfaceProxy( const IConfigDb* pCfg )
        : CInterfaceProxy( pCfg )
    {}
    virtual const EnumClsid GetIid() const = 0;
    virtual bool SupportIid( EnumClsid ) const
    { return false; }
};

template< typename...Types >
struct CAggregatedObject< CAggInterfaceProxy, Types... >
    : Types...
{
    using virtbase = CAggInterfaceProxy;
    using ThisType = CAggregatedObject< CAggInterfaceProxy, Types... >;
    public:
    typedef virtbase super;
    CAggregatedObject( const IConfigDb* pCfg )
    : virtbase( pCfg ), Types( pCfg )...
    {
    }

    ITERATE_IF_VIRT_METHODS_IMPL_NOARG( InitUserFuncs, gint32 )

    ITERATE_IF_VIRT_METHODS_IMPL( OnPreStart, gint32,
        VA_LIST( IEventSink* pCallback ), VA_LIST( pCallback ) )

    ITERATE_IF_VIRT_METHODS_ASYNC_IMPL( 0, OnPostStart, gint32,
        VA_LIST( IEventSink* pCallback ), VA_LIST( pCallback ) )

    ITERATE_IF_VIRT_METHODS_IMPL( OnPostStop, gint32,
        VA_LIST( IEventSink* pCallback ), VA_LIST( pCallback ) )

    ITERATE_IF_VIRT_METHODS_IMPL( AddStartTasks, gint32,
        VA_LIST( IEventSink* pTaskGrp ), VA_LIST( pTaskGrp ) )

    ITERATE_IF_VIRT_METHODS_ASYNC_IMPL( 0, OnPreStop, gint32,
        VA_LIST( IEventSink* pCallback ), VA_LIST( pCallback ) )

    const EnumClsid GetIid() const
    { return this->GetClsid(); }

    const gint32 GetIids( std::vector< guint32 >& vecIids ) const
    {
        std::make_tuple( GetIidOfType( vecIids, static_cast< const Types* >( this ) )... );
        if( vecIids.empty() )
            return -ENOENT;
        return 0;
    }

    private:
    inline EnumClsid _GETIID( IConfigDb* pDataDesc )
    {
        CCfgOpener oDataDesc( pDataDesc );
        EnumClsid iid = clsid( Invalid );
        guint32* piid = ( guint32* )&iid;
        oDataDesc.GetIntProp( propIid, *piid );
        return iid;
    }

    public:
    DEFINE_UNIQUE_FETCH_IMPL( FetchData_Proxy, gint32,
        VA_LIST( IConfigDb* pDataDesc, gint32& fd, guint32& dwOffset, guint32& dwSize, IEventSink* pCallback ),
        VA_LIST( pDataDesc, fd, dwOffset, dwSize, pCallback ) )
};

#define DECLARE_AGGREGATED_PROXY( ClassName, ... )\
struct ClassName : CAggregatedObject< CAggInterfaceProxy, ##__VA_ARGS__ >\
{\
    using virtbase = CAggInterfaceProxy; \
    typedef CAggregatedObject< virtbase, ##__VA_ARGS__ > super;\
    ClassName( const IConfigDb* pCfg )\
        : virtbase( pCfg ), super( pCfg )\
    { this->SetClassId( clsid( ClassName ) ); }\
    gint32 InitUserFuncs()\
    {\
        gint32 ret = super::InitUserFuncs();\
        if( ERROR( ret ) ) return ret; \
\
        BEGIN_PROXY_MAP( false );\
        END_PROXY_MAP;\
\
        BEGIN_IFPROXY_MAP( IUnknown, false ); \
        ADD_USER_PROXY_METHOD_EX( 0,\
            ClassName::EnumInterfaces,\
            METHOD_EnumInterfaces );\
        END_IFPROXY_MAP;\
        return 0; \
    }\
    gint32 EnumInterfaces(\
        IntVecPtr& pClsids )\
    {\
        return FORWARD_IF_CALL( iid( IUnknown ), 0, METHOD_EnumInterfaces, pClsids );\
    }\
}

// declare the synchronous proxy methods
#define DECL_PROXY_METHOD_SYNC( i, _fname, ... ) \
    public:\
    template < int iNum, typename ...ARGs > struct Class##_fname { \
        _ThisClass* m_pIf;\
        Class##_fname( const Class##_fname& oIf ) : Class##_fname( oIf.m_pIf ){} \
        Class##_fname( _ThisClass* pIf ) : m_pIf( pIf ){ \
            EnumClsid _iIfId_ = m_pIf->_MyClsid();\
            _ThisClass* _pIf = m_pIf;\
            do{BEGIN_PROXY_MAP_COMMON( false, _iIfId_, m_pIf ); \
            ADD_USER_PROXY_METHOD_EX( iNum, _ThisClass::dummy##_fname, std::string( #_fname ) ); \
            END_PROXY_MAP; \
        }\
        gint32 operator()( ARGs&&... args ) \
        { return m_pIf->ProxyCall( _N( iNum ), std::string( #_fname ), args... ); }\
    };\
    using TemplSpec##_fname = Class##_fname< i, ##__VA_ARGS__ >;\
    TemplSpec##_fname _fname = TemplSpec##_fname( this ) ; \
    gint32 dummy##_fname( __VA_ARGS__ ){ return 0; } \
    
// declare the synchronous proxy methods
#define DECL_IF_PROXY_METHOD_SYNC( i, _ifName, _fname, ... ) \
    public:\
    template < int iNum, typename ...ARGs > struct Class##_fname { \
        _ThisClass* m_pIf;\
        Class##_fname( const Class##_fname& oIf ) : Class##_fname( oIf.m_pIf ){} \
        Class##_fname( _ThisClass* pIf ) : m_pIf( pIf ){ \
            do{ CInterfaceProxy* _pIf = pIf;\
            BEGIN_IFPROXY_MAP_COMMON( _ifName, false, _pIf ); \
            ADD_USER_PROXY_METHOD_EX( iNum, _ThisClass::dummy##_fname, std::string( #_fname ) ); \
            END_IFPROXY_MAP;\
        }\
        gint32 operator()( ARGs&&... args ) \
        {\
            CParamList oParams; \
            oParams.SetIntProp( propIid, iid( _ifName ) ); \
            InputCount< iNum > p( oParams.GetCfg() ); \
            return m_pIf->ProxyCall( &p, std::string( #_fname ), args... ); \
        }\
    };\
    Class##_fname< i, ##__VA_ARGS__ > _fname=Class##_fname< i, ##__VA_ARGS__ >(this);\
    gint32 dummy##_fname( __VA_ARGS__ ){ return 0; } \

#define BEGIN_DECL_PROXY_SYNC( _ClassName, _SuperClass ) \
class _ClassName : public _SuperClass{ \
    EnumClsid _MyClsid() { return clsid( _ClassName ); };\
    public: \
    typedef _SuperClass super; \
    using _ThisClass=_ClassName;\
    _ClassName( const IConfigDb* pCfg ) : super( pCfg ) \
    { SetClassId( clsid( _ClassName ) ); } \

#define END_DECL_PROXY_SYNC( _ClassName ) \
};

#define BEGIN_DECL_IF_PROXY_SYNC( _IfName, _ClassName ) \
class _ClassName : public virtual CAggInterfaceProxy{ \
    public: \
    typedef CAggInterfaceProxy super; \
    using _ThisClass=_ClassName;\
    _ClassName( const IConfigDb* pCfg ):super( pCfg ){} \
    const EnumClsid GetIid() const { return iid( _IfName ); };

#define END_DECL_IF_PROXY_SYNC( _ClassName ) \
    END_DECL_PROXY_SYNC( _ClassName )

BEGIN_DECL_PROXY_SYNC( CSimpleSyncIf, CInterfaceProxy )
END_DECL_PROXY_SYNC( CSimpleSyncIf )

/**
* @name AddSeqTaskTempl
* @brief Add a task `pTask' to the taskgroup
* `pQueuedTasks', and if the pQueuedTasks is empty
* or completed, a new taskgroup instance will be
* created, and scheduled to run.
* @note Make sure the class T has both GetIoMgr and
* GetLock methods implemented.
*
* @{ */
/**  @} */

template< class T >
gint32 AddSeqTaskTempl( T* pObj,
    TaskGrpPtr& pQueuedTasks,
    TaskletPtr& pTask,
    bool bLong )
{
    if( pTask.IsEmpty() || pObj == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    bool bNew = false;
    do{
        TaskGrpPtr ptrSeqTasks;
        CIoManager* pMgr = pObj->GetIoMgr();

        CStdRMutex oIfLock( pObj->GetLock() );
        if( pQueuedTasks.IsEmpty() )
        {
            CParamList oParams;
            oParams[ propIoMgr ] = ObjPtr( pMgr );

            ret = pQueuedTasks.NewObj(
                clsid( CIfTaskGroup ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            pQueuedTasks->SetRelation(logicNONE );
            bNew = true;
        }
        ptrSeqTasks = pQueuedTasks;
        oIfLock.Unlock();

        CIfRetryTask* pSeqTasks = ptrSeqTasks;
        CStdRTMutex oQueueLock(
            pSeqTasks->GetLock() );

        oIfLock.Lock();
        if( pQueuedTasks.IsEmpty() )
            continue;

        if( !( pQueuedTasks == ptrSeqTasks ) )
            continue;

        ret = pQueuedTasks->AppendTask( pTask );
        if( ERROR( ret ) && !bNew )
        {
            // the old seqTask is completed
            pQueuedTasks.Clear();
            continue;
        }
        else if( ERROR( ret ) && bNew )
        {
            // something bad happened
            DebugPrint( ret,
                "a fatal error happens "
                "on sequential queue..." );
            pQueuedTasks.Clear();
            break;
        }

        pTask->MarkPending();
        if( SUCCEEDED( ret ) && bNew )
        {
            // a new pQueuedTasks, add and run
            oIfLock.Unlock();
            oQueueLock.Unlock();

            CRpcServices* pIf = ObjPtr( pObj );
            if( unlikely( pIf == nullptr ) )
            {
                TaskletPtr pGrpTask( pSeqTasks );
                ret = pMgr->RescheduleTask(
                    pGrpTask, bLong );
                break;
            }
            TaskletPtr pDeferTask;
            ret = DEFER_CALL_NOSCHED(
                pDeferTask, ObjPtr( pObj ),
                &CRpcServices::RunManagedTask,
                pQueuedTasks, false );

            if( ERROR( ret ) )
                break;

            ret = pMgr->RescheduleTask(
                pDeferTask, bLong );

            break;
        }
        if( SUCCEEDED( ret ) && !bNew )
        {
            // pQueuedTasks is already running
            // just waiting for the turn.
        }
        break;

    }while( 1 );

    return ret;
}

/*
 * macro ADD_TIMER adds a timer of sepcified
 * seconds, with the callback and its parameters.
 * The callback has a parameter list as 
 *
 * [ IEventSink*, IConfigDb* pReqCtx_, ... ]
 *
 * The first parameter can be ignored.  The
 * limitations of this macro are
 * . the variable `ret' must be defined ahead.
 * . the pReqCtx's attribute propContext will be
 * overwritten with a timer object for cleanup
 * purpose.
 */
#define ADD_TIMER( pThis, _pReqCtx_, _sec, callback_, ... ) \
({\
    TaskletPtr pTask_; \
    do{ \
        ret = DEFER_IFCALLEX_NOSCHED2( \
            0, pTask_, ObjPtr( pThis ), callback_, \
            nullptr, _pReqCtx_, ##__VA_ARGS__ ); \
        if( ERROR( ret ) ) \
            break; \
        ObjPtr pTaskObj = pTask_;\
        CParamList oReqCtx( pReqCtx_ );\
        oReqCtx.SetObjPtr( propContext, pTaskObj );\
        CIfDeferCallTaskEx* pTaskEx = pTask_;\
        pTaskEx->EnableTimer( _sec, eventRetry );\
    }while(0); \
    if( ERROR( ret ) ) pTask_.Clear();\
    pTask_; \
})

}
