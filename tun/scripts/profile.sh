#!/bin/bash

cd ..

if [ ! -f ./build/test/general_profiling ]; then
    echo "Profiling test does not exist"
fi

mkdir -p html

./build/test/general_profiling
lcov --capture --directory . --output-file coverage.info || exit 1
genhtml coverage.info --output-directory html || exit 1

