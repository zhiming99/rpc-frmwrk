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
 * =====================================================================================
 */
#pragma once
#include "proxy.h"

// remove the reference or const modifiers from
// the type list
#define ValType( _T ) typename std::remove_cv< typename std::remove_pointer< typename std::decay< _T >::type >::type>::type

// cast to stdstr temporary
template< typename T,
    typename Allowed = typename std::enable_if<
        std::is_same< stdstr, T >::value, T >::type >
T CastTo( BufPtr& pBuf )
{
    return stdstr( pBuf->ptr() );
}

// cast to prime type reference except char* or const
// char* or stdstr
template< typename T,
    typename Allowed = typename std::enable_if<
        !std::is_base_of< IAutoPtr, ValType( T ) >::value && 
        !std::is_base_of< CObjBase, ValType( T ) >::value && 
        !std::is_same< stdstr, DecType( T ) >::value &&
        !std::is_same< char*, DecType( T ) >::value &&
        !std::is_same< const char*, DecType( T ) >::value, T >::type >
T& CastTo( BufPtr& pBuf )
{
    return ( T& )*pBuf;
}

// cast to CObjBase reference
template< typename T,
    typename T1= typename std::enable_if<
        !std::is_same< DBusMessage, T >::value, T>::type,
    typename T2= typename std::enable_if<
        std::is_base_of< CObjBase, T >::value, T >::type,
    typename T3=T >
T& CastTo( BufPtr& pBuf )
{
    CObjBase* p = (ObjPtr&)*pBuf;
    T* pT = static_cast< T* >( p );
    return *pT;
}

// cast to ObjPtr or DMsgPtr reference
template< typename T,
    typename Allowed= typename std::enable_if<
        std::is_same< DMsgPtr, T >::value ||
        std::is_same< ObjPtr, T >::value ||
        std::is_same< BufPtr, T >::value, T
        >::type,
        typename T2=Allowed,
        typename T3=T,
        typename T4=T
        >
T& CastTo( BufPtr& pBuf )
{
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
    ObjPtr* ppObj = ( ObjPtr* )pBuf->ptr();
    T1 oT( *ppObj );
    return oT;
}

#define DFirst DecType( First )
#define DSecond DecType( Second )
#define DTypes DecType( Types )

#define ValTypeFirst ValType( First )

template< typename First,
    typename Allowed = typename std::enable_if<
        !std::is_base_of< CObjBase, ValTypeFirst >::value &&
        !std::is_same< BufPtr, ValTypeFirst >::value &&
        !std::is_same< stdstr, ValTypeFirst >::value &&
        !std::is_same< char, ValTypeFirst >::value, First >
        ::type >
ValTypeFirst* CastTo( const BufPtr& pCBuf )
{
    BufPtr& pBuf = const_cast<BufPtr&>( pCBuf );
    DFirst& val = *pBuf;
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
    DFirst val = pBuf->ptr();
    return val;
}

// cast to CObjBase*
template< typename First,
    typename Allowed = typename std::enable_if<
        std::is_base_of< CObjBase, ValTypeFirst >::value &&
        std::is_pointer< First >::value, First >::type,
    typename T3=Allowed,
    typename T4=Allowed >
DFirst CastTo( BufPtr& pCBuf )
{
    BufPtr& pBuf = const_cast<BufPtr&>( pCBuf );
    ObjPtr& pObj = *pBuf;
    DFirst val = pObj;
    return val;
}

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
        guint32 dwParam1 = 0,
        guint32 dwParam2 = 0,
        guint32* pData = NULL  )
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
    typename T1 = typename std::enable_if<
        std::is_base_of<CObjBase, T>::value, T >::type >
BufPtr PackageTo( T* pObj )
{
    BufPtr pBuf( true );
    *pBuf = ObjPtr( pObj );
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

    void TupleToVec( std::tuple<>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence<> )
    {}

    template < int N >
    void TupleToVec( std::tuple< Args...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N > )
    {
        vec[ N ] = PackageTo< ValType( decltype( std::get< N >( oTuple ) ) ) >
            ( std::get< N >( oTuple ) );
    }

    template < int N, int M, int...S >
    void TupleToVec( std::tuple< Args...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N, M, S... > )
    {
        vec[ N ] = PackageTo< ValType( decltype( std::get< N >( oTuple ) ) )>
            ( std::get< N >( oTuple ) );
        TupleToVec( oTuple, vec, NumberSequence<M, S...>() );
    }

    gint32 operator()(
        CInterfaceProxy* pIf,
        IEventSink* pCallback,
        guint64& qwIoTaskId,
        Args... args )
    {
        if( pIf == nullptr )
            return -EINVAL;

        std::tuple<Args...> oTuple( args ... );
        std::vector< BufPtr > vec;

        if( sizeof...( Args ) )
        {
            vec.resize( sizeof...(Args) );
            TupleToVec( oTuple, vec,
                typename GenSequence< sizeof...( Args ) >::type() );
        }

        return pIf->SendProxyReq( pCallback,
            m_bNonDBus, m_strMethod, vec, qwIoTaskId );
    }
};

// construction of the major interface proxy map 
#define BEGIN_PROXY_MAP( bNonDBus ) \
do{ \
    EnumClsid _iIfId_ = GetClsid();\
    CCfgOpenerObj oCfg( this ); \
    std::string strIfName; \
    oCfg.GetStrProp( propIfName, strIfName ); \
    strIfName = IF_NAME_FROM_DBUS( strIfName ); \
    ToInternalName( strIfName );\
    CoAddIidName( strIfName, _iIfId_ ); \
    PROXY_MAP _oAnon_; \
    PROXY_MAP* _pMapProxies_ = &_oAnon_; \
    bool _bNonBus_ = bNonDBus; \
    PROXY_MAP* _pMap_ = nullptr; \
    do{ \
        _pMap_ = GetProxyMap( _iIfId_ ); \
        if( _pMap_ != nullptr ) \
            _pMapProxies_ = _pMap_; \
    }while( 0 )

template< typename...Args >
inline ObjPtr NewMethodProxy( bool bNonDBus,
    const std::string strName )
{
    ObjPtr pObj = new CMethodProxy< DecType( Args )...>
        ( bNonDBus, strName );

    pObj->DecRef();
    return pObj;
}

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
            SetProxyMap( *_pMapProxies_, _iIfId_ ); \
    }while( 0 ); \
}while( 0 )

// construction of the normal interface proxy map 
#define BEGIN_IFPROXY_MAP( _InterfName_, bNonDBus ) \
do{ \
    EnumClsid _iIfId_ = iid( _InterfName_ ); \
    std::string strIfName = #_InterfName_; \
    ToInternalName( strIfName ); \
    CoAddIidName( strIfName, _iIfId_ ); \
\
    PROXY_MAP _oAnon_; \
    PROXY_MAP* _pMapProxies_ = &_oAnon_; \
    bool _bNonBus_ = bNonDBus; \
    PROXY_MAP* _pMap_ = nullptr; \
    do{ \
        _pMap_ = GetProxyMap( _iIfId_ ); \
        if( _pMap_ != nullptr ) \
            _pMapProxies_ = _pMap_; \
    }while( 0 )

#define END_IFPROXY_MAP END_PROXY_MAP

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
    typedef gint32 ( ClassName::* FuncType)(
        IEventSink* pCallback, Args... ) ;

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

    // This is a virtual function in the template
    // class, where two styles of Polymorphisms meet,
    // virtual functions and template classes
    gint32 Delegate( 
        CObjBase* pObj,
        IEventSink* pCallback,
        std::vector< BufPtr >& vecParams )
    {
        if( pObj == nullptr ||
            pCallback == nullptr )
            return -EINVAL;

        ClassName* pClass = static_cast< ClassName* >( pObj );
        if( vecParams.size() != sizeof...( Args ) )
        {
            return -EINVAL;
        }

        std::vector< BufPtr > vecResp;
        for( auto& elem : vecParams )
            vecResp.push_back( elem );


        // std::tuple<typename std::decay<Args>::type...> oTuple =
        auto oTuple = VecToTuple< Args... >( vecResp );

        gint32 ret = CallUserFunc( pClass, pCallback, oTuple,
            typename GenSequence< sizeof...( Args ) >::type() );

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

// construction of the major interface handler map 

#define BEGIN_HANDLER_MAP_IID( _iid_ ) \
do{\
    EnumClsid _iIfId_ = ( _iid_ );\
    std::string strIfName; \
    CCfgOpenerObj oCfg( this ); \
    oCfg.GetStrProp( propIfName, strIfName ); \
    strIfName = IF_NAME_FROM_DBUS( strIfName ); \
    ToInternalName( strIfName ); \
    CoAddIidName( strIfName, _iIfId_ ); \
\
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
    ToInternalName( strIfName ); \
    CoAddIidName( strIfName, _iIfId_ ); \
\
    FUNC_MAP _oAnon_; \
    FUNC_MAP* _pCurMap_ = &_oAnon_; \
    FUNC_MAP* _pMap_ = nullptr; \
    do{ \
        _pMap_ = GetFuncMap( _iIfId_ ); \
        if( _pMap_ != nullptr ) \
            _pCurMap_ = _pMap_; \
    }while( 0 )

#define END_IFHANDLER_MAP END_HANDLER_MAP

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
        if( m_pObj.IsEmpty() ||
            m_vecArgs.empty() )
            return -EINVAL;

        return this->Delegate( m_pObj, m_vecArgs );
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

        gint32 ret = ( pObj->*m_pUserFunc )( *vecParams[ S ]... );

        return ret;
    }

    void TupleToVec( std::tuple<>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence<> )
    {}

    template < int N >
    void TupleToVec( std::tuple< Args...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N > )
    {
        vec[ N ] = PackageTo< ValType( decltype( std::get< N >( oTuple ) ) ) >
            ( std::get< N >( oTuple ) );
    }

    template < int N, int M, int...S >
    void TupleToVec( std::tuple< Args...>& oTuple,
        std::vector< BufPtr >& vec,
        NumberSequence< N, M, S... > )
    {
        vec[ N ] = PackageTo< ValType( decltype( std::get< N >( oTuple ) ) ) >
            ( std::get< N >( oTuple ) );
        TupleToVec( oTuple, vec, NumberSequence<M, S...>() );
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

        std::tuple< Args...> oTuple =
            std::make_tuple( args ... );

        std::vector< BufPtr > vec;

        if( sizeof...( args ) )
        {
            this->m_vecArgs.resize( sizeof...(Args) );
            TupleToVec( oTuple, this->m_vecArgs,
                typename GenSequence< sizeof...( Args ) >::type() );
        }
    }

    // well, this is a virtual function in the
    // template class. weird? the joint part where
    // two types of Polymorphisms meet, virtual
    // functions and template classes
    gint32 Delegate( CObjBase* pObj,
        std::vector< BufPtr >& vecParams )
    {
        if( pObj == nullptr )
            return -EINVAL;

        ClassName* pClass = dynamic_cast< ClassName* >( pObj );

        if( vecParams.size() != sizeof...( Args ) )
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

    if( pDeferredCall == nullptr )
        return -EFAULT;

    pCallback = pDeferredCall;
    pCallback->DecRef();
    return 0;
}

#define DEFER_CALL( pMgr, pObj, func, ... ) \
({ \
    TaskletPtr pTask; \
    gint32 ret_ = NewDeferredCall( pTask, pObj, func, __VA_ARGS__ ); \
    if( SUCCEEDED( ret ) ) \
        ret_ = pMgr->RescheduleTask( pTask ); \
    ret_; \
})

#define DEFER_CALL_NOSCHED( pTask, pObj, func, ... ) \
    NewDeferredCall( pTask, pObj, func, __VA_ARGS__ ); \

template<typename ClassName, typename ...Args>
class CDeferredCallOneshot :
    public CDeferredCall< CIfParallelTask, ClassName, Args... >
{
    typedef gint32 ( ClassName::* FuncType)( Args... ) ;

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
            iRet, 0, ( guint32* )pCallback );

        oCfg.RemoveProperty( propEventSink );

        return ret;
    }

    gint32 InterceptCallback( IEventSink* pCallback )
    {
        // change the propEventSink of the pCallback
        // with `this' task, and will call the original
        // propEventSink when we are done
        CCfgOpenerObj oTaskCfg( pCallback );
        CCfgOpener oCfg( ( IConfigDb* )this->GetConfig() );
        oCfg.CopyProp( propEventSink, pCallback );

        oTaskCfg.SetObjPtr(
            propEventSink, ObjPtr( this ) );

        oTaskCfg.SetBoolProp(
            propNotifyClient, true );

        return 0;
    }

    public:

    typedef CDeferredCall< CIfParallelTask, ClassName, Args... > super;

    CDeferredCallOneshot( IEventSink* pCallback,
        FuncType pFunc, ObjPtr& pObj, Args... args )
        : super( pFunc, pObj, args... )
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

        if( this->m_pObj.IsEmpty() )
            return -EINVAL;

        ret = this->Delegate(
            this->m_pObj, this->m_vecArgs );

        if( ret == STATUS_PENDING )
            return ret;

        if( pObjBase != nullptr )
            CallOrigCallback( pObjBase, iRet );

        return ret;
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

#define DEFER_CALL_ONESHOT( pTask, pEvent, pObj, func, ... ) \
do{ \
    NewDeferredCallOneshot( pEvent, pTask, pObj, func, __VA_ARGS__ ); \
    if( !pTask.IsEmpty() ) \
        ( *pTask )( eventZero ); \
}while( 0 )

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
                std::vector< guint32 > vecParams;

                gint32 ret = this->GetParamList( vecParams );

                if( ERROR( ret ) )
                    break;

                if( vecParams.size() < 4 )
                {
                    ret = -EINVAL;
                    break;
                }

                if( ERROR( vecParams[ 1 ] ) )
                {
                    ret = vecParams[ 1 ];
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

        gint32 ret = CallUserFunc( oTupleStore,
            typename GenSequence< sizeof...( Args ) >::type() );

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
            iRet, 0, ( guint32* )pCallback );

        oCfg.RemoveProperty( propEventSink );

        return ret;
    }

    gint32 InterceptCallback( IEventSink* pCallback )
    {
        // change the propEventSink of the pCallback
        // with `this' task, and will call the original
        // propEventSink when we are done
        CCfgOpenerObj oTaskCfg( pCallback );
        CCfgOpener oCfg( ( IConfigDb* )this->GetConfig() );
        oCfg.CopyProp( propEventSink, pCallback );

        oTaskCfg.SetObjPtr(
            propEventSink, ObjPtr( this ) );

        oTaskCfg.SetBoolProp(
            propNotifyClient, true );

        return 0;
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
            std::vector< guint32 > vecParams;

            ret = this->GetParamList( vecParams );

            if( ERROR( ret ) )
                break;

            if( vecParams.size() < 4 )
            {
                ret = -EINVAL;
                break;
            }

            pObjBase =
                reinterpret_cast< CObjBase* >( vecParams[ 3 ] );

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
    const std::string& strMethod,
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

        CParamList oResp( ( IConfigDb* )pResp );

        CParamList oParams(
            ( IConfigDb* )pTask->GetConfig() );

        oParams.CopyProp( propIfName,
            ( CObjBase* )pOptions );

        ret = CallUserProxyFunc( pTask, 
            qwIoTaskId, strMethod, args... ); 

        CIoReqSyncCallback* pSyncCall = pTask;
        if( ret == STATUS_PENDING ) 
        { 
            // return the task id for cancel purpose
            oResp[ propTaskId ] = qwIoTaskId;
            if( pSyncCall == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pSyncCall->WaitForComplete(); 

            if( SUCCEEDED( ret ) )
                ret = pSyncCall->GetError();
        } 
        if( ERROR( ret ) ) 
            break; 

        ObjPtr pObj; 
        CCfgOpenerObj oTask( ( CObjBase* )pTask ); 
        ret = oTask.GetObjPtr( propRespPtr, pObj ); 

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
gint32 CInterfaceServer::SendEvent(
    IEventSink* pCallback, // optional
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
        CMethodProxy< DecType( Args )...>* pProxy =
            new CMethodProxy< DecType( Args )... >( false, "test" );

        std::vector< BufPtr > vecArgs;
        vecArgs.resize( sizeof...(Args) );
        std::tuple< DecType( Args )...> oTuple( args ... );
        pProxy->TupleToVec( oTuple, vecArgs,
            typename GenSequence< sizeof...( Args ) >::type() );

        for( auto pBuf : vecArgs )
            oReq.Push<BufPtr&>( pBuf );

        if( pCallback == nullptr )
        {
            TaskletPtr pTask;
            pTask.NewObj( clsid( CIfDummyTask ) );
            BroadcastEvent( oReq.GetCfg(), pTask );
        }

        pProxy->Release();
        
    }while( 0 );

    return ret;
}

#define BROADCAST_USER_EVENT( _iid_, ... ) \
    SendEvent( nullptr, _iid_, USER_EVENT( __func__ ), "", __VA_ARGS__  );

#define BROADCAST_SYS_EVENT( _iid_, ... ) \
    SendEvent( nullptr, _iid_, SYS_EVENT( __func__ ), "", __VA_ARGS__  );

