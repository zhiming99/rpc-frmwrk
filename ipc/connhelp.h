/*
 * =====================================================================================
 *
 *       Filename:  connhelp.h
 *
 *    Description:  helper class for connection point container
 *
 *        Version:  1.0
 *        Created:  12/16/2016 10:55:50 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 * =====================================================================================
 */

#pragma once

#include "defines.h"
#include "registry.h"
#include "stlcont.h"

class CConnPointHelper
{
    CIoManager* m_pMgr;

    public:

    CConnPointHelper( CIoManager* pMgr )
    {
        m_pMgr = pMgr;
    }

    gint32 SubscribeEvent(
        gint32 iConnId, EventPtr& pEventSink ) const
    {
        gint32 ret = 0;

        if( pEventSink.IsEmpty() )
            return -EINVAL;

        do{
            std::string strPath =
                REG_IO_EVENTMAP_DIR( m_pMgr->GetModName() );

            CRegistry& oReg =
                m_pMgr->GetRegistry();

            CStdRMutex oLock( oReg.GetLock() );

            ret = oReg.ChangeDir( strPath );
            if( ERROR( ret ) )
                break;

            ObjPtr pObj;
            ret = oReg.GetObject( iConnId, pObj );
            if( ERROR( ret ) )
                break;

            CStlEventMap* pMap = pObj;
            if( pMap == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            ret = pMap->SubscribeEvent( pEventSink );

        }while( 0 );

        return ret;
    }

    gint32 UnsubscribeEvent(
        gint32 iConnId, EventPtr& pEventSink ) const
    {

        gint32 ret = 0;
        do{
            std::string strPath =
                REG_IO_EVENTMAP_DIR( m_pMgr->GetModName() );

            ObjPtr pObj;

            if( !strPath.empty() )
            {
                // find the registry
                CRegistry& oReg =
                    m_pMgr->GetRegistry();

                CStdRMutex oLock( oReg.GetLock() );
                ret = oReg.ChangeDir( strPath );
                if( ERROR( ret ) )
                    break;

                CCfgOpenerObj a( &oReg );
                ret = a.GetObjPtr( iConnId, pObj );
                if( ERROR( ret ) )
                    break;
            }

            CStlEventMap* pMap = pObj;
            if( pMap == nullptr )
            {
                ret = -EFAULT;
                break;
            }

            ret = pMap->UnsubscribeEvent( pEventSink );

        }while( 0 );

        return ret;
    }

    gint32 GetEventMap(
        gint32 iConnId, ObjPtr& pEvtMap ) const
    {
        gint32 ret = 0;
        do{
            std::string strPath =
                REG_IO_EVENTMAP_DIR( m_pMgr->GetModName() );


            CRegistry& oReg =
                m_pMgr->GetRegistry();

            CStdRMutex oLock( oReg.GetLock() );
            ret = oReg.ChangeDir( strPath );
            if( ERROR( ret ) )
                break;

            ObjPtr pObj;
            ret = oReg.GetObject( iConnId, pObj );

        }while( 0 );

        return ret;
    }

    gint32 BroadcastEvent(
        gint32 iConnId,
        EnumEventId iEvent,
        guint32 dwParam1,
        guint32 dwParam2,
        guint32* pData )
    {
        ObjPtr pEvtMap;
        gint32 ret = GetEventMap( iConnId, pEvtMap );
        if( ERROR( ret ) )
        {
            return ret;
        }
        else
        {
            CStlEventMap* pMap = pEvtMap;
            pMap->BroadcastEvent(
                iEvent, dwParam1, dwParam2, pData );
        }

        return 0;
    }
};
