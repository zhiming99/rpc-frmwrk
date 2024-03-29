#!/usr/bin/env python3
import json
import os
import sys 
from shutil import move
from copy import deepcopy
import multiprocessing

def UpdateDrv() :
    drvFile = os.path.dirname(os.path.realpath(__file__)) + "/driver.json"
    try:
        srcFp = open( drvFile, "r" )
        srcVals = json.load( srcFp )
        srcFp.close()
        
        if 'Arch' in srcVals:
            oArch = srcVals[ 'Arch' ]
        else:
            oArch = dict()

        oArch[ 'Processors' ] = str(
            multiprocessing.cpu_count() )

        srcVals[ 'Arch' ] = oArch

        destFp = open( drvFile, 'w' )
        json.dump( srcVals, destFp, indent = 4)
        destFp.close()

    except Exception as oErr :
        pass

def SyncCfg() :

    srcPath = '/usr/local/etc/rpcf/echodesc.json'
    destPath= './StressTestdesc.json'
    destObjs=['StressSvc']

    destPath = os.path.dirname(os.path.realpath(__file__)) + "/" + destPath
    try:
        srcFp = open( srcPath, "r" )
        srcVals = json.load( srcFp )
        srcCfgs = None
        if 'Objects' in srcVals :
            for elem in srcVals[ 'Objects' ] :
                try:
                    if elem[ "ObjectName" ] == "CEchoServer" :
                        srcCfgs = elem
                        break;
                except Exception as oErr :
                    pass

        srcFp.close()
        if srcCfgs is None :        
            return -1

        destFp = open( destPath, "r" )
        destVals = json.load( destFp )
        destFp.close()
        bChanged = False
        if 'Objects' in destVals :
            for elem in destVals[ 'Objects' ]:
                elem[ 'EnableSSL' ] = srcCfgs[ 'EnableSSL' ]
                elem[ 'PortNumber' ] = srcCfgs[ 'PortNumber' ]

                if 'AuthInfo' in srcCfgs :
                    elem[ 'AuthInfo' ] = srcCfgs[ 'AuthInfo' ]
                elif 'AuthInfo' in elem :
                    del elem[ 'AuthInfo' ]

                elem[ 'IpAddress' ] = srcCfgs[ 'IpAddress' ]
                elem[ 'EnableWS' ] = srcCfgs[ 'EnableWS' ]
                elem[ 'Compression' ] = srcCfgs[ 'Compression' ]
                elem[ 'RouterPath' ] = srcCfgs[ 'RouterPath' ]

                if 'DestURL' in srcCfgs :
                    elem[ 'DestURL' ] = srcCfgs[ 'DestURL' ]
                elif 'DestURL' in elem :
                    del elem[ 'DestURL' ]
                bChanged = True

        if bChanged : 
            destFp = open( destPath, 'w' )
            json.dump( destVals, destFp, indent = 4)
            destFp.close()

        UpdateDrv()
        print( "Configuration updated successfully" )

    except Exception as err:
        text = "Failed to sync desc file:" + str( err )
        exc_type, exc_obj, exc_tb = sys.exc_info()
        fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
        second_text = "@" + fname + ":" + str(exc_tb.tb_lineno)
        print( "%s, %s" % (text, second_text) )

    return 0

SyncCfg()
