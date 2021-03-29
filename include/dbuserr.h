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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 *
 * =====================================================================================
 */

#pragma once
#include "clsids.h"
#include "nsdef.h"

namespace rpcf
{

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

}
