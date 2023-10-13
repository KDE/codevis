FROM ubuntu:22.04 AS base

# it is an arg so it doesn't persist
ARG DEBIAN_FRONTEND=noninteractive

# Before installing the "real" dependencies we need new apt keys
# Note: wget is also used later in build-package.sh
RUN apt-get update && apt-get -qq install wget gnupg
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

# standardise build so everything uses llvm-14
RUN echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-14 main" >> /etc/apt/sources.list.d/llvm14.list

RUN apt-get update && apt-get -qq install \
    bison \
    build-essential \
    catch2 \
    clang-14 \
    clang-format \
    clang-tidy-14 \
    clang-tools-14 \
    cmake \
    cppcheck \
    flex \
    git \
    lcov \
    libclang-14-dev \
    libqt5core5a \
    libsqlite3-dev \
    llvm-14-dev \
    moreutils \
    python3-dev \
    qtbase5-dev \
    qtwebengine5-dev \
    libqt5svg5-dev \
    shellcheck \
    sqlite3

FROM base as dependencies

FROM dependencies as build-pre

ARG INSTALL_PREFIX="/usr/local/diagram-server"
COPY . /root/diagram-server
WORKDIR /root/diagram-server
RUN git submodule init
RUN git submodule update --recursive
WORKDIR /root/diagram-server/build
RUN cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" ..

FROM build-pre as pre-analysis

WORKDIR /root/diagram-server
RUN shellcheck scripts/**/*.sh lvtclp/run_tests.sh
RUN ./scripts/clang-format.sh --diff

FROM build-pre as build-qt

WORKDIR /root/diagram-server/build
RUN make -j`nproc` && make install

FROM build-qt as post-analysis

RUN apt-get update && apt-get -qq install \
    clazy

WORKDIR /root/diagram-server

ENV REPORT_DIR=cppcheck-report
ENV ARCHIVE=cppcheck-report.tar.gz
RUN ./scripts/ci/cppcheck.sh
RUN ./scripts/clang-tidy.sh lvt* desktopapp
RUN ./scripts/clazy.sh lvt* desktopapp

FROM build-qt as tests

RUN apt-get update && apt-get -qq install \
    python3-jinja2

WORKDIR /root/diagram-server/build

# Move the database schema files to XDG_DATA_DIRS
RUN mkdir -p /root/.local/share/codevis/database-spec/ \
    && cp /root/diagram-server/database-spec/codebase_db.sql /root/.local/share/codevis/database-spec/ \
    && cp /root/diagram-server/database-spec/cad_db.sql /root/.local/share/codevis/database-spec/

ENV LAKOSDIAGRAM_PYSCRIPTS_PATH="/root/diagram-server/python/codegeneration/"
ENV TEST_PRJ_PATH="/root/diagram-server/lvtclp/systemtests/"
ENV QT_QPA_PLATFORM="offscreen"
RUN ctest -VV

# Appimage does not include glibc in the bundled app. The app will use the host
# system's glibc. glibc is generally backwards but not forwards compatible so we
# should link against the oldest glibc we can.
FROM debian:buster as appimage

# it is an arg so it doesn't persist
ARG DEBIAN_FRONTEND=noninteractive

# Before installing the "real" dependencies we need new apt keys
# Note: wget is also used later in build-package.sh
RUN apt-get update && apt-get -qq install wget gnupg
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

# lvtclp depends upon modern clang libraries
RUN echo "deb http://apt.llvm.org/buster/ llvm-toolchain-buster-14 main" >> /etc/apt/sources.list.d/llvm14.list

RUN apt-get update && apt-get -qq install \
    bison \
    # Note: Catch2 is missing because there is no such package available at this point.
    #       compilation must be configured with no tests.
    clang-14 \
    clang-tools-14 \
    clang-format \
    clang-tidy-14 \
    cmake \
    cppcheck \
    flex \
    git \
    libclang-14-dev \
    libqt5core5a \
    libsqlite3-dev \
    llvm-14-dev \
    python-jinja2 \
    qt5-default \
    qt5-qmake \
    qtbase5-dev \
    qtwebengine5-dev \
    libqt5svg5-dev \
    shellcheck \
    sqlite3

# we installed new clang for the clang c++ parser libraries, as we have it, we
# may as well build with a modern compiler
ENV CXX=/usr/bin/clang++-14
ENV CC=/usr/bin/clang-14

COPY . /root/diagram-server

# FUSE doesn't work inside docker. This works around that so appimages can still run
ENV APPIMAGE_EXTRACT_AND_RUN=1

WORKDIR /root/diagram-server
RUN git submodule init
RUN git submodule update --recursive

ENV OUTDIR=/tmp/lvt-appimage
RUN mkdir -p "${OUTDIR}"
CMD scripts/packaging/build-package.sh --outdir "${OUTDIR}"

FROM appimage as deploy-cli-tools

WORKDIR /root/diagram-server
ENV OUTDIR=/tmp/package
ENV EXTERNAL_SO_DEPENDENCIES=/tmp/externaldeps
RUN scripts/packaging/build-package.sh --no-deploy
RUN mkdir -p "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/x86_64-linux-gnu/libQt5Core.so.5" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/llvm-14/lib/libclang-cpp.so.14" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/llvm-14/lib/libLLVM-14.so.1" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/x86_64-linux-gnu/libsqlite3.so.0" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/x86_64-linux-gnu/libicui18n.so.63" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/x86_64-linux-gnu/libicuuc.so.63" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/x86_64-linux-gnu/libpcre2-16.so.0" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/x86_64-linux-gnu/libffi.so.6" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/x86_64-linux-gnu/libxml2.so.2" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/x86_64-linux-gnu/libicudata.so.63" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/lib/x86_64-linux-gnu/libpcre.so.3" "${EXTERNAL_SO_DEPENDENCIES}" \
    && cp "/usr/lib/x86_64-linux-gnu/libdouble-conversion.so.1" "${EXTERNAL_SO_DEPENDENCIES}" \
    && mkdir -p "${OUTDIR}"
CMD scripts/packaging/build-cli-package.sh --outdir "${OUTDIR}" --buildfolder "/root/diagram-server/build/lvt/" --externaldepsfolder "${EXTERNAL_SO_DEPENDENCIES}"

FROM build-qt as generate-diagrams

RUN apt-get update && apt-get -qq install python3-jinja2

WORKDIR /root/
ENV OUTDIR=/tmp/results
ENV BUILD_PATH=/root/diagram-server/build/
ENV SEMRULES_PATH="/root/diagram-server/semrules/"

# Move the database schema files to XDG_DATA_DIRS
RUN mkdir -p /root/.local/share/codevis/database-spec/ \
    && cp /root/diagram-server/database-spec/codebase_db.sql /root/.local/share/codevis/database-spec/ \
    && cp /root/diagram-server/database-spec/cad_db.sql /root/.local/share/codevis/database-spec/

RUN mkdir -p "${OUTDIR}"
CMD "${BUILD_PATH}/lvtclp/create_codebase_db" \
    --source-path "/root/diagram-server/" \
    --silent \
    -j6 \
    --ignore *.t.cpp \
    --ignore *.m.cpp \
    --ignore *moc_* \
    --ignore *mocs_* \
    --ignore *ui_* \
    --replace \
    -o "${OUTDIR}/codevis.db" \
    --compile-commands-json "${BUILD_PATH}/compile_commands.json" \
    && \
    "${BUILD_PATH}/lvtprj/create_prj_from_db" \
    --source-path "/root/diagram-server/" \
    "${OUTDIR}/codevis.db" \
    "${OUTDIR}/codevis.lks"


FROM dependencies as code-coverage

COPY . /root/diagram-server
WORKDIR /root/diagram-server
RUN apt-get update && apt-get -qq install python3-jinja2 gcovr
RUN git submodule init
RUN git submodule update --recursive

# Move the database schema files to XDG_DATA_DIRS
RUN mkdir -p /root/.local/share/codevis/database-spec/ \
    && cp /root/diagram-server/database-spec/codebase_db.sql /root/.local/share/codevis/database-spec/ \
    && cp /root/diagram-server/database-spec/cad_db.sql /root/.local/share/codevis/database-spec/

WORKDIR /root/diagram-server/build
RUN cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_CODE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug ..
RUN cmake --build . -j$(nproc)
ENV LAKOSDIAGRAM_PYSCRIPTS_PATH="/root/diagram-server/python/codegeneration/"
ENV TEST_PRJ_PATH="/root/diagram-server/lvtclp/systemtests/"
ENV QT_QPA_PLATFORM="offscreen"

CMD ctest . -VV -j$(nproc) && make coverage
