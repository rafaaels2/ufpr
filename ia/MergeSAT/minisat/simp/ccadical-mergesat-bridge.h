/**********************************************************************[ccadical-mergesat-bridge.h]
MergeSat -- Copyright (c) 2021,      Norbert Manthey, Armin Biere

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

#ifndef MergeSat_CCaDiCaL_MergeSat_Bridge_h
#define MergeSat_CCaDiCaL_MergeSat_Bridge_h

/* Get MergeSat C Interface definition */
#include "simp/cmergesat.h"

/* This interface code is taken from CaDiCaL 1.4.0
 * Basically, each call is forwarded to the matching
 * CMergeSat interface.
 */

/*----- CaDiCaL interface - begin ----------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
/*------------------------------------------------------------------------*/

/*
 * C wrapper for CaDiCaL's C++ API following IPASIR.
 */

/* Set a CaDiCaL type to match the function signatures */
#define CCaDiCaL CMergeSat

inline const char *ccadical_signature(void) { return cmergesat_signature(); }
inline CCaDiCaL *ccadical_init(void) { return cmergesat_init(); }
inline void ccadical_release(CCaDiCaL *c) { return cmergesat_release(c); }
inline void ccadical_add(CCaDiCaL *c, int lit) { return cmergesat_add(c, lit); }
inline void ccadical_assume(CCaDiCaL *c, int lit) { return cmergesat_assume(c, lit); }
inline int ccadical_solve(CCaDiCaL *c) { return cmergesat_solve(c); }
inline int ccadical_val(CCaDiCaL *c, int lit) { return cmergesat_val(c, lit); }
inline int ccadical_failed(CCaDiCaL *c, int lit) { return cmergesat_failed(c, lit); }
inline void ccadical_set_terminate(CCaDiCaL *c, void *state, int (*terminate)(void *state))
{
    cmergesat_set_terminate(c, state, terminate);
}
inline void ccadical_set_learn(CCaDiCaL *c, void *state, int max_length, void (*learn)(void *state, int *clause))
{
    return cmergesat_set_learn(c, state, max_length, learn);
}

/*------------------------------------------------------------------------*/

// Non-IPASIR conformant 'C' functions.

inline void ccadical_set_option(CCaDiCaL *c, const char *name, int val) { return cmergesat_set_option(c, name, val); }
inline void ccadical_limit(CCaDiCaL *c, const char *name, int limit) { return cmergesat_limit(c, name, limit); }
inline int ccadical_get_option(CCaDiCaL *c, const char *name) { return cmergesat_get_option(c, name); }
inline void ccadical_print_statistics(CCaDiCaL *c) { return cmergesat_print_statistics(c); }
inline int64_t ccadical_active(CCaDiCaL *c) { return cmergesat_active(c); }
inline int64_t ccadical_irredundant(CCaDiCaL *c) { return cmergesat_irredundant(c); }
inline int ccadical_fixed(CCaDiCaL *c, int lit) { return cmergesat_fixed(c, lit); }
inline void ccadical_terminate(CCaDiCaL *c) { return cmergesat_terminate(c); }
inline void ccadical_freeze(CCaDiCaL *c, int lit) { return cmergesat_freeze(c, lit); }
inline int ccadical_frozen(CCaDiCaL *c, int lit) { return cmergesat_frozen(c, lit); }
inline void ccadical_melt(CCaDiCaL *c, int lit) { return cmergesat_melt(c, lit); }
inline int ccadical_simplify(CCaDiCaL *c) { return cmergesat_simplify(c); }
#if 0 /* parallel backend does not support constrain yet */
inline void ccadical_constrain(CCaDiCaL *c, int lit) { return cmergesat_constrain(c, lit); }
inline int ccadical_constraint_failed(CCaDiCaL *c) { return cmergesat_constraint_failed(c); }
#endif

/*------------------------------------------------------------------------*/

// Support legacy names used before moving to more IPASIR conforming names.

#define ccadical_reset ccadical_release
#define ccadical_sat ccadical_solve
#define ccadical_deref ccadical_val

/*------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*----- CaDiCaL interface - end ------------------------------------------*/

#endif
