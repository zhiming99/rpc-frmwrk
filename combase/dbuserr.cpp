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
 * =====================================================================================
 */

#include <dbus/dbus.h>
#include <vector>
#include <string>
#include <string.h>
#include "defines.h"
#include "dbuserr.h"

/**
* @name ErrnoFromDbusErr 
* @{ */
/** get the errno from the dbus error string @} */

gint32 ErrnoFromDbusErr( const char *error )
{
    if( error == nullptr )
        return 0;

    if (strcmp (error, DBUS_ERROR_FAILED) == 0)
        return ERROR_FAIL;
    else if (strcmp (error, DBUS_ERROR_NO_MEMORY) == 0)
        return -ENOMEM;
    else if (strcmp (error, DBUS_ERROR_IO_ERROR) == 0)
        return -EIO;
    else if (strcmp (error, DBUS_ERROR_BAD_ADDRESS) == 0)
        return -ERROR_ADDRESS;
    else if (strcmp (error, DBUS_ERROR_NOT_SUPPORTED) == 0)
        return -ENOTSUP;
    else if (strcmp (error, DBUS_ERROR_LIMITS_EXCEEDED) == 0)
        return -ERANGE;
    else if (strcmp (error, DBUS_ERROR_ACCESS_DENIED) == 0)
        return -EACCES;
    else if (strcmp (error, DBUS_ERROR_AUTH_FAILED) == 0)
        return -ECONNREFUSED;
    else if (strcmp (error, DBUS_ERROR_NO_SERVER) == 0)
        return -ENOSYS;
    else if (strcmp (error, DBUS_ERROR_TIMEOUT) == 0)
        return -ETIMEDOUT;
    else if (strcmp (error, DBUS_ERROR_NO_NETWORK) == 0)
        return -ENETUNREACH;
    else if (strcmp (error, DBUS_ERROR_ADDRESS_IN_USE) == 0)
        return -EADDRINUSE;
    else if (strcmp (error, DBUS_ERROR_DISCONNECTED) == 0)
        return -ECONNRESET;
    else if (strcmp (error, DBUS_ERROR_INVALID_ARGS) == 0)
        return -EINVAL;
    else if (strcmp (error, DBUS_ERROR_NO_REPLY) == 0)
        return -ENOMSG;
    else if (strcmp (error, DBUS_ERROR_FILE_NOT_FOUND) == 0)
        return -ENOENT;
    else if (strcmp (error, DBUS_ERROR_OBJECT_PATH_IN_USE) == 0)
        return -EADDRINUSE; 

    return 0;
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
