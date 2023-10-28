/*
 * =====================================================================================
 *
 *       Filename:  rtseqmgr.h
 *
 *    Description:  declaration of router bridge's sequence task group manager 
 *
 *        Version:  1.0
 *        Created:  10/15/2023 10:20:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once
#include "seqtgmgr.h"
#include "rpcroute.h"

namespace rpcf
{
struct CRouterSeqTgMgr :
    public CSeqTaskGrpMgr< guint32, CRpcRouterBridge >
{
    typedef CSeqTaskGrpMgr< guint32, CRpcRouterBridge > super;
    CRouterSeqTgMgr() : super()
    { SetClassId( clsid( CRouterSeqTgMgr ) ); }
};

}
