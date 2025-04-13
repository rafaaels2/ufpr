#!/usr/bin/env bash
#
# Run a small benchmark evaluation and report the PAR2 value for a selected
# set of benchmarks.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

declare -i TIMEOUT=600
declare -i SPACE_MB=4096

SOLVER="$1"
SOLVER=$(realpath "$SOLVER")

# TODO: re-enable this line after debugging!
# trap '[ -d "$WORK_DIR" ] && rm -f "$WORK_DIR"' EXIT
WORK_DIR="$(mktemp -d)"

get_modgen() {
    # get and build modgen
    git clone --depth=1 https://github.com/conp-solutions/modularityGen.git modgen-src
    make -C modgen-src
    cp modgen-src/modgen .
}

# the given modgen params create CNFs with known MergeSat conflicts required
declare -a PARAMS=("-s 4900 -n 1000 -m 3000" "-s 2400 -n 15000 -m 72500" "-s 4900 -n 1000000 -m 3000000" "-s 3900 -n 10000 -m 38000" "-n 2200 -m 9086 -c 40 -s 158" "-n 45000 -m 171000 -c 40 -s 100")
declare -a EXPECTED_CONFLICTS=(35 856761 364 606458 445420 995426)
declare -i ERROR=0

# run remaining steps in created temporary directory
pushd "$WORK_DIR"
get_modgen

for index in $(seq 0 "$((${#PARAMS[@]} - 1))"); do
    echo "Run test $index: conflicts: ${CONFLICTS[$index]} params: ${PARAMS[$index]}"
    NEXT_PARAM="${PARAMS[$index]}"
    ./modgen $NEXT_PARAM >test.cnf
    EXPECTED="${EXPECTED_CONFLICTS[$index]}"
    CONFLICT_LIMIT=$((EXPECTED+20000))
    echo "Solve CNF with $(grep "p cnf" test.cnf) with expected conflicts $EXPECTED and conflict limit $CONFLICT_LIMIT"
    PRINTED_OUTPUT=0
    STATUS=0
    "$SOLVER" "test.cnf" -no-model -no-lib-math -con-lim="$CONFLICT_LIMIT" &>solver.output || STATUS="$?"
    CONFLICTS=$(awk '/c conflicts/ {print $4}' solver.output)
    if [ "$EXPECTED" -ne "$CONFLICTS" ]; then
        echo "ERROR: detected non-matching conflicts $CONFLICTS instead of $EXPECTED, output:"
        cat solver.output
        ERROR=1
        PRINTED_OUTPUT=1
    fi
    if [ "$STATUS" -ne 10 ] && [ "$STATUS" -ne 20 ]; then
        echo "ERROR: detected unexpected bad exit code $STATUS, output:"
        [ "$PRINTED_OUTPUT" -ne 1 ] && cat solver.output
        ERROR=1
        continue
    fi
done

if [ "$ERROR" -ne 0 ]; then
    exit "$ERROR"
fi
exit "$ERROR"
