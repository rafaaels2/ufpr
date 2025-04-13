/***********************************************************************************[SimpSolver.cc]
MiniSat -- Copyright (c) 2006,      Niklas Een, Niklas Sorensson
           Copyright (c) 2007-2010, Niklas Sorensson

Chanseok Oh's MiniSat Patch Series -- Copyright (c) 2015, Chanseok Oh

Maple_LCM, Based on MapleCOMSPS_DRUP -- Copyright (c) 2017, Mao Luo, Chu-Min LI, Fan Xiao: implementing a learnt clause
minimisation approach Reference: M. Luo, C.-M. Li, F. Xiao, F. Manya, and Z. L. , “An effective learnt clause
minimization approach for cdcl sat solvers,” in IJCAI-2017, 2017, pp. to–appear.

Maple_LCM_Dist, Based on Maple_LCM -- Copyright (c) 2017, Fan Xiao, Chu-Min LI, Mao Luo: using a new branching heuristic
called Distance at the beginning of search


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

#include "simp/SimpSolver.h"
#include "mtl/Sort.h"
#include "utils/Options.h"
#include "utils/System.h"

using namespace MERGESAT_NSPACE;

//=================================================================================================
// Options:


static const char *_cat = "SIMP";

static BoolOption opt_use_asymm(_cat, "asymm", "Shrink clauses by asymmetric branching.", false, false);
static BoolOption opt_use_rcheck(_cat, "rcheck", "Check if a clause is already implied. (costly)", false, false);
static BoolOption opt_use_elim(_cat, "elim", "Perform variable elimination.", true);
static IntOption opt_grow(_cat, "grow", "Allow a variable elimination step to grow by a number of clauses.", 0);
static BoolOption opt_grow_iterations(_cat, "grow-iter", "Run iterative eliminations with growing.", true);
static IntOption opt_clause_lim(_cat,
                                "cl-lim",
                                "Variables are not eliminated if it produces a resolvent with a length above this "
                                "limit. -1 means no limit",
                                20,
                                IntRange(-1, INT32_MAX));
static IntOption
opt_subsumption_lim(_cat,
                    "sub-lim",
                    "Do not check if subsumption against a clause larger than this. -1 means no limit.",
                    1000,
                    IntRange(-1, INT32_MAX));
static DoubleOption opt_simp_garbage_frac(_cat,
                                          "simp-gc-frac",
                                          "The fraction of wasted memory allowed before a garbage collection is "
                                          "triggered during simplification.",
                                          0.5,
                                          DoubleRange(0, false, HUGE_VAL, false));
static Int64Option opt_max_simplify_step(_cat,
                                         "max-simp-steps",
                                         "Do not perform more simplification steps than this. -1 means no limit.",
                                         40000000000,
                                         Int64Range(-1, INT64_MAX));
static Int64Option
opt_max_simplify_accesses(_cat,
                          "max-simp-accesses",
                          "Do not perform more simplification accesses than this. -1 means no limit.",
                          -1,
                          Int64Range(-1, INT64_MAX));
static IntOption opt_max_simp_cls(_cat,
                                  "max-simp-cls",
                                  "If input has more clauses than the given number, disable simplification",
                                  INT32_MAX,
                                  IntRange(0, INT32_MAX));
static IntOption opt_all_strength_max(_cat,
                                      "all-strength-max",
                                      "Keep with at most X literals for multiple strengthening attempts around",
                                      0,
                                      IntRange(0, INT32_MAX));
static BoolOption opt_bveGates(_cat, "bve-gates", "Run gate detection for variables to be eliminated.", false);
static BoolOption opt_bveSemGates(_cat, "bve-sem-gates", "Run gate detection with propagation.", false);
static BoolOption opt_bveGateMini(_cat, "bve-gate-mini", "Do not produce redundant resolvents.", true);


//=================================================================================================
// Constructor/Destructor:


SimpSolver::SimpSolver()
  : simp_reparsed_options(updateOptions())
  , parsing(false)
  , grow(opt_grow)
  , grow_iterations(opt_grow_iterations)
  , clause_lim(opt_clause_lim)
  , subsumption_lim(opt_subsumption_lim)
  , simp_garbage_frac(opt_simp_garbage_frac)
  , max_simp_accesses(opt_max_simplify_accesses)
  , max_simp_steps(opt_max_simplify_step)
  , nr_max_simp_cls(opt_max_simp_cls)
  , allStrengtheningMaxSize(opt_all_strength_max)
  , use_asymm(opt_use_asymm)
  , use_rcheck(opt_use_rcheck)
  , use_elim(opt_use_elim)
  , merges(0)
  , asymm_lits(0)
  , eliminated_vars(0)
  , total_subsumed(0)
  , total_deleted_literals(0)
  , total_all_strengthened(0)
  , total_all_strength_candidates(0)
  , elimorder(1)
  , use_simplification(true)
  , occurs(ClauseDeleted(ca), counter_access)
  , elim_heap(ElimLt(n_occ))
  , bwdsub_assigns(0)
  , n_touched(0)
  , bveDetectGates(opt_bveGates)
  , bveSemanticDetection(opt_bveSemGates)
  , dropGateRedundantClauses(opt_bveGateMini)
{
    vec<Lit> dummy(1, lit_Undef);
    ca.extra_clause_field = true; // NOTE: must happen before allocating the dummy clause below.
    bwdsub_tmpunit = ca.alloc(dummy);
    remove_satisfied = false;
}


SimpSolver::~SimpSolver() {}


Var SimpSolver::newVar(bool sign, bool dvar)
{
    Var v = Solver::newVar(sign, dvar);

    frozen.push((char)false);
    eliminated.push((char)false);

    if (use_simplification) {
        n_occ.push(0);
        n_occ.push(0);
        occurs.init(mkLit(v, true));
        occurs.init(mkLit(v, false));
        touched.push(0);
        elim_heap.insert(v);
    }

    // TODO: make sure to add new data structures also to reserveVars below!

    return v;
}

void SimpSolver::reserveVars(Var v)
{
    Solver::reserveVars(v);

    frozen.capacity(v + 1);
    eliminated.capacity(v + 1);

    if (use_simplification) {
        n_occ.capacity(v + 1);
        n_occ.capacity(v + 1);
        occurs.init(mkLit(v, true));
        occurs.init(mkLit(v, false));
        touched.capacity(v + 1);
    }
}


int SimpSolver::max_simp_cls() { return nr_max_simp_cls; }

lbool SimpSolver::solve_(bool do_simp, bool turn_off_simp)
{
    vec<Var> extra_frozen;
    extra_frozen.clear();
    lbool result = l_True;

    systematic_branching_state = 1;

    do_simp &= use_simplification;
    double simp_time = cpuTime();

    if (do_simp) {
        // Assumptions must be temporarily frozen to run variable elimination:
        for (int i = 0; i < assumptions.size(); i++) {
            Var v = var(assumptions[i]);

            // If an assumption has been eliminated, remember it.
            assert(!isEliminated(v));

            if (!frozen[v]) {
                // Freeze and store.
                setFrozen(v, true);
                extra_frozen.push(v);
            }
        }

        result = lbool(eliminate(turn_off_simp));
    }

    use_simplification = false; // properly turning off simplification, as we cleanup structures here
    occurs.clear(true);
    touched.clear(true);
    occurs.clear(true);
    n_occ.clear(true);
    elim_heap.clear(true);
    subsumption_queue.clear(true);

    simp_time = cpuTime() - simp_time;      // stop timer and record time consumed until now
    check_satisfiability_simplified = true; // only check SAT answer after the call to extendModel()

    /* share units during initial call, only really useful in case preprocessing is used */
    if (solves == 1) {
        Solver::shareUnitClauses();
    }

    if (result == l_True)
        result = Solver::solve_();
    else if (verbosity >= 1)
        printf("c ===============================================================================\n");

    simp_time = cpuTime() - simp_time; // continue timer and consider time tracked already

    if (result == l_True) {
        extendModel();
        if (check_satisfiability) {
            if (!satChecker.checkModel(model)) {
                assert(false && "model should satisfy full input formula");
                throw("ERROR: detected model that does not satisfy input formula, abort");
                exit(1);
            } else if (verbosity)
                printf("c validated SAT answer after extending model\n");
        }
    }

    if (do_simp)
        // Unfreeze the assumptions that were frozen:
        for (int i = 0; i < extra_frozen.size(); i++) setFrozen(extra_frozen[i], false);

    systematic_branching_state = 0;
    statistics.simpSeconds += cpuTime() - simp_time; // stop timer and record time consumed until now

    deactivate_constrain_clause(result); // always indicate failure in case of unsat
    return result;
}


bool SimpSolver::addClause_(vec<Lit> &ps)
{
#ifndef NDEBUG
    bool is_sat = false;
    bool has_eliminated = false;
    for (int i = 0; i < ps.size(); i++) {
        if (value(ps[i]) == l_True) is_sat = true;
        if (isEliminated(var(ps[i]))) has_eliminated = true;
    }
    assert((is_sat || !has_eliminated) && "removing clauses is done lazily");
#endif

    int nclauses = clauses.size();

    if (use_rcheck && implied(ps)) return true;

    if (!parsing) {
        proof.addClause('a', ps);
    }

    if (!Solver::addClause_(ps)) return false;

    // Only simplify before actually solving
    if (use_simplification && clauses.size() == nclauses + 1 && solves == 0) {
        CRef cr = clauses.last();
        addToSimpStructures(cr, true);
    }

    return true;
}

void SimpSolver::addToSimpStructures(const CRef cr, bool useForSimplification)
{
    const Clause &c = ca[cr];
    statistics.simpSteps++;

    // NOTE: the clause is added to the queue immediately and then
    // again during 'gatherTouchedClauses()'. If nothing happens
    // in between, it will only be checked once. Otherwise, it may
    // be checked twice unnecessarily. This is an unfortunate
    // consequence of how backward subsumption is used to mimic
    // forward subsumption.
    if (useForSimplification) subsumption_queue.insert(cr);
    for (int i = 0; i < c.size(); i++) {
        occurs[c[i]].push(cr);
        n_occ[toInt(c[i])]++;
        touched[var(c[i])] = 1;
        n_touched++;
        if (useForSimplification) {
            if (elim_heap.inHeap(var(c[i]))) elim_heap.increase(var(c[i]));
        }
    }
}


void SimpSolver::removeClause(CRef cr)
{
    const Clause &c = ca[cr];
    statistics.simpSteps++;
    if (c.mark() != 0) return;

    if (use_simplification)
        for (int i = 0; i < c.size(); i++) {
            n_occ[toInt(c[i])]--;
            updateElimHeap(var(c[i]));
            occurs.smudge(c[i]);
        }

    Solver::removeClause(cr);
}


bool SimpSolver::strengthenClause(CRef cr, Lit l)
{
    Clause &c = ca[cr];
    statistics.simpSteps++;
    assert(decisionLevel() == 0);
    assert(use_simplification);

    if (c.size() == 2) {
        proof.strengthenClause(c, l);
        removeClause(cr);
        c.strengthen(l);
        proof.addUnitClause('a', c[0]);
        bool r = enqueue(c[0]) && propagate() == CRef_Undef;
        shareUnitClauses();
        return r;
    }

    if (c.size() > 2 && c.size() <= allStrengtheningMaxSize) {
        // we want to use another clause
        TRACE(std::cout << "c SUBSIMP use all strengthening with literal " << l << " on clause " << c << std::endl;)
        if (c.simplified())
            total_all_strengthened++;
        else
            total_all_strength_candidates++;
        CRef strengthenedCR = cr;
        cr = ca.copyalloc(cr); // note: reference d can be invalid after this point
        Clause &d = ca[cr];
        proof.addClause('a', d);
        d.strengthen(l);
        if (d.learnt())
            learnts_core.push(cr);
        else
            clauses.push(cr);
        proof.strengthenClause(d, l);
        addToSimpStructures(cr, false);         // drop literal here first!
        ca[strengthenedCR].setSimplified(true); // memorize to not use this clause for subsumption anymore
    } else {
        proof.strengthenClause(c, l);
        proof.addClause('d', c);
        detachClause(cr, true);
        remove(occurs[l], cr);
        c.strengthen(l);
    }
    attachClause(cr);

    n_occ[toInt(l)]--;
    updateElimHeap(var(l));
    subsumption_queue.insert(cr);
    return true;
}


// Returns FALSE if clause is always satisfied ('out_clause' should not be used).
bool SimpSolver::merge(const Clause &_ps, const Clause &_qs, Var v, vec<Lit> &out_clause)
{
    merges++;
    out_clause.clear();
    counter++;

    for (int i = 0; i < _ps.size(); i++) {
        if (var(_ps[i]) != v) {
            out_clause.push(_ps[i]);
            seen2[_ps[i].x] = counter;
        }
    }

    for (int i = 0; i < _qs.size(); i++) {
        if (var(_qs[i]) != v) {
            if (seen2[_qs[i].x] != counter) {
                if (seen2[(~_qs[i]).x] != counter) {
                    out_clause.push(_qs[i]);
                } else {
                    return false;
                }
            }
        }
    }

    return true;
}


// Returns FALSE if clause is always satisfied.
bool SimpSolver::merge(const Clause &_ps, const Clause &_qs, Var v, int &size)
{
    merges++;
    merge_count_cls.clear();
    bool ret = merge(_ps, _qs, v, merge_count_cls);
    size = merge_count_cls.size();
    return ret;
}


void SimpSolver::gatherTouchedClauses()
{
    if (n_touched == 0) return;

    int i, j;
    for (i = j = 0; i < subsumption_queue.size(); i++)
        if (ca[subsumption_queue[i]].mark() == 0) ca[subsumption_queue[i]].mark(2);
    statistics.simpSteps += subsumption_queue.size();

    for (i = 0; i < touched.size(); i++)
        if (touched[i]) {
            for (int pol = 0; pol < 1; ++pol) {
                const vec<CRef> &cs = occurs.lookup(mkLit(i, pol == 0));
                for (j = 0; j < cs.size(); j++)
                    if (ca[cs[j]].mark() == 0) {
                        subsumption_queue.insert(cs[j]);
                        ca[cs[j]].mark(2);
                    }
                statistics.simpSteps += cs.size();
            }
            touched[i] = 0;
        }

    for (i = 0; i < subsumption_queue.size(); i++)
        if (ca[subsumption_queue[i]].mark() == 2) ca[subsumption_queue[i]].mark(0);
    statistics.simpSteps += subsumption_queue.size();

    n_touched = 0;
}


bool SimpSolver::implied(const vec<Lit> &c)
{
    assert(decisionLevel() == 0);

    trail_lim.push(trail.size());
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True) {
            cancelUntil(0);
            return true;
        } else if (value(c[i]) != l_False) {
            assert(value(c[i]) == l_Undef);
            uncheckedEnqueue(~c[i], decisionLevel());
        }

    bool result = propagate() != CRef_Undef;
    cancelUntil(0);
    return result;
}


// Backward subsumption + backward subsumption resolution
bool SimpSolver::backwardSubsumptionCheck(bool verbose)
{
    int cnt = 0;
    int subsumed = 0;
    int deleted_literals = 0;
    assert(decisionLevel() == 0);

    TRACE(std::cout << "c attempt to run subsumption and strengthening with a queue with variable "
                    << subsumption_queue.size() << " elements" << std::endl;)
    while (subsumption_queue.size() > 0 || bwdsub_assigns < trail.size()) {

        // Empty subsumption queue and return immediately on user-interrupt:
        if (asynch_interrupt || !isInSimpLimit()) {
            subsumption_queue.clear();
            bwdsub_assigns = trail.size();
            break;
        }

        // Check top-level assignments by creating a dummy clause and placing it in the queue:
        if (subsumption_queue.size() == 0 && bwdsub_assigns < trail.size()) {
            Lit l = trail[bwdsub_assigns++];
            ca[bwdsub_tmpunit][0] = l;
            ca[bwdsub_tmpunit].calcAbstraction();
            subsumption_queue.insert(bwdsub_tmpunit);
            statistics.simpSteps++;
        }

        CRef cr = subsumption_queue.peek();
        subsumption_queue.pop();
        Clause &c = ca[cr];
        statistics.simpSteps++;

        if (c.mark() || c.simplified()) continue;
        TRACE(std::cout << "c attempt subsimp with clause " << c << std::endl;)

        c.setOnQueue(false);

        if (verbose && verbosity >= 2 && cnt++ % 1000 == 0)
            printf("c subsumption left: %10d (%10d subsumed, %10d deleted literals)\r", subsumption_queue.size(),
                   subsumed, deleted_literals);

        assert(c.size() > 1 || value(c[0]) == l_True); // Unit-clauses should have been propagated before this point.

        // Find best variable to scan:
        Var best = var(c[0]);
        int bestSize = occurs[mkLit(best, true)].size() + occurs[mkLit(best, false)].size();
        for (int i = 1; i < c.size(); i++) {
            if (occurs[c[i]].size() + occurs[~c[i]].size() < bestSize) {
                best = var(c[i]);
                bestSize = occurs[mkLit(best, true)].size() + occurs[mkLit(best, false)].size();
            }
        }


        // Search all candidates:
        for (int pol = 0; pol < 2; pol++) {
            vec<CRef> &cs = occurs[mkLit(best, pol == 0)];
            for (int j = 0; j < cs.size(); j++) {
                Clause &d = ca[cr];
                if (d.mark())
                    break;
                else if (statistics.simpSteps++ && !ca[cs[j]].mark() && cs[j] != cr &&
                         (subsumption_lim == -1 || ca[cs[j]].size() < subsumption_lim)) {
                    Lit l = d.subsumes(ca[cs[j]]);

                    if (l == lit_Undef) {
                        TRACE(std::cout << "c subsumed next clause and remove it: " << ca[cs[j]] << std::endl;)
                        subsumed++, removeClause(cs[j]);
                        total_subsumed++;
                    } else if (l != lit_Error) {
                        deleted_literals++;
                        total_deleted_literals++;
                        TRACE(std::cout << "c strengthen, via literal " << ~l << ", clause: " << ca[cs[j]] << std::endl;)
                        const CRef toStrengthenCR = cs[j];
                        if (!strengthenClause(cs[j], ~l)) return false; // Clause references might become invalid

                        // Did current candidate get deleted from cs? Then check candidate at index j again:
                        // ... but only, if we did not use all-strengthening
                        if (var(l) == best && !ca[toStrengthenCR].simplified()) {
                            TRACE(std::cout << "c SUBSIMP dropped best lit, retry slot" << std::endl;)
                            j--;
                        }
                    }
                }
            }
        }
    }

    return true;
}


bool SimpSolver::asymm(Var v, CRef cr)
{
    Clause &c = ca[cr];
    assert(decisionLevel() == 0);
    statistics.simpSteps++;

    if (c.mark() || satisfied(c)) return true;

    trail_lim.push(trail.size());
    Lit l = lit_Undef;
    for (int i = 0; i < c.size(); i++)
        if (var(c[i]) != v) {
            if (value(c[i]) != l_False) uncheckedEnqueue(~c[i], 0);
        } else
            l = c[i];

    if (propagate() != CRef_Undef) {
        cancelUntil(0);
        asymm_lits++;
        if (!strengthenClause(cr, l)) return false;
    } else
        cancelUntil(0);

    return true;
}


bool SimpSolver::asymmVar(Var v)
{
    assert(use_simplification);

    bool nonEmpty = true;
    for (int pol = 0; pol < 2; pol++) {
        const vec<CRef> &cls = occurs.lookup(mkLit(v, pol == 0));

        if (value(v) != l_Undef || cls.size() == 0) continue;
        nonEmpty = true;
        for (int i = 0; i < cls.size(); i++)
            if (!asymm(v, cls[i])) return false;
    }

    if (nonEmpty) return true;
    return backwardSubsumptionCheck();
}


static void mkElimClause(vec<uint32_t> &elimclauses, Lit x)
{
    elimclauses.push(toInt(x));
    elimclauses.push(1);
}


static void mkElimClause(vec<uint32_t> &elimclauses, Var v, Clause &c)
{
    int first = elimclauses.size();
    int v_pos = -1;

    // Copy clause to elimclauses-vector. Remember position where the
    // variable 'v' occurs:
    for (int i = 0; i < c.size(); i++) {
        elimclauses.push(toInt(c[i]));
        if (var(c[i]) == v) v_pos = i + first;
    }
    assert(v_pos != -1);

    // Swap the first literal with the 'v' literal, so that the literal
    // containing 'v' will occur first in the clause:
    uint32_t tmp = elimclauses[v_pos];
    elimclauses[v_pos] = elimclauses[first];
    elimclauses[first] = tmp;

    // Store the length of the clause last:
    elimclauses.push(c.size());
}

lbool SimpSolver::bveSemanticGateDetection(Var v, int &pGateCls, int &nGateCls)
{
    assert(decisionLevel() == 0);

    lbool foundGate = l_Undef;
    pGateCls = 0;
    nGateCls = 0;

    const Lit p = mkLit(v, false);
    const Lit n = ~p;
    vec<CRef> &neg = occurs[n];

    newDecisionLevel(); // visit level 1 for the positive variable
    TRACE(std::cerr << "c BVE semantic gate PRE trail: " << trail << std::endl);

    // pseudo-enqueue all clauses that would propagate with literal 'n'
    int kept = 0;
    bool nConflict = false; // did we find a conflict when propagating 'n' ?
    for (int i = 0; i < neg.size(); ++i) {
        const Clause &c = ca[neg[i]];
        if (c.mark() == 1) {
            TRACE(std::cerr << "c skip clause " << c << " as it is marked already" << std::endl;);
            continue;
        }
        neg[kept++] = neg[i];
        if (c.size() != 2) continue;
        const Lit impl = var(c[0]) == v ? c[1] : c[0];
        if (value(impl) == l_False) {
            nConflict = true; // propagating n fails
            while (i < neg.size()) neg[kept++] = neg[i];
            break;
        }
        if (value(impl) == l_True) continue;
        simpleUncheckEnqueue(impl, neg[i]);
    }
    neg.shrink(neg.size() - kept);
    occurs.unSmudge(n);

    TRACE(std::cerr << "c BVE sem gate: enqueue conflict: " << nConflict << std::endl);
    TRACE(std::cerr << "c BVE semantic gate post-enqueue trail: " << trail << std::endl);

    if (nConflict) {
        cancelUntil(0, false);
        vec<Lit> tmp;
        tmp.push(n);
        if (!addClause_(tmp)) ok = false; // found UNSAT
        return l_False;
    }

    CRef confl = CRef_Undef;
    assert(value(n) == l_Undef && "conflicts should be detected earlier");

    if (confl == CRef_Undef) {
        newDecisionLevel(); // visit level 2 to easily undo changes
        TRACE(std::cerr << "c BVE semantic gate pre-compl trail: " << trail << std::endl);
        // pseudo-enqueue literal to trigger propagation
        simpleUncheckEnqueue(n, CRef_Undef);
        // TODO: support another template parameter to limit performed UP with a parameter
        //       to (1) only propagate directly implied literals (x=1), or (2) a multiple x of that
        //       or (3) fully propagate (x=0)
        confl = simplePropagate<true>(v);
        TRACE(std::cerr << "c BVE semantic gate post trail: " << trail << std::endl);
    }

// TODO: implement actual search with learned clauses and resolution and fast cleanup later!
#if 0
    while (confl == CRef_Undef) {
        // only test literals of ternary clauses for both literals, and propagate once!

        // check for conflict, or run "full search" until given number of conflicts (with analysis and learning)
    }
#endif

    if (confl != CRef_Undef) {
        TRACE(std::cerr << "c BVE found semantic gate conflict for variable " << v + 1 << " with falsified clause "
                        << ca[confl] << std::endl);

        vec<CRef> &pos = occurs.lookup(p); // clean marked clauses, if required
        TRACE(std::cerr << "c BVE clauses for literal " << p << ":" << std::endl;
              for (int i = 0; i < pos.size(); ++i) std::cerr << "c BVE clauses: " << ca[pos[i]]
                                                             << " (marked: " << ca[pos[i]].mark() << ")" << std::endl);
        // this code is very similar to the analyzeFinal function of the solver
        assert(decisionLevel() > 0);
        Clause &d = ca[confl];
        TRACE(std::cerr << "c BVE, sem unsat core, start with " << d << std::endl);
        // mark relevant propagated variables to be resolved
        for (int i = 0; i < d.size(); ++i) {
            if (var(d[i]) == v) {
                assert(d.mark() == 0);
                d.mark(1); // temporarily mark clause for being relevant for 'v' gate
                TRACE(std::cerr << "c mark clause " << d << " for being relevant" << std::endl);
            } else if (level(var(d[i])) > 0) {
                assert(seen[var(d[i])] == 0 && "clean initial state");
                seen[var(d[i])] = 1;
                bveGateStats.n_semantic_lits_sum++; // track relevant variables
            }
        }

        // From here on, any relevant clause is not marked!

        // resolve until all relevant variables are gone
        for (int i = trail.size() - 1; i >= trail_lim[0]; i--) {
            Var x = var(trail[i]);
            if (seen[x]) {
                assert(x != v && "we strictly exclude target variable v everywhere");
                if (reason(x) != CRef_Undef) {
                    Clause &c = ca[reason(x)];
                    TRACE(std::cerr << "c BVE, sem unsat core, use " << c << " that implied " << trail[i] << std::endl);
                    for (int j = c.size() == 2 ? 0 : 1; j < c.size(); j++) {
                        if (var(c[j]) == v) {
                            assert(c.mark() == 0);
                            c.mark(1); // temporarily mark clause for being relevant for 'v' gate
                            TRACE(std::cerr << "c mark clause " << c << " for being relevant" << std::endl);
                        } else if (level(var(c[j])) > 0) {
                            if (seen[var(c[j])] == 0) bveGateStats.n_semantic_lits_sum++; // track relevant variables
                            seen[var(c[j])] = 1;
                        }
                    }
                    statistics.solveSteps++;
                } else {
                    // ignore decision variable on purpose
                }
                seen[x] = 0;
            }
        }
        for (int i = 0; i < pos.size(); ++i) {
            if (ca[pos[i]].mark() == 1) {
                ca[pos[i]].mark(0);
                pos.swap(pGateCls++, i);
            }
        }
        for (int i = 0; i < neg.size(); ++i) {
            if (ca[neg[i]].mark() == 1) {
                ca[neg[i]].mark(0);
                neg.swap(nGateCls++, i);
            }
        }

        // cleanup final variables of conflict
        for (int i = 0; i < d.size(); ++i) seen[var(d[i])] = 0;
        d.mark(0);
        bveGateStats.n_detected_semantic_gates++;
        foundGate = l_True; // indicate that we found a gate definition
    } else {
        TRACE(std::cerr << "c BVE did not find semantic gate for variable " << v + 1 << std::endl);
    }


    cancelUntil(0, false); // reset state

    return foundGate;
}

lbool SimpSolver::getGateClauseOccurrences(Var v, int &pGateCls, int &nGateCls)
{
    // Return the number of detected clauses that define this variable as output of a gate.
    // The defining clauses are located at the front of the pos and neg vector.
    // The number of defining clauses per polarity is returned as well.
    pGateCls = 0;
    nGateCls = 0;

    if (bveDetectGates) {

        assert(decisionLevel() == 0);

        bveGateStats.n_gate_attempts++;

        if (bveSemanticDetection) {
            lbool foundGate = bveSemanticGateDetection(v, pGateCls, nGateCls);
            if (foundGate != l_Undef) return foundGate;
        }

        vec<CRef> &pos = occurs[mkLit(v, false)];
        vec<CRef> &neg = occurs[mkLit(v, true)];

        binaryLookupPos.capacity(2 * nVars());
        binaryLookupNeg.capacity(2 * nVars());

        // collect literals in binary clauses and ternary clauses
        for (int pol = 0; pol < 2; pol++) {
            int i = 0, j = 0;
            vec<CRef> &cls = pol == 0 ? pos : neg;
            MarkArray &ma = pol == 0 ? binaryLookupPos : binaryLookupNeg;
            ma.nextStep();
            for (; i < cls.size(); ++i) {
                const Clause &c = ca[cls[i]];
                if (c.mark() == 1) continue; // drop this clause
                if (c.size() == 2) {         // AND-gate?
                    ma.setCurrentStep(toInt(var(c[0]) == v ? c[1] : c[0]));
                    cls[j++] = cls[i]; // keep the clause
                    // we could swap binary clauses to the front of the list
                } else {
                    cls[j++] = cls[i]; // keep the clause
                    // TODO: implement ITE and XOR gate gathering here
                }
            }
            cls.shrink(i - j);
        }

        // if all marked clauses are dropped, remove the relevant entry
        occurs.unSmudge(mkLit(v, true));
        occurs.unSmudge(mkLit(v, false));

        for (int pol = 0; pol < 2; pol++) {
            int i = 0;
            vec<CRef> &cls = pol == 0 ? pos : neg;
            MarkArray &lookupMa = pol == 1 ? binaryLookupPos : binaryLookupNeg; // invert for lookup

            binaryLookupPos.nextStep();
            for (; i < cls.size(); ++i) {
                const Clause &c = ca[cls[i]];
                assert(c.mark() != 1 && "Clauses should not be marked anymore");
                if (c.size() == 3) { // ITE-gate?
                    // TODO: implement ITE or XOR gate detection here
                } else {
                    // check for AND gate (complements of all lits of the clause have to be marked)
                    bool foundAndGate = true;
                    for (int j = 0; j < c.size(); ++j) {
                        const Lit l = c[j];
                        if (var(l) == v) continue; // skip variable to be eliminated
                        if (!lookupMa.isCurrentStep(toInt(~l))) {
                            foundAndGate = false;
                            break;
                        }
                    }
                    if (foundAndGate) {
                        // update lookupMa with literals to actually find
                        lookupMa.nextStep();
                        for (int j = 0; j < c.size(); ++j) {
                            const Lit l = c[j];
                            if (var(l) == v) continue; // skip variable to be eliminated
                            lookupMa.setCurrentStep(toInt(~l));
                        }
                        // re-arrange clauses in occurrence lists
                        cls.swap(0, i);
                        // find all matching other binary clauses
                        int binariesToFind = c.size() - 1;
                        vec<CRef> &complCls = pol != 0 ? pos : neg;
                        int k, usedBinaries = 0;
                        for (k = 0; k < complCls.size(); ++k) {
                            const Clause &d = ca[complCls[k]];
                            if (d.size() != 2) continue; // only consider binary clauses for AND gates
                            Lit otherLit = var(d[0]) == v ? d[1] : d[0];
                            if (lookupMa.isCurrentStep(toInt(otherLit))) {
                                complCls.swap(usedBinaries, k);
                                usedBinaries++;
                                binariesToFind--;
                                lookupMa.reset(toInt(otherLit)); // make sure we count distinct literals
                                if (binariesToFind == 0) break;  // all literals have been found, stop
                            }
                        }
                        // AND gate with 1 pos clause, and |C| - 1 negative binary clauses
                        pGateCls = pol == 0 ? 1 : c.size() - 1;
                        nGateCls = pol != 0 ? 1 : c.size() - 1;
                        bveGateStats.n_detected_and_gates++;
                        bveGateStats.n_and_input_sum += c.size() - 1;
                        return l_True; // we detected an AND gate
                    }
                }
            }
        }
    }
    return l_Undef;
}

bool SimpSolver::eliminateVar(Var v)
{
    assert(!frozen[v]);
    assert(!isEliminated(v));
    assert(value(v) == l_Undef);

    // Split the occurrences into positive and negative:
    //
    // const vec<CRef> &cls = occurs.lookup(v);
    int pGateCls = 0, nGateCls = 0;
    lbool foundGate;
    foundGate = getGateClauseOccurrences(v, pGateCls, nGateCls);
    if (foundGate == l_False) {
        return okay() && (propagate() == CRef_Undef);
    }
    int varFsize = occurs[mkLit(v, true)].size() + occurs[mkLit(v, false)].size();

    vec<CRef> &elimVarClsPos = occurs.lookup(mkLit(v, false));
    vec<CRef> &elimVarClsNeg = occurs.lookup(mkLit(v, true));
    TRACE(std::cerr << "c eliminate variable " << v + 1 << " with " << elimVarClsPos.size() << " pos and "
                    << elimVarClsNeg.size() << " neg cls" << std::endl);

    // Check wether the increase in number of clauses stays within the allowed ('grow'). Moreover, no
    // clause must exceed the limit on the maximal clause size (if it is set):
    //
    int cnt = 0;
    int clause_size = 0;

    TRACE(std::cerr << "c eliminate variable " << v + 1 << " with the following clauses:" << std::endl;
          std::cerr << "c pos (gate: " << pGateCls << "): " << std::endl;
          for (int i = 0; i < elimVarClsPos.size(); ++i) { std::cerr << "c " << ca[elimVarClsPos[i]] << std::endl; } std::cerr
          << "c neg (gate: " << nGateCls << "): " << std::endl;
          for (int i = 0; i < elimVarClsNeg.size(); ++i) { std::cerr << "c " << ca[elimVarClsNeg[i]] << std::endl; } std::cerr
          << "c found gate: " << foundGate << std::endl;);

    // we found a failed literal
    if (foundGate == l_True) {
        Lit l = lit_Undef;
        if (nGateCls > 0 && pGateCls == 0) {
            l = mkLit(v, true);
        } else if (nGateCls == 0 && pGateCls > 0) {
            l = mkLit(v, false);
        }
        if (l != lit_Undef) {
            TRACE(std::cerr << "c found failed gate literal: " << l << std::endl;)
            proof.addUnitClause('a', l);
            bool r = enqueue(l) && propagate() == CRef_Undef;
            shareUnitClauses();
            return r;
        }
    }

    // in case we found a gate, it has to be two sided at this point
    assert((foundGate == l_Undef || (pGateCls > 0 && nGateCls > 0)) &&
           "single sided should result in unit, to be implemented!");

    for (int i = 0; i < elimVarClsPos.size(); i++) {
        statistics.simpSteps += elimVarClsNeg.size();
        for (int j = 0; j < elimVarClsNeg.size(); j++) {
            // Resolving gate defining clauses results in a tautology
            // TODO: introduce a GATE type (syntactic, semantic, ... and handle this case accordingly
            // if (i < pGateCls && j < nGateCls) continue;
            // When a gate was found, drop redundant clauses
            if (foundGate == l_True && dropGateRedundantClauses && (i >= pGateCls && j >= nGateCls)) continue;
            // Count resolvents and check for formula growth
            if (merge(ca[elimVarClsPos[i]], ca[elimVarClsNeg[j]], v, clause_size) &&
                (++cnt > varFsize + grow || (clause_lim != -1 && clause_size > clause_lim))) {
                return true;
            }
        }
    }

    // Delete and store old clauses:
    eliminated[v] = true;
    setDecisionVar(v, false);
    eliminated_vars++;
    if (foundGate == l_True) bveGateStats.n_used_detected_gate++;

    if (elimVarClsPos.size() > elimVarClsNeg.size()) {
        for (int i = 0; i < elimVarClsNeg.size(); i++) {
            mkElimClause(elimclauses, v, ca[elimVarClsNeg[i]]);
            TRACE(std::cerr << "c add elim clause " << ca[elimVarClsNeg[i]] << std::endl);
        }
        mkElimClause(elimclauses, mkLit(v));
        TRACE(std::cerr << "c add elim clause " << mkLit(v) << std::endl);
        statistics.simpSteps += elimVarClsNeg.size();
    } else {
        for (int i = 0; i < elimVarClsPos.size(); i++) {
            mkElimClause(elimclauses, v, ca[elimVarClsPos[i]]);
            TRACE(std::cerr << "c add elim clause " << ca[elimVarClsPos[i]] << std::endl);
        }
        mkElimClause(elimclauses, ~mkLit(v));
        TRACE(std::cerr << "c add elim clause " << ~mkLit(v) << std::endl);
        statistics.simpSteps += elimVarClsPos.size();
    }

    // Produce clauses in cross product:
    vec<Lit> &resolvent = add_tmp;
    for (int i = 0; i < elimVarClsPos.size(); i++) {
        statistics.simpSteps += elimVarClsNeg.size();
        for (int j = 0; j < elimVarClsNeg.size(); j++) {
            // Resolving gate defining clauses results in a tautology
            // TODO: introduce a GATE type (syntactic, semantic, ... and handle this case accordingly
            if (false && i < pGateCls && j < nGateCls) {
                bveGateStats.n_skipped_resolutions++;
                continue;
            }
            // When a gate was found, drop redundant clauses
            if (foundGate == l_True && dropGateRedundantClauses && (i >= pGateCls && j >= nGateCls)) {
                bveGateStats.n_saved_resolvents++;
                continue;
            }
            if (merge(ca[elimVarClsPos[i]], ca[elimVarClsNeg[j]], v, resolvent) && !addClause_(resolvent)) return false;
        }
    }

    for (int pol = 0; pol < 2; pol++) {
        const vec<CRef> &cls = occurs[mkLit(v, pol == 0)];
        for (int i = 0; i < cls.size(); i++) {
            TRACE(std::cout << "c after elimination, remove clause " << ca[cls[i]] << std::endl;)
            removeClause(cls[i]);
        }
        statistics.simpSteps += cls.size();
    }

    // Free occurs list for this variable:
    occurs[mkLit(v, true)].clear(true);
    occurs[mkLit(v, false)].clear(true);

    // Free watchers lists for this variable, if possible:
    watches_bin[mkLit(v)].clear(true);
    watches_bin[~mkLit(v)].clear(true);
    watches[mkLit(v)].clear(true);
    watches[~mkLit(v)].clear(true);

    return backwardSubsumptionCheck();
}


void SimpSolver::extendModel()
{
    int i, j;
    Lit x;

    TRACE(std::cerr << "c pre-extend model: ";
          for (Var v = 0; v < nVars(); ++v) std::cerr << mkLit(v, model[v] == l_False) << " "; std::cerr << std::endl;);

    for (i = elimclauses.size() - 1; i > 0; i -= j) {
        for (j = elimclauses[i--]; j > 1; j--, i--)
            if (modelValue(toLit(elimclauses[i])) != l_False) goto next;

        x = toLit(elimclauses[i]);
        TRACE(std::cerr << "c satisfy literal " << x << " due to elim clause" << std::endl);
        model[var(x)] = lbool(!sign(x));
    next:;
    }
}

// Almost duplicate of Solver::removeSatisfied. Didn't want to make the base method 'virtual'.
void SimpSolver::removeSatisfied()
{
    int i, j;
    for (i = j = 0; i < clauses.size(); i++) {
        Clause &c = ca[clauses[i]];
        if (c.mark() == 0) {
            if (satisfied(c)) {
                removeClause(clauses[i]);
                c.mark(1); // label clause as removed
            } else {
                clauses[j++] = clauses[i];
            }
        }
    }
    clauses.shrink(i - j);
}

// The technique and code are by the courtesy of the GlueMiniSat team. Thank you!
// It helps solving certain types of huge problems tremendously.
bool SimpSolver::eliminate(bool turn_off_elim)
{
    bool res = true;
    int iter = 0;
    int n_cls, n_cls_init, n_vars;

    systematic_branching_state = 1;

    if (nVars() == 0 || !use_simplification) goto cleanup; // User disabling preprocessing.

    // Get an initial number of clauses (more accurately).
    if (trail.size() != 0) removeSatisfied();
    n_cls_init = nClauses();

    res = eliminate_(); // The first, usual variable elimination of MiniSat.
    if (!res) goto cleanup;

    n_cls = nClauses();
    n_vars = nFreeVars();

    if (verbosity >= 1) printf("c Reduced to %d vars, %d cls (grow=%d)\n", n_vars, n_cls, grow);

    if ((double)n_cls / n_vars >= 10 || n_vars < 10000 || !isInSimpLimit() || !grow_iterations) {
        if (verbosity > 0)
            printf("c No iterative elimination performed. (vars=%d, c/v ratio=%.1f)\n", n_vars, (double)n_cls / n_vars);
        goto cleanup;
    }

    grow = grow ? grow * 2 : 8;
    for (; grow < 10000; grow *= 2) {
        // Rebuild elimination variable heap.
        assert(elim_heap.capacity() >= nVars() && "all variables need to be accessible");

        for (int i = 0; i < clauses.size(); i++) {
            const Clause &c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (!elim_heap.inHeap(var(c[j])))
                    elim_heap.insert(var(c[j]));
                else
                    elim_heap.update(var(c[j]));
        }

        int n_cls_last = nClauses();
        int n_vars_last = nFreeVars();

        res = eliminate_();
        if (!res || n_vars_last == nFreeVars()) break;
        if (asynch_interrupt || !isInSimpLimit()) break;
        iter++;

        int n_cls_now = nClauses();
        int n_vars_now = nFreeVars();

        double cl_inc_rate = (double)n_cls_now / n_cls_last;
        double var_dec_rate = (double)n_vars_last / n_vars_now;

        if (verbosity >= 1) {
            printf("c Reduced to %d vars, %d cls (grow=%d)\n", n_vars_now, n_cls_now, grow);
            printf("c cl_inc_rate=%.3f, var_dec_rate=%.3f\n", cl_inc_rate, var_dec_rate);
        }

        if (n_cls_now > n_cls_init || cl_inc_rate > var_dec_rate) break;
    }
    if (verbosity >= 1) printf("c No. effective iterative eliminations: %d\n", iter);

cleanup:

    if (verbosity >= 1) {
        printf("c BVE Gates: %" PRIu64 " attempts, %" PRIu64 " used, %" PRIu64
               " saved resolvents\nc BVE Gates: %" PRIu64 " semantics (%lf avg input), %" PRIu64
               " ANDs (%lf avg input)\n",
               bveGateStats.n_gate_attempts, bveGateStats.n_used_detected_gate, bveGateStats.n_saved_resolvents,
               bveGateStats.n_detected_semantic_gates,
               bveGateStats.n_detected_semantic_gates == 0 ?
               0.0 :
               (double)bveGateStats.n_semantic_lits_sum / (double)bveGateStats.n_detected_semantic_gates,
               bveGateStats.n_detected_and_gates, // TODO: add other types!
               bveGateStats.n_detected_and_gates == 0 ?
               0.0 :
               (double)bveGateStats.n_and_input_sum / (double)bveGateStats.n_detected_and_gates);
    }

    touched.clear(true);
    occurs.clear(true);
    n_occ.clear(true);
    elim_heap.clear(true);
    subsumption_queue.clear(true);
    binaryLookupPos.destroy();
    binaryLookupNeg.destroy();

    use_simplification = false;
    remove_satisfied = true;
    ca.extra_clause_field = false;

    // Force full cleanup (this is safe and desirable since it only happens once):
    rebuildOrderHeap();
    garbageCollect();

    systematic_branching_state = 0;
    return res;
}


bool SimpSolver::eliminate_()
{
    double simp_time = cpuTime();
    if (!simplify())
        return false;
    else if (!use_simplification)
        return true;

    int trail_size_last = trail.size();

    // Main simplification loop:
    //
    while (n_touched > 0 || bwdsub_assigns < trail.size() || elim_heap.size() > 0) {

        if (!isInSimpLimit()) break;

        gatherTouchedClauses();
        // printf("  ## (time = %6.2f s) BWD-SUB: queue = %d, trail = %d\n", cpuTime(), subsumption_queue.size(), trail.size() - bwdsub_assigns);
        if ((subsumption_queue.size() > 0 || bwdsub_assigns < trail.size()) && !backwardSubsumptionCheck(true)) {
            ok = false;
            goto cleanup;
        }

        // Empty elim_heap and return immediately on user-interrupt:
        if (asynch_interrupt || !isInSimpLimit()) {
            assert(bwdsub_assigns == trail.size());
            assert(subsumption_queue.size() == 0);
            assert(n_touched == 0);
            elim_heap.clear();
            goto cleanup;
        }

        // printf("  ## (time = %6.2f s) ELIM: vars = %d\n", cpuTime(), elim_heap.size());
        for (int cnt = 0; !elim_heap.empty(); cnt++) {
            Var elim = elim_heap.removeMin();

            if (asynch_interrupt || !isInSimpLimit()) break;

            if (isEliminated(elim) || value(elim) != l_Undef) continue;
            TRACE(std::cout << "c attempt to eliminate variable " << elim + 1 << std::endl;)
            if (verbosity >= 2 && cnt % 100 == 0) printf("c elimination left: %10d\r", elim_heap.size());

            if (use_asymm) {
                // Temporarily freeze variable. Otherwise, it would immediately end up on the queue again:
                bool was_frozen = frozen[elim];
                frozen[elim] = true;
                if (!asymmVar(elim)) {
                    ok = false;
                    goto cleanup;
                }
                frozen[elim] = was_frozen;
            }

            // At this point, the variable may have been set by assymetric branching, so check it
            // again. Also, don't eliminate frozen variables:
            if (use_elim && value(elim) == l_Undef && !frozen[elim] && !eliminateVar(elim)) {
                ok = false;
                goto cleanup;
            }

            checkGarbage(simp_garbage_frac);
        }

        assert(subsumption_queue.size() == 0);
    }
cleanup:
    // To get an accurate number of clauses.
    if (trail_size_last != trail.size())
        removeSatisfied();
    else {
        int i, j;
        for (i = j = 0; i < clauses.size(); i++)
            if (ca[clauses[i]].mark() == 0) clauses[j++] = clauses[i];
        clauses.shrink(i - j);
    }
    checkGarbage();

    if (verbosity >= 1 && elimclauses.size() > 0)
        printf("c |  Eliminated clauses:     %10.2f Mb                                      |\n",
               double(elimclauses.size() * sizeof(uint32_t)) / (1024 * 1024));

    statistics.simpSeconds += cpuTime() - simp_time;

    return ok;
}


//=================================================================================================
// Garbage Collection methods:


void SimpSolver::relocAll(ClauseAllocator &to)
{
    if (!use_simplification) return;

    // All occurs lists:
    //
    occurs.cleanAll();
    if (occurs.size() >= nVars()) {
        for (int i = 0; i < nVars(); i++) {
            for (int pol = 0; pol < 2; pol++) {
                vec<CRef> &cs = occurs[mkLit(i, pol == 0)];
                assert((solves == 0 || cs.size() == 0) && "There should be no occurrences during solving");
                for (int j = 0; j < cs.size(); j++) ca.reloc(cs[j], to);
                statistics.simpSteps += cs.size();
            }
        }
    }

    // Subsumption queue:
    //
    assert((solves == 0 || subsumption_queue.size() == 0) &&
           "There should be no occurrences subsumption candidates during solving");
    for (int i = subsumption_queue.size(); i > 0; i--) {
        CRef cr = subsumption_queue.peek();
        subsumption_queue.pop();
        statistics.simpSteps++;
        if (ca[cr].mark()) continue;
        ca.reloc(cr, to);
        subsumption_queue.insert(cr);
    }

    // Temporary clause:
    //
    ca.reloc(bwdsub_tmpunit, to);
}


void SimpSolver::garbageCollect()
{
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(counter_access, ca.size() - ca.wasted());

    to.extra_clause_field = ca.extra_clause_field; // NOTE: this is important to keep (or lose) the extra fields.
    relocAll(to);
    Solver::relocAll(to);
    if (verbosity >= 2)
        printf("c |  Garbage collection:   %12d bytes => %12d bytes             |\n",
               ca.size() * ClauseAllocator::Unit_Size, to.size() * ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}

Lit SimpSolver::subsumes(Clause &c1, Clause &c2)
{

    Lit ret = lit_Undef;
    if (c1.size() > c2.size() || (c1.abstraction() & ~c2.abstraction()) != 0) {
        return lit_Error;
    }

    counter++;

    for (int i = 0; i < c2.size(); i++) seen2[c2[i].x] = counter;
    for (int i = 0; i < c1.size(); i++) {
        if (seen2[c1[i].x] != counter) {
            if (ret == lit_Undef && seen2[(~c1[i]).x] == counter)
                ret = c1[i];
            else
                ret = lit_Error;
        }
    }
    return ret;
}

void SimpSolver::printStats()
{
    printf("c simplification        : %" PRIu64 " elim.vars,  %" PRIu64 " subsumed\n", eliminated_vars, total_subsumed);
    printf("c strengthening         : %" PRIu64 " lits, %" PRIu64 " all-candidates, %" PRIu64 " all-used\n",
           total_deleted_literals, total_all_strength_candidates, total_all_strengthened);
    Solver::printStats();
}

void SimpSolver::diversify(int rank, int size)
{
    /* rank ranges from 0 to size-1 */

    /* special second configuration */
    if (rank == 1 && size > 1) {
        use_simplification = false;
    } else {
        /* keep first 2 configurations as is,
        and disable simplification for last 2 configurations */
        if (rank > 1 && rank >= size - 2) use_simplification = false;

        /* Use gate detection in BVE for every 3rd thread */
        if (rank > 2 && (rank % 3) == 0) bveDetectGates = true;

        /* Use semantic gate detection in BVE for every 3rd thread, offset 1 */
        if (rank > 2 && (rank % 3) == 1) {
            bveDetectGates = true;
            bveSemanticDetection = true;
        }

        /* Keep small clauses that can be strengthened longer */
        if (rank > 2 && (rank % 5) == 0) {
            allStrengtheningMaxSize = 3;
        }

        /* allow higher grow value for last 2 configurations with simplfication */
        if (rank > 4 && rank >= size - 4) grow = 8;

        /* have a configuration allowed to simplify more on longer clauses */
        if (rank > 6 && rank >= size - 5) clause_lim = 40;

        /* have a few more configurations not use simplification */
        if (rank > 5 && rank % 7 == 2) use_simplification = false;
    }

    Solver::diversify(rank, size);

    /* in case we shall not*/
    if (!use_simplification) eliminate(true);
}

void SimpSolver::addConstrainClause(vec<Lit> &newConstrainClause)
{
    Solver::addConstrainClause(newConstrainClause);

    /* Make sure we cannot modify the semantics of this clause*/
    for (int i = 0; i < constrain_clause.size(); ++i) setFrozen(var(constrain_clause[i]), true);
}

void SimpSolver::reset_constrain_clause()
{
    for (int i = 0; i < constrain_clause.size(); ++i) setFrozen(var(constrain_clause[i]), false);
    Solver::reset_constrain_clause();
}