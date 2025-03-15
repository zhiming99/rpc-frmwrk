/*
 * =====================================================================================
 *
 *       Filename:  rtfiles.cpp
 *
 *    Description:  implementations of the file objects exported by rpcrouter
 *
 *        Version:  1.0
 *        Created:  07/02/2022 05:51:33 PM
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
#include "rpc.h"

using namespace rpcf;
#include "ifhelper.h"
#include "frmwrk.h"
#include "rpcroute.h"
#include "fuseif.h"
#include "jsondef.h"
#include "security/security.h"
#include "taskschd.h"

#include "routmain.h"
#include "rtfiles.h"

#ifdef FUSE3
namespace rpcf
{
gint32 CFuseBdgeList::UpdateContent() 
{
    return DumpBdgeList(
        GetUserObj(), m_strContent );
}

gint32 CFuseBdgeProxyList::UpdateContent() 
{
    return DumpBdgeProxyList(
        GetUserObj(), m_strContent );
}

gint32 CFuseSessionFile::UpdateContent() 
{
    return DumpSessions(
        GetUserObj(), m_strContent );
}

gint32 CFuseReqProxyList::UpdateContent()
{
    return DumpReqProxyList(
        GetUserObj(), m_strContent );
}

gint32 CFuseReqFwdrInfo::UpdateContent()
{
    return DumpReqFwdrInfo(
        GetUserObj(), m_strContent );
}


gint32 AddFilesAndDirsBdge(
    CRpcServices* pSvc )
{
    gint32 ret = 0;
    do{
        CRpcRouterManagerImpl* prmgr =
            ObjPtr( pSvc );
        if( prmgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }
 
        std::vector< InterfPtr > vecRts;
        prmgr->GetRouters( vecRts );
        if( vecRts.empty() )
            break;

        CRpcRouterBridge* pRouter = nullptr;
        for( auto& elem : vecRts )
        {
            pRouter = elem;
            if( pRouter == nullptr )
                continue;
            break;
        }

        InterfPtr pRootIf = GetRootIf();
        CFuseRootServer* pSvr = pRootIf;
        if( pSvr == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        ObjPtr pObj( pRouter );
        CFuseObjBase* pList =
            new CFuseBdgeList( nullptr );
        pList->SetMode( S_IRUSR );
        pList->DecRef();
        pList->SetUserObj( pObj );

        auto pEnt = DIR_SPTR( pList );
        pSvr->Add2UserDir( pEnt );

        pList = new CFuseBdgeProxyList( nullptr );
        pList->SetMode( S_IRUSR );
        pList->DecRef();
        pList->SetUserObj( pObj );

        pEnt = DIR_SPTR( pList );
        pSvr->Add2UserDir( pEnt );

        auto pSess =
            new CFuseSessionFile( nullptr );
        pSess->SetMode( S_IRUSR );
        pSess->DecRef();
        pSess->SetUserObj( pObj );
        pEnt = DIR_SPTR( pSess );
        pSvr->Add2UserDir( pEnt );

        auto preql =
            new CFuseReqProxyList( nullptr );
        preql->SetMode( S_IRUSR );
        preql->DecRef();
        preql->SetUserObj( pObj );
        pEnt = DIR_SPTR( preql );
        pSvr->Add2UserDir( pEnt );

    }while( 0 );

    return ret;
}

gint32 AddFilesAndDirsReqFwdr(
    CRpcServices* pSvc )
{
    gint32 ret = 0;
    do{
        CRpcRouterManagerImpl* prmgr =
            ObjPtr( pSvc );
        if( prmgr == nullptr )
        {
            ret = -EFAULT;
            break;
        }
 
        std::vector< InterfPtr > vecRts;
        prmgr->GetRouters( vecRts );
        if( vecRts.empty() )
            break;

        CRpcRouterReqFwdr* pRouter = nullptr;
        for( auto& elem : vecRts )
        {
            pRouter = elem;
            if( pRouter == nullptr )
                continue;
            break;
        }

        ObjPtr pObj( pRouter );
        InterfPtr pRootIf = GetRootIf();
        CFuseRootProxy* pProxy = pRootIf;
        if( pProxy == nullptr )
        {
            ret = -EFAULT;
            break;
        }

        auto pList =
            new CFuseBdgeProxyList( nullptr );
        pList->SetMode( S_IRUSR );
        pList->DecRef();
        pList->SetUserObj( pObj );

        auto pEnt = DIR_SPTR( pList );
        pProxy->Add2UserDir( pEnt );

        auto pRfInfo =
            new CFuseReqFwdrInfo( nullptr );
        pRfInfo->SetMode( S_IRUSR );
        pRfInfo->DecRef();
        pRfInfo->SetUserObj( pObj );

        pEnt = DIR_SPTR( pRfInfo );
        pProxy->Add2UserDir( pEnt );

    }while( 0 );

    return ret;
}

}

using namespace rpcf;
extern "C" gint32 AddFilesAndDirs(
    bool bProxy, CRpcServices* pSvc )
{
    if( pSvc == nullptr )
        return -EFAULT;

    if( bProxy )
        return AddFilesAndDirsReqFwdr( pSvc );

    return AddFilesAndDirsBdge( pSvc );
}

#endif
