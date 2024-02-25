/*
 * =====================================================================================
 *
 *       Filename:  msgmatch.h
 *
 *    Description:  declaration of message match classes
 *
 *        Version:  1.0
 *        Created:  10/16/2016 10:06:25 AM
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

#include <sys/time.h>
#include "defines.h"
#include "utils.h"
#include "configdb.h"
#include "namespc.h"

namespace rpcf
{

typedef enum {

     matchServer,
     matchClient,
     matchInvalid

}EnumMatchType;

class IMessageMatch : public CObjBase
{
    public:

    // if a event message this interface has subcribed
    virtual gint32 IsMyMsgIncoming(
        DBusMessage* pMsg ) const = 0;

    virtual gint32 IsMyMsgOutgoing(
        DBusMessage* pMsg ) const = 0;

    // if a event message this interface has subcribed
    virtual gint32 IsMyMsgIncoming(
        IConfigDb* pMsg ) const = 0;

    virtual gint32 IsMyMsgOutgoing(
        IConfigDb* pMsg ) const = 0;

    // a equalness comparison
    virtual bool operator==(
        const IMessageMatch& rhs ) const = 0;

    virtual const IMessageMatch& operator=(
        const IMessageMatch& rhs ) = 0;

    virtual stdstr ToDBusRules(
        gint32 iMsgType ) const = 0;

    virtual stdstr ToString() const = 0;

    virtual EnumMatchType GetType() const = 0;

    virtual bool IsMyDest(
        const stdstr& strDest ) const = 0;

    virtual bool IsMyObjPath(
        const stdstr& strObjPath ) const = 0;

    virtual stdstr GetDest() const = 0;

    virtual const CfgPtr& GetCfg() const = 0;
};

typedef CAutoPtr< Clsid_Invalid, IMessageMatch > MatchPtr;

}

namespace std
{
    using namespace rpcf;
    template<>
    struct less<MatchPtr>
    {
        bool operator()(const MatchPtr& k1, const MatchPtr& k2) const
            {
                stdstr strVal1 = k1->ToString();
                stdstr strVal2 = k2->ToString();

                if( strVal1.empty() || strVal2.empty() )
                    return false;

                if( strVal1 < strVal2 )
                    return true;

                return false;
            }
    };
}

namespace rpcf
{
class CMessageMatch : public IMessageMatch
{
    protected:

    stdstr     m_strObjPath;
    stdstr     m_strIfName;
    EnumMatchType   m_iMatchType;
    CfgPtr          m_pCfg;

    // if a request message this interface can handle
    gint32 IsMyMsgBasic( DBusMessage* pMessage ) const;
    gint32 IsMyMsgBasic( IConfigDb* pMessage ) const;

    public:
    typedef IMessageMatch super;

    struct SERI_HEADER : public SERI_HEADER_BASE
    {
        typedef SERI_HEADER_BASE super;

        gint32 iMatchType;
        SERI_HEADER() : SERI_HEADER_BASE()
        {
            iMatchType = 0;
        }

        void ntoh()
        {
            super::ntoh();
            iMatchType = ntohl( iMatchType );
        }

        void hton()
        {
            super::hton();
            iMatchType = htonl( iMatchType );
        }
    };

    CMessageMatch() : m_pCfg( true )
    {
        SetClassId( clsid( CMessageMatch ) );
    }

    // if a event message this interface has subcribed
    gint32 IsMyMsgIncoming(
        IConfigDb* pMsg ) const override;

    gint32 IsMyMsgOutgoing(
        IConfigDb* pMsg ) const override;

    CMessageMatch( const IConfigDb* pCfg );

    gint32 IsMyMsgIncoming(
        DBusMessage* pMessage ) const override;

    // if a event message this interface has subcribed
    gint32 IsMyMsgOutgoing(
        DBusMessage* pMessage ) const override;

    stdstr GetDest() const override;
    
    bool IsMyObjPath(
        const stdstr& strObjPath ) const override;

    bool IsMyDest(
        const stdstr& strDest ) const override;

    // a equalness comparison
    bool operator==(
        const IMessageMatch& rhs ) const override;

    const IMessageMatch& operator=(
        const IMessageMatch& rhs ) override;

    EnumMatchType GetType() const override
    { return m_iMatchType; }

    inline void SetType( EnumMatchType iType )
    { m_iMatchType = iType; }

    inline gint32 GetIfName(
        stdstr& strIfName ) const
    {
        strIfName = m_strIfName;
        return 0;
    }

    inline void SetIfName(
        const stdstr& strIfName )
    { m_strIfName = strIfName; }

    inline gint32 GetObjPath(
        stdstr& strObjPath ) const
    {
        strObjPath = m_strObjPath;
        return 0;
    }

    inline void SetObjPath(
        const stdstr& strObjPath )
    { m_strObjPath = strObjPath; }

    stdstr ToDBusRules(
        gint32 iMsgType ) const override;

    stdstr ToString() const override;

    gint32 EnumProperties(
        std::vector< gint32 >& vecProps ) const override;

    gint32 GetPropertyType(
        gint32 iProp, gint32& iType ) const override;

    bool exist( gint32 iProp ) const;

    gint32 GetProperty(
        gint32 iProp,
        Variant& oBuf ) const override;

    gint32 SetProperty(
        gint32 iProp,
        const Variant& oBuf ) override;

    gint32 RemoveProperty( gint32 iProp ) override;

    const CMessageMatch& operator=(
        const CMessageMatch& rhs );

    gint32 Serialize(
        CBuffer& oBuf ) const override;

    gint32 Deserialize(
        const CBuffer& oBuf ) override;

    gint32 Deserialize(
        const char* pBuf,
        guint32 dwSize ) override;

    const CfgPtr& GetCfg() const
    { return m_pCfg; }

    CfgPtr& GetCfg()
    { return m_pCfg; }

    gint32 Clone( CMessageMatch* rhs );
};

class CProxyMsgMatch : public CMessageMatch
{
    public:
    typedef CMessageMatch super;
    CProxyMsgMatch( const IConfigDb* pCfg );
    
    // test if a event message this match is
    // listening to

    gint32 IsMyMsgIncoming(
        DBusMessage* pDBusMsg ) const override;

    stdstr ToString() const override;
};

class CDBusDisconnMatch : public CMessageMatch
{
    public:
    typedef CMessageMatch super;
    CDBusDisconnMatch( const IConfigDb* pCfg );

    gint32 IsMyMsgIncoming(
        DBusMessage* pMessage ) const override;

    stdstr ToString() const override;
};

class CDBusSysMatch : public CMessageMatch
{
    public:
    CDBusSysMatch( const IConfigDb* pCfg = nullptr );
};

class CDBusLoopbackMatch : public CMessageMatch
{
    gint32 FilterMsgS2P(
        DBusMessage* pDBusMsg, bool bIncoming ) const;

    gint32 FilterMsgP2S(
        DBusMessage* pDBusMsg, bool bIncoming ) const;

    gint32 AddRemoveBusName(
        const stdstr& strName, bool bRemove );

    public:
    typedef CMessageMatch super;
    CDBusLoopbackMatch ( const IConfigDb* pCfg );

    gint32 IsMyMsgIncoming(
        DBusMessage* pDBusMsg ) const override;

    gint32 IsMyMsgOutgoing(
        DBusMessage* pDBusMsg ) const override;

    gint32 AddBusName(
        const stdstr& strName );

    gint32 RemoveBusName(
        const stdstr& strName );

    gint32 IsRegBusName(
        const stdstr& strBusName ) const;

};

class CRouterRemoteMatch : public CMessageMatch
{
    public:
    typedef CMessageMatch super;
    CRouterRemoteMatch( const IConfigDb* pCfg = nullptr );

    virtual gint32 CopyMatch( IMessageMatch* pMatch );
    stdstr ToString() const override;

    gint32 IsMyReqToRelay(
        IConfigDb* pReqCtx,
        DBusMessage* pReqMsg ) const;

    gint32 IsMyModEvent(
        DBusMessage* pEvtMsg ) const;

    gint32 IsMyEvtToForward(
        IConfigDb* pEvtCtx,
        DBusMessage* pEvtMsg ) const;

    virtual gint32 IsMyModule(
        const stdstr strModName );

    guint32 GetPortId();
};

class CRouterLocalMatch : public CRouterRemoteMatch
{
    public:
    typedef CRouterRemoteMatch super;
    CRouterLocalMatch( const IConfigDb* pCfg = nullptr );

    gint32 CopyMatch( IMessageMatch* pMatch ) override;

    stdstr ToString() const override;

    gint32 IsMyReqToForward(
        IConfigDb* pReqCtx,
        DBusMessage* pReqMsg ) const;

    gint32 IsMyEvtToRelay(
        IConfigDb* pEvtCtx,
        DBusMessage* pEvtMsg ) const;

    gint32 IsMyModule(
        const stdstr strModName ) override;
};

}
