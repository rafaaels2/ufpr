
# Overview

The content of this directory is highly based on the scripts and tools
explained in the AWS repository to setup infrastructure for the SAT
competition.

https://github.com/aws-samples/aws-batch-comp-infrastructure-sample.git

# Running with Infrastructure

Use the script build_images.sh to build all relevant infrastructure.
Note: if not present, the base infrastructure will be built as well.

## Setup Docker Network

To setup the required docker network, run:

    docker network create mergesat-test

## Solving a Formula

To run the parallel solver with the infrastructure, use the script
run_parallel.sh

#### Example Command Sequence

Running the below commands results in building all relevant infrastructure
for running MergeSat in AWS' competition infrastructure as parallel solver,
as well as solving an example formula with the solver.

    ./build_images.sh
    ./run_parallel.sh satcomp-mergesat example_cnfs/rook-8-0-0.cnf 
