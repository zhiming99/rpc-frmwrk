cmake_minimum_required(VERSION 3.18)
project(rpcf_usermgmt LANGUAGES NONE) # No C/C++ compilation required here

# Expose standard directory variables (maps bindir, mandir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Installation Routines
# ==============================================================================

# Replaces: if ENABLE_FUSE3 / dist_usrmgt_SCRIPTS=$(bindist) / usrmgtdir=$(bindir)/rpcf
if(ENABLE_FUSE3)
    install(PROGRAMS
        inituser.sh
        pubfuncs.sh
        rpcfaddg.sh
        rpcfaddu.sh
        rpcfmodu.sh
        rpcfrmg.sh
        rpcfrmu.sh
        rpcfshow.sh
        updattr.py
        DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf
    )
endif()

# Replaces: man_MANS= rpcfaddu.1 rpcfmodu.1 rpcfshow.1 updattr.1
# Note: Usually man pages are installed regardless of conditional scripts,
# but if your root configuration bounds them to ENABLE_FUSE3, wrap this block inside the if().
install(FILES
    rpcfaddu.1
    rpcfmodu.1
    rpcfshow.1
    updattr.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)

