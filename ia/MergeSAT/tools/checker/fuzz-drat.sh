#!/bin/bash
#
# Generate solvers and check whether the DRAT proof is valid.
# Solving and verifying is done in the script check-drat.sh

LIMIT=$1
TIMEOUT=$2

[ -n "$TIMEOUT" ] || exit 2

SCRIPT_DIR="$(cd $(dirname "$0") && pwd)"

c="$SCRIPT_DIR/modgen"

I=0
f=fuzz_input.cnf
declare -i found_sat=0
declare -i found_unsat=0

FAILED=0
while true; do
    I=$((I + 1))
    [ "$LIMIT" -ge "$I" ] || break
    # echo "[$SECONDS] run $I ..."
    seed="$(echo $(($(date +%s%N) / 1000000)))"
    cls=""
    while [ -z "$cls" ]; do
        cls="$(bash -c 'echo $((9000 + $(date +%4N) ))' 2>/dev/null)"
    done
    modgen_cli_args="-s $seed -n 3500 -m $cls"
    $c $modgen_cli_args >$f
    STATUS=0
    timeout "$TIMEOUT" "$SCRIPT_DIR"/check-drat.sh $f &>output.log || STATUS=$?
    # ignore time outs
    [ "$STATUS" -ne 124 ] || continue

    # make failures permanent
    if [ "$STATUS" -ne 0 ] && [ "$STATUS" -ne 10 ] && [ "$STATUS" -ne 20 ]; then
        echo "[$SECONDS] failed for try $I with status $STATUS and header $(grep "p cnf" $f) and generator: $c $modgen_cli_args"
        mv $f fuzz_input.$I.cnf
        mv output.log output.$I.log
        FAILED=$((FAILED + 1))
    fi
    [ "$STATUS" -eq 10 ] && found_sat=$((found_sat + 1))
    [ "$STATUS" -eq 20 ] && found_unsat=$((found_unsat + 1))
done
echo "failed $FAILED out of $LIMIT"
echo "($SECONDS s) sat: $found_sat unsat: $found_unsat"
[ "$FAILED" -eq 0 ] || exit 1
exit 0
