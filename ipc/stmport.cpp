/*
 * =====================================================================================
 *
 *       Filename:  stmport.cpp
 *
 *    Description:  Definition of stream port class 
 *
 *        Version:  1.0
 *        Created:  08/14/2022 12:37:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
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
#include "stlcont.h"
#include "emaphelp.h"
#include "ifhelper.h"
#include "stmport.h"

CDBusStreamPdo::CDBusStreamPdo(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;
    do{
        CCfgOpener oCfg( pCfg );
        ret = oCfg.GetObjPtr(
            propIfPtr, m_pIf );
        if( ERROR( ret ) )
            break;

        CRpcServices* pSvc = m_pIf;
        m_bServer = pSvc->IsServer();

    }while( 0 );

    if( ERROR( ret ) )
    {
        string strMsg = DebugMsg( ret,
            "Error in CDBusStreamPdo ctor" );
        throw std::runtime_error( strMsg );
    }
    return;
}

