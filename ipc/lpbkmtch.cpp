/*
 * =====================================================================================
 *
 *       Filename:  lpbkmtch.cpp
 *
 *    Description:  implementation of the CDBusLoopbackMatch class
 *
 *        Version:  1.0
 *        Created:  04/08/2017 07:12:35 PM
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
#include <dbus/dbus.h>
#include <vector>
#include <string>
#include <regex>
#include "defines.h"
#include "port.h"
#include "dbusport.h"

CDBusLoopbackMatch::CDBusLoopbackMatch(
    const IConfigDb* pCfg ) : super()
{
    SetClassId( clsid( CDBusLoopbackMatch ) );

    gint32 ret = 0;
    do{
        if( pCfg == nullptr )
        {
            ret = -EINVAL;
            break;
        }

        CCfgOpener oInitCfg( pCfg );
        std::string strUniqName;

        ret = oInitCfg.GetStrProp(
            propSrcUniqName, strUniqName );

        if( ERROR( ret ) )
            break;

        ret = AddBusName( strUniqName );
        if( ERROR( ret ) )
            break;

        guint32* pMatchType =
            ( guint32* )&m_iMatchType;

        ret = oInitCfg.GetIntProp(
            propMatchType, *pMatchType );

        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "error in CDBusLoopbackMatch's ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CDBusLoopbackMatch::IsRegBusName(
    const std::string& strBusName ) const
{
    gint32 ret = 0;
    do{
        ObjPtr pObj;
        CCfgOpenerObj oMatch( this );

        ret = oMatch.GetObjPtr(
            propValList, pObj );

        if( ERROR( ret ) )
            break;

        StrSetPtr pDestSet( pObj );
        if( pDestSet.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        std::set< std::string >& oStrSet =
            ( *pDestSet )();

        if( oStrSet.find( strBusName ) ==
            oStrSet.end() )
        {
            ret = ERROR_FALSE;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusLoopbackMatch::FilterMsgS2P(
    DBusMessage* pDBusMsg, bool bIncoming ) const
{
    if( pDBusMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        DMsgPtr pMsg( pDBusMsg );
        gint32 iType = pMsg.GetType();
        if( !( iType == DBUS_MESSAGE_TYPE_METHOD_RETURN
            || iType == DBUS_MESSAGE_TYPE_SIGNAL
            || iType == DBUS_MESSAGE_TYPE_ERROR ) )
        {
            ret = ERROR_FALSE;
            break;
        }

        guint32 dwLen = sizeof(
            LOOPBACK_DESTINATION ) - 1;

        std::string strName;

        if( iType == DBUS_MESSAGE_TYPE_ERROR ||
            iType == DBUS_MESSAGE_TYPE_METHOD_RETURN )
        {
            // server side
            strName = pMsg.GetDestination();
            if( strName.empty() )
            {
                ret = -EBADMSG;
                break;
            }


            if( strName.substr( 0, dwLen ) !=
                LOOPBACK_DESTINATION )
                ret = ERROR_FALSE;
        }
        else if( iType == DBUS_MESSAGE_TYPE_SIGNAL )
        {
            if( bIncoming )
            {
                // proxy side
                strName = pMsg.GetSender();
                if( strName.empty() )
                {
                    ret = -EBADMSG;
                    break;
                }
                ret = IsRegBusName( strName );
            }
            else
            {
                // for outgoing signal,always let pass
                // the check, that is, the signal
                // message will go through the loopback
                // path 
                strName = pMsg.GetDestination();
                if( strName.empty() )
                {
                    ret = 0;
                }
                else if( strName.substr( 0, dwLen ) !=
                    LOOPBACK_DESTINATION )
                {
                    ret = ERROR_FALSE;
                }
                else
                {
                    ret = 0;
                }
                
            }
        }
        else
        {
            ret = ERROR_FALSE;
            break;
        }


    }while( 0 );

    return ret;
}

gint32 CDBusLoopbackMatch::FilterMsgP2S(
    DBusMessage* pDBusMsg, bool bIncoming ) const
{
    if( pDBusMsg == nullptr )
        return -EINVAL;

    gint32 ret = 0;

    do{
        DMsgPtr pMsg( pDBusMsg );
        gint32 iType = pMsg.GetType();
        if( iType != DBUS_MESSAGE_TYPE_METHOD_CALL )
        {
            ret = ERROR_FALSE;
            break;
        }

        std::string strName;

        strName = pMsg.GetSender();
        if( strName.empty() )
        {
            ret = -EBADMSG;
            break;
        }

        guint32 dwLen = sizeof(
            LOOPBACK_DESTINATION ) - 1;

        if( strName.substr( 0, dwLen ) !=
            LOOPBACK_DESTINATION )
            ret = ERROR_FALSE;

    }while( 0 );

    return ret;
}

gint32 CDBusLoopbackMatch::IsMyMsgIncoming(
    DBusMessage* pDBusMsg ) const
{
    gint32 ret = 0;
    if( GetType() == matchClient )
    {
        ret = FilterMsgS2P( pDBusMsg, true );
    }
    else if( GetType() == matchServer )
    {
        ret = FilterMsgP2S( pDBusMsg, true );
    }
    else
    {
        ret = -ENOTSUP;
    }
    return ret;
}

gint32 CDBusLoopbackMatch::AddBusName(
    const std::string& strName )
{
    return AddRemoveBusName( strName, false );
}

gint32 CDBusLoopbackMatch::RemoveBusName(
    const std::string& strName )
{
    return AddRemoveBusName( strName, true );
}

gint32 CDBusLoopbackMatch::AddRemoveBusName(
    const std::string& strName, bool bRemove )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( ( IConfigDb*) GetCfg() );
        if( unlikely( !oCfg.exist( propValList ) ) )
        {
            if( bRemove )
            {
                ret = -ENOENT;
            }
            else
            {
                StrSetPtr pStrSet( true );
                ( *pStrSet )().insert( strName );
                oCfg[ propValList ] = ObjPtr( pStrSet );
            }
        }
        else
        {
            ObjPtr pObj = oCfg[ propValList ];
            if( pObj.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }

            StrSetPtr pStrSet;
            pStrSet = pObj;
            if( pStrSet.IsEmpty() )
            {
                ret = -EFAULT;
                break;
            }
            if( bRemove )
            {
                if( ( *pStrSet )().find( strName ) !=
                    ( *pStrSet )().end() )
                {
                    (*pStrSet)().erase( strName );
                }
                else
                {
                    ret = -ENOENT;
                }
            }
            else
            {
                if( ( *pStrSet )().find( strName ) ==
                    ( *pStrSet )().end() )
                {
                    (*pStrSet)().insert( strName );
                }
                else
                {
                    ret = -EEXIST;
                }
            }
        }

    }while( 0 );

    return ret;
}

gint32 CDBusLoopbackMatch::IsMyMsgOutgoing(
    DBusMessage* pDBusMsg ) const
{
    gint32 ret = 0;
    if( GetType() == matchClient )
    {
        ret = FilterMsgP2S( pDBusMsg, false );
    }
    else if( GetType() == matchServer )
    {
        ret = FilterMsgS2P( pDBusMsg, false );
    }
    else
    {
        ret = -ENOTSUP;
    }
    return ret;
}
