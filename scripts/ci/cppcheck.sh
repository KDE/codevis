#!/bin/bash

REPORT_XML_DEFAULT="cppcheck.xml"
REPORT_XML="${REPORT_XML:-${REPORT_XML_DEFAULT}}"

REPORT_DIR_DEFAULT="cppcheck"
REPORT_DIR="${REPORT_DIR:-${REPORT_DIR_DEFAULT}}"

if [ -f "$REPORT_XML" ]; then
	rm "$REPORT_XML"
fi

cppcheck --enable=all \
	-i ./build \
	-i ./cmake \
	-i ./conan \
	-i ./doc \
	-i ./imgs \
	-i ./python \
	-i ./scripts \
	-i ./submodules \
	-i ./thirdparty \
	-i ./venv \
	-i ./windows \
	-i ./macos \
	-i ./lvtclp/systemtests \
	-i ./build_QT \
	-i ./cmake-build-debug \
	--force --error-exitcode=1 \
	--suppress=assertWithSideEffect \
	--suppress=knownConditionTrueFalse \
	--suppress=mismatchingContainerExpression \
	--suppress=missingInclude \
	--suppress=noConstructor \
	--suppress=stlIfStrFind \
	--suppress=unknownMacro \
	--suppress=unmatchedSuppression \
	--suppress=unusedFunction \
	--suppress=useStlAlgorithm \
	--suppress=unassignedVariable \
	--suppress=unusedStructMember \
	--suppress=selfAssignment \
	--suppress=syntaxError \
	--inline-suppr \
	-i /usr/include \
	-i ./thirdparty \
	-i ./submodules \
	-U DOXYGEN_ONLY \
	-U _MSC_VER \
	-U _MSVC_LANG \
	-U _WIN32 \
	-U WIN32 \
	.

err_code="$?"

exit "$err_code"
