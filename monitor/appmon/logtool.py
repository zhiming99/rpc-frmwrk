#!/usr/bin/env python3

import re
import sys
import struct
import time
import argparse
#from . import namedlock
from namedlock import NamedProcessLock as npl
import os
import shutil
from datetime import datetime

# EnumTypeId mapping (adjust as needed)
TYPE_MAP = {
    1: ('B', 1),   # typeByte
    2: ('H', 2),   # typeUInt16
    3: ('I', 4),   # typeUInt32
    5: ('f', 4),   # typeFloat
    6: ('d', 8),   # typeDouble
    4: ('Q', 8),   # typeUInt64
}

TYPE_NAME_MAP = {
    1 : "byte" ,
    2: "int16",
    3: "int32",
    5: "float",
    6: "double",
    4: "int64"
}

NAME_TYPE_MAP = {
    "byte" : 1,
    "int16": 2,
    "int32": 3,
    "float": 5,
    "double": 6,
    "int64": 4
}

bPretty = False

def PrintLog(filename):
    with open(filename, 'rb') as f:
        # Read LOGHDR (16 bytes: 4+4+2+2+1+3)
        hdr = f.read(16)
        if len(hdr) < 16:
            print("File too short for LOGHDR")
            return
        dwMagic, dwCounter, wTypeId, wRecSize, byUnit, *_ = struct.unpack('>I I H H B 3s', hdr)
        if dwMagic != 0x706c6f67:
            print("Invalid LOG_MAGIC")
            return
        # Determine type format and size
        if wTypeId not in TYPE_MAP:
            print(f"Unknown type id: {wTypeId}")
            return
        data_fmt, data_size = TYPE_MAP[wTypeId]
        rec_fmt = '>I' + data_fmt  # timestamp (uint32) + data
        rec_size = 4 + data_size
        if wRecSize < rec_size:
            print("Record size too small for type")
            return
        print(f"LOGHDR: Magic={dwMagic}" )
        print(f"LOGHDR: Record Num={dwCounter}")
        print(f"LOGHDR: RecSize={wRecSize}" )
        print(f"LOGHDR: Unit={byUnit}")
        print(f"LOGHDR: format=<timestamp, {TYPE_NAME_MAP[wTypeId]}>")

        global bBrief
        if bBrief:
            return
        # Read records
        rec_num = dwCounter
        for i in range(rec_num):
            rec = f.read(wRecSize)
            if len(rec) < rec_size:
                break
            ts, data = struct.unpack(rec_fmt, rec[:rec_size])
            global bPretty
            if bPretty:
                print(f"{time.strftime('%Y-%m-%d-%H:%M:%S', time.localtime(ts))} {data}")
            else:
                print(f"{ts} {data}")

def ClearLog(filename):
    with open(filename, 'r+b') as f:
        hdr = f.read(16)
        dwMagic, dwCounter, wTypeId, wRecSize, byUnit, *_ = struct.unpack('>I I H H B 3s', hdr)
        if dwMagic != 0x706c6f67:
            print("Invalid LOG_MAGIC")
            return
        f.seek(0)
        f.write(struct.pack('>I I H H B 3s', dwMagic, 0, wTypeId, wRecSize, byUnit, b'\x00\x00\x00'))
        f.truncate()

def InitializeLog( filename, iType ):
    with open(filename, 'wb') as f:
        dwMagic = 0x706c6f67
        dwCounter = 0
        wTypeId = iType
        wRecSize = TYPE_MAP[iType][1] + 4
        byUnit = 0
        f.seek(0)
        f.write(struct.pack('>I I H H B 3s', dwMagic, dwCounter, wTypeId, wRecSize, byUnit, b'\x00\x00\x00'))
        f.truncate()

def BackupLog( pointName ):
    pass

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Print monitor log content.")
    parser.add_argument('filePath', help="Path to the log file or the 'point name' whose log will be processed.")
    parser.add_argument( '--pretty', action='store_true', help="print timestamp in pretty format.")
    parser.add_argument( '-p', action='store_true', help="print the logs.")
    parser.add_argument( '--brief', action='store_true', help="print the log header only, must be used with '-p'.")
    parser.add_argument( '-c', action='store_true', help='clear all the log records' )
    parser.add_argument( '-i', nargs=1, metavar=('<type>'), type=str,
                        help='initialize an empty log file, type can be "byte",' +
                        '"int16", "int32", "float", "double", or "int64"' )
    parser.add_argument( '-b', action='store_true', help="backup log files of the specified point")
    parser.add_argument( '-r', action='store_true', help="rotate log files of the specified point")
    parser.add_argument( '-l', action='store_true', help="list log files of the specified point")

    args = parser.parse_args()
    if args.pretty:
        bPretty = True

    if args.brief and not args.p:
        print("Error: --brief must be used with -p")
        quit(-1)
    
    bBrief = True if args.brief else False

    if not args.filePath :
        print( "Error missing log file path")
        quit(-1)

    filePath = os.path.realpath(args.filePath)
    try:
        if not args.b and not args.r:
            result = re.search(r"^(.*/appreg/apps/.*/points/.*/logs/)(.*)$", filePath)
        else:
            result = None
        if not result:
            rootPath = 0
        else:
            rootPath = result.start() + len("/appreg")
        if args.p:
            if rootPath > 0:
                print( filePath[rootPath :] )
                with npl( filePath[rootPath :]) as locked:
                    PrintLog(filePath)
            else:
                PrintLog(filePath)
            quit(0) 
        if args.c:
            if rootPath > 0:
                with npl( filePath[rootPath :]) as locked:
                    ClearLog(filePath)
            else:
                ClearLog(filePath)
            quit(0)
        if args.i:
            if args.i[0] in NAME_TYPE_MAP:
                iType = NAME_TYPE_MAP[args.i[0]]
            else:
                iType=int(args.i[0])

            if iType not in TYPE_MAP:
                print(f"Unknown type id: {iType}")
                quit(-1)
            if rootPath > 0:
                with npl( filePath[rootPath :]) as locked:
                    InitializeLog(filePath, iType)
            else:
                InitializeLog(filePath, iType)
            quit(0)
        if args.b:
            appName,pointName=args.filePath.split("/")
            homeDir = str( os.path.realpath( os.path.expanduser('~') )) 
            rootDir = f"/apps/{appName}/points/{pointName}/logs"
            logPath = homeDir + f"/.rpcf/appmonroot/appreg" + rootDir
            if not os.path.exists(logPath) or not os.path.exists( logPath + "/ptr0-0" ):
                print(f"Error Log path {logPath} is not valid.")
                quit(-1)
            with npl( rootDir + "/ptr0-0" ) as locked:
                print(f"Backup log files for {logPath}")
                tarFile=f"{homeDir}/{appName}-{pointName}-{datetime.now().strftime('%m-%d')}.tar.gz"
                cmdLine = f"cd {logPath} && tar zcf {tarFile} ptr*"
                ret = os.system( cmdLine )
                if ret != 0:
                    print(f"Error creating backup tar file {tarFile}")
                else:
                    print(f"Logs are backed up to {tarFile} successfully.")
            quit(0)

        if args.r:
            maxFiles = 9
            appName,pointName=args.filePath.split("/")
            homeDir = str( os.path.realpath( os.path.expanduser('~') )) 
            rootDir = f"/apps/{appName}/points/{pointName}/logs"
            logPath = homeDir + f"/.rpcf/appmonroot/appreg" + rootDir
            if os.path.exists( logPath + "/ptr0-extfile"):
                print(f"Error {logPath}/ptr0-extfile is outside the registry and is not rotated.")
                quit(-1)

            if not os.path.exists( logPath + "/ptr0-0" ):
                print(f"Error Log path {logPath} is not valid.")
                quit(-1)
            for i in range(maxFiles, 0, -1):
                if os.path.exists( logPath + f"/ptr0-{i}" ):
                    if i == maxFiles:
                        os.remove( logPath + f"/ptr0-{i}" )
                    else:
                        os.rename( logPath + f"/ptr0-{i}", logPath + f"/ptr0-{i+1}" )
            with npl( rootDir + "/ptr0-0" ) as locked:
                shutil.copy( logPath + "/ptr0-0", logPath + "/ptr0-1" )
                ClearLog( logPath + "/ptr0-0" )
                print(f"Successfully rotated the logs in {logPath}")
            quit(0)

        if args.l:
            appName,pointName=args.filePath.split("/")
            homeDir = str( os.path.realpath( os.path.expanduser('~') )) 
            rootDir = f"/apps/{appName}/points/{pointName}/logs"
            logPath = homeDir + f"/.rpcf/appmonroot/appreg" + rootDir
            os.system( f"ls -ltr {logPath}" )
            quit( 0 )

    except Exception as e:
        print(f"Error: {e}")
        quit(-1)
