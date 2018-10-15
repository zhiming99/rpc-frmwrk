/*
 * =====================================================================================
 *
 *       Filename:  dbuserr.h
 *
 *    Description:  declaration of CDBusError
 *
 *        Version:  1.0
 *        Created:  01/13/2018 06:42:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *
 * =====================================================================================
 */

#pragma once
#include "glib.h"
gint32 ErrnoFromDbusErr( const char *error );

class CDBusError
{
    // a dbus error wrapper
    DBusError m_oError;
    public:
    CDBusError();
    ~CDBusError();

    void Reset();
    const char* GetName() const;
    const char* GetMessage() const;
    gint32 Errno() const;
    operator DBusError*();
};
