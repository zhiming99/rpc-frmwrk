#!/bin/bash
# this is a script to create an application for
# a modbus thermometer device.
app=$1
if [[ -z "$app" ]]; then
	echo Error missing dev name
	exit 1
fi
# create the applicaton, 'thermometer' in our case
noptlog=true rpcfctl addapp $app

# create the points specific to this thermometer 
# an output point for temperature with 'float' data type 
rpcfctl addpoint $app temperature output f
rpcfctl setattr  $app temperature avgalgo i 1 
# the unit to show on the monitor's app detail page
rpcfctl setattr  $app temperature unit s "C"

# an output point for humidity with 'float' data type 
rpcfctl addpoint $app humidity output f
rpcfctl setattr  $app humidity unit s "%"
rpcfctl setattr  $app humidity avgalgo i 1 

# point 'display_points' to tell the monitor the
# featured points to show on the monitor's app
# detail page. points 'temperator' and 'humidity'
# in our case
rpcfctl addpoint $app display_points setpoint blob
rpcfctl setpv $app display_points blob '["temperature","humidity"]'
rpcfctl setpv $app app_class s "device"

# enable logging of the point values. by default,
# the point value will be recorded once per 5
# minutes. for detailed rpcfctl command, please
# refer to rpcfctl's manual page with command
# 'man rpcfctl' 
if rpcfctl enablelog  $app temperature ; then
    rpcfctl enablelog  $app humidity 
fi

