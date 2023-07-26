FROM centos:7 AS base

ENV LLVM_VERSION=15.0.6 \
    CMAKE_VERSION=3.25.2 \
    OPENSSL_VERSION=1.1.1k \
    QT_VERSION=5.15.7

RUN yum install -y centos-release-scl \
    && yum install -y devtoolset-9

RUN echo "source /opt/rh/devtoolset-9/enable" >> /etc/bashrc
SHELL ["/bin/bash", "--login", "-c"]

RUN yum -y install epel-release \
    && yum -y makecache \
    && yum -y update && yum -y install wget gnupg git wheel python3-pip bash ninja zlib-devel

FROM base AS compile_openssl

WORKDIR /codevis

# CMake openssl 7 is too old, we need to compile/install our own, so cmake can work.
RUN echo "Downloading OpenSSL" \
    && wget https://ftp.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz \
    && tar -xf openssl-${OPENSSL_VERSION}.tar.gz \
    && rm openssl-${OPENSSL_VERSION}.tar.gz \
    && cd openssl-${OPENSSL_VERSION} \
    && ./config --prefix=/usr --openssldir=/etc/ssl --libdir=lib no-shared zlib-dynamic \
    && make -j$(nproc) \
    && make install \
    && cd /codevis \
    && rm -rf /codevis/openssl-${OPENSSL_VERSION}

FROM compile_openssl AS compile_cmake

# CMake on CentOS 7 is too old to build our software, their version is 2.8 and llvm requires 3.13. currently CMake is 3.25
# so, let's go latest. This is needed to build llvm.
RUN echo "Downloading CMake" \
    && wget "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz" \
    && tar -xf cmake-${CMAKE_VERSION}.tar.gz \
    && rm cmake-${CMAKE_VERSION}.tar.gz \
    && cd cmake-${CMAKE_VERSION} \
    && ./bootstrap --prefix=/usr/ \
    && make -j$(nproc) \
    && make install \
    && cd /codevis \
    && rm -rf /codevis/cmake-${CMAKE_VERSION}

# llvm is also too old, so we need to compile it as well.
FROM compile_cmake AS compile_llvm
RUN echo "Downloading LLVM" \
    && wget "https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/llvm-project-${LLVM_VERSION}.src.tar.xz" \
    && tar -xf llvm-project-${LLVM_VERSION}.src.tar.xz \
    && rm llvm-project-${LLVM_VERSION}.src.tar.xz \
    && mkdir build \
    && mkdir build/llvm-project

RUN cd build/llvm-project \
    && cmake -DBUILD_SHARED_LIBS=Off \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=/usr \
            -DLIBOMP_INSTALL_ALIASES=OFF \
            -DLLDB_ENABLE_LUA=OFF \
            -DLLDB_ENABLE_LZMA=ON \
            -DLLDB_ENABLE_PYTHON=OFF \
            -DLLDB_INCLUDE_TESTS=OFF \
            -DLLDB_USE_SYSTEM_DEBUGSERVER=OFF \
            -DLLVM_BUILD_EXTERNAL_COMPILER_RT=OFF \
            -DLLVM_BUILD_LLVM_C_DYLIB=OFF \
            -DLLVM_ENABLE_DIA_SDK=1 \
            -DLLVM_ENABLE_EH=ON \
            -DLLVM_ENABLE_FFI=OFF \
            -DLLVM_ENABLE_LIBCXX=ON \
            -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" \
            -DLLVM_ENABLE_RTTI=ON \
            -DLLVM_ENABLE_RUNTIMES="compiler-rt" \
            -DLLVM_ENABLE_Z3_SOLVER=OFF \
            -DLLVM_INCLUDE_DOCS=OFF \
            -DLLVM_INCLUDE_TESTS=OFF \
            -DLLVM_INSTALL_UTILS=ON \
            -DLLVM_LINK_LLVM_DYLIB=ON \
            -DLLVM_OPTIMIZED_TABLEGEN=ON \
            -DLLVM_POLLY_LINK_INTO_TOOLS=ON \
            -DLLVM_TARGETS_TO_BUILD="X86" \
            -B /codevis/build/llvm-${LLVM_VERSION} \
            -S /codevis/llvm-project-${LLVM_VERSION}.src/llvm

RUN cmake --build /codevis/build/llvm-${LLVM_VERSION} --parallel $(nproc) --config Release \
    && cmake --install /codevis/build/llvm-${LLVM_VERSION} \
    && rm -rf /codevis/build/llvm-${LLVM_VERSION} \
    && rm -rf /codevis/llvm-project-${LLVM_VERSION}.src

FROM compile_llvm AS compile_qt

# Install qt dependencies dependencies that are not needed for cmake, llvm or openssl.
RUN yum -y install                 \
    libXScrnSaver-devel            \
    libXaw-devel                   \
    libXcomposite-devel            \
    libXcursor-devel               \
    libXdmcp-devel                 \
    libXinerama-devel              \
    libXrandr-devel                \
    libXres-devel                  \
    libXtst-devel                  \
    libXv-devel                    \
    libXvMC-devel                  \
    libfontenc-devel               \
    libglvnd-devel                 \
    libuuid-devel                  \
    libxcb                         \
    libxcb-devel                   \
    libxkbcommon-devel             \
    libxkbcommon-x11-devel         \
    libxkbfile-devel               \
    mesa-libGL-devel               \
    sqlite-devel                   \
    xcb-util                       \
    xcb-util-devel                 \
    xcb-util-image-devel           \
    xcb-util-keysyms-devel         \
    xcb-util-renderutil-devel      \
    xcb-util-wm-devel              \
    xkeyboard-config-devel

# get the code and extract
RUN cd /codevis \
    && wget https://download.qt.io/archive/qt/5.15/${QT_VERSION}/single/qt-everywhere-opensource-src-${QT_VERSION}.tar.xz \
    && tar -xf qt-everywhere-opensource-src-${QT_VERSION}.tar.xz \
    && rm qt-everywhere-opensource-src-${QT_VERSION}.tar.xz

# out-of-source build
RUN mkdir /codevis/build/qt5 \
    && cd /codevis/build/qt5 \
    && ../../qt-everywhere-src-${QT_VERSION}/configure -opensource -confirm-license -xcb -xcb-xlib -bundled-xcb-xinput \
        -nomake examples -nomake tests  --prefix=/usr

RUN cd /codevis/build/qt5 \
    && make -j$(nproc) \
    && make install \
    && rm -rf /codevis/build/qt5 \
    && rm -rf /codevis/qt-everywhere-opensource-src-${QT_VERSION}

FROM compile_qt AS compile_catch2

RUN git clone https://github.com/catchorg/Catch2.git \
    && cd Catch2 \
    && cmake -Bbuild -H. -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr \
    && cmake --build build/ --target install

FROM compile_catch2 AS dependencies

FROM dependencies AS compile_diagram_server

COPY . /codevis/diagram-server

RUN yum -y install python-devel

RUN mkdir /codevis/build/diagram-server \
    && export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/lib \
    && cd /codevis/diagram-server \
    && git submodule init \
    && git submodule update --recursive \
    && cd /codevis \
    && cmake -DCMAKE_BUILD_TYPE=Release \
        -DCOMPILE_TESTS=OFF \
        -DBUILD_DESKTOP_APP=OFF \
        -B /codevis/build/diagram-server \
        -S /codevis/diagram-server \
    && cmake --build /codevis/build/diagram-server --parallel $(nproc) --config Release

FROM compile_diagram_server AS create_centos_package

WORKDIR /codevis/diagram-server
ENV OUTDIR=/tmp/package
ENV EXTERNAL_SO_DEPENDENCIES=/tmp/externaldeps
RUN yum -y install python-jinja2 \
    && mkdir -p "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/codevis/build/diagram-server/thirdparty/libquazip.so" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/libQt5Core.so.5" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/lib/libLLVM-15.so" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/lib/libclang-cpp.so.15" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/lib64/libsqlite3.so.0" "${EXTERNAL_SO_DEPENDENCIES}" \
    && mkdir -p "${OUTDIR}"
CMD scripts/packaging/build-cli-package.sh --buildfolder "/codevis/build/diagram-server" --outdir "${OUTDIR}" --externaldepsfolder "${EXTERNAL_SO_DEPENDENCIES}"
