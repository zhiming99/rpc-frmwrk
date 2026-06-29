# 1. --- Define Documentation Files (EXTRA_DIST Translation) ---
set(DOC_FILES
    "index_cn.md"
    "Tut-HelloWorld_cn-1.md"
    "Tut-SendEvent_cn-2.md"
    "Tut-AsyncRequest_cn-3.md"
    "Tut-CancelRequest_cn-4.md"
    "Tut-Serialization_cn-5.md"
    "Tut-Overview_cn-0.md"
    "Tut-Stream_cn-6.md"
    "Tut-AsyncProgramming_cn-7.md"
    "Tut-Debug_cn-8.md"
    "Tut-HowToBuild_cn-9.md"
    "Tut-Index.md"
)

# 2. --- Create a Custom Target for IDE Organization ---
# This does not compile a binary, but exposes the markdown files neatly to IDEs
add_custom_target(documentation_files SOURCES ${DOC_FILES})

# 3. --- Optional: Define Documentation Installation Rules ---
# If you want running 'make install' to deploy your markdown guides into 
# your system's share/doc path, you can uncomment this block:
#
# include(GNUInstallDirs)
# install(FILES ${DOC_FILES}
#     DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/rpcf
# )

