include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty)

qt5_add_resources(db_res_SRCS
    ../database-spec/databases.qrc
)

AddTargetLibrary(
    LIBRARY_NAME lvtmdb
    SOURCES
        ct_lvtmdb_componentobject.cpp
        ct_lvtmdb_databaseobject.cpp
        ct_lvtmdb_errorobject.cpp
        ct_lvtmdb_fieldobject.cpp
        ct_lvtmdb_fileobject.cpp
        ct_lvtmdb_functionbase.cpp
        ct_lvtmdb_functionobject.cpp
        ct_lvtmdb_lockable.cpp
        ct_lvtmdb_methodobject.cpp
        ct_lvtmdb_namespaceobject.cpp
        ct_lvtmdb_objectstore.cpp
        ct_lvtmdb_packageobject.cpp
        ct_lvtmdb_repositoryobject.cpp
        ct_lvtmdb_typeobject.cpp
        ct_lvtmdb_util.cpp
        ct_lvtmdb_variableobject.cpp
        ct_lvtmdb_soci_writer.cpp
        ct_lvtmdb_soci_reader.cpp
        ${db_res_SRCS}
    HEADERS
        ct_lvtmdb_componentobject.h
        ct_lvtmdb_databaseobject.h
        ct_lvtmdb_errorobject.h
        ct_lvtmdb_fieldobject.h
        ct_lvtmdb_fileobject.h
        ct_lvtmdb_functionobject.h
        ct_lvtmdb_lockable.h
        ct_lvtmdb_methodobject.h
        ct_lvtmdb_namespaceobject.h
        ct_lvtmdb_objectstore.h
        ct_lvtmdb_packageobject.h
        ct_lvtmdb_repositoryobject.h
        ct_lvtmdb_typeobject.h
        ct_lvtmdb_util.h
        ct_lvtmdb_variableobject.h
        ct_lvtmdb_soci_writer.h
        ct_lvtmdb_soci_writer.h
    LIBRARIES
        Codethink::lvtshr
        Soci::Core
        Soci::soci_sqlite3
        ${SQLite3_LIBRARIES}
)

target_include_directories(lvtmdb
    PRIVATE
    ${SQLite3_INCLUDE_DIRS}
)

if (COMPILE_TESTS)
    add_executable(test_lvtmdb ct_lvtmdb.t.cpp)
    target_link_libraries(test_lvtmdb Codethink::lvtprj lvttst)
    add_test(NAME test_lvtmdb COMMAND test_lvtmdb)
endif()
