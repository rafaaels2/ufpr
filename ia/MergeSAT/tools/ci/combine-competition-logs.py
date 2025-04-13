#!/usr/bin/env python3
#
# Copyright (C) 2022, Norbert Manthey <nmanthey@conp-solutions.com>

import csv
import logging
import sys

# Create logger
log = logging.getLogger(__name__)


def main():

    logging.basicConfig(
        format="%(asctime)s,%(msecs)d %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s",
        datefmt="%Y-%m-%d:%H:%M:%S",
        level=logging.DEBUG,
    )

    solvers = set()
    benchmarks = set()
    benchmark_data = {}

    # benchmark;solver;sat-status;sat-status;time;cputime;memory;status,runlim-status
    for logfile in sys.argv[1:]:
        log.info("Process logfile '%s'", logfile)

        with open(logfile, newline="") as csvfile:
            reader = csv.DictReader(csvfile, delimiter=";")
            for row in reader:
                benchmark = row["benchmark"]
                if ".cnf" not in benchmark:
                    continue
                solver = row["solver"]
                if solver not in solvers:
                    solvers.add(solver)

                if benchmark not in benchmarks:
                    benchmarks.add(benchmark)

                if benchmark not in benchmark_data:
                    benchmark_data[benchmark] = {}
                benchmark_data[benchmark][solver] = {
                    "status": row["status"],
                    "time": row["time"],
                }

    log.info(
        "Detected %d benchmarks and %d solvers\n", len(benchmark_data), len(solvers)
    )

    sorted_solvers = sorted([x for x in solvers])

    out_string = f"benchmark "
    for solver in sorted_solvers:
        out_string += "{} ".format(solver.replace(" ", "_"))
    print(out_string)

    for benchmark in benchmarks:
        out_string = f"{benchmark} "
        for solver in sorted_solvers:
            d = benchmark_data[benchmark][solver]
            if d["status"] == "ok":
                out_string += "{} ".format(str(d["time"]))
            else:
                out_string += "- "
        print(out_string)


if __name__ == "__main__":
    sys.exit(main())
