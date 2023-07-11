## Mac build

Run the script on `$PROJECT_DIR/packaging/macos/llvm-build.sh` specifying
the root folder, the build folder, the install folder and the number of cores:

```
./$PROJECT_DIR/macos/llvm-build.sh ~/Projects ~/Projects/build ~/Projects/install 4
```

This will download and install all tools and dependencies, and trigger a build of the
tool. The final executable is bundled inside of `~/Projects/build/desktopapp/`.

If the automated script does not work, the following instructions might apply:

First install brew by following instructions at https://brew.sh

Brew will install XCode Command Line Tools, which includes git etc. **But you
should install full XCode from the app store and accept the license agreement
because it is needed to build Qt via conan**.

Now use brew to install everything we need:
```
brew install conan cmake llvm bison
```

We need to find where llvm/clang are installed:
```
find /usr -name ClangConfig.cmake
```

The path should look something like this:
```
/usr/local/Cellar/llvm/<version>/lib/cmake/clang
```

We can now use this path with cmake (second to last line):

```
cd $PROJECT_DIR
git submodule update
mkdir build
cd build
conan install --build=missing .. -pr ../conan/mac_11_profile
Clang_DIR=/usr/local/Cellar/llvm/<version>/lib/cmake/clang cmake ..
make
```

Run by clicking on `codevis.app` from Finder.
