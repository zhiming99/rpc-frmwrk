/*
 * =====================================================================================
 *
 *       Filename:  tractgrp.cpp
 *
 *    Description:  implementation of Transaction group 
 *
 *        Version:  1.0
 *        Created:  05/14/2023 02:02:10 PM
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

#include "tractgrp.h"

namespace rpcf
{
CIfTransactGroup::CIfTransactGroup(
    const IConfigDb* pCfg )
    : super( pCfg )
{
    gint32 ret = 0;
    do{
        SetClassId( clsid( CIfTransactGroup ) );

        ret = m_pTaskGrp.NewObj(
            clsid( CIfTaskGroup ),
            pCfg );

        ret = m_pRbackGrp.NewObj(
            clsid( CIfTaskGroup ),
            pCfg );

        SetRelation( logicOR );
        m_pTaskGrp->SetRelation( logicAND );
        m_pRbackGrp->SetRelation( logicNONE );

        TaskletPtr pTask = m_pTaskGrp;
        super::AppendTask( pTask );

        pTask = m_pRbackGrp;
        super::AppendTask( pTask );

    }while( 0 );

    if( ERROR( ret ) )
    {
        std::string strMsg = DebugMsg( ret,
            "Error in CIfTransactGroup ctor" );
        throw std::runtime_error( strMsg );
    }
}

gint32 CIfTransactGroup::AppendTask(
    TaskletPtr& pTask )
{
    if( pTask.IsEmpty() )
        return -EINVAL;

    if( m_pTaskGrp.IsEmpty() )
        return -EFAULT;

    CCfgOpenerObj oTaskCfg(
        ( CObjBase* )pTask );

    oTaskCfg.SetIntPtr(
        propTransGrpPtr, ( guint32* )this );

    return m_pTaskGrp->AppendTask( pTask );
}

gint32 CIfTransactGroup::AddRollback(
    TaskletPtr& pRbTask,
    bool bBack )
{
    if( pRbTask.IsEmpty() )
        return -EINVAL;

    if( m_pRbackGrp->IsRunning() )
        return ERROR_STATE;

    gint32 ret = 0;

    if( bBack )
        ret = m_pRbackGrp->AppendTask( pRbTask );
    else
        ret = m_pRbackGrp->InsertTask( pRbTask );

    return ret;
}

gint32 CIfTransactGroup::SetTaskRelation(
    EnumLogicOp iop )
{
    m_pTaskGrp->SetRelation( iop );
    return 0;
}

gint32 CIfTransactGroup::SetRbRelation(
    EnumLogicOp iop )
{
    m_pRbackGrp->SetRelation( iop );
    return 0;
}

gint32 CIfTransactGroup::OnChildComplete(
    gint32 iRet, CTasklet* pChild )
{
    gint32 ret = 0;
    if( pChild == nullptr )
        return -EINVAL;

    do{
        CStdRTMutex oTaskLock( GetLock() );
        if( m_vecRetVals.empty() && ERROR( iRet ) )
        {
            oTaskLock.Unlock();

            CCfgOpenerObj oGrp(
                ( CObjBase* )m_pRbackGrp );

            oGrp.SetIntProp(
                propReturnValue, iRet );
        }
        ret = super::OnChildComplete(
            iRet, pChild );

    }while( 0 );

    return ret;
}

gint32 CIfTransactGroup::OnComplete( gint32 iRet )
{
    // don't use m_pTaskGrp's GetError here,
    // because at the moment, the m_pTaskGrp
    // could still be in its OnComplete call
    // with the `m_iRet' unset yet if the
    // process's load is very high.
    //
    iRet = m_vecRetVals[ 0 ];
    super::OnComplete( iRet );

    m_pRbackGrp->RemoveProperty(
        propParentTask );

    m_pTaskGrp->RemoveProperty(
        propParentTask );

    TaskletPtr pTask( m_pRbackGrp );
    RemoveTask( pTask );
    pTask = m_pTaskGrp;
    RemoveTask( pTask );

    return iRet;
}

}
