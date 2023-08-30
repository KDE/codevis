## Linux build

### Easy Build using kdesrc-build

We use `kdesrc-build` in order to speed up and automate the build process,
downloading the needed libraries, and building the ones we need in a multitude of linux distributions.
If you have never used `kdesrc-build` before, it's quite simple, and this is the startup point:

clone the repository, or install from your package manager the `kdesrc-build` tool, the repository can be found in `https://invent.kde.org/sdk/kdesrc-build/`

Run `kdesrc-build --initial-setup` to install the necessary packages for your linux distribution. Even if you already have `kdesrc-build`, you are encouraged to run `--initial-setup` often, to install newer dependencies that are added to the list from time to time, running this again will not override your configuration file.

Open and edit `~/.config/kdesrc-buildrc` to your liking, specially the `kdedir`, `source-dir` and `build-dir` packages.

run `kdesrc-build codevis`
This will:
    - Download the sources from the KDE infrastructure, on `source-dir`
    - Configure the sources for building
    - Build the sources into a compiled binary on `build-dir`
    - Install the compiled binary into `kdedir`

After you build the software for the first time with kdesrc-build, you can load the source on any IDE, and point the build folder to `$build-dir/codevis`, the IDE should pickup the `CMakeCache.txt` and the `compile_commands.json` and you can go from there.

### Building natively on Ubuntu 20.10

First install the dependencies:
```
apt-get update && apt-get install \
    build-essential \
    clang \
    clang-tools-13 \
    llvm-dev \
    cmake \
    git \
    lcov \
    libclang-13-dev \
    libqt5core5a \
    qtbase5-dev \
    qtwebengine5-dev \
    libsqlite3-dev \
    sqlite3 \
    catch2 \
    extra-cmake-modules
```

Build and test the project
```
cd $PROJECT_PATH
git submodule init
git submodule update --recursive
mkdir build
cd build
cmake ..
make -j$(nproc)
make test
```

The GUI executable is at `build/desktopapp/codevis_desktop`.

### Building using the docker images

To build inside of docker use

```
export DOCKER_BUILDKIT=1
cd $PROJECT_PATH
docker build --target build-qt -t ctlvt/build-qt .
```

The build is located in `$PROJECT_PATH/build`.

Unless your host machine is running very similar library versions to the docker
container, the built binaries will not be very useful: this is mostly good for
CI. Use appimage to create re-distributable binaries.

To get files out of the docker image we need to start a container then copy files
out of it

In one terminal emulator:
```
docker run -it --name "lvtbuild" ctlvt/build-qt
```

In another:
```
docker cp lvtbuild:$PROJECT_PATH/build path/to/destination
```

### Appimage

There is a handy docker image to build an appimage package. Build it using
```
export DOCKER_BUILDKIT=1
docker pull debian:buster
cd $PROJECT_PATH
docker build --target appimage -t ctlvt/appimage .
```

To output the appimage to the directory `/foo`, run
```
docker run -v /foo:/tmp/lvt-appimage ctlvt/appimage
```

It is very important to build the appimage using the docker container and not by
running the appimage build script nativly because appimage does not bundle glibc.
glibc is forwards but not backwards compatible so the appimage has to be built
to the oldest libc practically available.

The appimage is the most useful way to distribute Linux executables.

### Centos 7

There's a Docker script that builds the tool natively on Centos 7, The script is
slow because it needs to compile llvm, qt and cmake beforehand.
Invoke the script from the toplevel source directory:

```
$ docker build -f packaging/centos7/Dockerfile . --progress=plain
```

Invoking it from the toplevel directory is needed so that Docker can access
the source files of this project while building it.

# Code coverage

```
cmake -DENABLE_CODE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j$(nproc)
ctest . -j$(nproc)
make coverage
```
