/* Copyright (C) 2011 - 2014, Armin Biere, Johannes Kepler University, Linz */
/* Copyright (C) 2014, Mathias Preiner, Johannes Kepler University, Linz */
/* Copyright (C) 2019, Norbert Manthey */

/* This file contains code original developped for the Boolector project,
 * but is also available under the standard IPASIR license.
 */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include "parallel/ParSolver.h"

#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;
using namespace MERGESAT_NSPACE;

extern "C" {
static const char *sig = "mergesat";
#include <sys/resource.h>
#include <sys/time.h>
static double getime(void)
{
    struct rusage u;
    double res;
    if (getrusage(RUSAGE_SELF, &u)) return 0;
    res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
    res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
    return res;
}
}

class IPAsirMiniSAT : public ParSolver
{
    vec<Lit> ipasir_assumptions, ipasir_clause, ipasir_constrain_clause;
    std::vector<int> learn_clause;
    vec<unsigned char> fmap;
    bool nomodel, use_constrain_clause;
    unsigned long long calls;
    void reset(bool reset_constraint = false)
    {
        fmap.clear();
        if (reset_constraint) reset_constrain_clause();
    }
    void ana()
    {
        fmap.clear();
        fmap.growTo(2 * nVars(), 0);
        for (int i = 0; i < conflict.size(); i++) {
            int tmp = toInt(~conflict[i]);
            assert(0 <= tmp && tmp < fmap.size());
            fmap[tmp] = 1;
        }
    }
    double ps(double s, double t) { return t ? s / t : 0; }

    public:
    IPAsirMiniSAT() : nomodel(false), use_constrain_clause(false), calls(0)
    {
        // MiniSAT by default produces non standard conforming messages.
        // So either we have to set this to '0' or patch the sources.
        verbosity = 0;
        disable_simplification();
    }
    ~IPAsirMiniSAT() { reset(); }
    void add(int lit)
    {
        reset();
        nomodel = true;
        if (lit) {
            ipasir_clause.push(importLit(lit));
        } else {
            // okay to modify clause, we'll clear it anyways
            addClause_(ipasir_clause), ipasir_clause.clear();
        }
    }
    void assume(int lit)
    {
        reset();
        nomodel = true;
        ipasir_assumptions.push(importLit(lit));
    }
    int solve(bool simp = false)
    {
        calls++;
        reset(use_constrain_clause);
        lbool res = solveLimited(ipasir_assumptions, simp);
        use_constrain_clause = false;
        ipasir_assumptions.clear();
        ipasir_constrain_clause.clear();
        nomodel = (res != l_True);
        return (res == l_Undef) ? 0 : (res == l_True ? 10 : 20);
    }
    int solve_final()
    {
        // allow simplification, in case we did not call the solver so far
        return solve(calls == 0);
    }
    Lit importLit(int lit)
    {
        int L = abs(lit);
        while (L > nVars()) (void)newVar();
        return mkLit(Var(L - 1), (lit < 0));
    }
    int val(int lit)
    {
        if (nomodel) return 0;
        lbool res = modelValue(importLit(lit));
        return (res == l_True) ? lit : -lit;
    }
    int top_val(int lit)
    {
        if (decisionLevel() != 0) return 0;
        lbool res = value(importLit(lit));
        return ((res == l_True) ? lit : ((res == l_False) ? -lit : 0));
    }
    int failed(int lit)
    {
        if (fmap.size() != 2 * nVars()) ana();
        int tmp = toInt(importLit(lit));
        assert(0 <= tmp && tmp < fmap.size());
        return fmap[tmp] != 0;
    }
    void stats()
    {
        double t = getime();
        printf("c [%s]\n"
               "c [%s]        calls %12llu   %9.1f per second\n"
               "c [%s]     restarts %12llu   %9.1f per second\n"
               "c [%s]    conflicts %12llu   %9.1f per second\n"
               "c [%s]    decisions %12llu   %9.1f per second\n"
               "c [%s] propagations %12llu   %9.1f per second\n"
               "c [%s]\n",
               sig, sig, (unsigned long long)calls, ps(calls, t), sig, (unsigned long long)starts, ps(starts, t), sig,
               (unsigned long long)conflicts, ps(conflicts, t), sig, (unsigned long long)decisions, ps(decisions, t),
               sig, (unsigned long long)propagations, ps(propagations, t), sig);
        fflush(stdout);
    }
    void learn(int lit)
    {
        if (lit) {
            if ((size_t)learnCallbackLimit < learn_clause.size()) {
                learn_clause.push_back(lit);
            }
            return;
        }
        if ((size_t)learnCallbackLimit >= learn_clause.size()) {
            learn_clause.clear();
            return;
        }
        learnCallback(learnCallbackState, &(learn_clause[0]));
        learn_clause.clear();
    }
    bool terminate()
    {
        if (!termCallback) return false;
        return termCallback(termCallbackState);
    }
#if 0 /* parallel backend does not support constrain yet */
    void constrain(int lit)
    {
        if (lit != 0) {
            ipasir_constrain_clause.push(importLit(lit));
        } else {
            addConstrainClause(ipasir_constrain_clause);
            ipasir_constrain_clause.clear();
            use_constrain_clause = true; /*Use constrain clause in next call.*/
        }
    }
#endif
};

extern "C" {
#include "core/ipasir.h"
static IPAsirMiniSAT *import(void *s) { return (IPAsirMiniSAT *)s; }
const char *ipasir_signature() { return sig; }
void *ipasir_init() { return new IPAsirMiniSAT(); }
void ipasir_release(void *s) { delete import(s); }
int ipasir_solve(void *s) { return import(s)->solve(); }
int ipasir_solve_final(void *s) { return import(s)->solve_final(); }
void ipasir_add(void *s, int l) { import(s)->add(l); }
void ipasir_assume(void *s, int l) { import(s)->assume(l); }
int ipasir_val(void *s, int l) { return import(s)->val(l); }
int ipasir_failed(void *s, int l) { return import(s)->failed(l); }
void ipasir_set_terminate(void *s, void *state, int (*callback)(void *state))
{
    import(s)->setTermCallback(state, callback);
}
void ipasir_terminate(void *s) { import(s)->terminate(); }
void ipasir_set_learn(void *s, void *state, int max_length, void (*learn)(void *state, int *clause))
{
    import(s)->setLearnCallback(state, max_length, learn);
}
}

/* MergeSat C interface */
extern "C" {
#include "simp/cmergesat.h"
const char *cmergesat_signature(void) { return ipasir_signature(); }

CMergeSat *cmergesat_init(void) { return (CMergeSat *)ipasir_init(); }

void cmergesat_release(CMergeSat *wrapper) { ipasir_release((void *)wrapper); }

void cmergesat_set_option(CMergeSat *wrapper, const char *name, int val)
{
    // ((Wrapper*) wrapper)->solver->set (name, val);
    assert(false && "not implemented yet");
    return;
}

void cmergesat_limit(CMergeSat *wrapper, const char *name, int val)
{
    /* not implemented yet */
    assert(false && "not implemented yet");
    return;
}

int cmergesat_get_option(CMergeSat *wrapper, const char *name)
{
    /* not implemented yet */
    assert(false && "not implemented yet");
    return 0;
}

void cmergesat_add(CMergeSat *wrapper, int lit) { ipasir_add((void *)wrapper, lit); }

void cmergesat_assume(CMergeSat *wrapper, int lit) { ipasir_assume((void *)wrapper, lit); }

int cmergesat_solve(CMergeSat *wrapper) { return ipasir_solve((void *)wrapper); }

int cmergesat_simplify(CMergeSat *wrapper)
{
    /* not implemented yet */
    assert(wrapper && "can only simplify the formula with an active solver");
    if (!wrapper) return 0;
    import(wrapper)->eliminate();
    /* we do not know whether a formula is SAT (10), hence, do only return UNKNOWN(0) or UNSAT(20). */
    return import(wrapper)->okay() ? 0 : 20;
}

int cmergesat_val(CMergeSat *wrapper, int lit) { return ipasir_val((void *)wrapper, lit); }

int cmergesat_failed(CMergeSat *wrapper, int lit) { return ipasir_failed((void *)wrapper, lit); }

void cmergesat_print_statistics(CMergeSat *wrapper)
{
    assert(wrapper && "can only get clause statistics with an active solver");
    if (!wrapper) return;
    import(wrapper)->stats();
}

void cmergesat_terminate(CMergeSat *wrapper) { ipasir_terminate((void *)wrapper); }

int64_t cmergesat_active(CMergeSat *wrapper)
{
    assert(wrapper && "can only get clause statistics with an active solver");
    if (!wrapper) return 0;
    return import(wrapper)->nClauses();
    ;
}

int64_t cmergesat_irredundant(CMergeSat *wrapper)
{
    assert(wrapper && "can only get clause statistics with an active solver");
    if (!wrapper) return 0;
    return import(wrapper)->nLearnts();
    ;
}

int cmergesat_fixed(CMergeSat *wrapper, int lit)
{
    assert(wrapper && "can only get fixed info with an active solver");
    if (!wrapper) return 0;
    int value = import(wrapper)->top_val(lit);
    return value > 0 ? 1 : (value < 0 ? -1 : 0);
}

void cmergesat_set_terminate(CMergeSat *ptr, void *state, int (*terminate)(void *))
{
    ipasir_set_terminate((void *)ptr, state, terminate);
}

void cmergesat_set_learn(CMergeSat *ptr, void *state, int max_length, void (*learn)(void *state, int *clause))
{
    ipasir_set_learn((void *)ptr, state, max_length, learn);
}

void cmergesat_freeze(CMergeSat *ptr, int lit)
{
    assert(ptr && "can only set freeze information with an active solver");
    if (!ptr) return;
    import(ptr)->setFrozen(MERGESAT_NSPACE::Var(abs(lit) - 1), true);
    return;
}

void cmergesat_melt(CMergeSat *ptr, int lit)
{
    assert(ptr && "can only set freeze information with an active solver");
    if (!ptr) return;
    import(ptr)->setFrozen(MERGESAT_NSPACE::Var(abs(lit) - 1), false);
    return;
}

int cmergesat_frozen(CMergeSat *ptr, int lit)
{
    assert(ptr && "can only set freeze information with an active solver");
    if (!ptr) return 0;
    return import(ptr)->getFrozen(MERGESAT_NSPACE::Var(abs(lit) - 1));
}

#if 0 /* parallel backend does not support constrain yet */
void cmergesat_constrain(CMergeSat *ptr, int lit)
{
    assert(ptr && "can only constrain search with an active solver");
    if (!ptr) return;
    return import(ptr)->constrain(lit);
}
int cmergesat_constraint_failed(CMergeSat *ptr)
{
    assert(ptr && "can only check for constraint failures with an active solver");
    if (!ptr) return 0;
    return import(ptr)->failed_constraint();
}
#endif
}
