#!/bin/bash

# Download and compile everything.
# Arguments:
# $1 = source folder.
# $2 = build folder.
# $3 = install prefix.
# $4 = Processor Count
#
# Returns a full path of the file or directory without relative subpaths
# parameters
#   $1 the path
#   $2 the result variable
#
#

function show_help() {
    echo "Usage: $(basename "$0") source_folder build_folder install_folder processor_count"
    echo ""
    echo "---------------"
    echo "source_folder: the directory used to download and extract the sources"
    echo "build_folder: the directory where the build artifacts will be generated"
    echo "install_folder: the base folder used to install the compiled software"
    echo "processor_count: the number of processors for the parallel build"
}

if [ "$1" == "-h" ]; then
    show_help
    exit 0
fi
if [ "$#" -ne 4 ]; then
    echo "Number of parameters are incorrect. $#, needs 4"
    show_help
    exit 1
fi

# Fancy shell error handling.
print_error()
{
    echo "$@" 1>&2
}

err_exit()
{
    print_error "FATAL: $*"
    exit 1
}

assert()
{
    if ! "$@"; then
        err_exit "Assertion failed: '$*'"
    fi
}

assert mkdir -p "$1"
assert mkdir -p "$2"
assert mkdir -p "$3"

## macos lacks the tool "realpath", so we cd and pwd.
function fake_realpath() {
    assert cd -- "$1" > /dev/null 2>&1
    pwd
}

function full_path {
    local __resultvar=$2
    if [ -f "$1" ]; then
        local result=""
        result=$(fake_realpath "$(dirname "$1")")
        eval "$__resultvar"="$result"
    elif [ -d "$1" ]; then
        local result=""
        result=$(fake_realpath "$1")
        eval "$__resultvar"="$result"
    fi
}

full_path "$0" PROJECTPATH
full_path "$PROJECTPATH/../.." PROJECTPATH

full_path "$1" SOURCE_FOLDER
full_path "$2" BUILD_FOLDER
full_path "$3" INSTALL_FOLDER

echo "Configured Folders"
echo "$1 -- $SOURCE_FOLDER"
echo "$2 -- $BUILD_FOLDER"
echo "$3 -- $INSTALL_FOLDER"

export PROCESSOR_COUNT="$4"
export MACOSX_DEPLOYMENT_TARGET="10.15"
export LLVM_VERSION="14.0.4"
# Make sure we have all the software.

if ! which brew >/dev/null; then
    if ! /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"; then
        echo "Error installing brew, please install it manually"
        exit 1
    fi
fi

# update git submodules on the project.
assert cd "$PROJECTPATH"
assert git submodule init
assert git submodule update --recursive

BREW_DEPENDENCIES=(conan wget cmake git bison)
for dep in "${BREW_DEPENDENCIES[@]}"
do
    if ! which "$dep" > /dev/null; then
        assert brew install "$dep"
    fi
done

# Download and extract llvm source to SOURCE_FOLDER/llvm.
if [ ! -d "$SOURCE_FOLDER" ]; then
    assert mkdir "$SOURCE_FOLDER"
fi

if [ ! -d "$SOURCE_FOLDER/llvm-project-llvmorg-$LLVM_VERSION" ]; then
    assert cd "$SOURCE_FOLDER"
    assert wget https://github.com/llvm/llvm-project/archive/llvmorg-$LLVM_VERSION.tar.gz
    assert tar -xf llvmorg-$LLVM_VERSION.tar.gz --directory="$SOURCE_FOLDER"
fi

if [ ! -d "$BUILD_FOLDER" ]; then
    assert mkdir "$BUILD_FOLDER"
fi

if [ ! -d "$BUILD_FOLDER/llvm-$LLVM_VERSION" ]; then
    assert mkdir "$BUILD_FOLDER/llvm-$LLVM_VERSION"
fi

# clang takes *too long* to compile, so let's not try to compile it more than what's needed.
if [ ! -d "$INSTALL_FOLDER/llvm-$LLVM_VERSION" ]; then
    assert cd "$BUILD_FOLDER/llvm-$LLVM_VERSION"

    # llvm project is inside of a folder called `llvm` on the `llvm` project folder.
    # confusing, but yeah.
    result="$(cmake "$SOURCE_FOLDER/llvm-project-llvmorg-$LLVM_VERSION/llvm" \
        -DLLVM_ENABLE_LIBCXX=ON \
        -DLLVM_BUILD_LLVM_C_DYLIB=ON \
        -DLLVM_POLLY_LINK_INTO_TOOLS=ON \
        -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON \
        -DLLVM_LINK_LLVM_DYLIB=ON \
        -DLLVM_ENABLE_EH=ON \
        -DLLVM_ENABLE_FFI=ON \
        -DLLVM_ENABLE_RTTI=ON \
        -DLLVM_INCLUDE_DOCS=OFF \
        -DLLVM_INCLUDE_TESTS=OFF \
        -DLLVM_INSTALL_UTILS=ON \
        -DLLVM_ENABLE_Z3_SOLVER=OFF \
        -DLLVM_OPTIMIZED_TABLEGEN=ON \
        -DLLVM_TARGETS_TO_BUILD=all \
        -DLLDB_USE_SYSTEM_DEBUGSERVER=ON \
        -DLLDB_ENABLE_PYTHON=ON \
        -DLLDB_ENABLE_LUA=OFF \
        -DLLDB_ENABLE_LZMA=ON \
        -DLLDB_INCLUDE_TESTS=OFF \
        -DLIBOMP_INSTALL_ALIASES=OFF \
        -DLLVM_CREATE_XCODE_TOOLCHAIN=ON \
        -DLLVM_TARGETS_TO_BUILD=Native \
        -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld;mlir;polly" \
        -DLLVM_ENABLE_RUNTIMES="compiler-rt;libcxx;libcxxabi;libunwind" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_FOLDER/llvm-$LLVM_VERSION" > llvm_configuration.log)"

    if [ "$result" -ne 0 ]; then
        echo "Error configuring LLVM, check the logs".
    fi

    assert make
    assert make install # if this step finishes correctly, we will have the $(INSTALL_PREFIX)/llvm folder.
fi

if [ ! -d "$BUILD_FOLDER/diagram-server" ]; then
    assert mkdir "$BUILD_FOLDER/diagram-server"
fi

assert cd "$BUILD_FOLDER/diagram-server"
# We always need to clean the build folder because different CI's will
# have different paths, and cmake hates different paths. Conan uses
# cache, so that's not a problem, and the installation is fast.

# Clean the previous build. 
# gitlab CI wil not run things in parallel, so this is not a problem.
rm -rf *

assert conan install "$PROJECTPATH" --build=missing -pr="$PROJECTPATH/conan/mac_11_profile" > conan_install.log
if [ ! -f "conan_paths.cmake" ]; then
    echo "Error running conan install."
    exit 1
fi

export Clang_DIR="$INSTALL_FOLDER/llvm-$LLVM_VERSION/lib/cmake/clang"

# Always run CMake configure, it takes care of just updating what is needed.
assert cmake "$PROJECTPATH" -DBUILD_DESKTOPAPP=ON -DUSE_QT_WEBENGINE=OFF
assert make "-j$PROCESSOR_COUNT"
