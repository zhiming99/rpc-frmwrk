/*
 * =====================================================================================
 *
 *       Filename:  rtappman.h
 *
 *    Description:  declaration of callbacks for appstore and data producer
 *
 *        Version:  1.0
 *        Created:  02/28/2025 10:20:12 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#include "AppManagercli.h"
#define RTAPPNAME "rpcrouter1"
#define O_BDGE_LIST "bdge_list"
#define O_BPROXY_LIST "bdge_proxy_list"
#define O_RPROXY_LIST "req_proxy_list"

struct CAsyncAMCallbacks : public CAsyncStdAMCallbacks
{
    gint32 GetPointValuesToUpdate(
        InterfPtr& pIf,
        std::vector< KeyValue >& veckv ) override;

    gint32 GetPointValuesToInit(
        InterfPtr& pIf,
        std::vector< KeyValue >& veckv ) override;
};

