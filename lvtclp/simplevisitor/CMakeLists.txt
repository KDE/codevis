if (USE_LLVM_STATIC_LIBS OR MSVC)
    SET(CLANG_EXTRA_LIBRARIES
        clangIndex
        clangFormat
        clangTooling
        clangToolingInclusions
        clangToolingCore
        clangFrontend
        clangParse
        clangSerialization
        clangSema
        clangAST
        clangLex
        clangDriver
        clangBasic
    )
elseif(APPLE)
    # Craft - The tool we use to manage builds on windows and apple
    # probably has a bug on the llvm recipe that makes libLLVM.dylib
    # not useful. I'v discovered that by linking to libLLVMSupport we at least
    # have a successfull build.
    SET(CLANG_EXTRA_LIBRARIES
        clang-cpp
        LLVMSupport
    )

else()
    SET(CLANG_EXTRA_LIBRARIES
        clang-cpp
        LLVM
    )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_visitor
    logicaldepscanner.cpp
    logicaldepvisitor.cpp
    test_visitor.t.cpp
)

target_link_libraries(test_visitor
    ${CLANG_EXTRA_LIBRARIES}
)
