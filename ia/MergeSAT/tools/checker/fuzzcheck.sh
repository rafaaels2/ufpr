#!/bin/bash

#
# create cnf files via fuzzing, check with verifier, apply cnfdd if required
#

prg=$1

# this can be initialized to a max. size of an unreduced buggy formula
bestcls=50000

cnf=/dev/shm/uzz-$$.cnf
sol=/dev/shm/fuzz-$$.sol
err=/dev/shm/fuzz-$$.err
out=/dev/shm/fuzz-$$.out
log=fuzz-$$.log
rm -f $log $cnf $sol
echo "[fuzz] running $prg"
echo "[fuzz] logging $log"
echo "run $1 $2" >$log
echo "[fuzz] run with pid $$"
echo "[fuzz] started at $(date)"
i=0

# set limit for number of instances to be solved
limit=1
if [ "x$2" != "x" ]; then
    limit=$2
fi
test=0

declare -i status=0
declare -i found_sat=0
declare -i found_unsat=0

while [ "$test" -lt "$limit" ]; do
    rm -f $cnf
    seed="$(echo $(($(date +%s%N) / 1000000)))"
    cls=""
    while [ -z "$cls" ]; do
        cls="$(bash -c 'echo $((6000 + $(date +%4N) ))' 2>/dev/null)"
    done
    modgen_cli_args="-s $seed -n 2800 -m $cls"
    ./modgen $modgen_cli_args >$cnf
    # control whether algorithm should sleep
    seed=$(grep 'c   value seed =' $cnf | head -1 | awk '{print $NF}')
    head="$(awk '/p cnf /{print $3, $4}' $cnf)"
    [ -n "${VERBOSE_FUZZING:-}" ] && printf "%d %16d          %6d    %6d               \r" "$i" "$seed" $head
    i=$(expr $i + 1)
    rm -f $sol

    # set limit for number of instances to be solved
    if [ "x$2" != "x" ]; then
        test=$(($test + 1))
    fi

    thiscls=$(awk '/p cnf /{print $4}' $cnf)
    if [ "$bestcls" -ne "-1" ]; then
        if [ "$bestcls" -le "$thiscls" ]; then
            continue
        fi
    fi

    ./toolCheck.sh $prg $cnf >$out 2>$err
    res=$?
    #  echo "result $res"
    case $res in
    124)
        echo "($SECONDS s) timeout with $seed" >$log
        mv $cnf timeout-$seed.cnf
        continue
        ;;
    10)
        found_sat=$((found_sat + 1))
        continue
        ;;
    20)
        found_unsat=$((found_unsat + 1))
        continue
        ;;
    15) ;;

    25) ;;

    esac

    status=1

    head="$(awk '/p cnf /{print $3, $4}' $cnf)"
    echo "($SECONDS s) [fuzz] bug-$seed $head             with exit code $res"
    echo "($SECONDS s) To reproduce, run modgen: './modgen $modgen_cli_args > reproduce.cnf' and '$prg reproduce.cnf'"
    echo $seed >>$log
    echo "($SECONDS s) out"
    cat $out
    echo "($SECONDS s) err"
    cat $err

    #
    # consider only bugs that are smaller then the ones we found already
    #
    if [ "$bestcls" -eq "-1" ]; then
        bestcls=$thiscls
        echo "($SECONDS s) init bestcls with $bestcls"
    elif [ "$bestcls" -le $thiscls ]; then
        echo "($SECONDS s) reject new bug with $thiscls clauses"
        continue
    fi

    bestcls="$thiscls" # store better clause count!
    echo "($SECONDS s) set bestcls to $bestcls"
    red=red-$seed.cnf
    bug=bug-$seed.cnf
    mv $cnf $bug

    if [ -f "$red" ]; then
        head="$(awk '/p cnf /{print $3, $4}' $red)"
        echo "($SECONDS s) [fuzz] $red $head"
    fi
    gzip "$bug"
done

echo "($SECONDS s) sat: $found_sat unsat: $found_unsat"
echo "($SECONDS s) did $test out of $limit tests"

exit $status
