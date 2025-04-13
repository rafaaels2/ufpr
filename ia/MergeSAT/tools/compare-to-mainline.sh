#!/usr/bin/env bash
#
# Run a small benchmark evaluation and report the PAR2 value for a selected
# set of benchmarks.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

declare -i TIMEOUT=600
declare -i SPACE_MB=4096
declare LOG_DUMP=""
declare RESULTS_DIRS=""
declare OUTPUT_FILE=""
declare -i VERBOSE=0
declare -i ERROR=0
declare -a BENCHMARK_ITEMS
declare -i IGNORE_ERRORS=0
PRODUCTION_MERGESAT=""

get_production_mergesat() {
    trap '[ -d "$MERGESAT_TMPD" ] && rm -rf "$MERGESAT_TMPD"' EXIT
    COPY_PWD=${PWD}
    MERGESAT_TMPD=$(mktemp -d)
    pushd "$MERGESAT_TMPD"
    git clone https://github.com/conp-solutions/mergesat.git mergesat

    if [ -z "$PRODUCTION_MERGESAT" ]; then
        pushd mergesat
        PRODUCTION_MERGESAT="mergesat-production-$(git describe)"
        PRODUCTION_MERGESAT="$COPY_PWD/$PRODUCTION_MERGESAT"
        popd
    fi

    pushd mergesat
    make r -j $(nproc)
    cp build/release/bin/mergesat "$PRODUCTION_MERGESAT"
    popd
    popd
}

get_benchmark() {
    pushd "$SCRIPT_DIR" &>/dev/null
    mkdir -p benchmarks
    cd benchmarks

    # some SAT 2020 benchmarks
    [ -r "prime2209-84.cnf.xz" ] || wget --content-disposition https://gbd.iti.kit.edu/file/af66c2b2ff8a9cd900d9f2f79e53f6a7
    [ -r "fclqcolor-18-14-11.cnf.gz.CP3-cnfmiter.cnf.xz" ] || wget --content-disposition https://gbd.iti.kit.edu/file/72a1b82b2c1f17b0e5e0d4a44937d848
    [ -r "k2fix_gr_2pinvar_w8.shuffled.cnf.xz" ] || wget --content-disposition https://gbd.iti.kit.edu/file/b400f2362d15334aa6d05ef99e315fc1
    [ -r "3bitadd_32.cnf.gz.CP3-cnfmiter.cnf.xz" ] || wget --content-disposition https://gbd.iti.kit.edu/file/3d38ffe08887da6cbe9b17ce50c4b34c
    [ -r "Timetable_C_437_E_62_Cl_29_S_28.cnf.xz" ] || wget --content-disposition https://gbd.iti.kit.edu/file/8e905dfa09f45f7f50099f70cc38714c
    [ -r "tseitingrid7x185_shuffled.cnf.xz" ] || wget --content-disposition https://gbd.iti.kit.edu/file/4a3bee9d892a695c3d63191f4d1fbdff

    declare -a PRE_BENCHMARK_ITEMS=(
        "prime2209-84.cnf.xz"
        "fclqcolor-18-14-11.cnf.gz.CP3-cnfmiter.cnf.xz"
        "k2fix_gr_2pinvar_w8.shuffled.cnf.xz"
        "3bitadd_32.cnf.gz.CP3-cnfmiter.cnf.xz"
        "tseitingrid7x185_shuffled.cnf.xz"
    )

    for benchmark in "${PRE_BENCHMARK_ITEMS[@]}"; do
        BENCHMARK_ITEMS+=("$PWD/$benchmark")
    done

    popd &>/dev/null
}

usage() {
    cat <<EOF
  MergeSat comparison tool

  Run the current solver and compare the behavior with the current production
  variant.

  Usage:

    $0 [OPTIONS] "solver call"

    OPTIONS

      -b BenchmarkDir ...... use files from this directory for the benchmark.
      -P solver ............ use this solver as reference solvery
      -v ................... increase verbosity of output and dumped log
      -y ................... only print
EOF
}

BENCHMARKDIR="$SCRIPT_DIR"/benchmarks

for tool in runlim bc awk; do
    if ! command -v "$tool" &>/dev/null; then
        echo "error: failed to find tool $tool"
    fi
done

# do we want to package Riss(for Coprocessor) or Sparrow as well?
while getopts "b:P:vy" OPTION; do
    case $OPTION in
    b)
        BENCHMARKDIR="$OPTARG"
        BENCHMARKURLFILE=""
        ;;
    P)
        PRODUCTION_MERGESAT="$OPTARG"
        ;;
    v)
        VERBOSE=$((VERBOSE + 1))
        ;;
    y)
        IGNORE_ERRORS=1
        ;;
    *)
        usage
        exit 1
        ;;
    esac
done
shift "$((OPTIND - 1))"
declare -r TIMEOUT
declare -r SPACE_MB

# in case no benchmark directory is specified, get a small benchmark
if [ "$BENCHMARKDIR" = "$SCRIPT_DIR"/benchmarks ] && [ -z "$RESULTS_DIRS" ]; then
    get_benchmark
else
    PREBENCHMARKS=()
fi

if [ -n "$PRODUCTION_MERGESAT" ] && [ -x "$PRODUCTION_MERGESAT" ]; then
    echo "Using solver '$PRODUCTION_MERGESAT' as reference"
else
    echo "Getting upstream solver for comparison"
    get_production_mergesat
fi

declare -a COMPARE_SOLVER="$@"
echo "Solver to analyze: ${COMPARE_SOLVER[*]}"

# only walk through non-default benchmark directory
if [ "${#BENCHMARK_ITEMS[@]}" -eq 0 ]; then
    BENCHMARK_ITEMS=($(find "${BENCHMARKDIR}" -type f | head -n 10))
fi
for benchmarkitem in "${BENCHMARK_ITEMS[@]}"; do
    # check if we should abort
    if [ -n "$STOPFILE" ]; then
        echo "Checking stopfile '$STOPFILE' ..."
        if [ -r "$STOPFILE" ]; then
            echo "... abort benchmark due to existing stopfile, and removing it"
            rm -f "$STOPFILE"
            break
        fi
    fi
    benchmark="$(realpath "$benchmarkitem")"
    # solve benchmark for all solvers
    echo ""
    [ "$VERBOSE" -gt 0 ] && echo "Test benchmark: $benchmark"
    STATUS=0
    tools/check-solver-behavior.sh "$benchmarkitem" "${COMPARE_SOLVER[@]}" $PRODUCTION_MERGESAT || STATUS=$?
    if [ "$STATUS" -ne 0 ]; then
        ERROR=1
        # abort early, we found a diff already
        break
    fi
done

echo "Compare status '$ERROR' (with solver '$PRODUCTION_MERGESAT' (after $SECONDS seconds)"

[ "$IGNORE_ERRORS" -eq 0 ] && exit 0
exit "$ERROR"
