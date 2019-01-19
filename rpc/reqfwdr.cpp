/*
 * =====================================================================================
 *
 *       Filename:  reqfwdr.cpp
 *
 *    Description:  implementation of CRpcReqForwarder interface
 *
 *        Version:  1.0
 *        Created:  06/25/2017 09:50:22 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#include "configdb.h"
#include "defines.h"
#include "proxy.h"
#include "rpcroute.h"
#include "dbusport.h"

using namespace std;

// ---interface related information---
//
// ----open port information----
//
// propPortClass={DBusLocalPdo/DBusProxyPdo/...}
// propPortId={bus_name/ipaddr:bus_name/...}
//
// ----dbus related information----
//
// propObjPath=`object path'
// propIfName=`interface name'
// propDestDBusName=`dest module name'
// propSrcDBusName=`src module name'

CRpcReqForwarder::CRpcReqForwarder(
    const IConfigDb* pCfg )
    : CAggInterfaceServer( pCfg ),
    super( pCfg )
{
    gint32 ret = 0;
    // SetClassId( clsid( CRpcReqForwarder ) );
    do{
        ObjPtr pObj;
        CCfgOpener oCfg( pCfg );

        ret = oCfg.GetObjPtr(
            propParentPtr, pObj );

        if( ERROR( ret ) )
            break;

        m_pParent = pObj;
        if( m_pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error occurs in CRpcReqForwarder ctor" );
        throw runtime_error( strMsg );
    }
}

gint32 CRpcReqForwarder::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( CRpcReqForwarder );

    ADD_SERVICE_HANDLER(
        CRpcReqForwarder::OpenRemotePort,
        SYS_METHOD_OPENRMTPORT );

    ADD_SERVICE_HANDLER(
        CRpcReqForwarder::CloseRemotePort,
        SYS_METHOD_OPENRMTPORT );

    ADD_SERVICE_HANDLER(
        CRpcReqForwarder::EnableRemoteEvent,
        SYS_METHOD_ENABLERMTEVT );

    ADD_SERVICE_HANDLER(
        CRpcReqForwarder::DisableRemoteEvent,
        SYS_METHOD_DISABLERMTEVT );

    END_HANDLER_MAP;

    return 0;
}

gint32 CRpcReqForwarder::CheckReqToFwrd(
    CRpcRouter* pRouter,
    const string &strIpAddr,
    DMsgPtr& pMsg,
    MatchPtr& pMatchHit )
{
    gint32 ret = pRouter->CheckReqToFwrd(
        strIpAddr, pMsg, pMatchHit );

    return ret;
}

gint32 CReqFwdrCloseRmtPortTask::RunTask()
{
    gint32 ret = 0;    
    do{
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        CRpcInterfaceProxy* pProxy = nullptr;
        ret = oCfg.GetPointer( 0, pProxy );
        if( ERROR( ret ) )
            break;

        ret = pProxy->Shutdown( this );

    }while( 0 );

    if( ret != STATUS_PENDING )
        OnTaskComplete( ret );

    return ret;
}


gint32 CReqFwdrCloseRmtPortTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;    
    CParamList oTaskCfg(
        ( IConfigDb* )GetConfig() );

    do{
        CParamList oParams;
        oParams[ propReturnValue ] = iRetVal;

        CInterfaceServer* pIf = nullptr;
        ret = oTaskCfg.GetPointer( propIfPtr, pIf );
        if( ERROR( ret ) )
            break;

        EventPtr pEvt;
        ret = GetInterceptTask( pEvt );
        if( ERROR( ret ) )
            break;

        pIf->SetResponse( pEvt, oParams.GetCfg() );

    }while( 0 );

    oTaskCfg.ClearParams();

    return iRetVal;
}

gint32 CReqFwdrOpenRmtPortTask::CreateInterface(
    InterfPtr& pIf )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        CIoManager* pMgr = nullptr;
        ret = oCfg.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        bool bServer = true;
        if( m_iState == stateStartBridgeProxy )
            bServer = false;
        // ---interface related information---
        //
        // ----open port information----
        //
        // propPortClass={DBusLocalPdo/DBusProxyPdo/...}
        // propPortId={bus_name/ipaddr:bus_name/...}
        //
        // ----dbus related information----
        //
        // propObjPath=`object path'
        // propIfName=`interface name'
        // propDestDBusName=`dest module name'
        // propSrcDBusName=`src module name'
        //
        // ---other properties
        // propIoMgr
        // propRouterPtr

        CParamList oParams;
        ret = CRpcServices::LoadObjDesc(
            ROUTER_OBJ_DESC,
            OBJNAME_TCP_BRIDGE,
            bServer,
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        ret =oParams.CopyProp( propIpAddr, this );
        if( ERROR( ret ) )
            break;

        oParams.CopyProp( propSrcTcpPort, this );

        bool bNoPortId = true;
        ret = oParams.CopyProp( propPortId, this );
        if( SUCCEEDED( ret ) )
            bNoPortId =false;

        ObjPtr pObj;
        ret = oCfg.GetObjPtr( propRouterPtr, pObj );
        if( ERROR( ret ) )
            break;

        oParams.SetObjPtr( propParentPtr, pObj );

        if( bNoPortId )
        {
            guint32 dwAddr = 0;
            string strIpAddr;

            oParams.GetStrProp(
                propIpAddr, strIpAddr );

            dwAddr = ntohl(
                inet_addr( strIpAddr.c_str() ) );

            oParams[ propPortId ] = dwAddr;
        }

        if( ERROR( ret ) )
            break;

        oParams.SetPointer( propIoMgr, pMgr );

        pIf.Clear();

        if( m_iState == stateStartBridgeProxy )
        {
            ret = pIf.NewObj(
                clsid( CRpcTcpBridgeProxyImpl ),
                oParams.GetCfg() );
        }
        else if( m_iState == stateStartBridgeServer )
        {
            ret = pIf.NewObj(
                clsid( CRpcTcpBridgeImpl ),
                oParams.GetCfg() );
        }

    }while( 0 );

    return ret;
}

gint32 CReqFwdrOpenRmtPortTask::OnServiceComplete(
    gint32 iRetVal )
{
    if( iRetVal == STATUS_PENDING ||
        Retriable( iRetVal ) )
        return -EINVAL;

    gint32 ret = 0;
    do{
        EventPtr pEvt;
        ret = GetInterceptTask( pEvt );
        if( ERROR( ret ) )
            break;

        CRpcReqForwarder* pIf = nullptr;
        CCfgOpener oCfg(
            ( IConfigDb* )GetConfig() );

        ret = oCfg.GetPointer( propIfPtr, pIf );

        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams[ propReturnValue ] = iRetVal;

        ret = pIf->SetResponse( 
            pEvt, oParams.GetCfg() );

    }while( 0 );

    return ret;
}

gint32 CReqFwdrOpenRmtPortTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = RunTaskInternal( iRetVal );

    if( ret == STATUS_PENDING )
        return ret;

    if( ret == STATUS_MORE_PROCESS_NEEDED ||
        ret == -EAGAIN )
    {
        if( CanRetry() )
        {
            ret = STATUS_MORE_PROCESS_NEEDED;
            return ret;
        }
        ret = ERROR_FAIL;
    }

    OnServiceComplete( ret );
    return ret;
}

gint32 CReqFwdrOpenRmtPortTask::RunTask()
{
    // call with an unlikely retval to indicate it
    // comes from RunTask
    gint32 ret = RunTaskInternal( 0x7fffffff );

    if( ret == STATUS_PENDING )
        return ret;

    if( ret == STATUS_MORE_PROCESS_NEEDED ||
        ret == -EAGAIN )
    {
        if( CanRetry() )
            return ret;

        ret = ERROR_FAIL;
    }
    OnServiceComplete( ret );

    return ret;
}

gint32 CReqFwdrOpenRmtPortTask::RunTaskInternal(
    gint32 iRetVal )
{
    gint32 ret = 0;
    CCfgOpener oCfg( ( IConfigDb* )GetConfig() );

    // five parameters needed:
    // propIoMgr
    // propIfPtr: the ptr to CRpcReqForwarder
    // propEventSink: the callback
    // propSrcDBusName: the source dbus
    // propIpAddr: the destination ip address
    // propRouterPtr: the pointer to the router

    do{

        CIoManager* pMgr = nullptr;
        CRpcRouter* pRouter = nullptr;

        ret = oCfg.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        CRpcReqForwarder* pReqFwdr = nullptr;
        ObjPtr pObj;

        ret = AdvanceState();
        if( ERROR( ret ) )
            break;

        bool bRouter = false;

        ret = oCfg.GetPointer(
            propRouterPtr, pRouter );

        if( SUCCEEDED( ret ) )
        {
            // this is a request from router to
            // create a bridge interface
            bRouter = true;
        }
        else
        {
            // this is a request from reqfwdr to
            // create a bridge proxy interface
        }

        ret = oCfg.GetPointer(
            propIfPtr, pReqFwdr );

        if( !bRouter && ERROR( ret ) )
            break;

        if( ( !bRouter && pReqFwdr == nullptr ) ||
            ( bRouter && pRouter == nullptr ) )
        {
            // either of them should exist
            ret = -EINVAL;
            break;
        }

        string strIpAddr;
        ret = oCfg.GetStrProp(
            propIpAddr, strIpAddr );

        if( ERROR( ret ) )
            break;

        string strUniqName;
        if( !bRouter )
        {
            ret = oCfg.GetStrProp(
                propSrcUniqName, strUniqName );

            if( ERROR( ret ) )
                break;
        }

        if( m_iState == stateDone )
        {
            if( bRouter )
            {
                if( SUCCEEDED( iRetVal ) ||
                    iRetVal == 0x7fffffff )
                {
                    ret = pRouter->
                        AddBridge( m_pServer ); 
                }
            }
            else
            {
                if( SUCCEEDED( iRetVal ) ||
                    iRetVal == 0x7fffffff )
                {
                    ret = pRouter->
                        AddBridgeProxy( m_pProxy ); 
                    
                    if( ERROR( ret ) )
                        break;

                    string strSender;
                    ret = oCfg.GetStrProp(
                        propSrcDBusName, strSender );

                    if( ERROR( ret ) )
                        break;

                    pReqFwdr->AddRefCount(
                        strIpAddr,
                        strUniqName,
                        strSender );
                }
            }
        }
        else 
        {
            InterfPtr pBridgeIf;
            ret = CreateInterface( pBridgeIf );
            if( ERROR( ret ) )
                break;

            if( m_iState == stateStartBridgeProxy )
            {
                m_pProxy = pBridgeIf;
            }
            else if( m_iState == stateStartBridgeServer )
            {
                m_pServer = pBridgeIf;
            }

            ret = pBridgeIf->StartEx( this );
            if( SUCCEEDED( ret ) )
                continue;
        }
        break;

    }while( 1 );

    if( ret != STATUS_PENDING )
    {
        m_pProxy.Clear();
        m_pServer.Clear();
    }

    return ret;
}

gint32 CReqFwdrOpenRmtPortTask::OnCancel(
    gint32 dwContext )
{
    OnServiceComplete( -ECANCELED );
    super::OnCancel( dwContext );
    return AdvanceState();
}
// active disconnecting
gint32 CRpcReqForwarder::CloseRemotePort(
    IEventSink* pCallback,
    const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        if( pCfg == nullptr ||
            pCallback == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CCfgOpener oCfg( pCfg );
        string strIpAddr;

        ret = oCfg.GetStrProp(
            propIpAddr, strIpAddr );

        if( ERROR( ret ) )
            break;

        string strSender;
        ret = oCfg.GetStrProp(
            propSrcDBusName, strSender );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oCfgInvoke( pCallback );
        DMsgPtr pMsg;

        ret = oCfgInvoke.GetMsgPtr(
            propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        string strSrcUniqName = pMsg.GetSender();
        if( strSrcUniqName.empty() )
        {
            ret = -EINVAL;
            break;
        }

        InterfPtr pIf;
        CRpcRouter* pRouter = GetParent();
        ret = pRouter->GetBridgeProxy(
            strIpAddr, pIf );

        if( ERROR( ret ) )
            break;

        bool bStopProxy = false;
        ret = DecRefCount( strIpAddr,
            strSrcUniqName, strSender );
        if( ERROR( ret ) )
            break;

        if( ret == 0 )
            bStopProxy = true;
        else
            ret = 0;

        if( bStopProxy )
        {
            // no reference to this proxy, start
            // the stop process.
            if( true )
            {
                CStdRMutex oIfLock(
                    pRouter->GetLock() );
                pRouter->RemoveBridgeProxy( pIf );
                pRouter->RemoveLocalMatchByAddr(
                    strIpAddr );
            }

            CParamList oParams;
            oParams.CopyProp( propIpAddr, pCfg );
            oParams.SetPointer( propIoMgr, GetIoMgr() );
            oParams.SetObjPtr( propEventSink,
                ObjPtr( pCallback ) );

            oParams.SetPointer( propIfPtr, this );
            oParams.Push( ObjPtr( pIf ) );

            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CReqFwdrCloseRmtPortTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            // must be pending
            pTask->MarkPending();
            ret = GetIoMgr()->RescheduleTask( pTask );
            if( SUCCEEDED( ret ) )
                ret = STATUS_PENDING;
        }
        break;

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        CParamList oParams;
        oParams[ propReturnValue ] = ret;
        SetResponse( pCallback, oParams.GetCfg() );
    }

    return ret;
}

gint32 CRpcReqForwarder::OpenRemotePort(
    IEventSink* pCallback,
    const IConfigDb* pCfg )
{
    gint32 ret = 0;
    do{
        if( pCfg == nullptr ||
            pCallback == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CCfgOpener oCfg( pCfg );
        string strIpAddr;

        ret = oCfg.GetStrProp(
            propIpAddr, strIpAddr );

        if( ERROR( ret ) )
            break;

        string strSender;
        ret = oCfg.GetStrProp(
            propSrcDBusName, strSender );

        if( ERROR( ret ) )
            break;

        CCfgOpenerObj oCfgInvoke( pCallback );
        DMsgPtr pMsg;

        ret = oCfgInvoke.GetMsgPtr(
            propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        string strSrcUniqName = pMsg.GetSender();
        if( strSrcUniqName.empty() )
        {
            ret = -EINVAL;
            break;
        }

        InterfPtr pIf;

        ret = GetParent()->GetBridgeProxy(
            strIpAddr, pIf );

        if( SUCCEEDED( ret ) )
        {
            AddRefCount( strIpAddr,
                strSrcUniqName, strSender );
        }
        else if( ret == -ENOENT )
        {
            CParamList oParams;

            oParams.CopyProp( propIpAddr, pCfg );
            oParams.SetPointer( propIoMgr, GetIoMgr() );

            oParams.SetObjPtr( propEventSink,
                ObjPtr( pCallback ) );

            oParams.SetObjPtr( propIfPtr,
                ObjPtr( this ) );

            oParams.SetStrProp(
                propSrcDBusName, strSender );

            oParams.SetObjPtr( propRouterPtr,
                ObjPtr( GetParent( ) ) );

            oParams.SetStrProp(
                propSrcUniqName, strSrcUniqName );

            TaskletPtr pTask;

            ret = pTask.NewObj(
                clsid( CReqFwdrOpenRmtPortTask ),
                oParams.GetCfg() );

            if( ERROR( ret ) )
                break;

            // must be pending
            pTask->MarkPending();
            ret = DEFER_CALL( GetIoMgr(), this,
                &CRpcServices::RunManagedTask,
                pTask, false );

            if( SUCCEEDED( ret ) )
                ret = STATUS_PENDING;
        }
        break;

    }while( 0 );

    if( ret != STATUS_PENDING )
    {
        CParamList oParams;
        oParams[ propReturnValue ] = ret;
        SetResponse( pCallback, oParams.GetCfg() );
    }

    return ret;
}

gint32 CRpcReqForwarder::AddRefCount(
    const std::string& strIpAddr,
    const std::string& strSrcUniqName,
    const std::string& strSrcDBusName )
{
    if( strIpAddr.empty() ||
        strSrcDBusName.empty() ||
        strSrcUniqName.empty() )
        return -EINVAL;
       
    RegObjPtr pRegObj( true );

    CCfgOpener oCfg( ( IConfigDb* )pRegObj );

    oCfg.SetStrProp(
        propSrcDBusName, strSrcDBusName );

    oCfg.SetStrProp(
        propIpAddr, strIpAddr );

    oCfg.SetStrProp(
        propSrcUniqName, strSrcUniqName );

    CStdRMutex oIfLock( GetLock() );
    if( m_mapRefCount.find( pRegObj ) !=
        m_mapRefCount.end() )
    {
        ++m_mapRefCount[ pRegObj ];
    }
    else
    {
        m_mapRefCount[ pRegObj ] = 1;
    }
    return 0;
}

// NOTE: the return value on success is the
// reference count to the ipAddress, but not the
// reference count to the regobj.
gint32 CRpcReqForwarder::DecRefCount(
    const std::string& strIpAddr,
    const std::string& strSrcUniqName,
    const std::string& strSrcDBusName )
{
    if( strIpAddr.empty() ||
        strSrcDBusName.empty() ||
        strSrcUniqName.empty() )
        return -EINVAL;

    RegObjPtr pRegObj( true );

    CCfgOpener oCfg( ( IConfigDb* )pRegObj );

    oCfg.SetStrProp(
        propSrcDBusName, strSrcDBusName );

    oCfg.SetStrProp(
        propIpAddr, strIpAddr );

    oCfg.SetStrProp(
        propSrcUniqName, strSrcUniqName );

    gint32 ret = 0;

    CStdRMutex oIfLock( GetLock() );
    if( m_mapRefCount.find( pRegObj ) !=
        m_mapRefCount.end() )
    {
        return -ENOENT;
    }
    else
    {
        gint32 iRef = --m_mapRefCount[ pRegObj ];
        if( iRef <= 0 )
        {
            m_mapRefCount.erase( pRegObj );
            iRef = 0;
            for( auto elem : m_mapRefCount )
            {
                const RegObjPtr& pReg = elem.first;
                if( pReg->GetIpAddr() == strIpAddr )
                    iRef += elem.second;
            }
        }
        ret = std::max( iRef, 0 );
    }

    return ret;
}

gint32 CRpcReqForwarder::ClearRefCountByAddr(
    const string& strIpAddr,
    vector< string > vecUniqNames )
{
    CStdRMutex oIfLock( GetLock() );
    auto itr = m_mapRefCount.begin();
    while( itr != m_mapRefCount.end() )
    {
        if( itr->first->GetIpAddr() == strIpAddr )
        {
            vecUniqNames.push_back(
                itr->first->GetUniqName() );
            itr = m_mapRefCount.erase( itr );
            continue;
        }
        itr++;
    }
    return vecUniqNames.size();
}

gint32 CRpcReqForwarder::CheckMatch(
    IMessageMatch* pMatch )
{
    if( pMatch == nullptr )
        return -EINVAL;

    CStdRMutex oIfLock( GetLock() );
    for( auto&& oPair : m_mapRefCount )
    {
        if( oPair.first->IsMyMatch( pMatch ) )
            return 0;
    }
    return -ENOENT;
}

gint32 CRpcReqForwarder::DisableRemoteEvent(
    IEventSink* pCallback,
    IMessageMatch* pMatch )
{
    return EnableDisableEvent(
        pCallback, pMatch, false );
}

gint32 CRpcReqForwarder::EnableRemoteEvent(
    IEventSink* pCallback,
    IMessageMatch* pMatch )
{
    return EnableDisableEvent(
        pCallback, pMatch, true );
}

gint32 CRpcReqForwarder::EnableDisableEvent(
    IEventSink* pCallback,
    IMessageMatch* pMatch,
    bool bEnable )
{
    if( pMatch == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oCfgInvoke( pCallback );
        DMsgPtr pMsg;

        ret = oCfgInvoke.GetMsgPtr(
            propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        string strSrcUniqName = pMsg.GetSender();
        if( strSrcUniqName.empty() )
        {
            ret = -EINVAL;
            break;
        }
        
        CCfgOpenerObj oCfg( pMatch );

        oCfg.SetStrProp(
            propSrcUniqName, strSrcUniqName );

        // check if the match comes from a
        // registered module
        ret = CheckMatch( pMatch );
        if( ERROR( ret ) )
            break;

        CParamList oParams;
        oParams.SetPointer( propIoMgr, GetIoMgr() );

        oParams.SetObjPtr(
            propEventSink, ObjPtr( pCallback ) );

        oParams.SetObjPtr(
            propIfPtr, ObjPtr( this ) );

        oParams.CopyProp( propIpAddr, pMatch );

        oParams.Push( ObjPtr( pMatch ) );
        oParams.Push( bEnable );

        TaskletPtr pTask;
        ret = pTask.NewObj(
            clsid( CReqFwdrEnableRmtEventTask ),
            oParams.GetCfg() );

        if( ERROR( ret ) )
            break;

        pTask->MarkPending();
        ret = DEFER_CALL( GetIoMgr(), this,
            &CRpcServices::RunManagedTask,
            pTask, false );

        if( SUCCEEDED( ret ) )
            ret = STATUS_PENDING;

    }while( 0 );

    if( ERROR( ret ) )
    {
        CParamList oResp;
        oResp[ propReturnValue ] = ret;
        SetResponse( pCallback, oResp.GetCfg() );
    }

    return ret;
}

gint32 CRpcReqForwarder::CheckSendDataToFwrd(
    IConfigDb* pDataDesc )
{
    gint32 ret = 0;
    if( pDataDesc == nullptr )
        return -EINVAL;

    do{
        CRpcRouter* pRouter = GetParent();
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpener oParams( pDataDesc );
        string strIpAddr;
        ret = oParams.GetStrProp(
            propIpAddr, strIpAddr );
        if( ERROR( ret ) )
            break;

        DMsgPtr pMsgToCheck;
        ret = pMsgToCheck.NewObj();
        if( ERROR( ret ) )
            break;

        string strObjPath;
        ret = oParams.GetStrProp(
            propObjPath, strObjPath );

        if( ERROR( ret ) )
            break;
        pMsgToCheck.SetPath( strObjPath );

        string strIf;
        ret = oParams.GetStrProp(
            propIfName, strIf );
        if( ERROR( ret ) )
            break;
        pMsgToCheck.SetInterface( strIf );

        string strSender;
        ret = oParams.GetStrProp(
            propSrcDBusName, strSender );
        if( ERROR( ret ) )
            break;
        pMsgToCheck.SetSender( strIf );

        MatchPtr pMatch;
        ret = pRouter->CheckReqToFwrd(
            strIpAddr, pMsgToCheck, pMatch );

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarder::SendData_Server(
    IConfigDb* pDataDesc,           // [in]
    gint32 fd,                      // [in]
    guint32 dwOffset,               // [in]
    guint32 dwSize,                 // [in]
    IEventSink* pCallback )
{
    if( pDataDesc == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ret = CheckSendDataToFwrd( pDataDesc );
        if( ERROR( ret ) )
            break;
        ret = super::SendData_Server( pDataDesc,
            fd, dwOffset, dwSize, pCallback );

    }while( 0 );
    return ret;
}

gint32 CReqFwdrEnableRmtEventTask::RunTask()
{
    gint32 ret = 0;
    do{

        CParamList oParams( GetConfig() );
        CRpcReqForwarder* pIf;
        ObjPtr pObj;
        ret = oParams.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        CRpcRouter* pRouter = pIf->GetParent();
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        string strIpAddr;
        ret = oParams.GetStrProp(
            propIpAddr, strIpAddr );
        if( ERROR( ret ) )
            break;

        InterfPtr bridgePtr;
        ret = pRouter->GetBridgeProxy(
            strIpAddr, bridgePtr );

        CRpcTcpBridgeProxy* pProxy = bridgePtr;
        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        bool bEnable = false;

        oParams.GetBoolProp( 1, bEnable );
        if( ERROR( ret ) )
            break;

        oParams.GetObjPtr( 0, pObj );
        if( ERROR( ret ) )
            break;

        IMessageMatch* pMatch = pObj;
        if( pMatch == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CCfgOpenerObj oMatch( pMatch );

        oMatch.CopyProp(
            propSrcTcpPort, pProxy );

        if( bEnable )
        {
            ret = pProxy->EnableRemoteEvent(
                this, pMatch );
        }
        else
        {
            ret = pProxy->DisableRemoteEvent(
                this, pMatch );
        }

    }while( 0 );

    if( ret != STATUS_PENDING && !Retriable( ret ) )
    {
        OnTaskComplete( ret );
    }

    return ret;
}

gint32 CReqFwdrEnableRmtEventTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;

    do{
        EventPtr pEvent;
        ret = GetInterceptTask( pEvent );
        if( ERROR( ret ) )
            break;

        IMessageMatch* pMatch = nullptr;
        bool bEnable = true;

        ObjPtr pObj;
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ret = oCfg.GetBoolProp( 1, bEnable );
        if( ERROR( ret ) )
        {
            // fatal error, cannot recover
            break;
        }
        ret = oCfg.GetObjPtr( 0, pObj );
        if( ERROR( ret ) )
            break;

        pMatch = pObj;
        if( pMatch == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcReqForwarder* pReqFwdr = pObj;
        if( pReqFwdr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        if( SUCCEEDED( iRetVal ) )
        {
            // add the match
            CRpcRouter* pRouter =
                pReqFwdr->GetParent();

            if( bEnable )
            {
                pRouter->AddLocalMatch( pMatch );
            }
            else
            {
                pRouter->RemoveLocalMatch(
                    pMatch );
            }
        }

        if( IsPending() )
        {
            CParamList oParams;
            oParams.SetIntProp(
                propReturnValue, iRetVal );
            
            // the response will finally be sent in
            // this method
            ret = pReqFwdr->SetResponse(
                pEvent, oParams.GetCfg() ); 
        }

    }while( 0 );

    if( ret != STATUS_PENDING && !Retriable( ret ) )
    {
        // clear the objects
        CParamList oParams( GetConfig() );
        bool bEnable;
        oParams.Pop( bEnable );
        ObjPtr pObj;
        oParams.Pop( pObj );
    }

    return ret;
}

gint32 CRegisteredObject::IsMyMatch(
    IMessageMatch* pMatch )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( this );

        ret = oCfg.IsEqualProp(
            propIpAddr, pMatch );
        if( ERROR( ret ) )
            break;

        ret = oCfg.IsEqualProp(
            propSrcUniqName, pMatch );
        if( ERROR( ret ) )
            break;

        ret = oCfg.IsEqualProp(
            propSrcDBusName, pMatch );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    return ret;
}

bool CRegisteredObject::operator<(
    CRegisteredObject& rhs ) const
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( this );
        CCfgOpener oCfg2( &rhs );

        std::string strVal, strVal2;
        guint8 ip1[ IPV6_ADDR_BYTES ];
        guint8 ip2[ IPV6_ADDR_BYTES ];

        ret = oCfg.GetStrProp(
            propIpAddr, strVal );
        if( ERROR( ret ) )
            break;

        ret = oCfg2.GetStrProp(
            propIpAddr, strVal2 );
        if( ERROR( ret ) )
            break;

        guint32 dwSize = IPV6_ADDR_BYTES;
        ret = Ip4AddrToBytes(
            strVal, ip1, dwSize );

        if( ERROR( ret ) ||
            dwSize < sizeof( guint32 ) )
            break;

        dwSize = IPV6_ADDR_BYTES;
        ret = Ip4AddrToBytes(
            strVal2, ip2, dwSize );

        if( ERROR( ret ) ||
            dwSize < sizeof( guint32 ) )
            break;

        if( *( guint32* )ip1 < *( guint32* )ip2 )
            break;

        if( *( guint32* )ip1 > *( guint32* )ip2 )
        {
            ret = ERROR_FALSE;
            break;
        }

        ret = oCfg.GetStrProp(
            propSrcUniqName, strVal );
        if( ERROR( ret ) )
            break;

        ret = oCfg2.GetStrProp(
            propSrcUniqName, strVal2 );
        if( ERROR( ret ) )
            break;

        if( strVal < strVal2 )
            break;

        if( strVal2 < strVal )
        {
            ret = ERROR_FALSE;
            break;
        }

        ret = oCfg.GetStrProp(
            propSrcDBusName, strVal );
        if( ERROR( ret ) )
            break;

        ret = oCfg2.GetStrProp(
            propSrcDBusName, strVal2 );

        if( ERROR( ret ) )
            break;

        if( strVal < strVal2 )
            break;

        if( strVal2 < strVal )
        {
            ret = ERROR_FALSE;
            break;
        }

    }while( 0 );

    if( SUCCEEDED( ret ) )
        return true;

    return false;
}

gint32 CReqFwdrForwardRequestTask::RunTask()
{
    gint32 ret = 0;

    ObjPtr pObj;
    CParamList oParams( GetConfig() );
    DMsgPtr pRespMsg;

    do{
        CRpcReqForwarder* pIf;
        ret = oParams.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = oParams.GetObjPtr(
            propRouterPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = pObj;
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        string strIpAddr;

        ret = oParams.GetStrProp( 0, strIpAddr );
        if( ERROR( ret ) )
            break;

        DMsgPtr pMsg;
        ret = oParams.GetMsgPtr( 1, pMsg );
        if( ERROR( ret ) )
            break;

        string strDest;
        ret = oParams.GetStrProp( 2, strDest );
        if( ERROR( ret ) )
            break;

        InterfPtr bridgePtr;

        if( pIf->GetClsid() ==
            clsid( CRpcReqForwarderImpl ) )
        {
            ret = pRouter->GetBridgeProxy(
                strIpAddr, bridgePtr );

            CRpcTcpBridgeProxy* pProxy = bridgePtr;
            if( pProxy == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pProxy->ForwardRequest(
                strIpAddr, pMsg, pRespMsg, this );
        }
        else if( pIf->GetClsid() ==
            clsid( CRpcTcpBridgeImpl ) )
        {
            ret = pRouter->GetReqFwdrProxy(
                strDest, bridgePtr );

            CRpcReqForwarderProxy* pProxy =
                bridgePtr;

            if( pProxy == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pProxy->ForwardRequest(
                strIpAddr, pMsg, pRespMsg, this );
        }

    }while( 0 );

    if( ret != STATUS_PENDING && !Retriable( ret ) )
    {
        oParams.GetObjPtr(
            propRespPtr, pObj );

        CParamList oRespCfg(
            ( IConfigDb* )pObj );

        oRespCfg.SetIntProp(
            propReturnValue, ret );

        if( !pRespMsg.IsEmpty() )
            oRespCfg.Push( pRespMsg );

        OnTaskComplete( ret );
    }

    return ret;
}

gint32 CReqFwdrForwardRequestTask::OnTaskComplete(
    gint32 iRetVal )
{
    gint32 ret = 0;

    do{
        ObjPtr pObj;
        CCfgOpener oCfg( ( IConfigDb* )*this );

        ret = oCfg.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcInterfaceServer* pIfSvr = pObj;
        if( pIfSvr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        TaskletPtr pCallerTask;
        ret = GetCallerTask( pCallerTask );
        if( SUCCEEDED( ret ) )
        {
            CCfgOpenerObj oRespCfg(
                ( CObjBase* )pCallerTask );

            ret = oRespCfg.GetObjPtr(
                propRespPtr, pObj );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            ret = oCfg.GetObjPtr(
                propRespPtr, pObj );
            if( ERROR( ret ) )
                break;
        }

        if( IsPending() )
        {
            EventPtr pEvent;
            ret = GetInterceptTask( pEvent );
            if( ERROR( ret ) )
                break;

            // the response will finally be sent
            // in this method
            ret = pIfSvr->OnServiceComplete(
                pObj, pEvent ); 
        }

    }while( 0 );

    if( ret != STATUS_PENDING && !Retriable( ret ) )
    {
        // clear the objects
        CParamList oParams( GetConfig() );
        oParams.ClearParams();
    }

    return ret;
}

gint32 CRpcReqForwarder::BuildBufForIrpRmtSvrEvent(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    DMsgPtr pOfflineMsg;

    do{
        CReqOpener oReq( pReqCall );
        string strSrcIp;
        bool bOnline = false;

        ret = oReq.GetStrProp( 0, strSrcIp );
        if( ERROR( ret ) )
            break;

        ret = oReq.GetBoolProp( 1, bOnline );
        if( ERROR( ret ) )
            break;

        ret = pOfflineMsg.NewObj( ( EnumClsid )
            DBUS_MESSAGE_TYPE_SIGNAL );

        if( ERROR( ret ) )
            break;

        pOfflineMsg.SetInterface(
            DBUS_IF_NAME( IFNAME_REQFORWARDER ) );

        pOfflineMsg.SetPath( DBUS_OBJ_PATH(
            MODNAME_RPCROUTER, OBJNAME_REQFWDR ) );

        pOfflineMsg.SetMember(
            SYS_EVENT_RMTSVREVENT );

        string strVal;
        ret = oReq.GetStrProp(
            propDestDBusName, strVal );
        if( ERROR( ret ) )
            break;

        pOfflineMsg.SetDestination( strVal );

        const char* pszIp = strSrcIp.c_str();
        if( !dbus_message_append_args( pOfflineMsg,
            DBUS_TYPE_STRING, &pszIp,
            DBUS_TYPE_BOOLEAN, &bOnline,
            DBUS_TYPE_INVALID ) )
        {
            ret = -ENOMEM;
            break;
        }

        *pBuf = pOfflineMsg;

    }while( 0 );

    if( ERROR( ret ) && !pOfflineMsg.IsEmpty() )
        pOfflineMsg.Clear();

    return ret;
}

gint32 CRpcReqForwarder::BuildBufForIrpFwrdEvt(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    DMsgPtr pFwrdMsg;

    do{
        CReqOpener oReq( pReqCall );
        string strSrcIp;

        ret = oReq.GetStrProp( 0, strSrcIp );
        if( ERROR( ret ) )
            break;

        BufPtr pEvtBuf( true );
        ret = oReq.GetProperty( 1, *pEvtBuf );
        if( ERROR( ret ) )
            break;
        
        ret = pFwrdMsg.NewObj( ( EnumClsid )
            DBUS_MESSAGE_TYPE_SIGNAL );

        if( ERROR( ret ) )
            break;

        ObjPtr pObj;
        ret = oReq.GetObjPtr( propMatchPtr, pObj );
        if( ERROR( ret ) )
            break;

        MatchPtr pMatch;
        pMatch = pObj;
        if( pMatch.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        string strVal;
        CCfgOpenerObj oCfg( ( CObjBase* )pMatch );

        ret = oCfg.GetStrProp(
            propDestDBusName, strVal );
        if( ERROR( ret ) )
            break;

        pFwrdMsg.SetSender( strVal );

        ret = oCfg.GetStrProp(
            propIfName, strVal );
        if( ERROR( ret ) )
            break;

        pFwrdMsg.SetInterface( strVal );

        ret = oCfg.GetStrProp(
            propObjPath, strVal );
        if( ERROR( ret ) )
            break;

        pFwrdMsg.SetPath( strVal );

        pFwrdMsg.SetMember(
            SYS_EVENT_FORWARDEVT );

        ret = oCfg.GetStrProp(
            propSrcDBusName, strVal );
        if( ERROR( ret ) )
            break;

        pFwrdMsg.SetDestination( strVal );

        const char* pszIp = strSrcIp.c_str();
        if( !dbus_message_append_args( pFwrdMsg,
            DBUS_TYPE_STRING, &pszIp,
            DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
            &pEvtBuf->ptr(), pEvtBuf->size(),
            DBUS_TYPE_INVALID ) )
        {
            ret = -ENOMEM;
            break;
        }

        *pBuf = pFwrdMsg;

    }while( 0 );

    if( ERROR( ret ) && !pFwrdMsg.IsEmpty() )
        pFwrdMsg.Clear();

    return ret;
}

gint32 CRpcReqForwarder::BuildBufForIrp(
    BufPtr& pBuf, IConfigDb* pReqCall )
{
    if( pReqCall == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        if( pBuf.IsEmpty() )
        {
            ret = pBuf.NewObj();
            if( ERROR( ret ) )
                break;
        }

        CReqOpener oReq( pReqCall );
        string strMethod;
        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        if( strMethod == SYS_EVENT_RMTSVREVENT )
        {
            ret = BuildBufForIrpRmtSvrEvent(
                pBuf, pReqCall );
        }
        else
        {
            ret = super::BuildBufForIrp(
                pBuf, pReqCall );
        }

    }while( 0 );

    return ret;
}
    
gint32 CRpcReqForwarder::ForwardEvent(
    const std::string& strSrcIp,
    DBusMessage* pEventMsg,
    IEventSink* pCallback )
{
    if( pEventMsg == nullptr ||
        strSrcIp.empty() ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CRpcRouter* pRouter = GetParent();
        vector< MatchPtr > vecMatches;
        DMsgPtr pEvtMsg( pEventMsg );

        ret = pRouter->CheckEvtToFwrd(
            strSrcIp, pEvtMsg, vecMatches );

        if( ERROR( ret ) )
            break;

        if( vecMatches.empty() )
            break;

        BufPtr pBuf( true );
        ret = pEvtMsg.Serialize( *pBuf );
        if( ERROR( ret ) )
            break;

        for( auto&& pMatch : vecMatches )
        {
            // dispatch the event to all the
            // subscribers
            CReqBuilder oBuilder( this );

            oBuilder.Push( strSrcIp );
            oBuilder.Push( *pBuf );

            ObjPtr pObj;
            pObj = pMatch;

            oBuilder.SetObjPtr(
                propMatchPtr, pObj );

            oBuilder.SetMethodName(
                SYS_EVENT_FORWARDEVT );

            oBuilder.SetCallFlags( 
               DBUS_MESSAGE_TYPE_SIGNAL 
               | CF_ASYNC_CALL );

            oBuilder.SetIfName( DBUS_IF_NAME(
                IFNAME_REQFORWARDER ) );

            CCfgOpenerObj oMatch(
                ( CObjBase* )pObj );

            std::string strDest;
            ret = oMatch.GetStrProp(
                propSrcDBusName, strDest );

            if( ERROR( ret ) )
                break;

            // set the signal's destination 
            oBuilder.SetDestination( strDest );

            // a tcp default round trip time
            oBuilder.SetTimeoutSec(
                IFSTATE_DEFAULT_IOREQ_TIMEOUT ); 

            oBuilder.SetKeepAliveSec(
                IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ); 

            CParamList oRespCfg;
            CfgPtr pRespCfg = oRespCfg.GetCfg();
            if( pRespCfg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            TaskletPtr pTask;
            ret = pTask.NewObj(
                clsid( CIfDummyTask ) );

            if( ERROR( ret ) )
                break;

            ret = RunIoTask( oBuilder.GetCfg(),
                pRespCfg, pTask );

            if( ERROR( ret ) )
                break;

            if( SUCCEEDED( ret ) )
            {
                CCfgOpener oCfg(
                    ( IConfigDb* )pRespCfg );

                gint32 iRet = 0;
                ret = oCfg.GetIntProp(
                    propReturnValue,
                    ( guint32& )iRet );

                if( ERROR( ret ) )
                    break;

                ret = iRet;
            }
        }

        ret = 0;

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarder::OnKeepAliveRelay(
    IEventSink* pTask )
{
    if( pTask == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        CCfgOpenerObj oCfg( pTask );
        DMsgPtr pMsg;

        ret = oCfg.GetMsgPtr( propMsgPtr, pMsg );
        if( ERROR( ret ) )
            break;

        if( pMsg.GetType() !=
            DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            ret = -ENOTSUP;
            break;
        }
        // note that we only do keep-alive for
        // FORWARD_REQUEST
        if( pMsg.GetMember() !=
            SYS_METHOD_FORWARDREQ )
        {
            ret = -EINVAL;
            break;
        }

        string strIfName, strObjPath;
        string strSender = pMsg.GetSender();
        if( !IsConnected( strSender.c_str() ) )
        {
            ret = ERROR_STATE;
            break;
        }

        strIfName = pMsg.GetInterface();
        if( IsPaused( strIfName ) )
        {
            ret = ERROR_PAUSED;
            break;
        }

        CReqBuilder okaReq( this );

        guint64 iTaskId = 0;
        if( true )
        {
            // retrieve the task id
            DMsgPtr pOrigMsg;
            ret = pMsg.GetMsgArgAt( 1, pOrigMsg );
            if( ERROR( ret ) )
                break;

            if( pOrigMsg.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            ObjPtr pObj;
            ret = pOrigMsg.GetObjArgAt( 0, pObj );
            if( ERROR( ret ) )
                break;

            IConfigDb* pOrigReq = pObj;
            if( pOrigReq == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            CReqOpener oOrigReq( pOrigReq );
            ret = oOrigReq.GetTaskId( iTaskId );
            if( ERROR( ret ) )
                break;

            ret = oOrigReq.GetIfName( strIfName );
            if( ERROR( ret ) )
                break;

            ret = oOrigReq.GetObjPath( strObjPath );
            if( ERROR( ret ) )
                break;
        }

        okaReq.Push( iTaskId );
        okaReq.Push( ( guint32 )KATerm );
        okaReq.Push( strIfName );
        okaReq.Push( strObjPath );

        okaReq.SetMethodName(
            SYS_EVENT_KEEPALIVE );

        okaReq.SetCallFlags( 
           DBUS_MESSAGE_TYPE_SIGNAL
           | CF_ASYNC_CALL );

        string strVal = pMsg.GetDestination( );
        if( strVal.empty() )
        {
            ret = -EINVAL;
            break;
        }
        okaReq.SetSender( strVal );

        strVal = pMsg.GetSender( );
        okaReq.SetDestination( strVal );

        TaskletPtr pDummyTask;

        ret = pDummyTask.NewObj(
            clsid( CIfDummyTask ) );

        if( ERROR( ret ) )
            break;

        ret = BroadcastEvent(
            okaReq.GetCfg(), pDummyTask );

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarder::OnRmtSvrOffline(
    const string& strIpAddr,
    HANDLE hPort )
{
    gint32 ret = 0;
    do{
        vector< string > vecUniqNames;
        ret = ClearRefCountByAddr(
            strIpAddr, vecUniqNames );

        // no client referring this addr
        if( ret == 0 )
            break;

        for( auto& strDest : vecUniqNames )
        {
            CReqBuilder oParams( this );

            oParams.Push( strIpAddr );
            oParams.Push( false );

            // correct the ifname 
            oParams.SetIfName( DBUS_IF_NAME(
                IFNAME_REQFORWARDER ) );

            oParams.SetMethodName(
                SYS_EVENT_RMTSVREVENT );

            oParams.SetDestination( strDest );

            oParams.SetCallFlags( 
               DBUS_MESSAGE_TYPE_SIGNAL
               | CF_ASYNC_CALL );

            TaskletPtr pDummyTask;
            ret = pDummyTask.NewObj(
                clsid( CIfDummyTask ) );

            if( ERROR( ret ) )
                break;

            ret = BroadcastEvent(
                oParams.GetCfg(), pDummyTask );
        }
    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarder::OnRmtSvrEvent(
    EnumEventId iEvent,
    const std::string& strIpAddr,
    HANDLE hPort )
{
    if( hPort == INVALID_HANDLE )
        return -EINVAL;

    if( iEvent != eventRmtSvrOffline )
        return super::OnRmtSvrEvent(
            iEvent, strIpAddr, hPort );

    return OnRmtSvrOffline( strIpAddr, hPort );
}

CRpcReqForwarderProxy::CRpcReqForwarderProxy(
    const IConfigDb* pCfg )
    : CInterfaceProxy( pCfg ),
    super( pCfg )
{
    gint32 ret = 0;
    // SetClassId( clsid( CRpcReqForwarderProxy ) );

    do{
        ObjPtr pObj;
        CCfgOpener oCfg( pCfg );

        ret = oCfg.GetObjPtr(
            propParentPtr, pObj );

        if( ERROR( ret ) )
            break;

        m_pParent = pObj;
        if( m_pParent == nullptr )
        {
            ret = -EFAULT;
            break;
        }
    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error occurs in CRpcReqForwarderProxy ctor" );
        throw runtime_error( strMsg );
    }
}

CRpcReqForwarderProxy::~CRpcReqForwarderProxy()
{
}

gint32 CRpcReqForwarderProxy::SetupReqIrpFwrdReq(
    IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pReqCall == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        guint32 dwCallFlags = 0;
        CReqOpener oReq( pReqCall );
        ret = oReq.GetCallFlags( dwCallFlags );
        if( ERROR( ret ) )
            break;

        // the `msg' object
        DMsgPtr pReqMsg;
        ret = oReq.Pop( pReqMsg );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 
        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );
        pIrpCtx->SetCtrlCode( CTRLCODE_SEND_REQ );

        // FIXME: we don't know if the request
        // needs a reply, and the flag is set
        // to CF_WITH_REPLY mandatorily
        guint32 dwIoDir = IRP_DIR_OUT;
        if( oReq.HasReply() )
            dwIoDir = IRP_DIR_INOUT;

        pIrpCtx->SetIoDirection( dwIoDir ); 

        BufPtr pBuf( true );
        *pBuf = pReqMsg;

        pIrpCtx->SetReqData( pBuf );
        pIrp->SetCallback( pCallback, 0 );
        pIrp->SetIrpThread( GetIoMgr() );

        guint32 dwTimeoutSec = 0;
        ret = oReq.GetTimeoutSec( dwTimeoutSec );
        if( ERROR( ret ) )
        {
            dwTimeoutSec =
                IFSTATE_ENABLE_EVENT_TIMEOUT;
            ret = 0;
        }
        pIrp->SetTimer( dwTimeoutSec, GetIoMgr() );

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderProxy::SetupReqIrpEnableEvt(
    IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{

    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pReqCall == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        guint32 dwCallFlags = 0;
        CReqOpener oReq( pReqCall );
        ret = oReq.GetCallFlags( dwCallFlags );
        if( ERROR( ret ) )
            break;

        // the `match' object
        ObjPtr pObj;
        ret = oReq.Pop( pObj );
        if( ERROR( ret ) )
            break;

        bool bEnable;
        ret = oReq.Pop( bEnable );
        if( ERROR( ret ) )
            break;

        IrpCtxPtr& pIrpCtx = pIrp->GetTopStack(); 
        pIrpCtx->SetMajorCmd( IRP_MJ_FUNC );
        pIrpCtx->SetMinorCmd( IRP_MN_IOCTL );

        if( bEnable ) 
        {
            pIrpCtx->SetCtrlCode(
                CTRLCODE_REG_MATCH );
        }
        else
        {
            pIrpCtx->SetCtrlCode(
                CTRLCODE_UNREG_MATCH );
        }

        guint32 dwIoDir = IRP_DIR_OUT;
        if( oReq.HasReply() )
            dwIoDir = IRP_DIR_INOUT;

        pIrpCtx->SetIoDirection( dwIoDir ); 

        BufPtr pBuf( true );
        *pBuf = pObj;

        pIrpCtx->SetReqData( pBuf );
        pIrp->SetCallback( pCallback, 0 );
        pIrp->SetIrpThread( GetIoMgr() );

        guint32 dwTimeoutSec = 0;
        ret = oReq.GetTimeoutSec( dwTimeoutSec );
        if( ERROR( ret ) )
        {
            dwTimeoutSec =
                IFSTATE_ENABLE_EVENT_TIMEOUT;
            ret = 0;
        }
        pIrp->SetTimer( dwTimeoutSec, GetIoMgr() );

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderProxy::SetupReqIrp(
    IRP* pIrp,
    IConfigDb* pReqCall,
    IEventSink* pCallback )
{
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pReqCall == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CReqOpener oReq( pReqCall );
        string strMethod;

        ret = oReq.GetMethodName( strMethod );
        if( ERROR( ret ) )
            break;

        if( strMethod == IF_METHOD_ENABLEEVT ||
            strMethod == IF_METHOD_DISABLEEVT )
        {
            ret = SetupReqIrpEnableEvt(
                pIrp, pReqCall, pCallback );
            break;
        }
        else if( strMethod == SYS_METHOD_FORWARDREQ )
        {
            ret = SetupReqIrpFwrdReq(
                pIrp, pReqCall, pCallback );
            break;

        }
        else
        {
            ret = super::SetupReqIrp(
                pIrp, pReqCall, pCallback );
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderProxy::FillRespData(
    IRP* pIrp, CfgPtr& pResp )
{
    gint32 ret = 0;
    if( pIrp == nullptr ||
        pIrp->GetStackSize() == 0 )
        return -EINVAL;

    if( pResp.IsEmpty() )
        return -EINVAL;

    // retrieve the data from the irp
    IrpCtxPtr& pCtx = pIrp->GetTopStack();
    if( pCtx->GetMajorCmd() != IRP_MJ_FUNC ||
        pCtx->GetMinorCmd() != IRP_MN_IOCTL )
        return -ENOTSUP;

    CParamList oParams( (IConfigDb*) pResp );
    guint32 dwCtrlCode = pCtx->GetCtrlCode();

    switch( dwCtrlCode )
    {
    case CTRLCODE_REG_MATCH:
    case CTRLCODE_UNREG_MATCH:
        {
            break;
        }
    case CTRLCODE_FORWARD_REQ:
        {
            DMsgPtr pMsg;
            pMsg = ( DMsgPtr& )*pCtx->m_pRespData;
            oParams.Push( pMsg );
            break;
        }
    default:
        {
            ret = super::FillRespData(
                pIrp, pResp );
            break;
        }
    }
    return ret;
}

gint32 CRpcReqForwarderProxy::ForwardRequest(
    const std::string& strSrcIp,
    DBusMessage* pMsg,
    DMsgPtr& pRespMsg,
    IEventSink* pCallback )
{
    if( pMsg == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        DMsgPtr pReqMsg( pMsg );
        CReqBuilder oBuilder( this );

        CIoManager* pMgr;

        oBuilder.GetPointer( propIoMgr, pMgr );
        string strModName = pMgr->GetModName();
        string strSender =
            DBUS_DESTINATION( strModName );

        // set the correct sender
        pReqMsg.SetSender( strSender );

        oBuilder.Push( pReqMsg );

        oBuilder.SetMethodName(
            SYS_METHOD_FORWARDREQ );

        oBuilder.SetCallFlags( CF_WITH_REPLY
           | DBUS_MESSAGE_TYPE_METHOD_CALL 
           | CF_ASYNC_CALL );

        // a tcp default round trip time
        oBuilder.SetTimeoutSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT ); 

        oBuilder.SetKeepAliveSec(
            IFSTATE_DEFAULT_IOREQ_TIMEOUT / 2 ); 

        CParamList oRespCfg;
        CfgPtr pRespCfg = oRespCfg.GetCfg();
        if( pRespCfg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        ret = RunIoTask( oBuilder.GetCfg(),
            pRespCfg, pCallback );

        if( ret == STATUS_PENDING )
            break;

        if( SUCCEEDED( ret ) )
        {
            CCfgOpener oCfg(
                ( IConfigDb* )pRespCfg );

            gint32 iRet = 0;
            ret = oCfg.GetIntProp(
                propReturnValue,
                ( guint32& )iRet );

            if( ERROR( ret ) )
                break;

            ret = iRet;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderProxy::DoInvoke(
    DBusMessage* pEvtMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;

    do{
        if( pEvtMsg == nullptr
            || pCallback == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        DMsgPtr pMsg( pEvtMsg );

        if( pMsg.GetType()
            != DBUS_MESSAGE_TYPE_SIGNAL )
        {
            ret = -EINVAL;
            break;
        }

        string strMethod = pMsg.GetMember();
        if( strMethod.empty() )
        {
            ret = -EBADMSG;
            break;
        }

        if( strMethod == SYS_EVENT_KEEPALIVE )
        {
            ret = OnKeepAlive( pCallback, KAOrigin ); 
            break;
        }
        else
        {
            // ignore ip address 
            string strSrcIp( "" );
            ret = ForwardEvent( strSrcIp,
                pEvtMsg, pCallback );
        }

    }while( 0 );

    if( ret == -ENOTSUP )
    {
        ret = super::DoInvoke(
            pEvtMsg, pCallback );
    }

    return ret;
}

gint32 CRpcRfpForwardEventTask::RunTask()
{
    gint32 ret = 0;

    do{
        CParamList oParams( GetConfig() );
        ObjPtr pObj;
        ret = oParams.GetObjPtr(
            propRouterPtr, pObj );

        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = pObj;
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        DMsgPtr pEvtMsg;
        ret = oParams.GetMsgPtr(
            propMsgPtr, pEvtMsg );

        if( ERROR( ret ) )
            break;

        guint32 dwPortId = 0;
        ret = oParams.GetIntProp(
            propPortId, dwPortId );

        if( ERROR( ret ) )
            break;

        InterfPtr pIf;
        ret = pRouter->GetBridge( dwPortId, pIf );
        if( ERROR( ret ) )
            break;

        CRpcTcpBridge* pBridge = pIf;
        if( pBridge == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        string strSrcIp( "" );
        ret = pBridge->ForwardEvent(
            strSrcIp, pEvtMsg, this );

    }while( 0 );

    return ret;

}

gint32 CRpcReqForwarderProxy::ForwardEvent(
    const std::string& strSrcIp,
    DBusMessage* pEventMsg,
    IEventSink* pCallback )
{
    gint32 ret = 0;
    if( pEventMsg == nullptr ||
        pCallback == nullptr )
        return -EINVAL;

    do{
        CRpcRouter* pRouter = GetParent();

        CParamList oParams;
        oParams.SetObjPtr(
            propRouterPtr, ObjPtr( pRouter ) );

        oParams.SetMsgPtr(
            propMsgPtr, DMsgPtr( pEventMsg ) );

        CCfgOpenerObj oCfg( pCallback );
        CRouterRemoteMatch* pMatch = nullptr;
        gint32 ret = oCfg.GetPointer(
            propMatchPtr, pMatch );

        if( ERROR( ret ) )
            break;

        ret = oParams.CopyProp(
            propPortId, pMatch );
        if( ERROR( ret ) )
            break;

        ret = GetIoMgr()->ScheduleTask(
            clsid( CRpcRfpForwardEventTask ),
            oParams.GetCfg() );

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderProxy::CustomizeRequest(
    IConfigDb* pReqCfg,
    IEventSink* pCallback )
{
    if( pReqCfg == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        CParamList oParams( pReqCfg );
        ObjPtr pObj = oParams[ 0 ];
        IConfigDb* pDesc = pObj;
        if( pDesc == nullptr )
        {
            ret = EFAULT;
            break;
        }
        if( pDesc->exist( propMethodName ) )
            break;

        CCfgOpener oDesc( pDesc );
        string strMethod = oDesc[ propMethodName ];
        if( strMethod == SYS_METHOD_SENDDATA ||
            strMethod == SYS_METHOD_FETCHDATA )
        {
            // set the destination information with the
            // ones in the data description
            oParams.CopyProp(
                propIfName, pDesc );

            oParams.CopyProp(
                propObjPath, pDesc );

            oParams.CopyProp(
                propDestDBusName, pDesc );
        }
    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderProxy::InitUserFuncs()
{
    BEGIN_IFHANDLER_MAP( CRpcReqForwarder );
    END_HANDLER_MAP;
    return 0;
}

gint32 CRpcReqForwarderProxy::RebuildMatches()
{
    do{
        // add interface id to all the matches
        CCfgOpenerObj oIfCfg( this );
        guint32 dwQueSize = MAX_PENDING_MSG;

        // empty all the matches, the interfaces will
        // be added to the vector on the remote req.
        m_vecMatches.clear();

        // set the interface id for this match
        CCfgOpenerObj oMatchCfg(
            ( CObjBase* )m_pIfMatch );

        oMatchCfg.SetBoolProp(
            propDummyMatch, true );

        oMatchCfg.SetIntProp(
            propIid, GetClsid() );

        dwQueSize = MAX_PENDING_MSG;

        oMatchCfg.SetIntProp(
            propQueSize, dwQueSize );

        // also append the match for master interface
        m_vecMatches.push_back( m_pIfMatch );

    }while( 0 );

    return 0;
}

gint32 CRpcReqForwarderProxy::ForwardModOnOfflineEvent(
    EnumEventId iEvent,
    const std::string& strModule )
{
    return 0;
}

gint32 CRpcReqForwarderProxy::ForwardDBusEvent(
    EnumEventId iEvent )
{
    return 0;
}

gint32 CRpcReqForwarderProxy::AddInterface(
    IMessageMatch* pMatch )
{
    if( pMatch == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        MatchPtr ptrMatch( pMatch );
        CStdRMutex oIfLock( GetLock() );
        std::map< MatchPtr, IFREF >::iterator
            itr = m_mapMatchRefs.find( ptrMatch );

        if( itr == m_mapMatchRefs.end() )
        {
            m_mapMatchRefs[ ptrMatch ] =
                IFREF(m_vecMatches.size(), 1 );

            m_vecMatches.push_back( ptrMatch );
        }
        else
        {
            ++itr->second.second;
        }

    }while( 0 );

    return ret;
}

gint32 CRpcReqForwarderProxy::RemoveInterface(
    IMessageMatch* pMatch )
{
    if( pMatch == nullptr )
        return -EINVAL;

    gint32 ret = 0;
    do{
        MatchPtr ptrMatch( pMatch );
        CStdRMutex oIfLock( GetLock() );
        std::map< MatchPtr, IFREF >::iterator
            itr = m_mapMatchRefs.find( ptrMatch );

        if( itr != m_mapMatchRefs.end() )
        {
            --itr->second.second;
            if( itr->second.second <= 0 )
            {
                m_vecMatches.erase(
                    m_vecMatches.begin() +
                    itr->second.first );

                m_mapMatchRefs.erase( ptrMatch );
            }
        }

    }while( 0 );

    return ret;
}

gint32 CReqFwdrSendDataTask::RunTask()
{
    gint32 ret = 0;

    ObjPtr pObj;
    CParamList oParams( GetConfig() );
    DMsgPtr pRespMsg;

    do{
        CRpcReqForwarder* pIf;
        ret = oParams.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = oParams.GetObjPtr(
            propRouterPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = pObj;
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        string strIpAddr;
        oParams.GetStrProp( 0, strIpAddr );
        if( ERROR( ret ) )
            break;

        ObjPtr pDataDesc;
        oParams.GetObjPtr( 1, pDataDesc );
        if( ERROR( ret ) )
            break;

        guint32 dwFd = 0;
        oParams.GetIntProp( 2, dwFd );
        if( ERROR( ret ) )
            break;

        guint32 dwOffset = 0;
        oParams.GetIntProp( 3, dwOffset );
        if( ERROR( ret ) )
            break;

        guint32 dwSize = 0;
        oParams.GetIntProp( 4, dwSize );
        if( ERROR( ret ) )
            break;

        IConfigDb* pDesc = pDataDesc;
        if( unlikely( pDesc == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        CParamList oDesc( pDesc );
        string strDest;
        ret = oDesc.GetStrProp(
            propDestDBusName, strDest );
        if( ERROR( ret ) )
            break;

        InterfPtr bridgePtr;

        if( pIf->GetClsid() ==
            clsid( CRpcReqForwarderImpl ) )
        {
            ret = pRouter->GetBridgeProxy(
                strIpAddr, bridgePtr );

            CRpcTcpBridgeProxy* pProxy = bridgePtr;
            if( pProxy == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pProxy->SendData( pDataDesc,
                dwFd, dwOffset, dwSize, this );
        }
        else if( pIf->GetClsid() ==
            clsid( CRpcTcpBridgeImpl ) )
        {
            ret = pRouter->GetReqFwdrProxy(
                strDest, bridgePtr );

            CRpcReqForwarderProxy* pProxy =
                bridgePtr;

            if( pProxy == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pProxy->SendData( pDataDesc,
                dwFd, dwOffset, dwSize, this );
        }

    }while( 0 );

    if( ret != STATUS_PENDING && !Retriable( ret ) )
    {
        oParams.GetObjPtr(
            propRespPtr, pObj );

        CParamList oRespCfg(
            ( IConfigDb* )pObj );

        oRespCfg.SetIntProp(
            propReturnValue, ret );

        if( !pRespMsg.IsEmpty() )
            oRespCfg.Push( pRespMsg );

        OnTaskComplete( ret );
    }

    return ret;
}

gint32 CReqFwdrSendDataTask::OnTaskComplete(
    gint32 iRetVal )
{
    if( iRetVal == STATUS_PENDING )
        return iRetVal;

    gint32 ret = 0;
    if( Retriable( iRetVal ) )
    {
        ret = STATUS_MORE_PROCESS_NEEDED;
    }
    else if( ERROR( iRetVal ) )
    {
        ret = ERROR_FAIL;
    }

    if( IsPending() )
        OnServiceComplete( ret );

    return ret;
}

gint32 CReqFwdrSendDataTask::OnServiceComplete(
    gint32 iRetVal )
{
    if( iRetVal == STATUS_PENDING ||
        Retriable( iRetVal ) )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ObjPtr pObj;
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ret = oCfg.GetObjPtr(
            propEventSink, pObj );

        if( ERROR( ret ) )
            break;

        IEventSink* pTask = pObj;
        if( pTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CInterfaceServer* pReqFwdr = pObj;
        if( pReqFwdr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        CParamList oParams;
        oParams.SetIntProp(
            propReturnValue, iRetVal );

        ret = pReqFwdr->OnServiceComplete( 
            CfgPtr( oParams.GetCfg() ), pTask );

    }while( 0 );

    return ret;
}

gint32 CReqFwdrFetchDataTask::RunTask()
{
    gint32 ret = 0;

    ObjPtr pObj;
    CParamList oParams( GetConfig() );
    DMsgPtr pRespMsg;

    do{
        CRpcReqForwarder* pIf;
        ret = oParams.GetObjPtr( propIfPtr, pObj );
        if( ERROR( ret ) )
            break;

        pIf = pObj;
        if( pIf == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ret = oParams.GetObjPtr(
            propRouterPtr, pObj );
        if( ERROR( ret ) )
            break;

        CRpcRouter* pRouter = pObj;
        if( pRouter == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        string strIpAddr;
        oParams.GetStrProp( 0, strIpAddr );
        if( ERROR( ret ) )
            break;

        ObjPtr pDataDesc;
        oParams.GetObjPtr( 1, pDataDesc );
        if( ERROR( ret ) )
            break;

        guint32 dwFd = 0;
        oParams.GetIntProp( 2, dwFd );
        if( ERROR( ret ) )
            break;

        guint32 dwOffset = 0;
        oParams.GetIntProp( 3, dwOffset );
        if( ERROR( ret ) )
            break;

        guint32 dwSize = 0;
        oParams.GetIntProp( 4, dwSize );
        if( ERROR( ret ) )
            break;

        IConfigDb* pDesc = pDataDesc;
        if( unlikely( pDesc == nullptr ) )
        {
            ret = -EFAULT;
            break;
        }

        CParamList oDesc( pDesc );
        string strDest;
        ret = oDesc.GetStrProp(
            propDestDBusName, strDest );
        if( ERROR( ret ) )
            break;

        InterfPtr bridgePtr;

        if( pIf->GetClsid() ==
            clsid( CRpcReqForwarderImpl ) )
        {
            ret = pRouter->GetBridgeProxy(
                strIpAddr, bridgePtr );

            CRpcTcpBridgeProxy* pProxy = bridgePtr;
            if( pProxy == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pProxy->FetchData( pDataDesc,
                ( gint32& )dwFd, dwOffset, dwSize, this );
        }
        else if( pIf->GetClsid() ==
            clsid( CRpcTcpBridgeImpl ) )
        {
            ret = pRouter->GetReqFwdrProxy(
                strDest, bridgePtr );

            CRpcReqForwarderProxy* pProxy =
                bridgePtr;

            if( pProxy == nullptr )
            {
                ret = -EFAULT;
                break;
            }
            ret = pProxy->FetchData( pDataDesc,
                ( gint32& )dwFd, dwOffset, dwSize, this );
        }

    }while( 0 );

    if( ret != STATUS_PENDING && !Retriable( ret ) )
    {
        oParams.GetObjPtr(
            propRespPtr, pObj );

        CParamList oRespCfg(
            ( IConfigDb* )pObj );

        oRespCfg.SetIntProp(
            propReturnValue, ret );

        if( !pRespMsg.IsEmpty() )
            oRespCfg.Push( pRespMsg );

        OnTaskComplete( ret );
    }

    return ret;
}

gint32 CReqFwdrFetchDataTask::OnTaskComplete(
    gint32 iRetVal )
{
    if( iRetVal == STATUS_PENDING )
        return iRetVal;

    gint32 ret = 0;
    if( Retriable( iRetVal ) )
    {
        ret = STATUS_MORE_PROCESS_NEEDED;
    }
    else if( ERROR( iRetVal ) )
    {
        ret = ERROR_FAIL;
    }

    if( IsPending() )
        OnServiceComplete( ret );

    return ret;
}

gint32 CReqFwdrFetchDataTask::OnServiceComplete(
    gint32 iRetVal )
{
    if( iRetVal == STATUS_PENDING ||
        Retriable( iRetVal ) )
        return -EINVAL;

    gint32 ret = 0;
    do{
        ObjPtr pObj;
        CCfgOpener oCfg( ( IConfigDb* )GetConfig() );
        ret = oCfg.GetObjPtr(
            propEventSink, pObj );

        if( ERROR( ret ) )
            break;

        IEventSink* pTask = pObj;
        if( pTask == nullptr )
        {
            ret = -EFAULT;
            break;
        }
        ret = oCfg.GetObjPtr( propIfPtr, pObj );

        if( ERROR( ret ) )
            break;

        CInterfaceServer* pReqFwdr = pObj;
        if( pReqFwdr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        TaskletPtr pCallerTask;
        ret = GetCallerTask( pCallerTask );
        if( ERROR( ret ) )
            break;

        // get the response
        CCfgOpenerObj oCallerCfg(
            ( CObjBase* )pCallerTask );

        ret = oCallerCfg.GetObjPtr(
            propRespPtr, pObj );

        if( ERROR( ret ) )
        {
            if( SUCCEEDED( iRetVal ) )
                iRetVal = ret;
            
            // if error, generate a response
            CCfgOpener oResp;
            oResp.SetIntProp(
                propReturnValue, iRetVal );

            oCfg.SetObjPtr( propRespPtr,
                oResp.GetCfg() );

            ret = pReqFwdr->OnServiceComplete( 
                oCfg.GetCfg(), pTask );
        }
        else
        {
            oCfg.SetObjPtr( propRespPtr, pObj );
            ret = pReqFwdr->OnServiceComplete( 
                pObj, pTask );
        }
        oCfg.RemoveProperty( propRespPtr );

    }while( 0 );

    return ret;
}
