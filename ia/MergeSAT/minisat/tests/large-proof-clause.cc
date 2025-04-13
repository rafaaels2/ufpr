/***************************************************************************[large-proof-clause.cc]
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
    // solver okay
    test_assert(okay(), "solver has to be okay");

    // solve formula
    vec<Lit> dummy;
    lbool ret = solveLimited(dummy);
    test_assert(ret == l_True, "The 1 clause formula has to be satisfiable");

    return true;
}

void test_very_large_proof_clause()
{
    TestSolver solver;
    solver.verbosity = 0;
    solver.proof.init("/dev/null", true, false);
    test_assert(solver.proof.enabled(), "Need a proof filehandle");

    // create the large proof clause
    vec<Lit> proof_clause;
    proof_clause.push(mkLit(1));

    for (int i = 0; i < 4 * 1024 * 1024; ++i) proof_clause.push(mkLit(2));

    // add sufficient variables to the solver
    while (solver.nVars() < 5) solver.newVar();

    // add large clause to solver after activating proof
    solver.parsing = true;
    solver.addClause(proof_clause);
    solver.parsing = false;

    bool ret = solver.test_entrypoint();
    test_assert(ret, "receiving clauses should be successful");

    solver.proof.finalize();
}

int main(int argc, char **argv)
{
    test_very_large_proof_clause();
    return 0;
}
