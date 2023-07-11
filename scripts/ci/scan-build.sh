#!/bin/bash

set -e

# scan-build executables are named with a version suffix e.g. scan-build-11
SCAN_BUILD_VER_DEFAULT="14"
SCAN_BUILD_VER="${SCAN_BUILD_VER:-${SCAN_BUILD_VER_DEFAULT}}"
SCAN_BUILD="scan-build-$SCAN_BUILD_VER"

SCAN_BUILD_PATH_DEFAULT="/usr/share/clang/$SCAN_BUILD/libexec"
SCAN_BUILD_PATH="${SCAN_BUILD_PATH:-${SCAN_BUILD_PATH_DEFAULT}}"

BUILD_DIR_DEFAULT="$(realpath build)"
BUILD_DIR="${BUILD_DIR:-${BUILD_DIR_DEFAULT}}"

# the directory into which scan-build writes its html report
REPORT_DIR_DEFAULT="$PWD/scan-build-report"
REPORT_DIR="${REPORT_DIR:-${REPORT_DIR_DEFAULT}}"

CLANG="$(command -v clang-"$SCAN_BUILD_VER")"
CLANGXX="$(command -v clang++-"$SCAN_BUILD_VER")"

# tell cmake to build using clang
export CC="$CLANG"
export CXX="$CLANGXX"
export CCC_CC="$CC"
export CCC_CXX="$CXX"

pr_err()
{
    echo "ERROR: $*" 1>&2
}

build()
{
    local suffix="$1"
    local options="$2"
    local dir="${BUILD_DIR}_${suffix}"

    git submodule init
    git submodule update --recursive

    mkdir -p "$dir"
    cd "$dir"
    make clean || true

    cmake \
        -DCMAKE_C_COMPILER="$SCAN_BUILD_PATH/ccc-analyzer" \
        -DCMAKE_CXX_COMPILER="$SCAN_BUILD_PATH/c++-analyzer" \
        -DSCANBUILD=ON \
        "$options" .. || return 1

    "$SCAN_BUILD" \
        --status-bugs \
        -o "$REPORT_DIR" \
        --use-analyzer "$CLANG" \
        --exclude "../thirdparty" \
        --exclude "../submodules" \
        make -j "$(nproc)" || return 1
}

build_qt()
{
    if ! build "QT" ""; then
        pr_err "Issues found with Qt"
        exit 1
    fi
}

build_qt
