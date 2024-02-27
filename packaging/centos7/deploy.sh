while getopts b:c: flag
do
    case "${flag}" in
        b) BRANCH=${OPTARG};;
        c) CMAKE_ARGS=${OPTARG};;
    esac
done
BRANCH=${BRANCH:-master}
CMAKE_ARGS=${CMAKE_ARGS:-"-DQT_MAJOR_VERSION=5 -DBUILD_DESKTOP_APP=OFF -DENABLE_PLUGINS=OFF"}
export CODEVIS_PKG_ARTIFACTS_PATH=/artifacts
mkdir -p $CODEVIS_PKG_ARTIFACTS_PATH || exit

echo "DEPLOY OPTIONS:"
echo "Codevis branch (default=master): $BRANCH";
echo "CMake args: $CMAKE_ARGS";
echo "Artifacts path: $CODEVIS_PKG_ARTIFACTS_PATH";
echo ""

export PATH=/opt/rh/devtoolset-9/root/usr/bin:$PATH
git clone https://invent.kde.org/sdk/codevis.git || exit
cd codevis/ || exit
git checkout "$BRANCH" || exit
mkdir -p build/ && cd build/ || exit
mkdir -p /codevis-install/ || exit
cmake .. $CMAKE_ARGS -DCMAKE_INSTALL_RPATH="\$ORIGIN/lib/" -DCMAKE_INSTALL_PREFIX="/codevis-install/" || exit
cmake --build . -j"$(nproc)" || exit
cmake --install .

export CODEVIS_ZIP_PATH=/codevis-install/zip/
export CODEVIS_ZIP_CONTENTS_PATH=${CODEVIS_ZIP_PATH}/codevis/
mkdir -p ${CODEVIS_ZIP_CONTENTS_PATH}
mkdir -p ${CODEVIS_ZIP_CONTENTS_PATH}/bin/ || exit
mkdir -p ${CODEVIS_ZIP_CONTENTS_PATH}/bin/lib/ || exit
cp /codevis-install/bin/* ${CODEVIS_ZIP_CONTENTS_PATH}/bin/
cp /codevis-install/lib64/*.so* ${CODEVIS_ZIP_CONTENTS_PATH}/bin/lib/

# Copy precompiled external dependencies
cp /usr/lib/libQt5Core.so* ${CODEVIS_ZIP_CONTENTS_PATH}/bin/lib/
cp /usr/local/lib64/libKF5Archive.so* ${CODEVIS_ZIP_CONTENTS_PATH}/bin/lib/
cp /usr/local/lib/libsqlite3.so* ${CODEVIS_ZIP_CONTENTS_PATH}/bin/lib/
cp /lib/libclang-cpp.so* ${CODEVIS_ZIP_CONTENTS_PATH}/bin/lib/
cp /lib/libLLVM-17*.so ${CODEVIS_ZIP_CONTENTS_PATH}/bin/lib/
cp /lib64/libpython3.6m.so* ${CODEVIS_ZIP_CONTENTS_PATH}/bin/lib/
cp -R /usr/lib64/python3.6/ ${CODEVIS_ZIP_CONTENTS_PATH}/bin/lib/python3.6/

# Create helper scripts
CODEVIS_ENV_SETUP="PYTHONHOME=\$(dirname \"\$0\")/bin/lib/python3.6/ PYTHONPATH=\$(dirname \"\$0\")/bin/lib/python3.6/"
echo '#!/bin/bash' > ${CODEVIS_ZIP_CONTENTS_PATH}/codevis_create_codebase_db
echo "$CODEVIS_ENV_SETUP \$(dirname \"\$0\")/bin/codevis_create_codebase_db \$@" >> ${CODEVIS_ZIP_CONTENTS_PATH}/codevis_create_codebase_db
chmod +x ${CODEVIS_ZIP_CONTENTS_PATH}/codevis_create_codebase_db

echo '#!/bin/bash' > ${CODEVIS_ZIP_CONTENTS_PATH}/codevis_create_prj_from_db
echo "$CODEVIS_ENV_SETUP \$(dirname \"\$0\")/bin/codevis_create_prj_from_db \$@" >> ${CODEVIS_ZIP_CONTENTS_PATH}/codevis_create_prj_from_db
chmod +x ${CODEVIS_ZIP_CONTENTS_PATH}/codevis_create_prj_from_db

echo '#!/bin/bash' > ${CODEVIS_ZIP_CONTENTS_PATH}/codevis_merge_databases
echo "$CODEVIS_ENV_SETUP \$(dirname \"\$0\")/bin/codevis_merge_databases \$@" >> ${CODEVIS_ZIP_CONTENTS_PATH}/codevis_merge_databases
chmod +x ${CODEVIS_ZIP_CONTENTS_PATH}/codevis_merge_databases

# Create tar
tar -czvf ${CODEVIS_PKG_ARTIFACTS_PATH}/codevis.tar.gz --mode='a+rwX' -C ${CODEVIS_ZIP_PATH} codevis/
