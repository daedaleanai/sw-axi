#!/bin/bash

set -e

if [[ $# -ne 2 ]]; then
    echo "usage: $0 module app"
    exit 1
fi

MODULE=$1
APP=$2

TEMPDIR=`mktemp -d -t sv-XXXXXXXXXX`
BDIR=${PWD}

cd ${TEMPDIR}
tar xf ${BDIR}/${APP}

DIR=`ls -1 | head -n 1`

if [[ ! -d ${DIR} ]]; then
    echo "[!] Missing app directory"
    exit 1
fi

cd ${DIR}

vlib work

for MOD in `ls -d */`; do
    if [[ ${MODULE} == "work/" ]]; then
        continue
    fi

    if [[ -f ${MOD}/package.sv ]]; then
        vlog -sv -nologo -quiet ${MOD}/package.sv
    fi
done

vlog -sv -nologo -quiet ${MODULE}.sv

echo '#!SV_LIBRARIES' > dpilibs

for LIB in `find -type f | grep .so$`; do
    echo `dirname ${LIB}`/`basename ${LIB} .so` >> dpilibs
done

cat <<EOF > batchmode.do
onfinish exit
run -all
EOF

vsim -c ${MODULE} -sv_liblist dpilibs -quiet -do batchmode.do

cd ${BDIR}
rm -rf ${TEMPDIR}

