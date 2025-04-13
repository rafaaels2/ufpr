   #!/usr/bin/env bash

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
declare -r SCRIPT_DIR

set -e
set -x

CADICAL=

get_cadical() {
    [ -d cadical ] || git clone https://github.com/arminbiere/cadical.git cadical
    pushd cadical
    CADICAL=$(pwd)
    mkdir -p debug
    pushd debug
    ../configure -g
    make -j $(nproc)
    popd
    popd
}

# work in the directory of this script
cd "$SCRIPT_DIR"

# build and test with mergesat
make BUILD_TYPE=parallel -j6 ld lsh tests-r-all -C "$SCRIPT_DIR/../.." VERB=
g++ cadical-interface-test.cc -o mergesat-test -g -O0 -std=c++11\
    -I "$SCRIPT_DIR/../../minisat" -L "$SCRIPT_DIR"/../../build/debug/lib \
    -lmergesat -DMERGESAT_CADICAL
./mergesat-test

# build and test with cadical
get_cadical
g++ cadical-interface-test.cc -o cadical-test -g -O0 -std=c++11\
    -I "$CADICAL"/src -L "$CADICAL"/debug -lcadical
./cadical-test
