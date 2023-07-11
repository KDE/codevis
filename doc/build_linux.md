## Linux build

There are three ways of building the Linux application:
1. [Building natively on your machine](#a)
2. Building using docker
3. Building an appimage (recommended for distribution)

### Building natively on Ubuntu 20.10

(For up to date instructions, see the Dockerfiles in the `packaging/` folder)

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
    libboost1.74-all-dev \
    libclang-13-dev \
    libqt5core5a \
    qtbase5-dev \
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

The GUI executable is at `build/desktopapp/codevis`.

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
