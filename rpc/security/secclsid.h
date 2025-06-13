/*
 * =====================================================================================
 *
 *       Filename:  secclsid.h
 *
 *    Description:  declarations of class ids for security objects 
 *
 *        Version:  1.0
 *        Created:  07/23/2020 09:31:04 PM
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
#pragma once
#include "clsids.h"

namespace rpcf
{

typedef enum 
{
    DECL_CLSID( CKdcRelayFdo ) = clsid( ClassFactoryStart ) + 20 ,
    DECL_CLSID( CAuthentProxyK5Impl ),
    DECL_CLSID( CKdcChannelProxy ),
    DECL_CLSID( CK5AuthServer ),
    DECL_CLSID( CKdcRelayProxy ),
    DECL_CLSID( CRpcSecFidoDrv ),
    DECL_CLSID( CRpcSecFido ),
    DECL_CLSID( CKdcRelayFdoDrv ),
    DECL_CLSID( CKdcRelayPdo ),
    DECL_CLSID( CKrb5InitHook ),
    DECL_CLSID( CRemoteProxyStateAuth ),
    DECL_CLSID( CKdcRelayProxyStat ),
    DECL_CLSID( COAuth2LoginProxyImpl ),
    DECL_CLSID( CSimpAuthLoginProxyImpl ),
    DECL_CLSID( CSimpleAuthCliWrapper ),

    DECL_IID( CRpcReqForwarderAuth ) = clsid( ReservedIidStart ) + 210,

} EnumSecClsid;

#define ENC_PACKET_MAGIC    ( *( ( guint32* )"PSec" ) )
#define CLEAR_PACKET_MAGIC    ( *( ( guint32* )"PCLE" ) )
#define SIGNED_PACKET_MAGIC    ( *( ( guint32* )"PSGN" ) )

#define PORT_CLASS_SEC_FIDO         "RpcSecFido"
#define PORT_CLASS_KDCRELAY_PDO     "KdcRelayPdo"
#define PORT_CLASS_KDCRELAY_FDO     "KdcRelayFdo"

}
