/*
 * =====================================================================================
 *
 *       Filename:  counters.h
 *
 *    Description:  declaration of the statistic counters interface
 *
 *        Version:  1.0
 *        Created:  11/18/2018 12:15:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */

#pragma once

#include "configdb.h"
#include "defines.h"
#include "ifhelper.h"
#include <exception>

#define METHOD_GetCounters "GetCounters"
#define METHOD_GetCounter  "GetCounter"

// a read-only interface
struct IStatCounters
{
    virtual gint32 GetCounters(
        CfgPtr& pCfg ) = 0;

    virtual gint32 GetCounter( guint32 iPropId,
        BufPtr& pBuf ) = 0;
};

class CStatCountersProxy:
    public virtual CAggInterfaceProxy,
    public IStatCounters
{

    public:
    typedef CAggInterfaceProxy super;
    CStatCountersProxy( const IConfigDb* pCfg )
        : super( pCfg )
    {}

    const EnumClsid GetIid() const
    { return iid( CStatCounters ); }

    virtual gint32 InitUserFuncs();
    gint32 GetCounters( CfgPtr& pCfg );
    gint32 GetCounter( guint32 iPropId, BufPtr& pBuf );
};

class CStatCountersServer:
    public virtual CAggInterfaceServer
{
    // storage for the counters
    CfgPtr m_pCfg;

    // message filter for message counter
    TaskletPtr m_pMsgFilter;

    public:
    typedef CAggInterfaceServer super;
    CStatCountersServer( const IConfigDb* pCfg )
        : super( pCfg )
    {}

    const EnumClsid GetIid() const
    { return iid( CStatCounters ); }

    virtual gint32 InitUserFuncs();
    virtual gint32 OnPreStart( IEventSink* pCallback );
    virtual gint32 OnPostStop( IEventSink* pCallback );

    gint32 IncMsgCount( EnumPropId iProp );

    gint32 IncMsgOutCount();

    gint32 GetCounters(
        IEventSink* pCallback,
        CfgPtr& pCfg );

    gint32 GetCounter(
        IEventSink* pCallback,
        guint32 iPropId,
        BufPtr& pBuf  );
};

class CMessageCounterTask : 
    public CThreadSafeTask,
    public IMessageFilter
{ 
    public:

    typedef CThreadSafeTask super;
    CMessageCounterTask( const IConfigDb* pCfg )
        : super( pCfg )
    { SetClassId( clsid( CMessageCounterTask ) ); }

    virtual gint32 FilterMsgIncoming(
        IEventSink* pInvTask, DBusMessage* pMsg )
    {
        CCfgOpenerObj oTaskCfg( this );    
        CStatCountersServer* pIf = nullptr;

        gint32 ret = oTaskCfg.GetPointer(
            propIfPtr, pIf );
        if( ERROR( ret ) )
            return 0;

        return pIf->IncMsgCount( propMsgCount );
    }

    virtual gint32 FilterMsgIncoming(
        IEventSink* pInvTask, IConfigDb* pMsg )
    { return 0; }

    virtual gint32 GetFilterStrategy(
        IEventSink* pCallback,
        DBusMessage* pMsg,
        EnumStrategy& iStrategy )
    { return stratIgnore; }

    virtual gint32 GetFilterStrategy(
        IEventSink* pCallback,
        IConfigDb* pMsg,
        EnumStrategy& iStrategy )
    { return stratIgnore; }

    virtual gint32 FilterMsgOutgoing(
        IEventSink* pReqTask, IConfigDb* pMsg )
    {
        CCfgOpenerObj oTaskCfg( this );    
        CStatCountersServer* pIf = nullptr;

        gint32 ret = oTaskCfg.GetPointer(
            propIfPtr, pIf );
        if( ERROR( ret ) )
            return 0;

        return pIf->IncMsgCount( propMsgRespCount );
    }

    virtual gint32 FilterMsgOutgoing(
        IEventSink* pReqTask, DBusMessage* pMsg )
    {
        CCfgOpenerObj oTaskCfg( this );    
        CStatCountersServer* pIf = nullptr;

        gint32 ret = oTaskCfg.GetPointer(
            propIfPtr, pIf );
        if( ERROR( ret ) )
            return 0;

        return pIf->IncMsgCount( propMsgOutCount );
    }

    virtual gint32 Process( guint32 dwContext )
    { return 0; }
};
