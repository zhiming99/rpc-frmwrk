/*
 * =====================================================================================
 *
 *       Filename:  portex.cpp
 *
 *    Description:  implementation of some extended functions of port and portstat
 *
 *        Version:  1.0
 *        Created:  04/01/2019 01:28:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi(woodhead99@gmail.com )
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

#include <vector>
#include <string>
#include <stdexcept>
#include "defines.h"
#include "port.h"
#include "frmwrk.h"
#include "stlcont.h"
#include "emaphelp.h"
#include "ifhelper.h"

gint32 CPort::GetStopResumeTask(
    PIRP pIrp, TaskletPtr& pTask )
{
    gint32 ret = 0;
    ret = DEFER_CALL_NOSCHED( pTask, 
        ObjPtr( GetIoMgr() ),
        &CIoManager::CompleteIrp,
        pIrp );
    return ret;
}
