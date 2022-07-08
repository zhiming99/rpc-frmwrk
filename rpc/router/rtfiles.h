/*
 * =====================================================================================
 *
 *       Filename:  rtfiles.h
 *
 *    Description:  declarations of file objects exported by the rpcrouter
 *
 *        Version:  1.0
 *        Created:  07/02/2022 05:46:38 PM
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

#pragma once

#define BDGELIST_FILE "InConnections"
#define BDGEPROXYLIST_FILE "OutConnections"

class CFuseBdgeList : public CFuseSvcStat
{
    public:
    typedef CFuseSvcStat super;
    CFuseBdgeList( CRpcServices* pIf )
        : super( pIf )
    { SetName( BDGELIST_FILE ); }
    virtual gint32 UpdateContent();
};

class CFuseBdgeProxyList : public CFuseSvcStat
{
    public:
    typedef CFuseSvcStat super;
    CFuseBdgeProxyList( CRpcServices* pIf )
        : super( pIf )
    { SetName( BDGEPROXYLIST_FILE ); }
    virtual gint32 UpdateContent();
};
