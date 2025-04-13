#!/bin/bash
#
# Given a (list of) solver(s), run them on different benchmarks and check
# whether their behavior is the same on multiple calls.

declare -i ROUNDS=10
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
declare -i STATUS=0
MODGEN=""
MODGEN_N="15000"
MODGEN_M="72500"

trap '[ -d "$WORKDIR" ] && rm -rf "$WORKDIR"' EXIT
WORKDIR=$(mktemp -d check-determinism-XXXXXX)
WORKDIR=$(realpath "$WORKDIR")
echo "Working in $WORKDIR"

if ! [ -x "$SCRIPT_DIR"/check-solver-behavior.sh ]; then
    echo "Error: did not find check-solver-behavior.sh script, aborting"
    exit 1
fi

usage() {
    cat <<EOF

$0 ... check solver(s) for deterministic behavior

  Call:
    $0 [OPTIONS] "solver1 [param [param 2 [...]]" [...]

  Options:
    -M <modgen> ... location of modgen binary
    -m <int> ...... '-m' parameter for modgen tool
    -n <int> ...... '-n' parameter for modgen tool
    -r <int> ...... number of CNFs to test for each solver (default $ROUNDS)

  Note:
    This script uses 'check-solver-behavior.sh' to compare solver output.
EOF
}

# Allow to specify modgen, parameters, and rounds
while getopts "m:M:n:r:" OPTION; do
    case $OPTION in
    m)
        MODGEN_M="$OPTARG"
        ;;
    M)
        MODGEN="$OPTARG"
        ;;
    n)
        MODGEN_N="$OPTARG"
        ;;
    r)
        ROUNDS="$OPTARG"
        ;;
    *)
        usage
        exit 1
        ;;
    esac
done
shift "$((OPTIND - 1))"

get_cnf_generator() {
    pushd "$WORKDIR"
    git clone https://github.com/conp-solutions/modularityGen .
    git reset --hard cb88f2f3c7790bb478c4ff9bc9a17d7a3625591a
    g++ modularityGen_v2.1.cpp -o modgen || echo "Failed compiling ..."
    [ -x "./modgen" ] && MODGEN="$WORKDIR/modgen"
    popd

    if [ -z "$MODGEN" ]; then
        echo "Error: failed to retrieve and build modgen, aborting"
        exit 1
    fi
}

# Do not get modgen, if a copy is presented already
[ -x "$MODGEN" ] || get_cnf_generator

declare -a MODGEN_CALL=()
for round in $(seq $ROUNDS); do
    for solver in "$@"; do
        SEED=$((RANDOM % 10000))
        echo "Checking: seed: $SEED solver: '$solver' ..."

        MODGEN_CALL=("$MODGEN" "-s" "$SEED" "-n" "15000" "-m" "72500")
        "${MODGEN_CALL[@]}" >"$WORKDIR"/input.cnf || echo "Failed generating input cnf"
        grep "^p cnf " "$WORKDIR"/input.cnf
        CALL_STATUS=0
        "$SCRIPT_DIR"/check-solver-behavior.sh \
            "$WORKDIR"/input.cnf \
            "$solver" \
            "$solver" &>"$WORKDIR"/out.log || CALL_STATUS=$?
        if [ "$CALL_STATUS" -ne 0 ]; then
            echo "Error: Detected determinism issue in combination modgen call: ${MODGEN_CALL[*]}  solver: $solver"
            cat "$WORKDIR"/out.log
            STATUS=1
        fi
    done
done

# Forward exit status
if [ "$STATUS" -ne 0 ]; then
    echo "Error: failed to execute comparison"
else
    echo "SUCCESS! Did not find any differences in runs"
fi
echo "Terimnate after $SECONDS seconds and $ROUNDS rounds"
exit $STATUS
