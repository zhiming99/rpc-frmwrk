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
 * =====================================================================================
 */
#include <dbus/dbus.h>
#include <glib.h>
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

        CCfgOpener matchCfg(
            ( IConfigDb* )m_pCfg );

        CCfgOpener oInitCfg( pCfg );

        CIoManager* pMgr = nullptr;
        ret = oInitCfg.GetPointer( propIoMgr, pMgr );
        if( ERROR( ret ) )
            break;

        std::string strDestName =
            DBUS_DESTINATION( pMgr->GetModName() );

        ret = matchCfg.SetStrProp(
            propDestDBusName, strDestName );

        if( ERROR( ret ) )
            break;

        ret = matchCfg.SetStrProp(
            propSrcDBusName, strDestName );

        if( ERROR( ret ) )
            break;
        
        ret = matchCfg.CopyProp(
            propSrcUniqName, pCfg );

        if( ERROR( ret ) )
            break;

        matchCfg.CopyProp( propDestUniqName, pCfg );

        if( ERROR( ret ) )
            break;

        ret = matchCfg.CopyProp(
            propMatchType, pCfg );
        
    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "error in CDBusLoopbackMatch's ctor" );

        throw std::runtime_error( strMsg );
    }
}

gint32 CDBusLoopbackMatch::Filter1(
    DBusMessage* pDBusMsg ) const
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

        if( iType == DBUS_MESSAGE_TYPE_ERROR
            || iType == DBUS_MESSAGE_TYPE_METHOD_RETURN )
        {
            std::string strDestName =
                pMsg.GetDestination();

            if( strDestName.empty() )
            {
                ret = -EBADMSG;
                break;
            }

            std::string strDestMatch;
            CCfgOpenerObj oMatch( this );
            ret = oMatch.GetStrProp(
                propDestDBusName, strDestMatch );

            if( ERROR( ret ) )
                break;

            if( strDestMatch != strDestName )
            {
                ret = ERROR_FALSE;
                break;
            }

            ret = oMatch.GetStrProp(
                propDestUniqName, strDestMatch );

            if( ERROR( ret ) )
                break;
            
            if( strDestMatch != strDestName )
            {
                ret = ERROR_FALSE;
                break;
            }
        }
        else if( iType == DBUS_MESSAGE_TYPE_SIGNAL )
        {
            std::string strSender =
                pMsg.GetSender();

            if( strSender.empty() )
            {
                ret = -EBADMSG;
                break;
            }

            std::string strSrcMatch;
            CCfgOpenerObj oMatch( this );
            ret = oMatch.GetStrProp(
                propSrcDBusName, strSrcMatch );

            if( ERROR( ret ) )
                break;

            if( strSrcMatch != strSender )
            {
                ret = ERROR_FALSE;
                break;
            }

            ret = oMatch.GetStrProp(
                propSrcUniqName, strSrcMatch );

            if( ERROR( ret ) )
                break;
            
            if( strSrcMatch != strSender )
            {
                ret = ERROR_FALSE;
                break;
            }
        }
        else
        {
            ret = ERROR_FALSE;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusLoopbackMatch::Filter2(
    DBusMessage* pDBusMsg ) const
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

        std::string strDestName =
            pMsg.GetDestination();

        if( strDestName.empty() )
        {
            ret = -EBADMSG;
            break;
        }

        std::string strDestMatch;
        CCfgOpenerObj oMatch( this );
        ret = oMatch.GetStrProp(
            propDestDBusName, strDestMatch );

        if( ERROR( ret ) )
            break;

        if( strDestMatch != strDestName )
        {
            ret = ERROR_FALSE;
            break;
        }

        ret = oMatch.GetStrProp(
            propDestUniqName, strDestMatch );

        if( ERROR( ret ) )
            break;
        
        if( strDestMatch != strDestName )
        {
            ret = ERROR_FALSE;
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CDBusLoopbackMatch::IsMyMsgIncoming(
    DBusMessage* pDBusMsg ) const
{
    gint32 ret = 0;
    if( GetType() == matchClient )
    {
        ret = Filter1( pDBusMsg );
    }
    else if( GetType() == matchServer )
    {
        ret = Filter2( pDBusMsg );
    }
    return ret;
}

gint32 CDBusLoopbackMatch::IsMyMsgOutgoing(
    DBusMessage* pDBusMsg ) const
{
    gint32 ret = 0;
    if( GetType() == matchClient )
    {
        ret = Filter2( pDBusMsg );
    }
    else
    {
        ret = Filter1( pDBusMsg );
    }
    return ret;
}
