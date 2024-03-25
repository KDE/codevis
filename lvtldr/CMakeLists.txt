include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty)

AddTargetLibrary(
    LIBRARY_NAME
        lvtldr
    SOURCES
        ct_lvtldr_componentnode.cpp
        ct_lvtldr_graphloader.cpp
        ct_lvtldr_nodestorage.cpp
        ct_lvtldr_packagenode.cpp
        ct_lvtldr_physicalloader.cpp
        ct_lvtldr_lakosiannode.cpp
        ct_lvtldr_lakosianedge.cpp
        ct_lvtldr_repositorynode.cpp
        ct_lvtldr_typenode.cpp
        ct_lvtldr_igraphloader.cpp
        ct_lvtldr_freefunctionnode.cpp
    HEADERS
        ct_lvtldr_componentnode.h
        ct_lvtldr_componentnodefields.h
        ct_lvtldr_databasehandler.h
        ct_lvtldr_graphloader.h
        ct_lvtldr_packagenode.h
        ct_lvtldr_packagenodefields.h
        ct_lvtldr_physicalloader.h
        ct_lvtldr_lakosianedge.h
        ct_lvtldr_repositorynode.h
        ct_lvtldr_repositorynodefields.h
        ct_lvtldr_sociutils.h
        ct_lvtldr_typenode.h
        ct_lvtldr_igraphloader.h
        ct_lvtldr_freefunctionnode.h
        ct_lvtldr_freefunctionnodefields.h
    QT_HEADERS
        ct_lvtldr_nodestorage.h
        ct_lvtldr_lakosiannode.h
    LIBRARIES
        Codethink::lvtshr
        Soci::core
        Soci::sqlite3
        Qt${QT_MAJOR_VERSION}::Core
        ${SQLite3_LIBRARIES}
)

target_include_directories(lvtldr
    PRIVATE
    ${SQLite3_INCLUDE_DIRS}
)

target_include_directories(lvtldr
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../thirdparty/
)

if (COMPILE_TESTS)
    add_executable(
        test_ct_lvtldr_lakosiannode
            ct_lvtldr_nodestoragetestutils.h
            ct_lvtldr_lakosiannode.t.cpp
    )
    target_link_libraries(test_ct_lvtldr_lakosiannode
        Codethink::lvtldr
        Codethink::lvtclp
        Codethink::lvtclp_test
        Codethink::lvttst
        Codethink::lvttst_tmpdir
        ${SYSTEM_EXTRA_LIBRARIES}
    )
    target_include_directories(test_ct_lvtldr_lakosiannode PUBLIC ${LLVM_INCLUDE_DIR})
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # for clang headers for lvtclp
        set_target_properties(test_ct_lvtldr_lakosiannode
                PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} ${SKIP_CLANG_WARNINGS}")
    endif()
    add_test(NAME test_ct_lvtldr_lakosiannode COMMAND test_ct_lvtldr_lakosiannode)

    add_executable(test_ct_lvtldr_nodestorage
        ct_lvtldr_nodestoragetestutils.h
        ct_lvtldr_nodestorage.t.cpp)
    target_link_libraries(test_ct_lvtldr_nodestorage
        Codethink::lvtldr
        Codethink::lvttst
        Codethink::lvttst_tmpdir
        Codethink::lvtmdb
        Codethink::lvtprj
        ${SYSTEM_EXTRA_LIBRARIES}
        Soci::core
        Soci::sqlite3
        Qt${QT_MAJOR_VERSION}::Core
        ${SQLite3_LIBRARIES}
    )
    target_include_directories(test_ct_lvtldr_nodestorage
        PRIVATE
        ${SQLite3_INCLUDE_DIRS}
    )
    add_test(NAME test_ct_lvtldr_nodestorage COMMAND test_ct_lvtldr_nodestorage)
endif()
