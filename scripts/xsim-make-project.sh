#!/bin/bash

NAME=$1
shift

PRJ_FILE="$NAME.prj"
rm -f $PRJ_FILE

for i in $@; do
    if [ x"${i##*.}" == xprj ]; then
        while IFS= read -r line; do
            echo "sv $NAME `dirname $i`/$line" >> $PRJ_FILE
        done < "$i"
    else
        echo "sv $NAME $i" >> $PRJ_FILE
    fi
done
