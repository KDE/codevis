#!/bin/bash

EXEC=clazy
BUILD_DIR=build
COMPILE_COMMANDS_JSON="$BUILD_DIR/compile_commands.json"
PARALLEL=parallel

TARGET_DIRS="$*"

pr_err()
{
    echo "ERROR: $*" 1>&2
}

fatal()
{
    pr_err "$*"
    exit 1
}

if ! [ -d "$BUILD_DIR" ]; then
    fatal "$BUILD_DIR is not a directory"
fi

if ! [ -f "$COMPILE_COMMANDS_JSON" ]; then
    fatal "$COMPILE_COMMANDS_JSON not found"
fi

TARGET_FILES=

for dir in $TARGET_DIRS; do
    if ! [ -d "$dir" ]; then
        fatal "$dir not found"
    fi

    for file in "$dir"/*.cpp "$dir"/*.h; do
        if [[ ! "$file" =~ .*\.t.cpp$ ]]; then
            TARGET_FILES="$TARGET_FILES $file"
        fi
    done
done

# deglobbing $TARGET_FILES is intentional
# shellcheck disable=SC2086
if [ "$CLAZY_SERIAL" != "" ]; then
    echo "Running in serial"
    set -x
    "$EXEC" --standalone --ignore-included-files -p "$BUILD_DIR" --extra-arg=-Wno-unknown-warning-option $TARGET_FILES
else
    echo "Running in parallel (default)"
    set -x
    "$PARALLEL" "$EXEC" --standalone --ignore-included-files -p "$BUILD_DIR" --extra-arg=-Wno-unknown-warning-option -- $TARGET_FILES
fi
