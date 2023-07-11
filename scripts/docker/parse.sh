#!/bin/bash
#
# Given a path to a codebase, this script creates a database in that folder,
# running CMake as needed to generate the compile commands. Primarily written
# for use in docker containers but can be used manually.

CODEBASE_PATH="${CODEBASE_PATH:-/codebase}"
BUILD_PATH="${BUILD_PATH:-$CODEBASE_PATH/build}"
PARSER_EXECUTABLE="${PARSER_EXECUTABLE:-create_codebase_db}"

sed -i.host "s/${CODEBASE_EXT_PATH//\//\\/}/${CODEBASE_PATH////\\/}/g" "${BUILD_PATH}/compile_commands.json"

cd "$CODEBASE_PATH" || { echo "invalid folder"; exit 1; }

# "$PARSER_EXECUTABLE" \
#    --update \
#    --silent \
#    -j "$(nproc)" \
#    --ignore "*.t.cpp" \
#    --ignore "*standalone*" \
#    "$CODEBASE_PATH/database.db" \
#    "$BUILD_PATH/compile_commands.json"

echo "Ignoring this temporarely untill Soci is the default."
