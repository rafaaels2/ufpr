/*************************************************************************[online-proof-checker.cc]
Copyright (c) 2022, Norbert Manthey

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

#include "simp/cmergesat.h"
#include "tests/TestSolver.h"

using namespace MERGESAT_NSPACE;


int sat_unsat_sat_unsat_sat_solver()
{
    printf("run sat_unsat_sat_unsat_sat_cmergesat test\n");
    SimpSolver solver;
    solver.eliminate(true);

    /* add all variables in advance! */
    while (solver.nVars() < var(mkLit(50 + 2))) solver.newVar();

    for (int i = 1; i < 50; i += 2) {
        solver.addClause(mkLit(i), mkLit(i + 1), mkLit(i + 2));
        solver.addClause(mkLit(i, true), mkLit(i + 1, true), mkLit(i + 2, true));
    }

    solver.newVar();

    lbool ret = l_Undef;
    bool r = false;
    printf("\n\ninitial SAT call\n");
    r = solver.solve();
    printf("c solved with status %d\n", (int)r);
    test_assert(r == true, "this formula needs to be satisfiable");

    solver.newVar();

    printf("\n\nssumption based UNSAT call\n");
    vec<Lit> assumps;
    assumps.clear();
    for (int i = 1; i < 7; ++i) assumps.push(mkLit(i));
    ret = solver.solveLimited(assumps);
    printf("c solved with status False: %d\n", ret == l_False);
    test_assert(ret == l_False, "only positive assumptions falsify the formula");

    solver.newVar();

    printf("\n\nsecond SAT call\n");
    r = solver.solve();
    printf("c solved with status %d\n", (int)r);
    test_assert(r == true, "this formula needs to be satisfiable again");

    solver.newVar();

    printf("\n\nsecond UNSAT call\n");
    assumps.clear();
    for (int i = 1; i < 7; ++i) assumps.push(mkLit(i, true));
    ret = solver.solveLimited(assumps);
    printf("c solved with status False: %d\n", ret == l_False);
    test_assert(ret == l_False, "only negative assumptions falsify the formula");

    solver.newVar();

    ret = solver.solveLimited(assumps);
    printf("c solved with status False: %d\n", ret == l_False);
    test_assert(ret == l_False, "only negative assumptions falsify the formula");

    vec<Lit> constain;
    constain.push(mkLit(1));
    constain.push(mkLit(2));
    constain.push(mkLit(3, true));
    solver.addConstrainClause(constain);

    printf("\n\nalmost final SAT call\n");
    r = solver.solve();
    printf("c solved with status %d\n", (int)r);
    test_assert(r == true, "this formula needs to be satisfiable with constrain clause");

    printf("\n\nfinal SAT call\n");
    r = solver.solve();
    printf("c solved with status %d\n", (int)r);
    test_assert(r == true, "this formula needs to be satisfiable with constrain clause");


    printf("\n\nconstraint + assume SAT call\n");
    assumps.clear();
    assumps.push(mkLit(1, true));
    assumps.push(mkLit(2, true));
    solver.addConstrainClause(constain);
    ret = solver.solveLimited(assumps);
    printf("c solved with status False: %d\n", ret == l_False);
    test_assert(ret == l_False, "only negative assumptions falsify the formula");
    test_assert(solver.failed_constraint(), "constrain clause must be marked as failed");
    test_assert(solver.failed_constraint(), "constrain clause must stay marked as failed");
    solver.newVar();
    test_assert(solver.failed_constraint(), "constrain clause must stay marked as failed even with new variables");

    printf("\n\npost-constraint + assume SAT call\n");
    ret = solver.solveLimited(assumps);
    printf("c solved with status False: %d\n", ret == l_False);
    test_assert(ret == l_True, "only small assumptions should allow to satisfy the formula");
    test_assert(!solver.failed_constraint(), "constrain clause must not be marked as failed");

    return 0;
}

int main(int argc, char **argv)
{
    int ret = sat_unsat_sat_unsat_sat_solver();
    return ret;
}
