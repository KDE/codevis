# Windows

The installation and instructions for the gitlab-runners on windows are a bit
sparse, this is a documentation on how to build Codevis on windows, via
gitlab-runner.

* Steps 1 - Prerequisites:
  install `conan`, `git`, `cmake`, `visual studio 2019 community edition` and `powershell`
  on the visual studio 2019 edition installer, also install the `windows 10 sdk`

Make sure you install everything and setup the PATHS to be accessible to every
user, not just your own.

* Step 2 - Build Prerequisites:
  Run the windows build script, in the windows/ folder with the following parameters

`build-llvm.bat c:\Project c:\Project\build c:\Project\install 4`

Running this script will create two new folders on the top level `C:\` drive:
* `c:\conan-data` - this folder stores the conan artifacts
* `c:\project` - this folder stores the binaries, and the build artifacts.

llvm will be installed in `c:\project\install`, we need that to get the library
paths.

this will download and extract `llvm`, compile, install, and setup the build.
if all the necessary software is installed, then the build should proceed without
errors.
Every time you run the script, the build folder for Codevis will be deleted
and a new build will happen, this is important to make sure we are not compiling on
top of an old, or cached build.

LLVM and the conan libraries are not affected on this, as that would take too long.

This step is necessary as conan fails to run inside of gitlab-runner for %some_reason%:

```
Error executing C:\Program Files (x86)\Microsoft Visual Studio\2019\Contents\bin\MSBuild.exe

Error 1
```

Since life is too short to understand what `Error 1` means, we are building the
software in two steps:

1 - Pre-build libraries and dependencies: outside of CI
2 - Build the software: inside of the CI

* Step 3 - Configure gitlab-runner:

download gitlab runner from `https://gitlab-runner-downloads.s3.amazonaws.com/latest/binaries/gitlab-runner-windows-amd64.exe`

create a folder under `C:\GitlabRunner`, and move it there.
Rename the binary to `gitlab-runner.exe`, the long name can create some issues on windows as the amount
of data the cmd can handle is not big.

open an `elevated priviledges command prompt` (right click on the command prompt, and go to `Run as Administrator`
go to `c:\GitlabRunner`, and run:
- `gitlab-runner.exe install`
- `gitlab-runner.exe start`
- `gitlab-runner.exe register`

the Register step needs manual intervention:
- go to the gitlab instance, and look for the CI configuration, for the address and key. and select `Shell` as executor.

- `gitlab-runner.exe stop`

now open `config.toml` that was created on the `C:\GitlabRunner` folder, and change the executor from `pwsh` to `powershell`, without that the CI will fail silently.

- `gitlab-runner.exe restart`

Note: the name of the instance on gitlab needs to be `windows-desktop`, so remember to put this as tag.

* Step 4: Verify that the runner is indeed running, on the gitlab CI information pane
* Step 5: Trigger a CI run by submitting code.
