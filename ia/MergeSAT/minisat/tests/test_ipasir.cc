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

#include "core/ipasir.h"
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

int sat_unsat_sat_unsat_sat_ipasir()
{
    printf("run sat_unsat_sat_unsat_sat_ipasir test\n");
    void *solver = ipasir_init();
    ipasir_set_learn(solver, 0, 2, learncb);

    for (int i = 1; i < 50; i += 2) {
        ipasir_add(solver, i);
        ipasir_add(solver, i + 1);
        ipasir_add(solver, i + 2);
        ipasir_add(solver, 0);
        ipasir_add(solver, -i);
        ipasir_add(solver, -(i + 1));
        ipasir_add(solver, -(i + 2));
        ipasir_add(solver, 0);
    }

    int ret = 0;
    ret = ipasir_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 10, "this formula needs to be satisfiable");

    for (int i = 1; i < 6; ++i) ipasir_assume(solver, i);
    ipasir_assume(solver, 0);
    ret = ipasir_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 20, "only positive assumptions falsify the formula");

    ret = ipasir_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 10, "without assumptions, the formula needs to be satisfiable again");

    for (int i = 1; i < 6; ++i) ipasir_assume(solver, -i);
    ipasir_assume(solver, 0);
    ret = ipasir_solve(solver);
    printf("c solved with status %d\n", ret);
    test_assert(ret == 20, "only negative assumptions falsify the formula");

    ret = ipasir_solve(solver);
    test_assert(ret == 10, "without assumptions, the formula still needs to be satisfiable");

    printf("c solved with status %d\n", ret);
    ipasir_release(solver);

    return 0;
}


int basic_ipasir()
{
    printf("run basic_ipasir test\n");
    void *solver = ipasir_init();
    ipasir_set_learn(solver, 0, 2, learncb);

    for (int i = 1; i < 4000; ++i) {
        ipasir_add(solver, i);
        ipasir_add(solver, i + 1);
        ipasir_add(solver, i + 2);
        ipasir_add(solver, 0);
        ipasir_add(solver, -i);
        ipasir_add(solver, -(i + 1));
        ipasir_add(solver, -(i + 2));
        ipasir_add(solver, 0);
    }

    for (int i = 1; i < 200; ++i) {
        ipasir_assume(solver, i % 2 == 1 ? -i : i);
    }

    int ret = ipasir_solve(solver);
    printf("c solved with status %d\n", ret);
    ipasir_release(solver);

    return 0;
}

int main(int argc, char **argv)
{
    int ret = basic_ipasir();
    ret = sat_unsat_sat_unsat_sat_ipasir() || ret;
    return ret;
}
