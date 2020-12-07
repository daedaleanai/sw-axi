#!/bin/bash

set -e

if [[ $# -ne 1 ]]; then
    echo "usage: $0 dir"
    exit 1
fi

DIR=$1

while IFS= read -r FILE
do
    if [ ${FILE: -3} == ".cc" -o ${FILE: -3} == ".hh" ]; then
        clang-format -i ${FILE}
    elif [ ${FILE: -3} == ".sv" ]; then
        verible-verilog-format --inplace ${FILE}
    fi
done < <(find ${DIR} -type f | egrep '\.cc$|\.hh$|\.sv$' | grep -v \.git)
