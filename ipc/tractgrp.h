/*
 * =====================================================================================
 *
 *       Filename:  tractgrp.h
 *
 *    Description:  declaration of CIfTransactGroup
 *
 *        Version:  1.0
 *        Created:  01/22/2019 05:21:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 * =====================================================================================
 */

#pragma once

#include "iftasks.h"

class CIfTransactGroup :
    public CIfTaskGroup
{
    TaskGrpPtr m_pTaskGrp;
    TaskGrpPtr m_pRbackGrp;

    public:
    typedef CIfTaskGroup super;
    CIfTransactGroup( const IConfigDb* pCfg )
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

    gint32 AppendTask(
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

    gint32 AddRollback(
        TaskletPtr& pRbTask,
        bool bBack = true )
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

    gint32 SetTaskRelation( EnumLogicOp iop )
    {
        if( m_pTaskGrp->IsRunning() )
            return ERROR_STATE;
        m_pTaskGrp->SetRelation( iop );
        return 0;
    }

    gint32 SetRbRelation( EnumLogicOp iop )
    {
        if( m_pRbackGrp->IsRunning() )
            return ERROR_STATE;
        m_pRbackGrp->SetRelation( iop );
        return 0;
    }

    gint32 OnComplete( gint32 iRet )
    {
        iRet = m_pTaskGrp->GetError();
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
};
