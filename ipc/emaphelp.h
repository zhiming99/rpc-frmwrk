/*
 * =====================================================================================
 *
 *       Filename:  emaphelp.h
 *
 *    Description:  implementation of event map helper
 *
 *        Version:  1.0
 *        Created:  11/09/2016 09:15:52 PM
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
#pragma once

#include "defines.h"
#include "registry.h"
#include "stlcont.h"

template <typename T>
class has_GetIoMgr
{
    // this class serves to test if the
    // class has a member GetIoMgr
    private:

    template<typename U, CIoManager* (U::*)() const> struct SFINAE {};
    template<typename U> static char Test(SFINAE<U, &U::GetIoMgr>*);
    template<typename U> static int Test(...);

    public:
    // static bool const value = sizeof(Test<T>(0)) == sizeof(Yes);
    static bool const value = ( sizeof(Test<T>(0)) == sizeof(char) ||
        has_GetIoMgr<class T::super>::value == sizeof( char ) );
};

template<>
class has_GetIoMgr< CObjBase >
{
    public:
    static bool const value =  sizeof(int);
};

template< class B,
     typename T = typename std::enable_if< has_GetIoMgr< B >::value, B >::type >
class CEventMapHelper
{
    T* m_pT;
    public:

    CEventMapHelper( T* pT )
    {
        m_pT = pT;
    }

    gint32 SubscribeEvent(
        EventPtr& pEventSink ) const
    {
        gint32 ret = 0;
        do{
            if( true )
            {
                std::string strPath;
                CCfgOpenerObj a( m_pT );
                ret = a.GetStrProp( propRegPath, strPath );
                if( ERROR( ret ) )
                    break;

                CRegistry& oReg = m_pT->GetIoMgr()->GetRegistry();
                CStdRMutex oLock( oReg.GetLock() );
                ret = oReg.ChangeDir( strPath );
                if( ERROR( ret ) )
                    break;

                ObjPtr pObj;
                ret = oReg.GetObject( propEventMapPtr, pObj );
                if( ERROR( ret ) )
                    break;
                
                CStlEventMap* pMap = pObj;
                if( pMap == nullptr )
                {
                    ret = -EFAULT;
                    break;
                }
                ret = pMap->SubscribeEvent( pEventSink );
            }

        }while( 0 );

        return ret;
    }

    gint32 UnsubscribeEvent(
        EventPtr& pEventSink ) const
    {

        gint32 ret = 0;
        do{
            ObjPtr pObj;
            CCfgOpener b( m_pT );
            std::string strPath;

            ret = b.GetStrProp(
                propRegPath, strPath );

            if( ERROR( ret ) )
            {
                break;
            }
            else
            {
                // find the registry
                CRegistry& oReg =
                    m_pT->GetIoMgr()->GetRegistry();

                CStdRMutex oLock( oReg.GetLock() );
                ret = oReg.ChangeDir( strPath );
                if( ERROR( ret ) )
                    break;

                ret = oReg.GetObject( propEventMapPtr, pObj );
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
        ObjPtr& pEvtMap ) const
    {
        gint32 ret = 0;
        do{
            CCfgOpenerObj b( m_pT );
            std::string strPath;
            ret = b.GetStrProp(
                propRegPath, strPath );

            if( ERROR( ret ) )
                break;

            CRegistry& oReg =
                m_pT->GetIoMgr()->GetRegistry();

            CStdRMutex oLock( oReg.GetLock() );
            ret = oReg.ChangeDir( strPath );
            if( ERROR( ret ) )
                break;

            ret = oReg.GetObject(
                propEventMapPtr, pEvtMap );

        }while( 0 );

        return ret;
    }

    gint32 BroadcastEvent(
        EnumEventId iEvent,
        LONGWORD dwParam1,
        LONGWORD dwParam2,
        LONGWORD* pData )
    {
        ObjPtr pEvtMap;
        gint32 ret = GetEventMap( pEvtMap );
        if( ERROR( ret ) )
        {
            return ret;
        }
        else
        {
            CStlEventMap* pMap = pEvtMap;
            ret = pMap->BroadcastEvent(
                iEvent, dwParam1, dwParam2, pData );
        }

        return 0;
    }
};
