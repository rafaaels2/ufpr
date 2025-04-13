/***********************************************************************************[sat-simple.cc]
Copyright (c) 2021, Norbert Manthey

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "tests/TestSolver.h"

using namespace MERGESAT_NSPACE;

bool TestSolver::test_entrypoint()
{
    /* Make sure the values are as expected after solving */
    test_assert(okay(), "solver has to be okay");

    rebuildOrderHeap();

    std::cout << "c testing creating plain partitions" << std::endl;
    if (true) {
        for (int partitions = 1; partitions < 10; partitions++) {
            vec<Lit> assumptions;
            vec<Lit> partitionClauses;
            lbool ret = partitionFormula(assumptions, partitions, partitionClauses);
            std::cout << "c created " << partitions << " partitions vars=" << nVars() << " with ret=" << ret << std::endl;
            printPartitionClauses(partitionClauses);
            test_assert(ret == l_True, "Creating partitions on base formula is possible");
        }
    }

    std::cout << "c testing creating partitions under assumptions" << std::endl;
    for (int partitions = 3; partitions < 5; partitions++) {
        vec<Lit> assumptions;
        assumptions.push(mkLit(0));
        vec<Lit> partitionClauses;
        lbool ret = partitionFormula(assumptions, partitions, partitionClauses);
        std::cout << "c created " << partitions << " partitions vars=" << nVars() << " with ret=" << ret << std::endl;
        printPartitionClauses(partitionClauses);
        test_assert(ret == l_True, "Creating partitions on base formula is possible");
    }

    std::cout << "c testing creating partitions under negated assumptions" << std::endl;
    for (int partitions = 3; partitions < 5; partitions++) {
        vec<Lit> assumptions;
        assumptions.push(~mkLit(0));
        vec<Lit> partitionClauses;
        lbool ret = partitionFormula(assumptions, partitions, partitionClauses);
        std::cout << "c created " << partitions << " partitions vars=" << nVars() << " with ret=" << ret << std::endl;
        printPartitionClauses(partitionClauses);
        test_assert(ret == l_True, "Creating partitions on base formula is possible");
    }

    return true;
}

void test_formula_partitioning()
{
    TestSolver solver;
    solver.verbosity = 0;

    // 3 variables
    solver.newVar();
    solver.newVar();
    solver.newVar();

    vec<Lit> clause;
    clause.push(mkLit(1));
    clause.push(mkLit(2));
    solver.addClause(clause);

    clause.clear();
    clause.push(mkLit(0));
    clause.push(~mkLit(1));
    solver.addClause(clause);

    solver.test_entrypoint();
}

int main(int argc, char **argv)
{
    test_formula_partitioning();
    return 0;
}
