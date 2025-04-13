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


void learncb(void *state, int *cls)
{
    printf("solver learned ");
    while (*cls) {
        printf("%d ", *cls);
        cls++;
    }
    printf("\n");
}


int sat_unsat_sat_cmergesat()
{
    printf("run sat_unsat_sat_cmergesat test\n");
    void *solver = cmergesat_init();
    cmergesat_set_learn(solver, 0, 2, learncb);

    for (int i = 1; i < 50; i += 2) {
        cmergesat_add(solver, i);
        cmergesat_add(solver, i + 1);
        cmergesat_add(solver, i + 2);
        cmergesat_add(solver, 0);
        cmergesat_add(solver, -i);
        cmergesat_add(solver, -(i + 1));
        cmergesat_add(solver, -(i + 2));
        cmergesat_add(solver, 0);
    }

    int ret = 0;
    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 10, "this formula needs to be satisfiable");

    for (int i = 1; i < 6; ++i) cmergesat_assume(solver, i);
    cmergesat_assume(solver, 0);

// re-enable if contrain is supported by backend again
#if 0
    int constraint_failed = 0;
    for (int i = 1; i < 6; ++i) cmergesat_constrain(solver, -i);
    cmergesat_constrain(solver, 0);

    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 20, "only positive assumptions falsify the formula");

    constraint_failed = cmergesat_constraint_failed(solver);
    printf("c constraint failed: %d\n", constraint_failed);
    test_assert(constraint_failed != 0, "constraint needs to fail");
#else
    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 20, "only positive assumptions falsify the formula");
#endif

    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 10, "without assumptions, the formula needs to be satisfiable again");

    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 10, "without assumptions, the formula needs to stay satisfiable");

    for (int i = 1; i < 6; ++i) cmergesat_assume(solver, -i);
    cmergesat_assume(solver, 0);
    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 20, "only negative assumptions falsify the formula");

    ret = cmergesat_solve(solver);
    test_assert(ret == 10, "without assumptions, the formula still needs to be satisfiable");

    printf("c solved with status %d\n", ret);
    cmergesat_release(solver);

    return 0;
}


int sat_unsat_sat_unsat_sat_cmergesat()
{
    printf("run sat_unsat_sat_unsat_sat_cmergesat test\n");
    void *solver = cmergesat_init();
    cmergesat_set_learn(solver, 0, 2, learncb);

    for (int i = 1; i < 50; i += 2) {
        cmergesat_add(solver, i);
        cmergesat_add(solver, i + 1);
        cmergesat_add(solver, i + 2);
        cmergesat_add(solver, 0);
        cmergesat_add(solver, -i);
        cmergesat_add(solver, -(i + 1));
        cmergesat_add(solver, -(i + 2));
        cmergesat_add(solver, 0);
    }

    int ret = 0;
    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 10, "this formula needs to be satisfiable");

    for (int i = 1; i < 6; ++i) cmergesat_assume(solver, i);
    cmergesat_assume(solver, 0);
    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 20, "only positive assumptions falsify the formula");

    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 10, "without assumptions, the formula needs to be satisfiable again");

    for (int i = 1; i < 6; ++i) cmergesat_assume(solver, -i);
    cmergesat_assume(solver, 0);
    ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 20, "only negative assumptions falsify the formula");

    ret = cmergesat_solve(solver);
    test_assert(ret == 10, "without assumptions, the formula still needs to be satisfiable");

    printf("c solved with status %d\n", ret);
    cmergesat_release(solver);

    return 0;
}


int basic_cmergesat()
{
    printf("run basic_cmergesat test\n");
    void *solver = cmergesat_init();
    cmergesat_set_learn(solver, 0, 2, learncb);

    for (int i = 1; i < 4000; ++i) {
        cmergesat_add(solver, i);
        cmergesat_add(solver, i + 1);
        cmergesat_add(solver, i + 2);
        cmergesat_add(solver, 0);
        cmergesat_add(solver, -i);
        cmergesat_add(solver, -(i + 1));
        cmergesat_add(solver, -(i + 2));
        cmergesat_add(solver, 0);
    }

    for (int i = 1; i < 200; ++i) {
        cmergesat_assume(solver, i % 2 == 1 ? -i : i);
    }

    int ret = cmergesat_solve(solver);
    printf("c solved with status %d\n", ret);
    cmergesat_release(solver);

    return 0;
}

int main(int argc, char **argv)
{
    int ret = basic_cmergesat();
    ret = sat_unsat_sat_unsat_sat_cmergesat() || ret;
    ret = sat_unsat_sat_cmergesat() || ret;
    return ret;
}
