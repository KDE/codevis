## Windows build

Necessary tools:

* CMake
* Git for Windows
* MSVC 2019 (untested in other versions)
    - For MSVC 2019 be aware that you just need the latest windows SDK and the C++ Compiler. There's no need to install everything.
* Conan 1.40+
* Qt 5.15.2 MSVC 2019 Build
* Powershell
* `awk` and `winflexbison3` installed via `choco` (see https://chocolatey.org/)

Please make sure that cmake and conan are acessible via %PATH%.

There's a script that takes care of downloading and building the executable in `%PROJECT_DIR%/packaging/windows/llvm-build.bat`.
It takes four parameters:
- Root folder for the script (for things that the script needs to download, such as llvm)
- Build folder (where we build the packages that are not supported yet by conan)
- Install folder (where we install those packages that are not supported by conan)
- Thread Count (number of parallel builds, not supported in every version of visual studio)

Example:

```
cd %PROJECT_DIR%/packaging/windows/
llvm-build.bat C:\Project C:\Project\Build C:\Project\Install 4
```

Note that it is expected that the first will take a long time as it will download and compile the dependencies.

The script will output the executable and dependencies on `C:\Project\Build\desktopapp\Release`

For more in-depth explanations on how the script works, refer to [the CI docs for Windows](ci_windows.md).
