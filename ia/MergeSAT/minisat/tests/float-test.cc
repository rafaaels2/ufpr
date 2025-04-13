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

#include "math.h"

using namespace MERGESAT_NSPACE;

bool TestSolver::test_entrypoint() { return true; }

bool test_iterative_pow()
{
    PowFixedBase twoPow(2, false);
    test_assert(twoPow.pow(1) == 2, "Pow 2^1 == 2");
    test_assert(twoPow.pow(2) == 4, "Pow 2^2 == 4");
    test_assert(twoPow.pow(3) == 8, "Pow 2^3 == 8");
    test_assert(twoPow.pow(4) == 16, "Pow 2^4 == 16");
    printf("bin pow 2^4: %lf\n", twoPow.pow(4));

    PowFixedBase fourPow(4, false);
    test_assert(fourPow.pow(0) == 1, "Pow 4^0 == 1");
    test_assert(fourPow.pow(1) == 4, "Pow 4^1 == 4");
    test_assert(fourPow.pow(2) == 16, "Pow 4^2 == 16");
    test_assert(fourPow.pow(3) == 64, "Pow 4^3 == 64");
    printf("bin pow 4^3: %lf\n", fourPow.pow(3));


    PowFixedBase onethreePow(13, false);
    test_assert(onethreePow.pow(5) == 371293, "Pow 13^5 == 371293");
    test_assert(onethreePow.pow(9) == 10604499373, "Pow 13^9 == 10604499373");
    test_assert(onethreePow.pow(11) == 1792160394037, "Pow 13^11 == 1792160394037");

    return true;
}

int main(int argc, char **argv)
{
    test_iterative_pow();
    return 0;
}
