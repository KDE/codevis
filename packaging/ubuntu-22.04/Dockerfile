FROM ubuntu:22.04 AS base

ARG DEBIAN_FRONTEND=noninteractive

RUN apt update
RUN apt install -y git cmake gcc g++ wget gpg lsb-release software-properties-common
RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 17
RUN apt install -y \
    libllvm17 llvm-17 llvm-17-dev \
    clang-17 clang-tools-17 libclang-common-17-dev libclang-17-dev libclang1-17 clang-format-17 \
    libmlir-17-dev mlir-17-tools \
    flang-17
RUN apt install -y \
    qtbase5-dev \
    qtscript5-dev \
    libqt5x11extras5-dev \
    libqt5webkit5-dev \
    libqt5xmlpatterns5-dev \
    libqt5svg5-dev \
    qttools5-dev \
    qtdeclarative5-dev \
    libphonon4qt5-dev
RUN apt install -y \
    extra-cmake-modules  \
    libkf5archive-dev \
    libkf5config-dev \
    libkf5crash-dev \
    libkf5coreaddons-dev \
    libkf5widgetsaddons-dev \
    libkf5texteditor-dev \
    libkf5xmlgui-dev \
    libkf5newstuff-dev \
    libkf5notifications-dev \
    libkf5kcmutils-dev \
    libkf5i18n-dev
RUN apt install -y libsqlite3-dev qtwebengine5-dev catch2 python3 python3-dev kdoctools5 libkf5doctools-dev

WORKDIR /
ADD deploy.sh /
RUN chmod +x /deploy.sh
CMD "/deploy.sh"
