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

#include "tests/TestSolver.h"

using namespace MERGESAT_NSPACE;


void test_parallel_proof()
{
    /* easy to read, but check incoming clauses */
    bool binary_format = false;
    bool check_proof = true;

    Proof proof;
    proof.init("/dev/null", binary_format, check_proof);

    /* Create sub proofs and register with parent */
    Proof subproof2;
    Proof subproof1;

    subproof2.init(nullptr, binary_format, check_proof, &proof, true);
    subproof1.init(nullptr, binary_format, check_proof, &proof, true);

    /* add input formula */
    vec<Lit> c;
    c.clear();
    c.push(mkLit(1));
    c.push(mkLit(2));
    proof.addParsedclause(c);
    c.clear();
    c.push(mkLit(1, true));
    c.push(mkLit(2));
    proof.addParsedclause(c);
    c.clear();
    c.push(mkLit(2, true));
    c.push(mkLit(3));
    proof.addParsedclause(c);
    c.clear();
    c.push(mkLit(3, true));
    c.push(mkLit(4));
    proof.addParsedclause(c);

    /* make the formula unsat */
    c.clear();
    c.push(mkLit(4, true));
    proof.addParsedclause(c);

    /* have 1st subproof add a first clause */
    c.clear();
    c.push(mkLit(1));
    c.push(mkLit(3));
    subproof1.addClause('a', c);
    subproof1.flush();

    /* have 2nd subproof add a first clause */
    c.clear();
    c.push(mkLit(2));
    c.push(mkLit(4));
    subproof2.addClause('a', c);

    /* have 1st subproof add a unit clause */
    c.clear();
    c.push(mkLit(2));
    subproof1.addClause('a', c);
    subproof1.flush();

    /* have 2nd subproof add a unit clause */
    c.clear();
    c.push(mkLit(4, true));
    subproof2.addClause('a', c);


    /* finish by adding an empty clause to the proof */
    proof.addClause('a', vec<Lit>());
    proof.finalize();
}

int main(int argc, char **argv)
{
    test_parallel_proof();
    return 0;
}
