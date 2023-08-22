# Model Libraries
# Those libraries defines a standard way for the interface
# To query data and present in list, tables and trees.

AddTargetLibrary(
  LIBRARY_NAME
    lvtmdl
  SOURCES
    ct_lvtmdl_basetablemodel.cpp
    ct_lvtmdl_basetreemodel.cpp
    ct_lvtmdl_circular_relationships_model.cpp
    ct_lvtmdl_debugmodel.cpp
    ct_lvtmdl_errorsmodel.cpp
    ct_lvtmdl_fieldstablemodel.cpp
    ct_lvtmdl_historylistmodel.cpp
    ct_lvtmdl_methodstablemodel.cpp
    ct_lvtmdl_modelhelpers.cpp
    ct_lvtmdl_namespacetreemodel.cpp
    ct_lvtmdl_packagetreemodel.cpp
    ct_lvtmdl_physicaltablemodels.cpp
    ct_lvtmdl_simpletextmodel.cpp
    ct_lvtmdl_treefiltermodel.cpp
    ct_lvtmdl_usesintheimpltablemodel.cpp
    ct_lvtmdl_usesintheinterfacetablemodel.cpp
  QT_HEADERS
    ct_lvtmdl_basetablemodel.h
    ct_lvtmdl_circular_relationships_model.h
    ct_lvtmdl_errorsmodel.h
    ct_lvtmdl_methodstablemodel.h
    ct_lvtmdl_simpletextmodel.h
    ct_lvtmdl_basetreemodel.h
    ct_lvtmdl_modelhelpers.h
    ct_lvtmdl_treefiltermodel.h
    ct_lvtmdl_debugmodel.h
    ct_lvtmdl_namespacetreemodel.h
    ct_lvtmdl_usesintheimpltablemodel.h
    ct_lvtmdl_fieldstablemodel.h
    ct_lvtmdl_packagetreemodel.h
    ct_lvtmdl_usesintheinterfacetablemodel.h
    ct_lvtmdl_historylistmodel.h
    ct_lvtmdl_physicaltablemodels.h

  LIBRARIES
    ${SYSTEM_EXTRA_LIBRARIES}
    Qt::Core
    Codethink::lvtldr
    Codethink::lvtclr
)

if (COMPILE_TESTS)
    add_executable(
        test_ct_lvtmdl_modelhelpers
        ct_lvtmdl_modelhelpers.t.cpp
    )
    target_link_libraries(test_ct_lvtmdl_modelhelpers
        Codethink::lvtmdl
        Codethink::lvtldr
        Codethink::lvtmdb
        Codethink::lvttst
        Codethink::lvttst_tmpdir
        ${SYSTEM_EXTRA_LIBRARIES}
    )
    add_test(NAME test_ct_lvtmdl_modelhelpers COMMAND test_ct_lvtmdl_modelhelpers)
endif()
