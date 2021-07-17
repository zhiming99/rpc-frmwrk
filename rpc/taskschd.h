/*
 * =====================================================================================
 *
 *       Filename:  taskschd.h
 *
 *    Description:  declarations of task scheduler 
 *
 *        Version:  1.0
 *        Created:  07/12/2021 04:02:53 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2020 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

namespace rpcf
{

struct ITaskScheduler : public CObjBase
{
    virtual gint32 SetSlotNumber(
        InterfPtr& pIf, guint32 dwNumSlot ) = 0;

    virtual gint32 ReallocSlots(
        InterfPtr& pIf,
        bool bKill = true ) = 0;

    virtual gint32 RunNextTaskGrp() = 0;
    virtual gint32 SchedNextTaskGrp() = 0;
    virtual gint32 GetNextTaskGrp(
        TaskGrpPtr& pGrp ) = 0;

    virtual gint32 RemoveTaskGrp(
        InterfPtr pIf, TaskGrpPtr& pGrp ) = 0;

    virtual gint32 RemoveTaskGrps(
        guint32 dwPortId ) = 0;

    virtual gint32 RemoveTaskGrps(
        const stdstr& strUniqName ) = 0;

    virtual gint32 AddTaskGrp(
        InterfPtr& pIf, TaskGrpPtr& pGrp ) = 0;

    virtual gint32 AddConn(
        InterfPtr& pIf ) = 0;

    virtual gint32 RemoveConn(
        InterfPtr& pIf ) = 0;
};

struct CONNQUE_ELEM
{
    std::list< TaskGrpPtr > m_lstWaitSlot;
    std::list< TaskGrpPtr >* m_plstReady;
    std::list< TaskGrpPtr >* m_plstNextSched;
    std::list< TaskGrpPtr > m_lstRunningGrps[ 2 ];

    TaskGrpPtr m_lstUrgent;

    guint32 m_dwPendingReqs = 0;
    guint32 m_dwFreeSlots = 0;

    guint32 m_dwMaxSlots =
        RFC_MAX_PENDINGS + RFC_MAX_REQS;

    CONNQUE_ELEM()
    {
        m_plstReady = &m_lstRunningGrps[ 0 ];
        m_plstNextSched = &m_lstRunningGrps[ 1 ];
    }
    guint32 GetTaskGrpCount()
    {
        return m_lstWaitSlot.size() +
            m_lstRunningGrps[ 0 ].size() +
            m_lstRunningGrps[ 1 ].size() +
    }

    gint32 CanSend() const
    { return m_dwMaxSlots > m_dwPendingReqs; }
};

class CRRTaskScheduler :
    public ITaskScheduler
{
    CRpcServices* m_pSchedMgr = nullptr;
    std::map< InterfPtr, CONNQUE_ELEM > m_mapConnQues;
    std::list< InterfPtr > m_lstConns;
    std::hashmap< guint32, InterfPtr > m_mapId2If;
    stdrmutex m_oSchedLock;

    gint32 GetNextTaskGrp(
        InterfPtr& pIf,
        TaskGrpPtr& pGrp );

    void SetLimitRunningGrps(
        CONNQUE_ELEM& ocq,
        guint32& dwNewLimit );

    void SetLimitGrps(
        CONNQUE_ELEM& ocq,
        guint32& dwNewLimit );

    void IncRunningGrps(
        CONNQUE_ELEM& ocq,
        guint32& dwCount );

    void SelTasksToKill(
        CONNQUE_ELEM& ocq,
        std::vector< TaskletPtr >& vecTasks,
        bool bWaitQue );

    void IncWaitingGrp(
        CONNQUE_ELEM& ocq,
        guint32& dwCount );

    gint32 IncSlotCount(
        CONNQUE_ELEM& ocq, guint32 dwCount );

    gint32 DecSlotCount(
        CONNQUE_ELEM& ocq,
        guint32 dwCount,
        std::vector< TaskletPtr >& vecTasks );

    void KillTasks(
        InterfPtr& pIf,
        std::vector< TaskletPtr >& vecTasks );

    public:
    typedef ITaskScheduler super;

    CRRTaskScheduler( const IConfigDb* pCfg );
    ~CRRTaskScheduler();
    CIoManager* GetIoMgr();
    stdrmutex& GetLock();

    gint32 SetSlotCount(
        InterfPtr& pIf,
        guint32 dwNumSlot ) override;

    gint32 ReallocSlots(
        InterfPtr& pIf,
        bool bKill = true ) override;

    gint32 RunNextTaskGrp() override;

    gint32 SchedNextTaskGrp() override;

    gint32 GetNextTaskGrp(
        TaskGrpPtr& pGrp ) override;

    gint32 RemoveTaskGrp(
        InterfPtr pIf,
        TaskGrpPtr& pGrp ) override;

    gint32 AddTaskGrp(
        InterfPtr& pIf,
        TaskGrpPtr& pGrp ) override;

    gint32 AddConn(
        InterfPtr& pIf ) override;

    gint32 RemoveConn(
        InterfPtr& pIf ) override;

    gint32 RemoveTaskGrps(
        guint32 dwPortId ) override;

    gint32 RemoveTaskGrps(
        const stdstr& strUniqName ) override;

};

}
