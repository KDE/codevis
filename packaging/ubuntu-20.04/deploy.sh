EXTRA_CMAKE_OPTIONS="$1"

# TODO: Review llvm .so version
ln -s /usr/lib/llvm-17/lib/libclang-17.so.17 /usr/lib/llvm-17/lib/libclang-17.so.1

# Ubuntu CMake is way too old, so we download a newer version
wget https://github.com/Kitware/CMake/releases/download/v3.26.6/cmake-3.26.6-linux-x86_64.sh
chmod +x cmake-3.26.6-linux-x86_64.sh
mkdir -p /cmake/
./cmake-3.26.6-linux-x86_64.sh --skip-license --prefix=/cmake/
export PATH=/cmake/bin:$PATH

git clone https://invent.kde.org/sdk/codevis.git
cd codevis/ || exit
mkdir -p build/ && cd build/ || exit
cmake .. -DQT_MAJOR_VERSION=5 -DWARNINGS_AS_ERRORS=OFF "$EXTRA_CMAKE_OPTIONS" || exit
cmake --build . -j"$(nproc)" || exit
# Make sure all plugin files are copied
rm -rf ./desktopapp/lks-plugins/
cmake .. -DQT_MAJOR_VERSION=5 -DWARNINGS_AS_ERRORS=OFF "$EXTRA_CMAKE_OPTIONS" && cmake --build . --target deploy_app_plugins

export CODEVIS_PKG_ARTIFACTS_PATH=/artifacts
mkdir -p $CODEVIS_PKG_ARTIFACTS_PATH
cp ./lvtqtd/liblvtqtd.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtcgn/liblvtcgn_gui.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtcgn/liblvtcgn_adapter.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtcgn/liblvtcgn_mdl.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtclp/liblvtclp.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtmdl/liblvtmdl.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtshr/liblvtshr.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtmdb/liblvtmdb.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtqtw/liblvtqtw.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtprj/liblvtprj.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtqtc/liblvtqtc.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtclr/liblvtclr.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtplg/liblvtplg.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtldr/liblvtldr.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./liblakospreferences.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./thirdparty/MRichTextEditor/libMRichTextEdit.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lib/libsoci_core.so.4.1 ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lib/libsoci_sqlite3.so.4.1 ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./desktopapp/codevis_desktop ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtclp/codevis_create_codebase_db ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtclp/codevis_merge_databases ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtprj/codevis_create_prj_from_db ${CODEVIS_PKG_ARTIFACTS_PATH}

cp -r ./desktopapp/lks-plugins/ ${CODEVIS_PKG_ARTIFACTS_PATH}
