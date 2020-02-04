/*
 * =====================================================================================
 *
 *       Filename:  jsondef.h
 *
 *    Description:  definitions of constant for json config files
 *
 *        Version:  1.0
 *        Created:  12/03/2016 09:47:28 AM
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
// driver.json names
#define JSON_ATTR_MODULES       "Modules"
#define JSON_ATTR_MODNAME       "ModName"
#define JSON_ATTR_DRVTOLOAD     "DriversToLoad"
#define JSON_ATTR_OBJTOLOAD     "PreloadableObjects"
#define JSON_ATTR_CLSFACTORY    "ClassFactories"
#define JSON_ATTR_BUSCLASS      "BusClass"
#define JSON_ATTR_INCLASS       "InternalClass"
#define JSON_ATTR_PORTCLASSES   "PortClasses"

#define JSON_ATTR_PORTS         "Ports"
#define JSON_ATTR_PORTDRV       "PortDriver"
#define JSON_VAL_BUSDRV         "BusDriver"
#define JSON_VAL_PORTDRV        "PortDriver"


#define JSON_ATTR_DRIVERS       "Drivers"
#define JSON_ATTR_DRVNAME       "DriverName"
#define JSON_ATTR_DRVTYPE       "DriverType"

#define JSON_ATTR_MATCH         "Match"
#define JSON_ATTR_FDODRIVER     "FdoDriver"
#define JSON_ATTR_FIDODRIVER    "FidoDriver"
#define JSON_ATTR_PROBESEQ      "ProbeSequence"
#define JSON_ATTR_PORTCLASS     "PortClass"

#define JSON_ATTR_ARCH          "Arch"
#define JSON_ATTR_NUM_CORE      "Processors"

// objdesc.json names
#define JSON_ATTR_VERSION       "Version"
#define JSON_ATTR_BUSNAME       "BusName"
#define JSON_ATTR_PORTID        "PortId"
#define JSON_ATTR_IPADDR        "IpAddress"
#define JSON_ATTR_TCPPORT       "PortNumber"
#define JSON_ATTR_PROXY_PORTID        "ProxyPortId"
#define JSON_ATTR_PROXY_PORTCLASS     "ProxyPortClass"
#define JSON_ATTR_SVRNAME       "ServerName"
#define JSON_ATTR_OBJARR        "Objects"
#define JSON_ATTR_FACTORIES     "ClassFactories"
#define JSON_ATTR_OBJNAME       "ObjectName"
#define JSON_ATTR_OBJCLASS      "ObjectClass"
#define JSON_ATTR_IFARR         "Interfaces"
#define JSON_ATTR_IFNAME        "InterfaceName"
#define JSON_ATTR_REQ_TIMEOUT   "RequestTimeoutSec"
#define JSON_ATTR_KA_TIMEOUT    "KeepAliveTimeoutSec"
#define JSON_ATTR_BIND_CLSID    "BindToClsid"
#define JSON_ATTR_PAUSABLE      "Pausable"
#define JSON_ATTR_PAUSE_START   "PauseOnStart"
#define JSON_ATTR_QUEUED_REQ    "QueuedRequests"
#define JSON_ATTR_ROUTER_ROLE   "Role"
#define JSON_ATTR_PARAMETERS    "Parameters"
#define JSON_ATTR_BINDADDR      "BindAddr"
#define JSON_ATTR_PDOCLASS      "PdoClass"
#define JSON_ATTR_PROTOCOL      "Protocol"
#define JSON_ATTR_ADDRFORMAT    "AddrFormat"
#define JSON_ATTR_ENABLE_SSL     "EnableSSL"
#define JSON_ATTR_ENABLE_WEBSOCKET "EnableWS"
#define JSON_ATTR_ENABLE_COMPRESS "Compression"
#define JSON_ATTR_CONN_RECOVER  "ConnRecover"
#define JSON_ATTR_DEST_URL      "DestURL"
#define JSON_ATTR_ROUTER_PATH      "RouterPath"
