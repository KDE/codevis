#!/bin/sh

EXEC=clang-format

echo "\033[0;33mFetching information from master...\033[0m"
git fetch origin master --quiet

echo "\033[0;33mRunning clang-format diff in relation to origin/master...\033[0m"
export OUT="$(git clang-format origin/master $*)"

echo "\033[1;34m$OUT\033[0m"
bash -c '[[ "$OUT" = "" ]] || [[ "$OUT" = *"no modified files to format"* ]] || [[ "$OUT" == *"clang-format did not modify any files"* ]]'
