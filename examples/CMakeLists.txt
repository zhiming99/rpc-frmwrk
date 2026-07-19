cmake_minimum_required(VERSION 3.18)
#project(rpcf_examples LANGUAGES NONE) # Verification & source sample directory, no local C++ compilation

# Expose standard directory variables (maps sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Verification Specification Installations (dist_examples_conf_DATA)
# ==============================================================================
# Replaces: examples_confdir=$(sysconfdir)/rpcf
install(FILES
    actcancel.ridl
    asynctst.ridl
    evtest.ridl
    iftest.ridl
    katest.ridl
    stmtest.ridl
    sftest.ridl
    hellowld.ridl
    testypes.ridl
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf
    COMPONENT Development
)

# ==============================================================================
# 2. Strict Whitelisting Reference Variable for Project Sources Packaging
# ==============================================================================
# As established in our packaging protection setup, listing these files explicitly 
# ensures that when CPack builds your source archive distribution, it tracks 
# only verified files. All uncommitted noise (.bak, temporary files, etc.) is ignored.

set(examples_DIST_FILES
    # --- Java Reference Tests ---
    java/src/org/rpcf/tests/actcancel/ActiveCancelcli.java
    java/src/org/rpcf/tests/actcancel/ActiveCancelsvr.java
    java/src/org/rpcf/tests/org/rpcf/tests/actcancel/maincli.java
    java/src/org/rpcf/tests/asynctst/AsyncTestcli.java
    java/src/org/rpcf/tests/asynctst/AsyncTestsvr.java
    java/src/org/rpcf/tests/asynctst/maincli.java
    java/src/org/rpcf/tests/evtest/EventTestcli.java
    java/src/org/rpcf/tests/evtest/maincli.java
    java/src/org/rpcf/tests/evtest/mainsvr.java
    java/src/org/rpcf/tests/iftest/IfTestsvr.java
    java/src/org/rpcf/tests/iftest/maincli.java
    java/src/org/rpcf/tests/katest/KeepAlivesvr.java
    java/src/org/rpcf/tests/katest/maincli.java
    java/src/org/rpcf/tests/stmtest/StreamTestcli.java
    java/src/org/rpcf/tests/stmtest/StreamTestsvr.java
    java/src/org/rpcf/tests/stmtest/TransferContext.java
    java/src/org/rpcf/tests/stmtest/maincli.java
    java/src/org/rpcf/tests/sftest/FileTransfercli.java
    java/src/org/rpcf/tests/sftest/FileTransfersvr.java
    java/src/org/rpcf/tests/sftest/TransFileContext.java
    java/src/org/rpcf/tests/sftest/TransferContext.java
    java/src/org/rpcf/tests/sftest/TransferContextCli.java
    java/src/org/rpcf/tests/sftest/maincli.java
    java/src/org/rpcf/tests/HelloWorld/HelloWorldSvcsvr.java
    java/src/org/rpcf/tests/HelloWorld/maincli.java

    # --- Python Reference Tests ---
    python/actcancel/ActiveCancelcli.py
    python/actcancel/ActiveCancelsvr.py
    python/actcancel/maincli.py
    python/asynctst/AsyncTestcli.py
    python/asynctst/AsyncTestsvr.py
    python/asynctst/maincli.py
    python/evtest/EventTestcli.py
    python/evtest/maincli.py
    python/evtest/mainsvr.py
    python/hellowld/HelloWorldSvcsvr.py
    python/hellowld/maincli.py
    python/iftest/IfTestsvr.py
    python/iftest/maincli.py
    python/katest/KeepAlivesvr.py
    python/katest/maincli.py
    python/sftest/FileTransfercli.py
    python/sftest/FileTransfersvr.py
    python/sftest/TransFileContext.py
    python/sftest/TransferContext.py
    python/sftest/maincli.py
    python/stmtest/StreamTestcli.py
    python/stmtest/StreamTestsvr.py
    python/stmtest/maincli.py
    python/thermalog/HelloWorldSvcsvr.py
    python/thermalog/HelloWorldSvcsvrbase.py
    python/thermalog/HelloWorlddesc.json
    python/thermalog/HelloWorldstructs.py
    python/thermalog/Makefile
    python/thermalog/__init__.py
    python/thermalog/adddev.sh
    python/thermalog/cmdline
    python/thermalog/driver.json
    python/thermalog/mainsvr.py
    python/thermalog/seribase.py
    python/thermalog/synccfg.py
    python/thermalog/README_cn.md
    python/thermalog/listing-thermal-app.png
    python/thermalog/temperature-chart.png
    python/thermalog/thermal-app-detail.png
    python/thermalog/thermometer-with-rs485.png

    # --- C++ Reference Tests ---
    cpp/actcancel/ActiveCancelcli.cpp
    cpp/actcancel/ActiveCancelcli.h
    cpp/actcancel/ActiveCancelsvr.cpp
    cpp/actcancel/ActiveCancelsvr.h
    cpp/actcancel/maincli.cpp
    cpp/asynctst/AsyncTestcli.cpp
    cpp/asynctst/AsyncTestcli.h
    cpp/asynctst/AsyncTestsvr.cpp
    cpp/asynctst/AsyncTestsvr.h
    cpp/asynctst/maincli.cpp
    cpp/evtest/EventTestcli.cpp
    cpp/evtest/maincli.cpp
    cpp/evtest/mainsvr.cpp
    cpp/hellowld/HelloWorldSvcsvr.cpp
    cpp/hellowld/maincli.cpp
    cpp/iftest/IfTestsvr.cpp
    cpp/iftest/maincli.cpp
    cpp/katest/KeepAlivesvr.cpp
    cpp/katest/KeepAlivesvr.h
    cpp/katest/maincli.cpp
    cpp/sftest/FileTransfercli.cpp
    cpp/sftest/FileTransfercli.h
    cpp/sftest/FileTransfersvr.cpp
    cpp/sftest/FileTransfersvr.h
    cpp/sftest/Makefile
    cpp/sftest/Readme.md
    cpp/sftest/maincli.cpp
    cpp/sftest/transctx.cpp
    cpp/sftest/transctx.h
    cpp/stmtest/StreamTestcli.cpp
    cpp/stmtest/StreamTestcli.h
    cpp/stmtest/StreamTestsvr.cpp
    cpp/stmtest/StreamTestsvr.h
    cpp/stmtest/commdefs.h
    cpp/stmtest/maincli.cpp
    cpp/testypes/maincli.cpp
    cpp/testypes/cmdline
    cpp/testypes/TestTypesSvcsvr.cpp
    cpp/rpcfs/actcancel2/ActiveCancelcli.cpp
    cpp/rpcfs/actcancel2/ActiveCancelcli.h
    cpp/rpcfs/actcancel2/ActiveCancelsvr.cpp
    cpp/rpcfs/actcancel2/ActiveCancelsvr.h
    cpp/rpcfs/actcancel2/maincli.cpp
    cpp/rpcfs/asynctst2/AsyncTestcli.cpp
    cpp/rpcfs/asynctst2/ActiveCancelcli.h
    cpp/rpcfs/asynctst2/AsyncTestsvr.cpp
    cpp/rpcfs/asynctst2/AsyncTestsvr.h
    cpp/rpcfs/asynctst2/maincli.cpp
    cpp/rpcfs/evtest2/EventTestcli.cpp
    cpp/rpcfs/evtest2/maincli.cpp
    cpp/rpcfs/evtest2/mainsvr.cpp
    cpp/rpcfs/hellowld2/HelloWorldSvcsvr.cpp
    cpp/rpcfs/hellowld2/maincli.cpp
    cpp/rpcfs/iftest2/IfTestsvr.cpp
    cpp/rpcfs/iftest2/maincli.cpp
    cpp/rpcfs/katest2/KeepAlivesvr.cpp
    cpp/rpcfs/katest2/KeepAlivesvr.h
    cpp/rpcfs/katest2/maincli.cpp
    cpp/rpcfs/sftest2/FileTransfercli.cpp
    cpp/rpcfs/sftest2/FileTransfercli.h
    cpp/rpcfs/sftest2/FileTransfersvr.cpp
    cpp/rpcfs/sftest2/FileTransfersvr.h
    cpp/rpcfs/sftest2/Makefile
    cpp/rpcfs/sftest2/Readme.md
    cpp/rpcfs/sftest2/maincli.cpp
    cpp/rpcfs/sftest2/transctx.cpp
    cpp/rpcfs/sftest2/transctx.h
    cpp/rpcfs/stmtest2/StreamTestcli.cpp
    cpp/rpcfs/stmtest2/StreamTestcli.h
    cpp/rpcfs/stmtest2/StreamTestsvr.cpp
    cpp/rpcfs/stmtest2/StreamTestsvr.h
    cpp/rpcfs/stmtest2/commdefs.h
    cpp/rpcfs/stmtest2/maincli.cpp

    # --- JavaScript Reference Tests (From Last Part) ---
    js/iftest/iftest.html
    js/sftest/sftest.html
    js/sftest/FileTransfercli.js
    js/sftest/maincli.js
    js/hellowld/HelloWorld.html
    js/hellowld/maincli.js
    js/actcancel/ActiveCancelcli.js
    js/actcancel/actcancel.html
    js/actcancel/maincli.js
    js/evtest/EventTestcli.js
    js/katest/KeepAlivecli.js
    js/katest/katest.html
    js/katest/maincli.js
    js/stmtest/StreamTestcli.js
    js/stmtest/maincli.js
    js/player/sftest.html
    js/player/FileTransfercli.js
    js/player/maincli.js
)

