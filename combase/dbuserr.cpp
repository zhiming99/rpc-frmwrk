/*
 * =====================================================================================
 *
 *       Filename:  dbuserr.cpp
 *
 *    Description:  implementation of the wrapper class of DBuseError
 *        Version:  1.0
 *        Created:  01/08/2017 08:50:44 AM
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
#include <string.h>
#include "defines.h"
#include "dbuserr.h"
#include <unordered_map>

/**
* @name ErrnoFromDbusErr 
* @{ */
/** get the errno from the dbus error string @} */

static std::unordered_map< std::string, gint32 > g_mapDBusError =
{
    { DBUS_ERROR_FAILED, ERROR_FAIL },
    { DBUS_ERROR_NO_MEMORY, -ENOMEM },
    { DBUS_ERROR_SERVICE_UNKNOWN, -ENOTCONN },
    { DBUS_ERROR_NO_REPLY, -ENOMSG },
    { DBUS_ERROR_IO_ERROR, -EIO },
    { DBUS_ERROR_BAD_ADDRESS, ERROR_ADDRESS },
    { DBUS_ERROR_NOT_SUPPORTED, -ENOTSUP },
    { DBUS_ERROR_LIMITS_EXCEEDED, -ERANGE },
    { DBUS_ERROR_ACCESS_DENIED, -EACCES },
    { DBUS_ERROR_AUTH_FAILED, -ECONNREFUSED },
    { DBUS_ERROR_NO_SERVER, -ENOSYS },
    { DBUS_ERROR_TIMEOUT, -ETIMEDOUT },
    { DBUS_ERROR_NO_NETWORK, -ENETUNREACH },
    { DBUS_ERROR_ADDRESS_IN_USE, -EADDRINUSE },
    { DBUS_ERROR_DISCONNECTED, -ECONNRESET },
    { DBUS_ERROR_INVALID_ARGS, -EINVAL },
    { DBUS_ERROR_FILE_NOT_FOUND, -ENOENT },
    { DBUS_ERROR_OBJECT_PATH_IN_USE, -EADDRINUSE  },
};

gint32 ErrnoFromDbusErr( const char *error )
{
    if( error == nullptr )
        return 0;

    std::unordered_map< std::string, gint32 >::iterator
        itr = g_mapDBusError.find( error );

    if( itr == g_mapDBusError.end() )
        return 0;

    return itr->second;
}

CDBusError::CDBusError()
{
    dbus_error_init( &m_oError );
}
CDBusError::~CDBusError()
{
    Reset();
}

void CDBusError::Reset()
{
    dbus_error_free( &m_oError );
}
const char* CDBusError::GetName() const
{
    return m_oError.name;
}
const char* CDBusError::GetMessage() const
{
    return m_oError.message;
}
gint32 CDBusError::Errno() const
{
    gint32 ret =
        ErrnoFromDbusErr( m_oError.name );

    if( ret == 0 )
        ret = ERROR_FAIL;

    return ret;
}

CDBusError::operator DBusError*()
{
    return &m_oError;
}
