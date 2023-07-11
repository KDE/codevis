#!/bin/bash

# Variables
bold=$(tput bold)
normal=$(tput sgr0)
scriptpath="$( cd "$(dirname "$0")" || exit; pwd -P )"
projectpath=${scriptpath}/../..
buildfolder=${projectpath}/build
lvtbuildfolder=${buildfolder}/lvt
deployfolder=${buildfolder}/deploy
outdir=/tmp
scriptname=$(basename "$0")
buildtype="Release"
numberofthreads=1
verboseoutput="/tmp/compile_output.txt"
echopid=0
linuxdeployfiles=(
    "${lvtbuildfolder}/desktopapp/codevis"
    "${lvtbuildfolder}/lvtclr/liblvtclr.so"
    "${lvtbuildfolder}/lvtmdl/liblvtmdl.so"
    "${lvtbuildfolder}/lvtqtc/liblvtqtc.so"
    "${lvtbuildfolder}/lvtqtw/liblvtqtw.so"
    "${lvtbuildfolder}/lvtshr/liblvtshr.so"
    "${lvtbuildfolder}/lvtplg/liblvtplg.so"
)

deploy=true
debug=false
help=false

terminate() {
    if ((echopid != 0)); then
        kill ${echopid}
    fi
}

trap "terminate" SIGINT

# Functions
echob() {
    echo "${bold}${1}${normal}"
}

printfb() {
    printf "%s%s%s" "${bold}" "${1}" "${normal}"
}

error() {
    echo -e "ERROR: $*" >&2
    exit 1
}

usage() {
    echo "USAGE: ${scriptname} --no-deploy, --debug, --outdir DIR, --help"
}

checktool() {
    name=""
    errormsg=""
    if (( $# > 1 )); then
        name=$2
        if (( $# >= 3 )); then
            errormsg=$3
        fi
    else
        name=( "${1}" )
    fi

# shellcheck disable=SC2128
    printfb "${name}: "
    eval "$1" >> $verboseoutput 2>&1

# shellcheck disable=SC2181
    [ $? == 0 ] || {
        echob "✖"
        cat $verboseoutput
        error "${name[0]} is not available. ${errormsg}"
    }
    echob "✔"
}

runstep() {
    runstep_name=$1
    okmessage=$2
    errormsg=$3
    printfb "${okmessage}: "

    # This prints a '.' while the command is evaluated
    sh -c "while sleep 1; do printf '.'; done;" &
    echopid=$!
    eval "$1" >>$verboseoutput 2>&1
    returncode=$?
    kill $echopid
    echopid=0
    [ $returncode == 0 ] || {
        echob "✖"
        cat $verboseoutput
        error "${runstep_name} failed. ${errormsg}"
    }
    echob "✔"
}

# Check for args
while [ "$1" != "" ]
do
case $1 in
    --debug)
    debug=true
    buildtype="Debug"
    shift ;;

    --no-deploy)
    deploy=false
    shift ;;

    --outdir)
    shift
    outdir="$1"
    shift ;;

    --help|-h)
    help=true
    shift ;;

    *)
    error "Unknow argument: $1"
    ;;
esac
done

if $help; then
    usage
    exit 1
fi

# Remove last compile output
rm $verboseoutput

echob "Project will be:"
printf "\t- Compiled in "
$debug && echo "Debug mode." || echo "Release mode."

printf "\t- "
$deploy && echo "Deployed." || echo "Not deployed."

echo ""

unameout="$(uname -s)"
case "$unameout" in
    Linux*)     machine="Linux 🐧";;
    *)          machine="UNKNOWN:${unameout}"
esac

if [[ $machine != *"Linux"* ]]; then
    error "Machine not compatible: ${machine}"
fi;

echob "Compiling for ${machine}"
echob "Checking for tools..."

checktool "git --version"
checktool "qmake --version"
checktool "python --version"
checktool 'python -c "import jinja2; print(jinja2.__version__);"' jinja2 "Version 2.10+ is necessary."

checktool "gcc --version"
numberofthreads=$(nproc --all)

echo ""
echob "Start to build project.."
echob "Number of threads: ""${numberofthreads}"
runstep "rm -rf ${buildfolder}" "Check for old build folder" "Failed to delete old build folder"
runstep "mkdir -p ${buildfolder}" "Root build folder created" "Failed to create build folder in ${buildfolder}"
runstep "mkdir -p ${lvtbuildfolder}" "Lvt build folder created" "Failed to create build folder in ${lvtbuildfolder}"

# lvt build
# Build type is necessary for both configuratio and compilation step
# - For the first when using a makefile based system (unix makefiles, nmake makefiles, mingw makefiles)
# - For the second when using multi-configuration build system tools (vscode, xcode)
# Parallel configuration will use the available cpus
runstep "cmake -S ${projectpath} -B ${lvtbuildfolder} -DCMAKE_BUILD_TYPE=${buildtype} -DCMAKE_INSTALL_PREFIX=${deployfolder} -DCOMPILE_TESTS=OFF" "Configure Lvt" "Project configuration failed"
runstep "cmake --build ${lvtbuildfolder} --parallel --config ${buildtype}" "Build Lvt" "Failed to build project"

if [ "$deploy" == "false" ]; then
    echob "Done!"
    exit 0
fi

echo ""
echob "Start to deploy project.."

runstep "mkdir -p ${deployfolder}" "Deploy folder created" "Failed to create deploy folder in ${deployfolder}"
runstep "make -C \"$lvtbuildfolder\" install" "Install lvt" "Failed to install lvt"

# we copy in the things we want manually from linuxdeployfiles anyway
runstep "rm -r \"$deployfolder/bin\"" "Delete duplicate binaries" "Failed to delete duplicate binaries"
runstep "rm -r \"$deployfolder/lib/\"*.so" "Delete duplicate libraries" "Failed to delete duplicate libraries"

for i in "${linuxdeployfiles[@]}"; do
    runstep "cp ${i} ${deployfolder}" "Move file to deploy folder ""${i}" "Failed to deploy file: ""${i}"
done
runstep "cp -r ${projectpath}/python/ ${deployfolder}" "Copy python scripts necessary for code generation module"

runstep "wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage -O /tmp/linuxdeploy.AppImage" "Download linuxdeploy" "Failed to download linuxdeploy"
runstep "wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage -O /tmp/linuxdeploy-plugin-qt.AppImage" "Download linuxdeploy Qt plugin" "Failed to download linuxdeploy Qt plugin"
runstep "chmod a+x /tmp/linuxdeploy.AppImage" "Convert linuxdeploy to executable" "Failed to turn linuxdeploy in executable"
runstep "chmod a+x /tmp/linuxdeploy-plugin-qt.AppImage" "Convert linuxdeploy Qt plugin to executable" "Failed to turn linuxdeploy Qt plugin in executable"
runstep "unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH;" "Unset some Qt variables" "Failed to unsed Qt variables"
runstep "export QML_SOURCES_PATHS=${projectpath}/qml" "Set QML_SOURCES_PATHS" "Failed to set QML_SOURCES_PATHS"
runstep "/tmp/linuxdeploy.AppImage --icon-file=${projectpath}/imgs/codevis.png --desktop-file=${projectpath}/codevis.desktop --executable=${deployfolder}/codevis --appdir=${deployfolder} --plugin qt --output appimage --verbosity=3" "Run linuxdeploy" "Failed to run linuxdeploy"
runstep "mv codevis-*.AppImage ${outdir}/codevis-x86_64.AppImage" "Move .AppImage folder to ${outdir}" "Faile to move .AppImage file"

echo "#############################################"
echo "## ONLY DISTRIBUTE IMAGES BUILT VIA DOCKER ##"
echo "#############################################"
