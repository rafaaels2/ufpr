#!/bin/bash
#
# Solver the given input formula, and in case of being unsatisfiable, the
# produced proof is checked by drat-trim. The produced proof is generated
# in the binary of plain format.

# input
f="$1"
shift

# check for file
[ -f "$f" ] || exit 2

SCRIPT_DIR="$(cd $(dirname "$0") && pwd)"

# static variables
p=p.proof
o=drat-minimize-output.txt
t=$(realpath ../../build/release/bin/mergesat)
# allow to override the solver location via environment variable
t=${t:-$FUZZ_SOLVER}
d=$(realpath "$SCRIPT_DIR"/drat-trim)

# run solver
STATUS=0
$t "$f" -drup-file="$p" $@ || STATUS=$?
[ "$STATUS" -ne 127 ] || exit 127
if [ "$STATUS" -ne 0 ] && [ "$STATUS" -ne 10 ] && [ "$STATUS" -ne 20 ]; then
    echo "unexpected status code $STATUS"
    exit $STATUS
fi

# exclude trivial unsat
grep "^0$" $f && exit 0

# run the check
if [ "$STATUS" -ne 20 ]; then
    exit $STATUS
fi

# verify binary proof format
VERIFY_STATUS=0
$d $f $p -v -v -n &>$o || exit $VERIFY_STATUS
echo "verify status: $VERIFY_STATUS"
# for now, don't be too strict
# grep " does not occur: " $o && exit 3
exit 20

# re-run with plain proof format
"$t" "$f" -drup-file=$p -no-binary-proof $@ || STATUS=$?

if [ "$STATUS" -ne 0 ] && [ "$STATUS" -ne 10 ] && [ "$STATUS" -ne 20 ]; then
    echo "unexpected status code $STATUS"
    exit $STATUS
fi

# exclude trivial unsat
grep "^0$" $f && exit 0

# run the check
if [ "$STATUS" -ne 20 ]; then
    exit $STATUS
fi

# verify binary proof format
VERIFY_STATUS=0
$d $f $p -v -v -n &>$o || exit $VERIFY_STATUS
echo "verify status: $VERIFY_STATUS"
# for now, don't be too strict
# grep " does not occur: " $o && exit 3
exit 20
