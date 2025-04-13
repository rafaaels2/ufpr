/*************************************************************************************[cmergesat.h]
MergeSat -- Copyright (c) 2021,      Norbert Manthey

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

#ifndef MergeSat_CMergeSat_h
#define MergeSat_CMergeSat_h

/* MergeSat interface */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Dummy type to improve usage */
typedef void CMergeSat;

/* IPASIR like functions */
const char *cmergesat_signature(void);
CMergeSat *cmergesat_init(void);
void cmergesat_release(CMergeSat *);

void cmergesat_add(CMergeSat *, int lit);
void cmergesat_assume(CMergeSat *, int lit);
int cmergesat_solve(CMergeSat *);
int cmergesat_val(CMergeSat *, int lit);
int cmergesat_failed(CMergeSat *, int lit);
void cmergesat_set_terminate(CMergeSat *, void *state, int (*terminate)(void *state));
void cmergesat_set_learn(CMergeSat *, void *state, int max_length, void (*learn)(void *state, int *clause));

/* Non-IPASIR conformant 'C' functions. */
void cmergesat_set_option(CMergeSat *, const char *name, int val);
void cmergesat_limit(CMergeSat *, const char *name, int limit);
int cmergesat_get_option(CMergeSat *, const char *name);
void cmergesat_print_statistics(CMergeSat *);
int64_t cmergesat_active(CMergeSat *);
int64_t cmergesat_irredundant(CMergeSat *);
int cmergesat_fixed(CMergeSat *, int lit);
void cmergesat_terminate(CMergeSat *);
void cmergesat_freeze(CMergeSat *, int lit);
int cmergesat_frozen(CMergeSat *, int lit);
void cmergesat_melt(CMergeSat *, int lit);
int cmergesat_simplify(CMergeSat *);
#if 0 /* parallel backend does not support constrain yet */
void cmergesat_constrain(CMergeSat *, int lit);
int cmergesat_constraint_failed(CMergeSat *);
#endif

#ifdef __cplusplus
}
#endif

#endif
