#!/bin/sh

TEST_DIR="lvtclp"
TEST_PREFIX="test_ct_lvtclp_"
TESTS="
field
function
namespace
relationships
sourcefile
udt
var
"

for name in $TESTS; do
    path="$TEST_DIR/${TEST_PREFIX}$name"
    if ! "$path"; then
        echo "TEST FAILED: $path" >&2
        exit 1
    fi
done
