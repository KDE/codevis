#!/bin/bash

# Variables
bold=$(tput bold)
normal=$(tput sgr0)
scriptpath="$( cd "$(dirname "$0")" || exit; pwd -P )"
projectpath=${scriptpath}/../..
buildfolder=${projectpath}/build/lvt
externaldepsfolder=${buildfolder}/externaldeps
outdir=/tmp
scriptname=$(basename "$0")
verboseoutput="/tmp/compile_output.txt"
echopid=0
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
    --outdir)
    shift
    outdir="$1"
    shift ;;

    --buildfolder)
    shift
    buildfolder="$1"
    shift ;;

    --externaldepsfolder)
    shift
    externaldepsfolder="$1"
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

linuxclideployfiles=(
    "${buildfolder}/lvtclp/create_codebase_db"
    "${buildfolder}/lvtprj/create_prj_from_db"
    "${buildfolder}/lvtclp/lvtclp_merge_databases"
    "${buildfolder}/lvtclp/liblvtclp.so"
    "${buildfolder}/lvtmdb/liblvtmdb.so"
    "${buildfolder}/lvtprj/liblvtprj.so"
    "${buildfolder}/lvtshr/liblvtshr.so"
    "${buildfolder}/lvtplg/liblvtplg.so"
)

unameout="$(uname -s)"
case "$unameout" in
    Linux*)     machine="Linux 🐧";;
    *)          machine="UNKNOWN:${unameout}"
esac

if [[ $machine != *"Linux"* ]]; then
    error "Machine not compatible: ${machine}"
fi;

echo ""
echob "Start to deploy project.."

deployclifolder="${outdir}/cli"
mkdir -p "${deployclifolder}"
for i in "${linuxclideployfiles[@]}"; do
    runstep "cp ${i} ${deployclifolder}" "Move file to deploy CLI folder ""${i}" "Failed to deploy file for CLI: ""${i}"
done
runstep "cp ""${externaldepsfolder}/*"" ${deployclifolder}" "Move external dependency files to deploy CLI folder" "Failed to move external dependency files to deploy CLI folder"
tar -czvf codevis-cli.tar.gz -C "${deployclifolder}" .
runstep "mv codevis-cli.tar.gz ${outdir}/codevis-cli.tar.gz" "Move CLI artifact to ${outdir}" "Faile to move CLI artifact to ${outdir}"

echo "#############################################"
echo "## ONLY DISTRIBUTE IMAGES BUILT VIA DOCKER ##"
echo "#############################################"
