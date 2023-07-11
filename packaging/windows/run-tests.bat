set SOURCE_PATH=%1
set BUILD_PATH=%2
set PATH=%PATH%;^
%BUILD_PATH%/lvtcgn/Release;^
%BUILD_PATH%/lvtclp/Release;^
%BUILD_PATH%/lvtclr/Release;^
%BUILD_PATH%/lvtldr/Release;^
%BUILD_PATH%/lvtmdb/Release;^
%BUILD_PATH%/lvtmdl/Release;^
%BUILD_PATH%/lvtprj/Release;^
%BUILD_PATH%/lvtqtc/Release;^
%BUILD_PATH%/lvtqtd/Release;^
%BUILD_PATH%/lvtqtw/Release;^
%BUILD_PATH%/lvtshr/Release;^
%BUILD_PATH%/lvttst/Release;^
%BUILD_PATH%/lvtplg/Release;^
%BUILD_PATH%/thirdparty/soci/Release;^
%BUILD_PATH%/Release;^
%BUILD_PATH%/desktopapp/Release

set QT_PLUGIN_PATH=%BUILD_PATH%\desktopapp\Release
set PYTHONPATH=%PYTHONPATH%;%BUILD_PATH%/desktopapp/Release/Lib/
set LAKOSDIAGRAM_PYSCRIPTS_PATH=%SOURCE_PATH%/python/codegeneration/
set DBSPEC_PATH=%SOURCE_PATH%/database-spec/
pushd %BUILD_PATH%
REM TODO [#478]: Fix lvtclp tests on Windows
ctest --output-on-failure -C Release -E "(lvtclp|test_ct_lvtldr_lakosiannode)"
if errorLevel 1 (
	popd
    exit /b 1
)
popd
