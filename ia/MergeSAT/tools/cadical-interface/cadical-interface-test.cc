/* Run simple test case for C slver interface. */
#include <cassert>
#include <iostream>
#include <vector>

using namespace std;

/* Flip, depending on whether we use CaDiCaL or MergeSat */
#ifndef MERGESAT_CADICAL
#include <ccadical.h>
#else
#include "core/ipasir.h"
#include "simp/ccadical-mergesat-bridge.h"
#endif

void binary_unsat ()
{
    cout << endl << endl << "run binary_unsat test" << endl;
    cout << "initializing cadical " << ccadical_signature() << endl;

    CCaDiCaL *c = ccadical_init();
    /* simple unsat formula */
    ccadical_add(c, 1);
    ccadical_add(c, -2);
    ccadical_add(c, 0);
    ccadical_add(c, -1);
    ccadical_add(c, -2);
    ccadical_add(c, 0);
    ccadical_add(c, -1);
    ccadical_add(c, 2);
    ccadical_add(c, 0);
    ccadical_add(c, 1);
    ccadical_add(c, 2);
    ccadical_add(c, 0);

    int sret = ccadical_simplify (c);
    cout << "simplification ret: " << sret << endl;
    assert (sret == 20 && "unsat on this simple formula");

    int ret = ccadical_solve(c);
    cout << "solved with result " << ret << endl;
    assert (ret == 20 && "unsat");

    cout << "active: " << ccadical_active (c) << " irredundant: " << ccadical_irredundant (c) << endl;
    cout << " fixed 1: " << ccadical_fixed (c, 1)
         << " fixed n1: " << ccadical_fixed (c, -1)
         << " fixed 2: " << ccadical_fixed (c, 2)
         << " fixed n2: " << ccadical_fixed (c, -2)
         << endl;

    ccadical_freeze (c, 2);
    cout << "n2 frozen: " << ccadical_frozen (c, -2) << endl;
    assert (ccadical_frozen (c, -2) == 1);
    ccadical_melt (c, -2);
    cout << "n2 frozen: " << ccadical_frozen (c, 2) << endl;
    assert (ccadical_frozen (c, 2) == 0);

    ccadical_release(c);
}


void constrain_unsat_multi ()
{
    cout << endl << endl << "run constrain_unsat_multi test" << endl;
    cout << "using cadical " << ccadical_signature() << endl;

    struct test_case {
        vector<int> lits;
        vector<int> constraint;
        vector<int> assumption;
        vector<int> expected_conflict;
        int expected_fail = 1;
        int expected_exit_code = 20;
    };

    vector<test_case> testcases = {
        {
            {1,-2,0,-1,-2,0,-1,2,0},  // sat formula
            {1,2,0},  // constraint
            {-1},  // assumptions
            {-1}, // assumption conflict
            1,  // expected constraint failure
            20 // expected exit code
        },
        {
            {1,-2,0,-1,-2,0,-1,2,0,-1,-2,0},  // unsat formula
            {1,2,0},  // constraint
            {},  // assumptions
            {}, // no conflict
            1,  // expected constraint failure
            20 // expected exit code
        },
        {
            {1,-2,0,-1,-2,0},  // sat formula
            {0},  // empty constraint
            {},  // empty assumptions
            {}, // no conflict
            1,  // expected constraint failure
            20 // expected exit code
        },
        {
            {1,-2,0,-1,-2,0},  // sat formula
            {2, 0},  // conflicting constraint
            {},  // empty assumptions
            {}, // no conflict
            1,  // expected constraint failure
            20 // expected exit code
        },
        {
            {1,-2,0,-1,-2,0},  // sat formula
            {2,3, 0},  // constraint with fresh variable
            {},  // empty assumptions
            {}, // no conflict
            0,  // expected constraint failure
            10 // expected exit code
        },
        {
            {1,-2,0,-1,-2,0},  // sat formula
            {},  // no constraint
            {},  // empty assumptions
            {}, // no conflict
            0,  // expected constraint failure
            10 // expected exit code
        },
        {
            {1,-2,0,-1,-2,0},  // sat formula
            {-2, 0},  // constraint matching assumptions
            {1},  // empty assumptions
            {}, // no conflict
            0,  // expected constraint failure
            10 // expected exit code
        },
        {
            {1,-2,0,-1,-2,0},  // sat formula
            {-1,2, 0},  // inverse assumptions
            {1,-2},  // empty assumptions
            {1}, // first assumption conflicts
            1,  // expected constraint failure
            20 // expected exit code
        }

    };

    for(size_t i = 0 ; i < testcases.size(); ++i)
    {
        test_case &T = testcases[i];
        cout << "    testcase " << i << endl;
        CCaDiCaL *c = ccadical_init();

        for(auto lit : T.lits) ccadical_add(c, lit);
        for(auto lit : T.constraint) ccadical_constrain(c, lit);
        for(auto lit : T.assumption) ccadical_assume(c, lit);
        int ret = ccadical_solve(c);
        assert (ret == T.expected_exit_code && "Need to match expected exit code of test case");
        if (ret == 20) {
            int failed = ccadical_constraint_failed(c);
            assert (failed == T.expected_fail && "Need to match expected constrain failure exit code");
            for(auto lit : T.expected_conflict) {
                int ret_failed = ccadical_failed(c, lit);
                assert(ret_failed && "expected literal as part of conflict is not part of actual conflict");
            }
        }
        ccadical_release(c);
    }

}

void constrain_unsat ()
{
    cout << endl << endl << "run constraint_unsat test" << endl;
    cout << "initializing cadical " << ccadical_signature() << endl;

    CCaDiCaL *c = ccadical_init();
    /* simple unsat formula */
    ccadical_add(c, 1);
    ccadical_add(c, -2);
    ccadical_add(c, 0);
    ccadical_add(c, -1);
    ccadical_add(c, -2);
    ccadical_add(c, 0);
    ccadical_add(c, -1);
    ccadical_add(c, 2);
    ccadical_add(c, 0);

    /* require to additionally satisfy this clause */
    ccadical_constrain(c, 1);
    ccadical_constrain(c, 2);
    ccadical_constrain(c, 0);

    int sret = ccadical_simplify (c);
    cout << "simplification ret: " << sret << endl;
    assert (sret != 20 && "this formula should not be unsatisfiable yet");

    int ret = ccadical_solve(c);
    cout << "solved with result " << ret << endl;
    assert (ret == 20 && "unsat");

    int failed = ccadical_constraint_failed(c);
    assert (failed == 1 && "constrain clause cannot be satisfied");

    /* assume a unit to make the formula unsatisfiable */
    ccadical_assume(c, -1);
    /* require to additionally satisfy this clause */
    ccadical_constrain(c, 1);
    ccadical_constrain(c, 2);
    ccadical_constrain(c, 0);

    ret = ccadical_solve(c);
    cout << "solved with result " << ret << endl;
    assert (ret == 20 && "unsat, assumption with constrain_clause do not work");

    failed = ccadical_constraint_failed(c);
    assert (failed == 1 && "constrain clause cannot be satisfied");

    /* assume a unit to make the formula unsatisfiable */
    ccadical_assume(c, -1);
    ret = ccadical_solve(c);
    cout << "solved with result " << ret << endl;
    assert (ret == 10 && "sat");

    ccadical_release(c);
}

void simple_unsat ()
{
    cout << endl << endl << "run simple_unsat test" << endl;
    cout << "initializing cadical " << ccadical_signature() << endl;

    CCaDiCaL *c = ccadical_init();
    ccadical_add(c, 1);
    ccadical_add(c, 0);
    ccadical_add(c, -2);
    ccadical_add(c, 0);
    ccadical_assume(c, -1);
    // ccadical_assume(c, 0);
    int ret = ccadical_solve(c);
    cout << "solved with result " << ret << endl;
    assert (ret == 20 && "unsat");
    int f1 = ccadical_failed (c, 1);
    int fn1 = ccadical_failed (c, -1);
    cout << "failed 1: " << f1 << " failed not1: " << fn1 << endl;
    assert(fn1 == 1);
    assert(f1 == 0);

    int f2 = ccadical_failed (c, 2);
    int fn2 = ccadical_failed (c, -2);
    cout << "failed 2: " << f2 << " failed not2: " << fn2 << endl;

    cout << "active: " << ccadical_active (c) << " irredundant: " << ccadical_irredundant (c) << endl;
    cout << " fixed 1: " << ccadical_fixed (c, 1)
         << " fixed n1: " << ccadical_fixed (c, -1)
         << " fixed 2: " << ccadical_fixed (c, 2)
         << " fixed n2: " << ccadical_fixed (c, -2)
         << endl;

    ccadical_freeze (c, 2);
    cout << "n2 frozen: " << ccadical_frozen (c, -2) << endl;
    assert (ccadical_frozen (c, -2) == 1);
    ccadical_melt (c, -2);
    cout << "n2 frozen: " << ccadical_frozen (c, 2) << endl;
    assert (ccadical_frozen (c, 2) == 0);

    int sret = ccadical_simplify (c);
    cout << "simplification ret: " << sret << endl;
    ccadical_print_statistics(c);
    ccadical_release(c);
}

int main () {
    /* run all tests we have */
    simple_unsat ();
    binary_unsat ();
    constrain_unsat ();
    constrain_unsat_multi ();
    return 0;
}