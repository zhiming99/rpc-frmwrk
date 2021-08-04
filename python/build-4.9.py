import os
import sipconfig

build_file = "rpcf.sbf"
config=sipconfig.Configuration()

#os.system( "rm ./sip4build/*" )
os.system( " ".join([config.sip_bin, "-c", "./sip4build", "-b", build_file, "rpcf.sip" ] ) )
makefile = sipconfig.SIPModuleMakefile(config, build_file)
makefile.dir = "./sip4build"
makefile.extra_libs = ["combase", "ipc" ]
makefile.extra_defines = ["DEBUG","_USE_LIBEV" ]
makefile.extra_library_dirs = ["../../combase/.libs", "../../ipc/.libs" ]
makefile.extra_include_dirs = [ "../../include","../../ipc", "../../test/stmtest", "/usr/include/dbus-1.0", "/usr/lib/x86_64-linux-gnu/dbus-1.0/include", "/usr/include/jsoncpp" ]
makefile.generate()
os.system( "make -C ./sip4build" )
