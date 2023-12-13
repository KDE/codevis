EXTRA_CMAKE_OPTIONS="$1"

export PATH=/opt/rh/devtoolset-9/root/usr/bin:$PATH
git clone https://invent.kde.org/sdk/codevis.git
cd codevis/ || exit
git checkout work/fortran-parser
mkdir -p build/ && cd build/ || exit
cmake .. -DQT_MAJOR_VERSION=5 -DWARNINGS_AS_ERRORS=OFF -DBUILD_DESKTOP_APP=OFF -DENABLE_PLUGINS=OFF "$EXTRA_CMAKE_OPTIONS" || exit
cmake --build . -j"$(nproc)" || exit

export CODEVIS_PKG_ARTIFACTS_PATH=/artifacts
mkdir -p $CODEVIS_PKG_ARTIFACTS_PATH
cp ./lvtcgn/liblvtcgn_mdl.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtclp/liblvtclp.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtshr/liblvtshr.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtmdb/liblvtmdb.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtprj/liblvtprj.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtldr/liblvtldr.so ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lib/libsoci_core.so.4.1 ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lib/libsoci_sqlite3.so.4.1 ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtclp/codevis_create_codebase_db ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtclp/codevis_merge_databases ${CODEVIS_PKG_ARTIFACTS_PATH}
cp ./lvtprj/codevis_create_prj_from_db ${CODEVIS_PKG_ARTIFACTS_PATH}

# Codevis GUI (Disabled, kept for reference)
#cp ./lvtqtd/liblvtqtd.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./lvtcgn/liblvtcgn_gui.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./lvtcgn/liblvtcgn_adapter.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./lvtmdl/liblvtmdl.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./lvtqtw/liblvtqtw.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./lvtqtc/liblvtqtc.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./lvtclr/liblvtclr.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./lvtplg/liblvtplg.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./liblakospreferences.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./thirdparty/MRichTextEditor/libMRichTextEdit.so ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp ./desktopapp/codevis_desktop ${CODEVIS_PKG_ARTIFACTS_PATH}
#cp -r ./desktopapp/lks-plugins/ ${CODEVIS_PKG_ARTIFACTS_PATH}

# Copy precompiled external dependencies
cp /usr/lib/libQt5Core.so.5 .
cp /usr/local/lib64/libKF5Archive.so.5 .
cp /lib64/libsqlite3.so.0 .
cp /lib64/libpython2.7.so.1.0 .
cp /lib/libclang-cpp.so.17 .
cp /lib/libLLVM-17.so .
