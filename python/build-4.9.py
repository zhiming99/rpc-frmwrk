#!/usr/bin/env python3
import os
import sys

from pathlib import Path
Path("./sip4build").mkdir(parents=True, exist_ok=True)
curPath=str( Path(__file__).parent.resolve() ) + "/"

import sipconfig
build_file = "rpcf.sbf"
config=sipconfig.Configuration()

if len( sys.argv ) > 1 :
    libdir=sys.argv[ 1 ]

#os.system( "rm ./sip4build/*" )
os.system( " ".join([config.sip_bin, "-c", "./sip4build", "-b", build_file, "rpcf.sip" ] ) )

macros = config.build_macros()
macros[ 'CC' ] = os.environ['CC']
macros[ 'CXX' ] = os.environ['CXX']                              
macros[ 'LINK' ] = os.environ['CXX']                             
macros[ 'LINK_SHLIB' ] = macros[ 'LINK' ]                        
sysroot = os.environ['SYSROOT']                                  

makefile = sipconfig.SIPModuleMakefile(config, build_file, export_all=1)
makefile.dir = "./sip4build"
makefile.extra_libs = ["combase", "ipc" ]
makefile.extra_defines = ["DEBUG","_USE_LIBEV" ]
makefile.extra_lib_dirs = [ \
    curPath + "../combase/.libs", \
    curPath + "../ipc/.libs" ]

try:
    pkgconfig=os.environ[ 'PKG_CONFIG' ]
    if pkgconfig is None or len( pkgconfig ) == 0:
        pkgconfig = 'pkg-config'
    armbuild = os.environ[ 'ARMBUILD' ]
    if armbuild == "1" :
        makefile.extra_libs.append( "atomic" )
except:
    pkgconfig = 'pkg-confg'

python_inc = pkgconfig + ' --cflags '
python_inc += os.popen( pkgconfig + ' --list-all | grep "python-3" | sort -r').read().split()[ 0 ] 
python_inc += '| sed \'s;-I;;g\''
python3_path = os.popen( python_inc ).read().split()

dbus_path = os.popen( pkgconfig + ' --cflags dbus-1 | sed "s/-I//g"').read().split()
jsoncpp_path = os.popen( pkgconfig + ' --cflags jsoncpp | sed "s/-I//g"').read().split()
makefile.extra_include_dirs = [ \
    curPath + "../include", \
    curPath + "../ipc", \
    curPath + "../test/stmtest", \
    sysroot + "/usr/include" ] + \
    dbus_path + jsoncpp_path + python3_path

if libdir is not None :
    rpaths = "-Wl,-rpath=" + libdir + ",-rpath=" + libdir + "/rpcf"
    makefile.extra_lflags.append( rpaths )

makefile.generate()
os.system( "make -C ./sip4build" )
