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

#include "parallel/Sharing.h"

using namespace MERGESAT_NSPACE;

bool TestSolver::test_entrypoint()
{
    /* Make sure the values are as expected after solving */
    test_assert(okay(), "solver has to be okay");

    ClausePool pool;
    std::vector<int> shared_clause;
    shared_clause.push_back(-1);
    shared_clause.push_back(-2);
    shared_clause.push_back(-3);
    pool.add_shared_clause(shared_clause, 3);
    shared_clause.push_back(-4);
    pool.add_shared_clause(shared_clause, 10);
    test_assert(pool.size() == 2, "we added 2 clauses to the pool");

    for (int idx = 0; idx < pool.size(); ++idx) {
        const Clause &c = pool.getClause(idx);
        /* add the clause to the thread's solver, and check for conflicts via propagation */
        bool receive_causes_successful;
        receive_causes_successful = this->addLearnedClause(c, c.lbd());
        test_assert(receive_causes_successful, "adding the given clauses should be successful");
    }

    pool.reset();
    test_assert(pool.size() == 0, "resetting pool needs to result in empty pool");
    return true;
}

void test_share_and_receive()
{
    TestSolver solver;
    solver.verbosity = 0;

    while (solver.nVars() < 5) solver.newVar();

    solver.addClause(mkLit(1));
    solver.addClause(mkLit(2), mkLit(3));

    bool ret = solver.test_entrypoint();
    test_assert(ret, "receiving clauses should be successful");
}

int main(int argc, char **argv)
{
    test_share_and_receive();
    return 0;
}
