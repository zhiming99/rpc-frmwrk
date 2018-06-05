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
 * =====================================================================================
 */
#pragma once
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
#define JSON_ATTR_PORTCLASS     "PortClass"

#define JSON_ATTR_ARCH          "Arch"
#define JSON_ATTR_NUM_CORE      "Processors"
