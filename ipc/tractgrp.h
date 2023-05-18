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
 *      Copyright:  2019 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once

#include "iftasks.h"

namespace rpcf
{

class CIfTransactGroup :
    public CIfTaskGroup
{
    TaskGrpPtr m_pTaskGrp;
    TaskGrpPtr m_pRbackGrp;

    public:
    typedef CIfTaskGroup super;
    CIfTransactGroup( const IConfigDb* pCfg );

    gint32 AppendTask(
        TaskletPtr& pTask );

    gint32 AddRollback(
        TaskletPtr& pRbTask,
        bool bBack = true );

    gint32 SetTaskRelation( EnumLogicOp iop );
    gint32 SetRbRelation( EnumLogicOp iop );

    gint32 OnChildComplete(
        gint32 iRet, CTasklet* pChild );

    gint32 OnComplete( gint32 iRet );
};

}
