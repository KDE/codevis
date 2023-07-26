#!/bin/sh

POS_CHECKS="cert-*,cppcoreguidelines-*,modernize-*,performance-*,readability-*"
NEG_CHECKS="-modernize-use-trailing-return-type,-cppcoreguidelines-special-member-functions,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-readability-implicit-bool-conversion,-cppcoreguidelines-pro-bounds-pointer-arithmetic,-cppcoreguidelines-owning-memory,-modernize-use-override,-readability-function-cognitive-complexity,-cppcoreguidelines-avoid-magic-numbers,-readability-magic-numbers,-readability-identifier-length,-readability-suspicious-call-argument,-cppcoreguidelines-init-variables,-cert-err58-cpp,-cppcoreguidelines-pro-type-reinterpret-cast,-cppcoreguidelines-pro-type-const-cast,-cppcoreguidelines-avoid-goto,-cppcoreguidelines-pro-type-vararg,-clang-analyzer-*"

EXEC=clang-tidy-14
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

    for file in "$dir"/*.cpp; do
        TARGET_FILES="$TARGET_FILES $file"
    done
done

for file in $TARGET_FILES; do
    if ! [ -f "$file" ]; then
        fatal "$file not found"
    fi
done

# deglobbing $TARGET_FILES is intentional
# shellcheck disable=SC2086
if [ "$CLANG_TIDY_SERIAL" != "" ]; then
    echo "Running in serial"
    set -x
    "$EXEC" --checks="$POS_CHECKS,$NEG_CHECKS" --header-filter="^ct_.*" -p "$BUILD_DIR" --quiet --warnings-as-errors='*' -extra-arg=-Wno-unknown-warning-option $TARGET_FILES
else
    echo "Running in parallel (default)"
    set -x
    "$PARALLEL" "$EXEC" --checks="$POS_CHECKS,$NEG_CHECKS" --header-filter="^ct_.*" -p "$BUILD_DIR" --quiet --warnings-as-errors='*' -extra-arg=-Wno-unknown-warning-option -- $TARGET_FILES
fi
