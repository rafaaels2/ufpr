/**************************************************************************************[IntTypes.h]
Copyright (c) 2009-2010, Niklas Sorensson

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

#ifndef MergeSat_IntTypes_h
#define MergeSat_IntTypes_h

#ifdef __sun
// Not sure if there are newer versions that support C99 headers. The
// needed features are implemented in the headers below though:

#include <sys/int_fmtio.h>
#include <sys/int_limits.h>
#include <sys/int_types.h>

#else

#include <inttypes.h>
#include <stdint.h>

#endif

#include <limits.h>
#include <math.h>

//=================================================================================================

inline uint64_t log2_64(uint64_t x)
{
    // calculate ld based on number of leading zeros
    return (uint64_t)(8 * sizeof(x) - __builtin_clzll((x)) - 1);
}

class PowFixedBase
{
    double base;
    bool useLegacy = false;

    public:
    PowFixedBase(double b, bool legacy = false) : base(b), useLegacy(legacy) {}

    double pow(unsigned exponent)
    {
        /* library */
        if (useLegacy) return ::pow(base, exponent);

        /* trivial case */
        if (!exponent) return (double)1;

        /* get binary representation */
        unsigned binary = ~(~0U >> 1);
        while (!(exponent & binary)) binary >>= 1;
        double intermediate = base;

        /* only as long as there are bits */
        while (binary >>= 1) {
            /* square for next bit in binary representation */
            intermediate *= intermediate;
            /* use intermediate result, if bit is set */
            if (exponent & binary) intermediate *= base;
        }
        return intermediate;
    }
};

#endif
