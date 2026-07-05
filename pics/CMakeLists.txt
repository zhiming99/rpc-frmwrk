cmake_minimum_required(VERSION 3.18)
#project(rpcf_pics LANGUAGES NONE)

# ------------------------------------------------------------------------------
# EXTRA_DIST Emulation via Explicit File Installation (Optional)
# ------------------------------------------------------------------------------
# If these assets ever need to be installed into a documentation or asset folder 
# (e.g., share/doc/rpcf/pics), listing them explicitly here guarantees that only 
# these tracked project files are deployed. Any untracked temporary files left 
# in the local directory by developers will be safely ignored.

set(pics_DIST_FILES
    hellowld.png
    hellowld-tree.png
    installer-name.png
    multihop.png
    ridldatatype.png
    ridl-req-mapping.png
    rpc-block-diagram.png
    rpcfg2.png
    rpcfg3.png
    rpcfg4.png
    rpcfg.png
    rpcfs-cli.png
    rpcfs-svr.png
    screenshot-2019-10-21-180941.png
    sync-async.png
    sync-async-svr.png
    OAuth2-1_cn.png
    OAuth2-2_cn.png
    gen-self-signed-key.png
    select-auth-mech.png
    rpcfg2-1.png
    rpcfg2-2.png
    rpcfg2-3.png
    rpcfg2-4.png
    new.png
    client-req-process.png
    server-req-process.png
    dumpobjs.png
    rpcfg-SimpAuth.png
    mon-login.png
    mon-main.png
    mon-rpcrouter.png
    mon-setpv.png
    mon-ptlog-chart.png
    rpcfgtui-main-menu.png
    rpcfgtui-connection-page.png
    rpcfgtui-config-list.png
    rpcfgtui-multihop-page.png
    rpcfgtui-security-page.png
)

# Example Deployment Hook: Un-comment and adjust path if deployment is desired
# include(GNUInstallDirs)
# install(FILES ${pics_DIST_FILES} DESTINATION ${CMAKE_INSTALL_DOCDIR}/pics)

