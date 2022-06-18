/*
 * =====================================================================================
 *
 *       Filename:  loopool.h
 *
 *    Description:  declarations of CLoopPools and CLoopPool
 *
 *        Version:  1.0
 *        Created:  06/15/2022 03:05:19 PM
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

#define NETSOCK_TAG 1
#define UXSOCK_TAG 2

namespace rpcf{

class CLoopPool : public CObjBase
{
    guint32 m_dwTag = 0;
    guint32 m_dwMaxLoops = 0;

    // lower limit to add new loop to the pool
    guint32 m_dwLowMark = 20;
    stdstr m_strPrefix;
    std::hashmap< MloopPtr, guint32 > m_mapLoops;
    bool m_bAutoClean = false;
    CIoManager* m_pMgr = nullptr;

    public:

    using LOOPMAP_ITR=
        std::hashmap< MloopPtr, guint32 >::iterator;

    typedef CObjBase super;
    CLoopPool( const IConfigDb* pCfg );

    gint32 CreateLoop(
        MloopPtr& pLoop );

    gint32 AllocMainLoop(
        MloopPtr& pLoop );
        
    gint32 ReleaseMainLoop(
        MloopPtr& pLoop );

    gint32 DestroyPool();

    inline CIoManager* GetIoMgr()
    { return m_pMgr; }
};

typedef CAutoPtr< clsid( CLoopPool ), CLoopPool >  LPoolPtr;

class CLoopPools : public IService
{
    std::hashmap< guint32, LPoolPtr > m_mapLPools;
    mutable stdrmutex m_oLock;
    CIoManager* m_pMgr = nullptr;

    public:
    typedef IService super;
    CLoopPools( const IConfigDb* pCfg );

    inline CIoManager* GetIoMgr()
    { return m_pMgr; }

    stdrmutex& GetLock() const
    { return m_oLock; }

    bool IsPool( guint32 dwTag ) const;

    gint32 CreatePool(
        guint32 dwTag,
        const stdstr& strPrefix, 
        guint32 dwMaxLoops = 0,
        guint32 dwLowMark = 20,
        bool bAutoClean = false );

    gint32 DestroyPool( guint32 dwTag );

    gint32 AllocMainLoop(
        guint32 dwTag, 
        MloopPtr& pLoop );
        
    gint32 ReleaseMainLoop(
        guint32 dwTag, 
        MloopPtr& pLoop );

    gint32 Start() override;
    gint32 Stop() override;

    gint32 OnEvent(
        EnumEventId iEvent,
        LONGWORD    dwParam1,
        LONGWORD    dwParam2,
        LONGWORD*   pData ) override
    { return -ENOTSUP; }
};
typedef CAutoPtr< clsid( CLoopPools ), CLoopPools >  LPoolsPtr;

}
