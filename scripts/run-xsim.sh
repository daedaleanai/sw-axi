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

rm -f prj
for MOD in `ls -d */`; do
    if [[ -f ${MOD}/package.sv ]]; then
        echo "sv ${MODULE} ${MOD}/package.sv" >> prj
    fi
done

echo "sv ${MODULE} ${MODULE}.sv" >> prj

mkdir dpi
DPI_PARAMS="--sv_root dpi"; \
for LIB in `find -type f | grep .so$`; do
    cp ${LIB} dpi
    DPI_PARAMS="$DPI_PARAMS --sv_lib `basename ${LIB} .so`";
done

cat <<EOF > batchmode.tcl
set objs [get_objects -r *]

if { [llength $${objs}] != 0 } {
    open_vcd
    log_vcd [get_objects -r *]
}

run all

if { [llength $${objs}] != 0 } {
    close_vcd
}

exit
EOF

xvlog -prj prj
eval xelab --timescale 1ns/1ps -debug typical $DPI_PARAMS ${MODULE}.${MODULE} -s testbench_sim -prj prj
xsim testbench_sim -t batchmode.tcl

cd ${BDIR}
rm -rf ${TEMPDIR}
