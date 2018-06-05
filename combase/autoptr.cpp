/*
 * =====================================================================================
 *
 *       Filename:  autoptr.cpp
 *
 *    Description:  implementation of part of the autoptr methods
 *
 *        Version:  1.0
 *        Created:  12/07/2016 08:22:24 PM
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
#include <map>
#include "defines.h"
#include "autoptr.h"
// #include "port.h"
// #include "dbusport.h"

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::Serialize( CBuffer* pBuf )
{
    if( IsEmpty() || pBuf == nullptr )
        return -EINVAL;

    guint32 dwSize = 0;
    guint8 *pData = nullptr;
    gint32 ret = 0;
            
    do{

        if( !dbus_message_marshal(
            m_pObj, ( char** )&pData, ( int* )&dwSize ) )
        {
            ret = -ENOMEM;
            break;
        }

        if( dwSize > DMSG_MAX_SIZE )
        {
            ret = -ENOMEM;
            break;
        }

        pBuf->Resize( 0 );

        ret = pBuf->Append(
            ( guint8* )&dwSize, sizeof( dwSize ) );

        if( ERROR( ret ) )
            break;

        ret = pBuf->Append( pData, dwSize );
        if( ERROR( ret ) )
            break;

    }while( 0 );

    if( pData != nullptr )
    {
        dbus_free( pData );
    }

    return ret;
}

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::Deserialize( CBuffer* pBuf )
{
    gint32 ret = 0;

    if( pBuf == nullptr
        || pBuf->size() == 0 )
        return -EINVAL;

    do{
        Clear();

        guint32 dwSize =
            *( guint32* )pBuf->ptr();

        guint8* pMsg =
            ( guint8* )pBuf->ptr() + sizeof( guint32 );

        CDBusError dbusError;

        m_pObj = dbus_message_demarshal(
            ( char* )pMsg, dwSize, dbusError );

        if( m_pObj == nullptr )
        {
            ret = dbusError.Errno();
            break;
        }

    }while( 0 );

    return ret;
}

gint32 CAutoPtr< Clsid_Invalid, DBusMessage >
    ::Clone( DBusMessage* pSrcMsg )
{
    if( pSrcMsg == nullptr )
        return -EINVAL;

    m_pObj = dbus_message_copy( pSrcMsg );
    if( m_pObj == nullptr )
        return -EFAULT;

    return 0;
}
