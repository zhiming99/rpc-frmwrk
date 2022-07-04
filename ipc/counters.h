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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
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

namespace rpcf
{

// a read-only interface
struct IStatCounters
{
    protected:
    std::hashmap< gint32, Variant > m_mapCounters;

    gint32 IncCounter(
        EnumPropId iProp, bool bNegative );

    CStdRMutex m_oStatLock;

    public:
    gint32 IncCounter(
        EnumPropId iProp );

    gint32 DecCounter(
        EnumPropId iProp );

    gint32 SetCounter(
        EnumPropId iProp,
        guint32 dwVal );

    gint32 SetCounter(
        EnumPropId iProp,
        guint64 qwVal );

    gint32 GetCounter2( 
        guint32 iPropId,
        guint32& dwVal  );

    gint32 GetCounter2( 
        guint32 iPropId,
        guint64& qwVal  );

    gint32 GetCounters(
        CfgPtr& pCfg );

    gint32 GetCounter(
        guint32 iPropId,
        BufPtr& pBuf  );
};

class CStatCountersProxy:
    public virtual CAggInterfaceProxy
{

    IStatCounters m_oCounters;
    public:
    typedef CAggInterfaceProxy super;
    CStatCountersProxy( const IConfigDb* pCfg )
        : super( pCfg )
    {}

    const EnumClsid GetIid() const
    { return iid( CStatCounters ); }

    gint32 IncCounter(
        EnumPropId iProp ) override
    {
        return m_oCounters.IncCounter( iProp );
    };

    gint32 DecCounter(
        EnumPropId iProp ) override
    {
        return m_oCounters.DecCounter( iProp );
    }
    gint32 SetCounter(
        EnumPropId iProp, guint32 dwVal ) override
    {
        return m_oCounters.SetCounter(
            iProp, dwVal );
    }

    gint32 SetCounter(
        EnumPropId iProp, guint64 qwVal ) override
    {
        return m_oCounters.SetCounter(
            iProp, qwVal );
    }

    gint32 GetCounters( CfgPtr& pCfg )
    { return m_oCounters.GetCounters( pCfg ); }

    gint32 GetCounter(
        guint32 iPropId, BufPtr& pBuf )
    {
        return m_oCounters.GetCounter(
            iPropId, pBuf );
    }

    gint32 GetCounter2(
        guint32 iPropId, guint32& dwVal )
    {
        return m_oCounters.GetCounter2(
            iPropId, dwVal );
    }

    gint32 GetCounter2(
        guint32 iPropId, guint64& qwVal )
    {
        return m_oCounters.GetCounter2(
            iPropId, qwVal );
    }
};

class CStatCountersServer:
    public virtual CAggInterfaceServer
{
    // message filter for message counter
    TaskletPtr m_pMsgFilter;
    IStatCounters m_oCounters;

    public:
    typedef CAggInterfaceServer super;
    CStatCountersServer( const IConfigDb* pCfg )
        : super( pCfg )
    {}

    const EnumClsid GetIid() const
    { return iid( CStatCounters ); }

    virtual gint32 OnPreStart(
        IEventSink* pCallback ) override;

    virtual gint32 OnPostStop(
        IEventSink* pCallback ) override;

    gint32 IncCounter(
        EnumPropId iProp ) override
    {
        return m_oCounters.IncCounter( iProp );
    };

    gint32 DecCounter(
        EnumPropId iProp ) override
    {
        return m_oCounters.DecCounter( iProp );
    }
    gint32 SetCounter(
        EnumPropId iProp, guint32 dwVal ) override
    {
        return m_oCounters.SetCounter(
            iProp, dwVal );
    }

    gint32 SetCounter(
        EnumPropId iProp, guint64 qwVal ) override
    {
        return m_oCounters.SetCounter(
            iProp, qwVal );
    }

    gint32 GetCounters( CfgPtr& pCfg )
    { return m_oCounters.GetCounters( pCfg ); }

    gint32 GetCounter(
        guint32 iPropId, BufPtr& pBuf )
    {
        return m_oCounters.GetCounter(
            iPropId, pBuf );
    }

    gint32 GetCounter2(
        guint32 iPropId, guint32& dwVal )
    {
        return m_oCounters.GetCounter2(
            iPropId, dwVal );
    }

    gint32 GetCounter2(
        guint32 iPropId, guint64& qwVal )
    {
        return m_oCounters.GetCounter2(
            iPropId, qwVal );
    }
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

        return pIf->IncCounter( propMsgCount );
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

        return pIf->IncCounter( propMsgRespCount );
    }

    virtual gint32 FilterMsgOutgoing(
        IEventSink* pReqTask, DBusMessage* pMsg );

    virtual gint32 Process( guint32 dwContext )
    { return 0; }
};

}
