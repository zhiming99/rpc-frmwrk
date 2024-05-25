// GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
// Copyright (C) 2024  zhiming <woodhead99@gmail.com>
// This program can be distributed under the terms of the GNU GPLv3.
// ridlc -O . ../oaproxy.ridl 
#include "rpc.h"
#include "iftasks.h"
using namespace rpcf;
#include "oa2check.h"

gint32 timespec::Serialize( BufPtr& pBuf_ )
{
    if( pBuf_.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        ret = CSerialBase::Serialize(
            pBuf_, m_dwMsgId );
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Serialize(
            pBuf_, tv_sec );
        if( ERROR( ret ) ) break;
        
    }while( 0 );

    return ret;
    
}

gint32 timespec::Deserialize( BufPtr& pBuf_ )
{
    if( pBuf_.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        guint32 dwMsgId = 0;
        ret = CSerialBase::Deserialize(
            pBuf_, dwMsgId );
        
        if( ERROR( ret ) ) return ret;
        if( m_dwMsgId != dwMsgId ) return -EINVAL;
        
        ret = CSerialBase::Deserialize(
            pBuf_, tv_sec );
        
        if( ERROR( ret ) ) break;
        
    }while( 0 );

    return ret;
    
}

timespec& timespec::operator=(
    const timespec& rhs )
{
    do{
        // data members
        if( GetObjId() == rhs.GetObjId() )
            break;
        
        tv_sec = rhs.tv_sec;
        
    }while( 0 );

    return *this;
}

gint32 USER_INFO::Serialize( BufPtr& pBuf_ )
{
    if( pBuf_.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        ret = CSerialBase::Serialize(
            pBuf_, m_dwMsgId );
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Serialize(
            pBuf_, strUserId );
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Serialize(
            pBuf_, strUserName );
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Serialize(
            pBuf_, strEmail );
        if( ERROR( ret ) ) break;
        
        tsExpireTime.SetIf( GetIf() );
        ret = SerialStruct( 
            pBuf_, tsExpireTime );
        if( ERROR( ret ) ) break;
        
    }while( 0 );

    return ret;
    
}

gint32 USER_INFO::Deserialize( BufPtr& pBuf_ )
{
    if( pBuf_.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        guint32 dwMsgId = 0;
        ret = CSerialBase::Deserialize(
            pBuf_, dwMsgId );
        
        if( ERROR( ret ) ) return ret;
        if( m_dwMsgId != dwMsgId ) return -EINVAL;
        
        ret = CSerialBase::Deserialize(
            pBuf_, strUserId );
        
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Deserialize(
            pBuf_, strUserName );
        
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Deserialize(
            pBuf_, strEmail );
        
        if( ERROR( ret ) ) break;
        
        tsExpireTime.SetIf( GetIf() );
        ret = DeserialStruct(
            pBuf_, tsExpireTime );
        if( ERROR( ret ) ) break;
        
    }while( 0 );

    return ret;
    
}

USER_INFO& USER_INFO::operator=(
    const USER_INFO& rhs )
{
    do{
        // data members
        if( GetObjId() == rhs.GetObjId() )
            break;
        
        strUserId = rhs.strUserId;
        strUserName = rhs.strUserName;
        strEmail = rhs.strEmail;
        tsExpireTime = rhs.tsExpireTime;
        
    }while( 0 );

    return *this;
}

gint32 OA2EVENT::Serialize( BufPtr& pBuf_ )
{
    if( pBuf_.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        ret = CSerialBase::Serialize(
            pBuf_, m_dwMsgId );
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Serialize(
            pBuf_, strUserId );
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Serialize(
            pBuf_, dwEventId );
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Serialize(
            pBuf_, strDesc );
        if( ERROR( ret ) ) break;
        
    }while( 0 );

    return ret;
    
}

gint32 OA2EVENT::Deserialize( BufPtr& pBuf_ )
{
    if( pBuf_.IsEmpty() )
        return -EINVAL;
    gint32 ret = 0;
    do{
        guint32 dwMsgId = 0;
        ret = CSerialBase::Deserialize(
            pBuf_, dwMsgId );
        
        if( ERROR( ret ) ) return ret;
        if( m_dwMsgId != dwMsgId ) return -EINVAL;
        
        ret = CSerialBase::Deserialize(
            pBuf_, strUserId );
        
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Deserialize(
            pBuf_, dwEventId );
        
        if( ERROR( ret ) ) break;
        
        ret = CSerialBase::Deserialize(
            pBuf_, strDesc );
        
        if( ERROR( ret ) ) break;
        
    }while( 0 );

    return ret;
    
}

OA2EVENT& OA2EVENT::operator=(
    const OA2EVENT& rhs )
{
    do{
        // data members
        if( GetObjId() == rhs.GetObjId() )
            break;
        
        strUserId = rhs.strUserId;
        dwEventId = rhs.dwEventId;
        strDesc = rhs.strDesc;
        
    }while( 0 );

    return *this;
}

gint32 IOAuth2Proxy_PImpl::InitUserFuncs()
{
    BEGIN_IFPROXY_MAP( OAuth2Proxy, false );

    ADD_USER_PROXY_METHOD_EX( 1,
        IOAuth2Proxy_PImpl::DoLoginDummy,
        "DoLogin" );

    ADD_USER_PROXY_METHOD_EX( 1,
        IOAuth2Proxy_PImpl::GetUserInfoDummy,
        "GetUserInfo" );

    ADD_USER_PROXY_METHOD_EX( 1,
        IOAuth2Proxy_PImpl::RevokeUserDummy,
        "RevokeUser" );

    END_IFPROXY_MAP;
    
    BEGIN_IFHANDLER_MAP( OAuth2Proxy );

    ADD_USER_EVENT_HANDLER(
        IOAuth2Proxy_PImpl::OnOA2EventWrapper,
        "OnOA2Event" );
    
    END_IFHANDLER_MAP;
    
    return STATUS_SUCCESS;
}

gint32 IOAuth2Proxy_PImpl::DoLogin( 
    IConfigDb* context,
    const std::string& strToken,
    bool& bValid )
{
    gint32 ret = 0;
    TaskletPtr pRespCb_;
    do{
        CParamList oOptions_;
        CfgPtr pResp_;
        oOptions_[ propIfName ] =
            DBUS_IF_NAME( "OAuth2Proxy" );
        oOptions_[ propSeriProto ] = 
            ( guint32 )seriRidl;
        
        CParamList oReqCtx_;
        ObjPtr pTemp( context );
        oReqCtx_.Push( pTemp );
        
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb_, ObjPtr( this ), 
            &IOAuth2Proxy_PImpl::DoLoginCbWrapper, 
            nullptr, oReqCtx_.GetCfg() );

        if( ERROR( ret ) ) break;
        
        //Serialize the input parameters
        BufPtr pBuf_( true );
        
        ObjPtr pSerialIf_(this);
        CSerialBase oSerial_( pSerialIf_ );
        do{
            ret = oSerial_.Serialize(
                pBuf_, strToken );
            if( ERROR( ret ) ) break;
            
            pBuf_->SetOffset( 0 );
        }while( 0 );

        if( ERROR( ret ) )
            return ret;
        
        ret = this->AsyncCall(
            ( IEventSink* )pRespCb_, 
            oOptions_.GetCfg(), pResp_,
            "DoLogin",
            pBuf_ );
        gint32 ret2 = pRespCb_->GetError();
        if( SUCCEEDED( ret ) )
        {
            if( ret2 != STATUS_PENDING )
            {
                // pRespCb_ has been called
                ret = STATUS_PENDING;
            }
            else
            {
                // immediate return
                ( *pRespCb_ )( eventCancelTask );
                pRespCb_.Clear();
            }
        }
        if( ret == STATUS_PENDING )
        {
            if( context == nullptr )
                break;
            CCfgOpener oContext( context );
            oContext.CopyProp(
                propTaskId, pResp_ );
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }
        // immediate return
        CCfgOpener oResp_( ( IConfigDb* )pResp_ );
        oResp_.GetIntProp(
            propReturnValue, ( guint32& )ret );
        if( ERROR( ret ) ) break;
        guint32 dwSeriProto_ = 0;
        ret = oResp_.GetIntProp(
            propSeriProto, dwSeriProto_ );
        if( ERROR( ret ) ||
            dwSeriProto_ != seriRidl )
            break;
        BufPtr pBuf2;
        ret = oResp_.GetBufPtr( 0, pBuf2 );
        if( ERROR( ret ) )
            break;
        
        ObjPtr pDeserialIf_(this);
        CSerialBase oDeserial_( pDeserialIf_ );
        do{
            guint32 dwOrigOff = pBuf2->offset();
            ret = oDeserial_.Deserialize(
                pBuf2, bValid );
            
            if( ERROR( ret ) ) break;
            
            pBuf2->SetOffset( dwOrigOff );
        }while( 0 );

        if( ERROR( ret ) )
            break;
            
        
    }while( 0 );
    
    if( ERROR( ret ) && !pRespCb_.IsEmpty() )
        ( *pRespCb_ )( eventCancelTask );
    
    return ret;
}
//Async callback wrapper
gint32 IOAuth2Proxy_PImpl::DoLoginCbWrapper( 
    IEventSink* pCallback, 
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    gint32 ret = 0;
    do{
        IConfigDb* pResp_ = nullptr;
        CCfgOpenerObj oReq_( pIoReq );
        ret = oReq_.GetPointer(
            propRespPtr, pResp_ );
        if( ERROR( ret ) )
            break;
        
        CCfgOpener oResp_( pResp_ );
        gint32 iRet = 0;
        ret = oResp_.GetIntProp( 
            propReturnValue, ( guint32& ) iRet );
        if( ERROR( ret ) ) break;

        IConfigDb* context = nullptr;
        CCfgOpener oReqCtx_( pReqCtx );
        ret = oReqCtx_.GetPointer( 0, context );
        if( ERROR( ret ) ) context = nullptr;
        
        bool bValid;
        
        if( SUCCEEDED( iRet ) )
        {
            guint32 dwSeriProto_ = 0;
            ret = oResp_.GetIntProp(
                propSeriProto, dwSeriProto_ );
            if( ERROR( ret ) ||
                dwSeriProto_ != seriRidl )
                break;
            BufPtr pBuf_;
            ret = oResp_.GetBufPtr( 0, pBuf_ );
            if( ERROR( ret ) )
                break;
            
            ObjPtr pDeserialIf_(this);
            CSerialBase oDeserial_( pDeserialIf_ );
            do{
                guint32 dwOrigOff = pBuf_->offset();
                ret = oDeserial_.Deserialize(
                    pBuf_, bValid );
                
                if( ERROR( ret ) ) break;
                
                pBuf_->SetOffset( dwOrigOff );
            }while( 0 );

            if( ERROR( ret ) )
                break;
                
            
        }
        this->DoLoginCallback(
            context, iRet,
            bValid );
        
    }while( 0 );
    
    return 0;
}

gint32 IOAuth2Proxy_PImpl::GetUserInfo( 
    IConfigDb* context,
    const std::string& strToken,
    USER_INFO& ui )
{
    gint32 ret = 0;
    TaskletPtr pRespCb_;
    do{
        CParamList oOptions_;
        CfgPtr pResp_;
        oOptions_[ propIfName ] =
            DBUS_IF_NAME( "OAuth2Proxy" );
        oOptions_[ propSeriProto ] = 
            ( guint32 )seriRidl;
        
        CParamList oReqCtx_;
        ObjPtr pTemp( context );
        oReqCtx_.Push( pTemp );
        
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb_, ObjPtr( this ), 
            &IOAuth2Proxy_PImpl::GetUserInfoCbWrapper, 
            nullptr, oReqCtx_.GetCfg() );

        if( ERROR( ret ) ) break;
        
        //Serialize the input parameters
        BufPtr pBuf_( true );
        
        ObjPtr pSerialIf_(this);
        CSerialBase oSerial_( pSerialIf_ );
        do{
            ret = oSerial_.Serialize(
                pBuf_, strToken );
            if( ERROR( ret ) ) break;
            
            pBuf_->SetOffset( 0 );
        }while( 0 );

        if( ERROR( ret ) )
            return ret;
        
        ret = this->AsyncCall(
            ( IEventSink* )pRespCb_, 
            oOptions_.GetCfg(), pResp_,
            "GetUserInfo",
            pBuf_ );
        gint32 ret2 = pRespCb_->GetError();
        if( SUCCEEDED( ret ) )
        {
            if( ret2 != STATUS_PENDING )
            {
                // pRespCb_ has been called
                ret = STATUS_PENDING;
            }
            else
            {
                // immediate return
                ( *pRespCb_ )( eventCancelTask );
                pRespCb_.Clear();
            }
        }
        if( ret == STATUS_PENDING )
        {
            if( context == nullptr )
                break;
            CCfgOpener oContext( context );
            oContext.CopyProp(
                propTaskId, pResp_ );
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }
        // immediate return
        CCfgOpener oResp_( ( IConfigDb* )pResp_ );
        oResp_.GetIntProp(
            propReturnValue, ( guint32& )ret );
        if( ERROR( ret ) ) break;
        guint32 dwSeriProto_ = 0;
        ret = oResp_.GetIntProp(
            propSeriProto, dwSeriProto_ );
        if( ERROR( ret ) ||
            dwSeriProto_ != seriRidl )
            break;
        BufPtr pBuf2;
        ret = oResp_.GetBufPtr( 0, pBuf2 );
        if( ERROR( ret ) )
            break;
        
        ObjPtr pDeserialIf_(this);
        CSerialBase oDeserial_( pDeserialIf_ );
        do{
            guint32 dwOrigOff = pBuf2->offset();
            ui.SetIf( oDeserial_.GetIf() );
            ret = oDeserial_.DeserialStruct(
                pBuf2, ui );
            if( ERROR( ret ) ) break;
            
            pBuf2->SetOffset( dwOrigOff );
        }while( 0 );

        if( ERROR( ret ) )
            break;
            
        
    }while( 0 );
    
    if( ERROR( ret ) && !pRespCb_.IsEmpty() )
        ( *pRespCb_ )( eventCancelTask );
    
    return ret;
}
//Async callback wrapper
gint32 IOAuth2Proxy_PImpl::GetUserInfoCbWrapper( 
    IEventSink* pCallback, 
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    gint32 ret = 0;
    do{
        IConfigDb* pResp_ = nullptr;
        CCfgOpenerObj oReq_( pIoReq );
        ret = oReq_.GetPointer(
            propRespPtr, pResp_ );
        if( ERROR( ret ) )
            break;
        
        CCfgOpener oResp_( pResp_ );
        gint32 iRet = 0;
        ret = oResp_.GetIntProp( 
            propReturnValue, ( guint32& ) iRet );
        if( ERROR( ret ) ) break;

        IConfigDb* context = nullptr;
        CCfgOpener oReqCtx_( pReqCtx );
        ret = oReqCtx_.GetPointer( 0, context );
        if( ERROR( ret ) ) context = nullptr;
        
        USER_INFO ui;
        
        if( SUCCEEDED( iRet ) )
        {
            guint32 dwSeriProto_ = 0;
            ret = oResp_.GetIntProp(
                propSeriProto, dwSeriProto_ );
            if( ERROR( ret ) ||
                dwSeriProto_ != seriRidl )
                break;
            BufPtr pBuf_;
            ret = oResp_.GetBufPtr( 0, pBuf_ );
            if( ERROR( ret ) )
                break;
            
            ObjPtr pDeserialIf_(this);
            CSerialBase oDeserial_( pDeserialIf_ );
            do{
                guint32 dwOrigOff = pBuf_->offset();
                ui.SetIf( oDeserial_.GetIf() );
                ret = oDeserial_.DeserialStruct(
                    pBuf_, ui );
                if( ERROR( ret ) ) break;
                
                pBuf_->SetOffset( dwOrigOff );
            }while( 0 );

            if( ERROR( ret ) )
                break;
                
            
        }
        this->GetUserInfoCallback(
            context, iRet,
            ui );
        
    }while( 0 );
    
    return 0;
}

gint32 IOAuth2Proxy_PImpl::RevokeUser( 
    IConfigDb* context,
    const std::string& strToken )
{
    gint32 ret = 0;
    TaskletPtr pRespCb_;
    do{
        CParamList oOptions_;
        CfgPtr pResp_;
        oOptions_[ propIfName ] =
            DBUS_IF_NAME( "OAuth2Proxy" );
        oOptions_[ propSeriProto ] = 
            ( guint32 )seriRidl;
        
        CParamList oReqCtx_;
        ObjPtr pTemp( context );
        oReqCtx_.Push( pTemp );
        
        ret = NEW_PROXY_RESP_HANDLER2(
            pRespCb_, ObjPtr( this ), 
            &IOAuth2Proxy_PImpl::RevokeUserCbWrapper, 
            nullptr, oReqCtx_.GetCfg() );

        if( ERROR( ret ) ) break;
        
        //Serialize the input parameters
        BufPtr pBuf_( true );
        
        ObjPtr pSerialIf_(this);
        CSerialBase oSerial_( pSerialIf_ );
        do{
            ret = oSerial_.Serialize(
                pBuf_, strToken );
            if( ERROR( ret ) ) break;
            
            pBuf_->SetOffset( 0 );
        }while( 0 );

        if( ERROR( ret ) )
            return ret;
        
        ret = this->AsyncCall(
            ( IEventSink* )pRespCb_, 
            oOptions_.GetCfg(), pResp_,
            "RevokeUser",
            pBuf_ );
        gint32 ret2 = pRespCb_->GetError();
        if( SUCCEEDED( ret ) )
        {
            if( ret2 != STATUS_PENDING )
            {
                // pRespCb_ has been called
                ret = STATUS_PENDING;
            }
            else
            {
                // immediate return
                ( *pRespCb_ )( eventCancelTask );
                pRespCb_.Clear();
            }
        }
        if( ret == STATUS_PENDING )
        {
            if( context == nullptr )
                break;
            CCfgOpener oContext( context );
            oContext.CopyProp(
                propTaskId, pResp_ );
            break;
        }
        else if( ERROR( ret ) )
        {
            break;
        }
        // immediate return
        CCfgOpener oResp_( ( IConfigDb* )pResp_ );
        oResp_.GetIntProp(
            propReturnValue, ( guint32& )ret );
        
    }while( 0 );
    
    if( ERROR( ret ) && !pRespCb_.IsEmpty() )
        ( *pRespCb_ )( eventCancelTask );
    
    return ret;
}
//Async callback wrapper
gint32 IOAuth2Proxy_PImpl::RevokeUserCbWrapper( 
    IEventSink* pCallback, 
    IEventSink* pIoReq,
    IConfigDb* pReqCtx )
{
    gint32 ret = 0;
    do{
        IConfigDb* pResp_ = nullptr;
        CCfgOpenerObj oReq_( pIoReq );
        ret = oReq_.GetPointer(
            propRespPtr, pResp_ );
        if( ERROR( ret ) )
            break;
        
        CCfgOpener oResp_( pResp_ );
        gint32 iRet = 0;
        ret = oResp_.GetIntProp( 
            propReturnValue, ( guint32& ) iRet );
        if( ERROR( ret ) ) break;

        IConfigDb* context = nullptr;
        CCfgOpener oReqCtx_( pReqCtx );
        ret = oReqCtx_.GetPointer( 0, context );
        if( ERROR( ret ) ) context = nullptr;
        
        RevokeUserCallback( context, iRet );
        
    }while( 0 );
    
    return 0;
}

gint32 IOAuth2Proxy_PImpl::OnOA2EventWrapper( 
    IEventSink* pCallback, BufPtr& pBuf_ )
{
    gint32 ret = 0;
    OA2EVENT oEvent;
    
    ObjPtr pDeserialIf_(this);
    CSerialBase oDeserial_( pDeserialIf_ );
    do{
        guint32 dwOrigOff = pBuf_->offset();
        oEvent.SetIf( oDeserial_.GetIf() );
        ret = oDeserial_.DeserialStruct(
            pBuf_, oEvent );
        if( ERROR( ret ) ) break;
        
        pBuf_->SetOffset( dwOrigOff );
    }while( 0 );

    if( ERROR( ret ) )
        return ret;
        
    OnOA2Event(oEvent );

    return ret;
}