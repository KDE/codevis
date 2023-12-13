FROM centos:7 AS base

ENV LLVM_VERSION=17.0.1 \
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

WORKDIR /codevis

# Openssl
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

# CMake
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

# LLVM
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

# QT
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
RUN cd /codevis \
    && wget https://download.qt.io/archive/qt/5.15/${QT_VERSION}/single/qt-everywhere-opensource-src-${QT_VERSION}.tar.xz \
    && tar -xf qt-everywhere-opensource-src-${QT_VERSION}.tar.xz \
    && rm qt-everywhere-opensource-src-${QT_VERSION}.tar.xz
RUN mkdir /codevis/build/qt5 \
    && cd /codevis/build/qt5 \
    && ../../qt-everywhere-src-${QT_VERSION}/configure -opensource -confirm-license -xcb -xcb-xlib -bundled-xcb-xinput \
        -nomake examples -nomake tests  --prefix=/usr
RUN cd /codevis/build/qt5 \
    && make -j$(nproc) \
    && make install \
    && rm -rf /codevis/build/qt5 \
    && rm -rf /codevis/qt-everywhere-opensource-src-${QT_VERSION}

# Catch2 (For testing only)
RUN git clone https://github.com/catchorg/Catch2.git \
    && cd Catch2 \
    && cmake -Bbuild -H. -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr \
    && cmake --build build/ --target install

RUN yum -y install python-devel

RUN rm -rf /codevis/

# KDE / KF5 libraries  (Old work, kept for reference)
#RUN wget https://www.cpan.org/src/5.0/perl-5.38.2.tar.gz
#RUN tar -xvf perl-5.38.2.tar.gz && cd perl-5.38.2
#RUN ./Configure -d -e
#RUN make && make install
#RUN mv /usr/local/bin/perl /usr/local/bin/perl.old
#RUN ln -s /usr/local/bin/perl5.38.2 /usr/local/bin/perl
#RUN mv /usr/bin/perl /usr/bin/perl.old
#RUN ln -s /usr/local/bin/perl5.38.2 /usr/bin/perl
#RUN mkdir /root/kde/src
#WORKDIR /root/kde/src/
#RUN git clone https://invent.kde.org/sdk/kdesrc-build.git
#WORKDIR /root/kde/src/kdesrc-build/
#RUN yum install -y dnf
#    NOTE: kdesrc-build fails due to not-found packages.
#          The solution was to avoid using kdesrc-build.
#RUN yes | ./kdesrc-build --initial-setup

# ECM
WORKDIR /
RUN git clone https://github.com/KDE/extra-cmake-modules.git
RUN cd /extra-cmake-modules/ \
    && mkdir -p build \
    && cd build/ \
    && cmake .. \
    && make \
    && make install

# KArchive
WORKDIR /
RUN git clone https://github.com/KDE/karchive.git
RUN cd /karchive/ \
    && git checkout kf5\
    && mkdir -p build \
    && cd build/ \
    && cmake .. \
    && make \
    && make install

WORKDIR /
ADD deploy.sh /
RUN chmod +x /deploy.sh
CMD ["/bin/bash", "/deploy.sh"]
