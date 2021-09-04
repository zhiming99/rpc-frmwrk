import os
import sys

from pathlib import Path
Path("./sip4build").mkdir(parents=True, exist_ok=True)

import sipconfig
build_file = "rpcf.sbf"
config=sipconfig.Configuration()

if len( sys.argv ) > 1 :
    libdir=sys.argv[ 1 ]

#os.system( "rm ./sip4build/*" )
os.system( " ".join([config.sip_bin, "-c", "./sip4build", "-b", build_file, "rpcf.sip" ] ) )
makefile = sipconfig.SIPModuleMakefile(config, build_file, export_all=1)
makefile.dir = "./sip4build"
makefile.extra_libs = ["combase", "ipc" ]
makefile.extra_defines = ["DEBUG","_USE_LIBEV" ]
makefile.extra_lib_dirs = ["../../combase/.libs", "../../ipc/.libs" ]

dbus_path = os.popen('pkg-config --cflags dbus-1 | sed "s/-I//g"').read().split()
jsoncpp_path = os.popen('pkg-config --cflags jsoncpp | sed "s/-I//g"').read().split()
makefile.extra_include_dirs = [ "../../include","../../ipc", "../../test/stmtest" ] + dbus_path + jsoncpp_path

if libdir is not None :
    rpaths = "-Wl,-rpath=" + libdir + ",-rpath=" + libdir + "/rpcf"
    makefile.extra_lflags.append( rpaths )

makefile.generate()
os.system( "make -C ./sip4build" )
