#!/bin/bash

set -e

NAME=$1
shift
TARGET=$1
shift
SRCDIR=$1
shift

TEMPDIR=`mktemp -d -t sv-XXXXXXXXXX`
BDIR=${PWD}

cd ${TEMPDIR}
mkdir ${NAME}
cd ${NAME}

for FILE in $@; do
    if [ ${FILE: -4} == ".tar" ]; then
        tar xf ${FILE}
    elif [ ${FILE: -3} == ".sv" ]; then
        cp ${SRCDIR}/${FILE} .
    fi
done

cd ..

rm -f ${TARGET}
tar cf ${TARGET} ${NAME}

cd ${BDIR}
rm -rf ${TEMPDIR}
