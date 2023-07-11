REM Load the x64 variables from msvc
if not defined LOADED_VC_VARS_INTERNAL (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    set LOADED_VC_VARS_INTERNAL=1
)

SET CONAN_USER_HOME=c:\conan-data

echo "Current Path"
echo %PATH%

@echo off

REM There are two ways of comenting on bat files, by REM and by ::.
REM Don't use :: because windows sometimes thinks that this is a Drive, like C: , but `:`:.

REM There's no simple way to fetch the number of arguments on a bash script,
REM we need to iterate over the parameters and count them. Eesh.
set argC=0
for %%x in (%*) do Set /A argC+=1
if not %argC% == 4 (
    echo "Incorrect number of arguments"
    echo "Please specify all of the following arguments, in order"
    echo "SourceFolder: Where the sources are."
    echo "BuildFolder: Where we will build the tool."
    echo "InstallFolder: Where the binaries will go after instalation"
    echo "ProcessorCount: The number of threads to build the project"
    echo ""
    echo "Example: llvm-build.bat c:\Project c:\Project\Build c:\Project\Install 4"
    exit /b 1
)

set SOURCE_FOLDER=%1
set BUILD_FOLDER=%2
set INSTALL_FOLDER=%3
set PROCESSOR_COUNT=%4
set LLVM_VERSION=14.0.4

echo %SOURCE_FOLDER%
echo %BUILD_FOLDER%
echo %INSTALL_FOLDER%
echo %PROCESSOR_COUNT%

REM Verify that the visual studio version we have is the
REM Correct version.
echo Step 1 - Locating Visual Studio
if not defined VisualStudioVersion (
    echo "Visual Studio not installed."
    exit /b 1
) else (
    if "%VisualStudioVersion%" NEQ "16.0" (
        echo "Incompatiblhe visual studio version, please install visual studio 2019."
        exit /b 1
    )
)

echo Step 2 - Verify Architecture
REM Verify that we are on the correct architecture of the command line tools.
if "%PROCESSOR_ARCHITECTURE%"=="x86" (
    if "%PROCESSOR_ARCHITEW6432%"=="AMD64" (
        echo Detected 64 bit system running 32 bit shell, please use the 64bits shell.
        exit /b 1
    ) else (
        echo The build system needs a 64 bit processor, aborting.
        exit /b 1
    )
) else (
    echo 64 bits command line tools used.
)

echo Step 3 - Look for Needed Software
REM Check for the needed software on the PATH.
for %%F in (cmake git) do (
    where %%F > nul

    REM errorlevel <number>	Specifies a true condition only if the previous
    REM program run by Cmd.exe returned an exit code equal to or greater than number.
    if %errorLevel% NEQ 0 (
        echo "Please install %%F, errorLevel = %errorLevel%"
        exit /b 1
    )
)

for %%F in (%SOURCE_FOLDER%, %BUILD_FOLDER%, %INSTALL_FOLDER%) do (
    if not exist %%F (
        echo Folder %%F does not exist, creating.
        mkdir %%F
        if errorLevel 1 (
            echo Error creating folder %%F
            exit /b 1
        )
    ) else (
        echo Folder %%F already exists.
    )
)

echo Step 4 - Download and Extract LLVM Source
REM Windows is *scarely slow*. uncompressing llvm using powershell is taking
REM more than 40 minutes on my NVMe.
if not exist %SOURCE_FOLDER%\llvm-project-llvmorg-%LLVM_VERSION% (

    REM Only try to download the package if the folder of the source doesn't exist.
    REM because gitlab keeps removing / redownloading it. =/
    if not exist llvmorg-%LLVM_VERSION%.zip (
        echo "Downloading llvmorg-%LLVM_VERSION%.zip"
        powershell -Command "Invoke-WebRequest https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-%LLVM_VERSION%.zip -OutFile llvmorg-%LLVM_VERSION%.zip"

        REM bat has no inequality comparissons.
        if errorLevel 1 (
            echo "Error downloading llvm"
            exit /b 1
        )
    ) else (
        echo "Latest version of llvmorg-%LLVM_VERSION%.zip exists"
    )

    powershell Expand-Archive  llvmorg-%LLVM_VERSION%.zip -DestinationPath %SOURCE_FOLDER% -Force
    if errorLevel 1 (
        echo "Error extracting LLVM"
        echo "Trying to remove the source folder"

        REM \S \Q is "delete everything without asking."
        REM We need to delete because there was an error extracting the zip, but we might
        REM have created the folder.
        REM we need to start from scratch.
        rmdir %SOURCE_FOLDER%\llvm-project-llvmorg-%LLVM_VERSION% \S \Q

        exit /b 1
    )
    echo "Finished Extracting LLVM"
) else (
    echo "Uncompressed folder for llvm already exists, no need to uncompress"
)

REM Okay, here we should have the uncompressed folder for llvm, cmake should be already installed.
REM So we can try to continue.

echo Step 5 - Create LLVM Build Folder

if not exist %BUILD_FOLDER%\llvm-%LLVM_VERSION% (
    mkdir %BUILD_FOLDER%\llvm-%LLVM_VERSION%
    if errorLevel 1 (
        echo Error creating folder %%F
        exit /b 1
    )
)

echo Step 6 - Figure out MSVC Directory

REM visual studio uses a specific application to tell you the paths, called vswhere.
set vswhere_path="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

REM This scary line just runs the command on the `IN` parenthesis, and saves the result in the `SET` variable.
FOR /F "tokens=* USEBACKQ" %%g IN (`%vswhere_path% -version 16^ -property installationPath`) do (SET "vs_path=%%g")

echo Step 7 - Configure LLVM with CMake

REM Configure LLVM if CMakeCache.txt doesn't exists.
REM The configuration for VS Studio takes too long, we can't
REM afford to run this all the time. If we need to reconfigure
REM just nuke the CMakeCache.txt and that will force a reconfiguration.
if not exist %BUILD_FOLDER%\llvm-%LLVM_VERSION%\CMakeCache.txt (
    echo "Configuring LLVM for build"

    cmake -G"Visual Studio 16 2019" ^
            -DBUILD_SHARED_LIBS=Off ^
            -DCMAKE_BUILD_TYPE=Release ^
            -DCMAKE_INSTALL_PREFIX=%INSTALL_FOLDER%\llvm-%LLVM_VERSION% ^
            -DLIBOMP_INSTALL_ALIASES=OFF ^
            -DLLDB_ENABLE_LUA=OFF ^
            -DLLDB_ENABLE_LZMA=ON ^
            -DLLDB_ENABLE_PYTHON=OFF ^
            -DLLDB_INCLUDE_TESTS=OFF ^
            -DLLDB_USE_SYSTEM_DEBUGSERVER=ON ^
            -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON ^
            -DLLVM_BUILD_LLVM_C_DYLIB=ON ^
            -DLLVM_ENABLE_DIA_SDK=1 ^
            -DLLVM_ENABLE_EH=ON ^
            -DLLVM_ENABLE_FFI=OFF ^
            -DLLVM_ENABLE_LIBCXX=ON ^
            -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;mlir;polly" ^
            -DLLVM_ENABLE_RTTI=ON ^
            -DLLVM_ENABLE_RUNTIMES="compiler-rt" ^
            -DLLVM_ENABLE_Z3_SOLVER=OFF ^
            -DLLVM_INCLUDE_DOCS=OFF ^
            -DLLVM_INCLUDE_TESTS=OFF ^
            -DLLVM_INSTALL_UTILS=ON ^
            -DLLVM_LINK_LLVM_DYLIB=ON ^
            -DLLVM_OPTIMIZED_TABLEGEN=ON ^
            -DLLVM_POLLY_LINK_INTO_TOOLS=ON ^
            -DLLVM_TARGETS_TO_BUILD="X86" ^
            -Thost=x64 ^
            -B %BUILD_FOLDER%\llvm-%LLVM_VERSION% ^
            -S %SOURCE_FOLDER%\llvm-project-llvmorg-%LLVM_VERSION%\llvm

    if errorLevel 1 (
        echo Error configuring LLVM

        REM /S means Recursive
        REM /Q means Quiet, no questions asked.
        REM rmdir  %BUILD_FOLDER%\llvm /S /Q
        exit /b 1
    )
    echo LLVM Configured correctly
) else (
    echo LLVM Already Configured, Going to the Build Phase.
)

echo Step 8 - Build LLVM
if not exist  %INSTALL_FOLDER%\llvm-%LLVM_VERSION% (
    echo "Building LLVM"
    cmake --build %BUILD_FOLDER%\llvm-%LLVM_VERSION% --parallel %PROCESSOR_COUNT% --config Release
    if errorLevel 1 (
        echo Error Building LLVM
        exit /b 1
    )

    echo "Installing LLVM"
    cmake --install %BUILD_FOLDER%\llvm-%LLVM_VERSION%
    if errorLevel 1 (
        echo Error Installing LLVM
        exit /b 1
    )
    echo LLVM Build and Installed Correctly
)

echo Step 9 -  Build Diagram Server

if exist %BUILD_FOLDER%\diagram-server (
    rmdir %BUILD_FOLDER%\diagram-server /S /Q
    if errorLevel 1 (
        echo Error nuking the diagram-server build folder.
        exit /b 1
    )
)

mkdir %BUILD_FOLDER%\diagram-server
if errorLevel 1 (
    echo Error creating diagram-server build folder.
    exit /b 1
)

REM discover the name of the script directory
pushd %~dp0
set script_dir=%CD%
popd

REM Install conan dependencies.
pushd %BUILD_FOLDER%\diagram-server
conan install %script_dir%\..\.. --build=missing
if errorLevel 1 (
    echo Error running conan to install the dependencies.
    exit /b 1
)
popd

REM Configure and build the diagram-server binary.
set Clang_DIR=%INSTALL_FOLDER%\llvm-%LLVM_VERSION%\lib\cmake\clang

echo %Clang_DIR%

cmake -G"Visual Studio 16 2019" ^
    -DCMAKE_PROGRAM_PATH=C:\ProgramData\chocolatey\bin ^
    -DUSE_QT_WEBENGINE=OFF ^
    -B %BUILD_FOLDER%\diagram-server ^
    -S %script_dir%\..\..

if errorLevel 1 (
    echo Error configuring CMake.
    exit /b 1
)

cmake --build %BUILD_FOLDER%\diagram-server --config Release --parallel %PROCESSOR_COUNT%
if errorLevel 1 (
    echo Error building diagram-server
    exit /b 1
)

xcopy "C:\Windows\System32\msvcp140.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\msvcp140_1.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\msvcp140_2.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\msvcp140_atomic_wait.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\msvcp140_clr0400.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\msvcp140_codecvt_ids.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\vcamp140.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\vccorlib140.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\vcomp140.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\vcruntime140.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\vcruntime140_1.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

xcopy "C:\Windows\System32\vcruntime140_clr0400.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release"
if errorLevel 1 (
    exit /b 1
)

REM /E – Copy subdirectories, including any empty ones.
REM /H - Copy files with hidden and system file attributes
REM /C - Continue copying even if an error occurs.
REM /I - If in doubt, always assume the destination is a folder. e.g. when the destination does not exist.
xcopy "%INSTALL_FOLDER%\llvm-%LLVM_VERSION%\lib\clang\%LLVM_VERSION%\include" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\clang" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)

REM Copy python scripts necessary for code generation module
echo "Copying python scripts for code generation... (from %SOURCE_FOLDER%diagram-server\python\ to Release\ folder)
dir %$CI_PROJECT_DIR%
xcopy "%$CI_PROJECT_DIR%python\" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\python\" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)

powershell -Command "Invoke-WebRequest https://drive.codethink.co.uk/s/D3nWEfWFw8Gi9SP/download/python-win-deps.zip -OutFile python-win-deps.zip"
powershell Expand-Archive python-win-deps.zip -DestinationPath . -Force
xcopy ".\python310.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\"
if errorLevel 1 (
    exit /b 1
)
xcopy ".\python3.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\"
if errorLevel 1 (
    exit /b 1
)
xcopy ".\python.exe" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\"
if errorLevel 1 (
    exit /b 1
)
xcopy ".\pythonw.exe" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\"
if errorLevel 1 (
    exit /b 1
)
xcopy ".\Lib\" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\Lib\" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)
xcopy ".\python\site-packages" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\python\site-packages" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)

REM Qt specific configurations e.g. DPI awareness bugfix
xcopy ".\desktopapp\qt.conf" "%BUILD_FOLDER%\diagram-server\desktopapp\Release" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)

REM Database related files
xcopy ".\database-spec" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\codevis\database-spec" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)

REM Default plugins
REM ===============
REM Cycle Detection Plugin
xcopy "%BUILD_FOLDER%\diagram-server\plugins\cycle_detection_plugin\Release\cycle-detection-plugin.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\codevis\lks-plugins\cycle-detection-plugin\" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)
xcopy ".\plugins\cycle_detection_plugin\README.md" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\codevis\lks-plugins\cycle-detection-plugin\" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)

REM Code Coverage Plugin
xcopy "%BUILD_FOLDER%\diagram-server\plugins\code_coverage_plugin\Release\code-cov-plugin.dll" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\codevis\lks-plugins\code-cov-plugin\" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)
xcopy ".\plugins\code_coverage_plugin\README.md" "%BUILD_FOLDER%\diagram-server\desktopapp\Release\codevis\lks-plugins\code-cov-plugin\" /E /H /C /I
if errorLevel 1 (
    exit /b 1
)
