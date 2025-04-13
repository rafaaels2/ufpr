#!/bin/bash
#
# This script runs a set of configurations on a set of randomly generated CNF
# formulas.

# Formulas to test per iteration
declare -i FORMULA_PER_CONFIG_RELEASE=3000
declare -i FORMULA_PER_CONFIG_DEBUG=400

FUZZ_MULTIPLY=${FUZZ_MULTIPLY:-1}
FORMULA_PER_CONFIG_RELEASE=$((FORMULA_PER_CONFIG_RELEASE * FUZZ_MULTIPLY))
FORMULA_PER_CONFIG_DEBUG=$((FORMULA_PER_CONFIG_DEBUG * FUZZ_MULTIPLY))

# List of configurations to consider
CONFIGURATIONS=("../../build/release/bin/mergesat"
    "../../build/debug/bin/mergesat -check-par-sat -check-proof=1 -cpu-lim=4 -drup-file=proof.drat -verb=0 -VSIDS-init-lim=100 -lcm-delay=10 -lcm-delay-inc=10 -rfirst=2"
    "../../build/debug/bin/mergesat -check-par-sat -check-proof=1 -cpu-lim=4 -drup-file=proof.drat -verb=0 -no-pre -VSIDS-init-lim=100 -lcm-delay=10 -lcm-delay-inc=10 -rfirst=2"
    "../../build/debug/bin/mergesat -check-par-sat"
    "../../build/debug/bin/mergesat -check-par-sat -no-pre"
    "../../build/debug/bin/mergesat -check-par-sat -rtype=4 -VSIDS-lim=50 -VSIDS-init-lim=50"
    "../../build/debug/bin/mergesat -check-par-sat -verb=2 -cores=1"
    "../../build/release/bin/mergesat -check-par-sat -la-dec-level=50 -la-every=2"
    "../../build/release/bin/mergesat -check-par-sat -i-uip -i-mini"
    "../../build/release/bin/mergesat -check-par-sat -all-strength-max=5"
    "../../build/release/bin/mergesat -check-par-sat -otfss-lbd=6"
    "../../build/release/bin/mergesat -check-par-sat -i-uip -i-mini -drup-file=proof.drat -check-proof=1"
    "../../build/release/bin/mergesat -check-par-sat -na-attempt-every=1 -drup-file=proof.drat -check-proof=1"
    "../../build/debug/bin/mergesat -check-par-sat -check-proof=1 -drup-file=proof.drat"
    "../../build/debug/bin/mergesat -bve-gates -bve-sem-gates -check-proof=1 -drup-file=proof.drat"
    "../../build/debug/bin/mergesat -cores=1 -bve-gates -bve-sem-gates -check-proof=1 -drup-file=proof.drat"
    "../../build/debug/bin/mergesat -bve-gates -check-par-sat -check-proof=1 -drup-file=proof.drat"
    "../../build/release/bin/mergesat -check-par-sat -cores=4 -cpu-lim=30 -mem-lim=2048"
    "../../build/release/bin/mergesat -check-par-sat -cores=8 -cpu-lim=30 -mem-lim=2048"
    "../../build/release/bin/mergesat -check-par-sat -cores=4 -cpu-lim=30 -mem-lim=2048 -drup-file=proof.drat -check-proof=1"
    )

# Current directory
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

cd "$SCRIPT_DIR"

declare -i overall_status=0
for CONFIG in "${CONFIGURATIONS[@]}"; do
    echo "test configuration: $CONFIG"

    echo "$CONFIG \$1" >check-solver-config.sh
    chmod u+x check-solver-config.sh

    status=0

    # select number of formulas to test based on configuration
    FORMULA_PER_CONFIG="$FORMULA_PER_CONFIG_RELEASE"
    grep -q "debug" check-solver-config.sh && FORMULA_PER_CONFIG="$FORMULA_PER_CONFIG_DEBUG"

    ./fuzzcheck.sh ./check-solver-config.sh "$FORMULA_PER_CONFIG" || status=$?
    if [ "$status" -ne 0 ]; then
        echo "configuration failed!"
        overall_status=$status
    fi
done

exit $overall_status
