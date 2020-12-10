#!/bin/bash

set -e

NAME=$1
shift
TARGET=$1
shift
SRCDIR=$1
shift

BDIR=$PWD
cd ${SRCDIR}
rm -f ${TARGET}
for FILE in $@; do
    tar --transform "s,^\.,${NAME}," -uf ${TARGET} ./${FILE}
done

cd ${BDIR}
LIBNAME=${NAME}-dpi.so
if [ -f ${LIBNAME} ]; then
    tar --transform "s,^\.,${NAME}," -uf ${TARGET} ./${LIBNAME}
fi
