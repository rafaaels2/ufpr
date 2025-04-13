/****************************************************************************************[Solver.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
           Copyright (c) 2007-2010, Niklas Sorensson

Chanseok Oh's MiniSat Patch Series -- Copyright (c) 2015, Chanseok Oh

Maple_LCM, Based on MapleCOMSPS_DRUP -- Copyright (c) 2017, Mao Luo, Chu-Min LI, Fan Xiao: implementing a learnt clause
minimisation approach Reference: M. Luo, C.-M. Li, F. Xiao, F. Manya, and Z. L. , “An effective learnt clause
minimization approach for cdcl sat solvers,” in IJCAI-2017, 2017, pp. to–appear.

Maple_LCM_Dist, Based on Maple_LCM -- Copyright (c) 2017, Fan Xiao, Chu-Min LI, Mao Luo: using a new branching heuristic
called Distance at the beginning of search

Maple_LCM_Dist-alluip-trail -- Copyright (c) 2020, Randy Hickey and Fahiem Bacchus,
Based on Trail Saving on Backtrack SAT 2020 paper.
MapleLCMDistChronoBT, based on Maple_LCM_Dist -- Copyright (c) 2018, Alexander Nadel, Vadim Ryvchin: "Chronological Backtracking" in SAT-2018, pp. 111-121.

MapleLCMDistChronoBT-DL, based on MapleLCMDistChronoBT -- Copyright (c) 2019, Stepan Kochemazov, Oleg Zaikin, Victor Kondratiev,
Alexander Semenov: The solver was augmented with heuristic that moves duplicate learnt clauses into the core/tier2 tiers depending on a number of parameters.

MapleLCMDistChronoBT-DL, based on MapleLCMDistChronoBT -- Copyright (c) 2020, Stepan Kochemazov, Oleg Zaikin, Victor Kondratiev,
Alexander Semenov: The solver was augmented with heuristic that moves duplicate learnt clauses into the core/tier2 tiers depending on a number of parameters.

MapleLCMDistChronoBT-DL-f2trc, based on MapleLCMDistChronoBT -- Copyright (c) 2020, Stepan Kochemazov
The deterministic variant of the DL-version with modified procedures for handling Tier 2 clauses
and with added procedures for purging Core learnts.

RelaxedLCMDCBDLnewTech -- Copyright (c) 2020, Xindi Zhang and Shaowei Cai: rephasing

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

#ifndef MergeSat_Solver_h
#define MergeSat_Solver_h

#define ANTI_EXPLORATION

#define GLUCOSE23
//#define INT_QUEUE_AVG
//#define LOOSE_PROP_STAT

#ifdef GLUCOSE23
#define INT_QUEUE_AVG
#define LOOSE_PROP_STAT
#endif

#include "core/Proofs.h"
#include "core/SolverTypes.h"
#include "mtl/Alg.h"
#include "mtl/Heap.h"
#include "mtl/Sort.h"
#include "mtl/Vec.h"
#include "utils/ccnr.h"


// Don't change the actual numbers.
#define LOCAL 0
#define TIER2 2
#define CORE 3

#ifdef DEBUG
#define TRACE(x)                                                                                                       \
    if (verbosity > 1) {                                                                                               \
        x;                                                                                                             \
    }
#else
#define TRACE(x)
#endif

#include <vector>

namespace MERGESAT_NSPACE
{

class Lookahead;
class ParSolver;

//=================================================================================================
// Solver -- the main class:

class Solver
{
    private:
    template <typename T> class MyQueue
    {
        int max_sz, q_sz;
        int ptr;
        int64_t sum;
        vec<T> q;

        public:
        MyQueue(int sz) : max_sz(sz), q_sz(0), ptr(0), sum(0)
        {
            assert(sz > 0);
            q.growTo(sz);
        }
        inline bool full() const { return q_sz == max_sz; }
#ifdef INT_QUEUE_AVG
        inline T avg() const
        {
            assert(full());
            return sum / max_sz;
        }
#else
        inline double avg() const
        {
            assert(full());
            return sum / (double)max_sz;
        }
#endif
        inline void clear()
        {
            sum = 0;
            q_sz = 0;
            ptr = 0;
        }
        void push(T e)
        {
            if (q_sz < max_sz)
                q_sz++;
            else
                sum -= q[ptr];
            sum += e;
            q[ptr++] = e;
            if (ptr == max_sz) ptr = 0;
        }
    };

    public:
    // Constructor/Destructor:
    //
    Solver();
    virtual ~Solver();

    // Problem specification:
    //
    Var newVar(bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.
    void reserveVars(Var vars);                         // Reserve space for given amount of variables

    bool addClause(const vec<Lit> &ps);  // Add a clause to the solver.
    bool addEmptyClause();               // Add the empty clause, making the solver contradictory.
    bool addClause(Lit p);               // Add a unit clause to the solver.
    bool addClause(Lit p, Lit q);        // Add a binary clause to the solver.
    bool addClause(Lit p, Lit q, Lit r); // Add a ternary clause to the solver.
    bool addClause_(vec<Lit> &ps);       // Add a clause to the solver without making superflous internal copy. Will
    // change the passed vector 'ps'.
    void addInputClause_(vec<Lit> &ps); // Add a clause to the online proof checker

    void addConstrainClause(vec<Lit> &newConstrainClause); // At least one literal of the given clause has to be satisfied in the next call to solve
    bool failed_constraint();                              // Indicate whether the constrain clause was respected
    void deactivate_constrain_clause(lbool status); // Memorize that we do not use the current constrain clause in the next iteration, but allow cleanup
    void reset_constrain_clause(); // Reset information about constrain_clause

    // Solving:
    //
    bool simplify();                     // Removes already satisfied clauses.
    bool solve(const vec<Lit> &assumps); // Search for a model that respects a given set of assumptions.
    lbool solveLimited(const vec<Lit> &assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
    bool solve();                                // Search without assumptions.
    bool solve(Lit p);                           // Search for a model that respects a single assumption.
    bool solve(Lit p, Lit q);        // Search for a model that respects two assumptions.
    bool solve(Lit p, Lit q, Lit r); // Search for a model that respects three assumptions.
    bool okay() const;               // FALSE means solver is in a conflicting state

    void toDimacs(FILE *f, const vec<Lit> &assumps); // Write CNF to file in DIMACS-format.
    void toDimacs(const char *file, const vec<Lit> &assumps);
    void toDimacs(FILE *f, Clause &c, vec<Var> &map, Var &max);

    // IPASIR:
    //
    void setTermCallback(void *state, int (*termCallback)(void *));
    void setLearnCallback(void *state, int maxLength, void (*learn)(void *state, int *clause));
    template <class V> void shareViaCallback(const V &v, int lbd = -1);

    // Convenience versions of 'toDimacs()':
    void toDimacs(const char *file);
    void toDimacs(const char *file, Lit p);
    void toDimacs(const char *file, Lit p, Lit q);
    void toDimacs(const char *file, Lit p, Lit q, Lit r);

    // Variable mode:
    //
    void setPolarity(Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
    void setDecisionVar(Var v, bool b); // Declare if a variable should be eligible for selection in the decision heuristic.

    // Read state:
    //
    lbool value(Var x) const; // The current value of a variable.
    lbool value(Lit p) const; // The current value of a literal.
    lbool modelValue(Var x) const; // The value of a variable in the last model. The last call to solve must have been satisfiable.
    lbool modelValue(Lit p) const; // The value of a literal in the last model. The last call to solve must have been satisfiable.
    int nAssigns() const;                          // The current number of assigned literals.
    size_t nClauses() const;                       // The current number of original clauses.
    int nLearnts() const;                          // The current number of learnt clauses.
    size_t nUnits() const;                         // The current number of unit clauses.
    const Lit getUnit(size_t unit_idx) const;      // The clause with a given index;
    const Clause &getClause(size_t cls_idx) const; // The clause with a given index;
    int nVars() const;                             // The current number of variables.
    int nFreeVars() const;

    // Resource contraints:
    //
    void setConfBudget(int64_t x);
    void setPropBudget(int64_t x);
    void budgetOff();
    void interrupt();      // Trigger a (potentially asynchronous) interruption of the solver.
    void clearInterrupt(); // Clear interrupt indicator flag.

    // Memory managment:
    //
    virtual void garbageCollect();
    void checkGarbage(double gf);
    void checkGarbage();

    // Incremental mode:
    void setIncrementalMode();
    lbool prefetchAssumptions(); /// check whether we can apply all assumptions in one go

    // Extra results: (read-only member variable)
    //
    vec<lbool> model;  // If problem is satisfiable, this vector contains the model (if any).
    vec<Lit> conflict; // If problem is unsatisfiable (possibly under assumptions),
    // this vector represent the final conflict clause expressed in the assumptions.

    // Mode of operation:
    //
    bool reparsed_options; // Indicate whether the update parameter method has been used
    int verbosity;
    int status_every; // print status update every X conflicts
    double step_size;
    double step_size_dec;
    double min_step_size;
    int var_decay_timer, var_decay_timer_init;
    double var_decay;
    double clause_decay;
    double random_var_freq;
    double random_seed;

    int ccmin_mode;          // Controls conflict clause minimization (0=none, 1=basic, 2=deep).
    int phase_saving;        // Controls the level of phase saving (0=none, 1=limited, 2=full).
    bool invert_pol;         // Use inverse polarity for initialization
    int init_act;            // Initialize variable activities (0=no, 1=with a small random value, 2=1/v, 3=v).
    int init_act_init;       // Initial value
    double garbage_frac;     // The fraction of wasted memory allowed before a garbage collection is triggered.
    bool gc_in_assign_order; // Use order of variables in trail and decision heap for

    int restart_first;        // The initial restart limit. (default 100)
    double learntsize_factor; // The intitial limit for learnt clauses is a factor of the original clauses. (default 1 / 3)
    double learntsize_inc;    // The limit for learnt clauses is multiplied with this factor each restart. (default 1.1)

    int learntsize_adjust_start_confl;
    double learntsize_adjust_inc;
    // AllUIP Learning
    class UIPmini
    {
        public:
        UIPmini(bool uipAnalyze, bool uipMinimization);
        bool i_uip;
        bool i_mini;
        // variable for recordings
        uint64_t i_uip_decisions;
        uint64_t i_uip_attempts;
        uint64_t i_uip_global_decisions;
        uint64_t i_uip_global_dropped_lits;
        uint64_t i_uip_global_attempts;
        int i_uip_gap;

        /// data structures for analysis
        vec<Lit> to_be_bumped;
        vec<Lit> to_be_cleaned;
        vec<CRef> clause_to_be_bumped;
        vec<Lit> resolved_away;
        vec<Lit> new_introduced;
        vec<Lit> new_analyze_toclear;
        vec<Lit> new_out_learnt;
        vec<int> i_uip_pathC;
        std::vector<int> top_i;

        /// clear all used container
        void prepareNextAnalysis();

        /// reset counters
        void restartUpdate();
    };
    UIPmini uipMini;

    // Statistics: (read-only member variable)
    //
    uint64_t solves, starts, decisions, rnd_decisions, propagations, conflicts, conflicts_VSIDS;
    uint64_t dec_vars, clauses_literals, learnts_literals, max_literals, tot_literals;
    uint64_t chrono_backtrack, non_chrono_backtrack;
    uint64_t backuped_trail_lits, used_backup_lits;

    vec<uint32_t> picked;
    vec<uint32_t> conflicted;
#ifdef ANTI_EXPLORATION
    vec<uint32_t> canceled;
#endif

    /** choose polarity to pick, if not phase saving (0: outside solver, 1: solving) **/
    int systematic_branching_state;
    /** Should we pick one polarity all the time? **/
    uint32_t posMissingInSome, negMissingInSome;

    struct Restart {
        uint64_t savedDecisions, savedPropagations, partialRestarts;
        uint32_t selection_type; // 0 = 0, 1 = matching trail, 2 = reused trail
        Restart(uint32_t type) : savedDecisions(0), savedPropagations(0), partialRestarts(0), selection_type(type) {}
    } restart;

    int restart_strategy;
    class RestartStrategy
    {
        enum Strategy {
            SPLIT = 1,
            LUBY = 2,
            DYNAMIC = 3,
        } strategy; // 1 = split luby+dynamic; 2 = luby only, 3 = dynamic only
        int current_luby_restarts;
        MyQueue<int> lbd_queue; // For computing moving averages of recent LBD values.
        float global_lbd_sum;
        uint64_t conflicts, conflicts_VSIDS;
        bool cache_restart_until_conflict;
        float lbd_avg_compare_limit;
        int max_lbd;

        public:
        double luby(int x)
        {
            // Find the finite subsequence that contains index 'x', and the
            // size of that subsequence:
            int size, seq;
            for (size = 1, seq = 0; size < x + 1; seq++, size = 2 * size + 1)
                ;

            while (size - 1 != x) {
                size = (size - 1) >> 1;
                seq--;
                x = x % size;
            }
            return 1 << seq; /* pow(2,seq) */
        }

        RestartStrategy(float opt_lbd_avg_compare_limit, int queue_size)
          : strategy(SPLIT)
          , current_luby_restarts(0)
          , lbd_queue(queue_size)
          , global_lbd_sum(0)
          , conflicts(0)
          , conflicts_VSIDS(0)
          , cache_restart_until_conflict(false)
          , lbd_avg_compare_limit(opt_lbd_avg_compare_limit)
          , max_lbd(50)
        {
        }

        /* give number of conflicts for the next search */
        int nextRestartConflicts(bool usingVSIDS, int restart_base)
        {
            if ((strategy == SPLIT && usingVSIDS) || (strategy == DYNAMIC)) return INT32_MAX;
            if ((strategy == SPLIT && !usingVSIDS) || (strategy == LUBY)) {
                int nof_conflicts = luby(current_luby_restarts) * restart_base;
                current_luby_restarts++;
                return nof_conflicts;
            }
            assert(false && "strategy not implemented, abort!");
            exit(1);
            return 0;
        }

        /* check whether a restart should be triggered */
        bool triggerRestart(bool usesVSIDS, int nof_conflicts)
        {
            bool restart = false;
            if ((strategy == SPLIT && !usesVSIDS) || strategy == LUBY) return nof_conflicts <= 0;
            if ((strategy == SPLIT && usesVSIDS) || strategy == DYNAMIC) {
                if (!cache_restart_until_conflict) {
                    restart = lbd_queue.full() && (lbd_queue.avg() * lbd_avg_compare_limit > global_lbd_sum / conflicts_VSIDS);
                    /* remember decision whether to trigger a restart until the next conflict */
                    cache_restart_until_conflict = true;
                }
            }
            return restart;
        }

        /* call before enterint the next call to the SAT solver for solving */
        void notifyNewSearch(int restart_strategy, bool usingVSIDS)
        {
            if (restart_strategy == 1)
                strategy = SPLIT;
            else if (restart_strategy == 2)
                strategy = LUBY;
            else if (restart_strategy == 3)
                strategy = DYNAMIC;

            current_luby_restarts = 0;
            /* might need to reset conflicts and conflicts_VSIDS as well, test for incremental setup */
        }

        /* call when a new conflict is identified */
        void notifyConflict(bool usingVSIDS, int lbd)
        {
            if ((strategy == SPLIT && usingVSIDS) || (strategy == DYNAMIC)) {
                cache_restart_until_conflict = false;
                conflicts_VSIDS++;
                lbd_queue.push(lbd);
                global_lbd_sum += (lbd > max_lbd ? max_lbd : lbd);
            }
            conflicts++;
        }

        /* reset structures for next restart calculation */
        void performRestart()
        {
            lbd_queue.clear();
            cache_restart_until_conflict = false;
        }
    } restartStrategy;

    /** calculate the level to jump to for restarts */
    int getRestartLevel();

    uint64_t VSIDS_conflicts;    // conflicts after which we want to switch back to VSIDS
    uint64_t VSIDS_propagations; // propagated literals after which we want to switch back to VSIDS
    bool reactivate_VSIDS;       // indicate whether we change the decision heuristic back to VSIDS

    uint64_t inprocessing_C, inprocessing_L, inprocess_mems, inprocessings; // stats wrt improcessing simplification

    /// Single object to hold most statistics
    struct SolverStats {
        double simpSeconds, solveSeconds;
        uint64_t simpSteps, solveSteps;

        SolverStats() : simpSeconds(0), solveSeconds(0), simpSteps(0), solveSteps(0) {}

    } statistics;

    // Proof related things
    Proof proof;

    protected:
    vec<char> check_hit_vec;

    /// Control when to use which technique
    class TechniqueStartConflicts
    {
        uint64_t minimizeConflicts = 1000;
        uint64_t litRedundantConflicts = 2000;
        uint64_t lcmConflicts = 3000;
        uint64_t currentConflicts = 0;

        public:
        TechniqueStartConflicts(uint64_t mini = 1000, uint64_t redundant = 2000, uint64_t lcm = 3000)
          : minimizeConflicts(mini), litRedundantConflicts(redundant), lcmConflicts(lcm), currentConflicts(0)
        {
        }
        bool allowMinimize() { return currentConflicts >= minimizeConflicts; }
        bool allowLitRedundant() { return currentConflicts >= litRedundantConflicts; }
        bool allowLcm() { return currentConflicts >= lcmConflicts; }
        void reset() { currentConflicts = 0; }
        void nextConflicts() { ++currentConflicts; }
    } techniqueStartConflicts;
    int64_t max_vars_to_bump_per_analysis;

    // Helper structures:
    //
    struct VarData {
        CRef reason;
        int level;
    };
    static inline VarData mkVarData(CRef cr, int l)
    {
        VarData d = { cr, l };
        return d;
    }

    // moved Watcher structure to solver types

    struct VarOrderLt {
        const vec<double> &activity;
        bool operator()(Var x, Var y) const { return activity[x] > activity[y]; }
        VarOrderLt(const vec<double> &act) : activity(act) {}
    };

    struct ConflictData {
        ConflictData() : nHighestLevel(-1), secondHighestLevel(-1), bOnlyOneLitFromHighest(false) {}

        int nHighestLevel;
        int secondHighestLevel;
        bool bOnlyOneLitFromHighest;
    };

    struct watchItem {
        Watcher w;
        Lit l;
        watchItem(const Watcher &_w, const Lit _l) : w(_w), l(_l) {}
    };
    vec<watchItem> lazySATwatch;

    Lookahead *lookAhead;              // look ahead methods
    int lookAheadDecisionLevel;        // decision level until which we will use look ahead based decisions
    int lookAheadRecompute;            // how frequent should preselection scores be recomputed
    bool lookAheadRepeat;              // repeat lookahead, as we returned a conflict last time
    bool dlaDecision;                  // use double look ahead during decisions
    int lookAheadEvery;                // only use look ahead every X attempts
    uint64_t la_stat_conflicts;        // conflicts detected by look ahead
    uint64_t la_stat_decisions;        // decisions performed by look ahead
    uint64_t la_stat_nodecisions;      // number of times LA did not return a decision, but also no error
    uint64_t la_stat_extra_recomputes; // number of times the pre-scores for LA had to be re-calculated
    uint64_t la_stat_attempts;         // number of times we actually tried a look ahead decision

    // Solver state:
    //
    bool ok; // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    void set_not_okay();    // Set solver object status to false
    vec<CRef> clauses;      // List of problem clauses.
    vec<CRef> learnts_core, // List of learnt clauses.
    learnts_tier2, learnts_local;
    double cla_inc;           // Amount to bump next clause with.
    vec<double> activity_CHB, // A heuristic measurement of the activity of a variable.
    activity_VSIDS, activity_distance;
    double var_inc;                                          // Amount to bump next variable with.
    OccLists<Lit, vec<Watcher>, WatcherDeleted> watches_bin, // Watches for binary clauses only.
    watches; // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    vec<lbool> assigns;   // The current assignments.
    vec<char> polarity;   // The preferred polarity of each variable.
    vec<char> decision;   // Declares if a variable is eligible for selection in the decision heuristic.
    vec<Lit> trail;       // Assignment stack; stores all assigments made in the order they were made.
    vec<int> trail_lim;   // Separator indices for different decision levels in 'trail'.
    vec<VarData> vardata; // Stores reason and level for each variable.
    int qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    bool use_backuped_trail; // store the trail during backtracking, and use it during propagation
    int old_trail_qhead;     // head of trail stored during backtracking
    vec<Lit> old_trail;      // trail stored during backtracking
    vec<CRef> oldreasons;    // reason clauses of trail stored during backtracking
    int simpDB_assigns;      // Number of top-level assignments since last execution of 'simplify()'.
    int64_t simpDB_props; // Remaining number of propagations that must be made before next execution of 'simplify()'.
    vec<Lit> assumptions; // Current set of assumptions provided to solve by the user.
    vec<Lit> constrain_clause; // At least one literals of these ones has to be satisfied, to satisfy the formula
    bool constraint_active;    // Are we currently using the constrain clause? Reset this flag after each call to solve.
    bool unsat_constraint;     // Indicate that the constrain clause has not been satisfied

    /* list the possible modes the decision heuristic could be in */
    enum decision_heuristic {
        VSIDS_CHB,      /* in VSIDS, switching back to CHB */
        VSIDS_DISTANCE, /* in VSIDS, switching back to DISTANCE */
        CHB,
        DISTANCE
    };
    decision_heuristic current_heuristic;
    bool usesVSIDS() const;
    bool usesCHB() const;
    bool usesDISTANCE() const;
    bool considersDISTANCE() const;
    void disableDISTANCEheuristic();
    void enableDISTANCEheuristic();
    vec<Var> decision_rebuild_vars; // Set of variables that need to be used as decisions
    vec<int> distance_level_incs;
    Heap<VarOrderLt> order_heap_VSIDS, order_heap_CHB, order_heap_DISTANCE;
    Heap<VarOrderLt> *order_heap; // A priority queue of variables ordered with respect to the variable activity.
    int max_lbd_calc_size;        // How many literals a clause should have before estimating its LBD
    PowFixedBase bumpPow;         // Use this object for fixed exponentiation

    int full_heap_size; // Store size of heap in case it is completely filled, to be able to compare it to current size
    bool remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.
    bool check_satisfiability;            // collect full formula, and check SAT answers before returning them
    bool check_satisfiability_simplified; // in case we used formula simplification, check satisfiability only after extendModel

    /// class to perform satisfiability checks inside the solver
    class SATchecker
    {
        vec<Lit> check_formula; // this vector represents all literals of clauses that have been added to the solver, lit_Undef separates clauses

        public:
        /** add clause to tracking */
        void addClause(const vec<Lit> &clause)
        {
            for (int i = 0; i < clause.size(); ++i) check_formula.push(clause[i]);
            check_formula.push(lit_Undef);
        }

        /** check model, return true if model satisfies formula */
        bool checkModel(const vec<lbool> &model)
        {
            bool valid = true;
            bool satisfied_current_clause = false;
            int last_litUndef = -1;
            vec<Lit> d;
            for (int i = 0; i < check_formula.size(); ++i) {
                const Lit &l = check_formula[i];

                // handle end of clause, abort if last clause was not satisfied
                if (l == lit_Undef) {
                    if (!satisfied_current_clause) {
                        valid = false;
                        d.clear();
                        for (int j = last_litUndef + 1; j < i; ++j) d.push(check_formula[j]);
                        std::cout << "c clause " << d << " is not satisfied by the model" << std::endl;
                        break;
                    }
                    satisfied_current_clause = false; // start fresh for next clause
                    last_litUndef = i;
                } else {
                    Var v = var(l);
                    if (model.size() < v) continue; // this variable is not covered by the model

                    // either clause is already satisfied, or this literal is satisfied
                    satisfied_current_clause =
                    satisfied_current_clause || (sign(l) && model[v] == l_False) || (!sign(l) && (model[v] != l_False));
                }
            }
            assert(valid && "the current clause should have been satisfied by the model");
            return valid;
        }
    };
    SATchecker satChecker;

    vec<Lit> learnt_clause; // Container, used to store result of conflict analysis
    vec<Lit> learntUIPclauses;

    int core_lbd_cut;
    int tier2_lbd_cut;
    int core_size_lim;
    float core_size_lim_inc;

    uint64_t next_T2_reduce, next_L_reduce;

    AccessCounter counter_access; // count stats
    ClauseAllocator ca;

    int64_t confl_to_chrono;
    int64_t chrono;

    // IPASIR data
    void *termCallbackState;
    int (*termCallback)(void *state);
    void *learnCallbackState;
    std::vector<int> learnCallbackBuffer;
    int learnCallbackLimit;
    void (*learnCallback)(void *state, int *clause);

    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    vec<char> seen;
    vec<Lit> analyze_stack;
    vec<Lit> analyze_toclear;
    vec<Lit> add_tmp;
    vec<Lit> add_oc;

    vec<uint64_t> seen2; // Mostly for efficient LBD computation. 'seen2[i]' will indicate if decision level or variable 'i' has been seen.
    uint64_t counter; // Simple counter for marking purpose with 'seen2'.

    // self-subsuming resolution and subsumption during search
    vec<uint64_t> M;
    std::vector<std::vector<CRef>> O; // occurrence data structure
    uint64_t T, inprocess_attempts, inprocess_next_lim;
    int L, inprocess_learnt_level;
    double inprocess_inc; // control how frequent inprocessing is triggered
    uint64_t inprocess_penalty;
    bool inprocessing();

    bool check_invariants(); // check consistency of solver state

    double max_learnts;
    double learntsize_adjust_confl;
    int learntsize_adjust_cnt;
    int maxLearnAllUIPsLevel;
    uint64_t allUIPlearntClauses, allUIPlearntClausesTotalSize, allUIPlearntClausesTotalLBDs;

    struct OTFSS {
        CRef cr;
        int backjumpLevel;
        Lit l;
        OTFSS(CRef ref, int jumplevel, Lit lit) : cr(ref), backjumpLevel(jumplevel), l(lit) {}
    };
    vec<OTFSS> otfsss;
    /// process the otfss candidates, might backjump, enqueue and report conflicts
    CRef processOTFSScandidates(int lbd);
    int otfssMaxLBD; // only process otfss candidates when we have an LBD of this or less
    uint64_t totalOtfssClauses, totalOtfssClauseSizes, otfssUnits, otfssImplied, otfssConflicts;

    class NecessaryAssignments
    {
        Solver &s;
        int verbosity;

        public:
        int max_necc_binary; // select opt_necessary_binary binary clauses (at most 16 to fit in 32bit vector)
        int attempt_every;   // run neccessary assignment analysis every X level-1 decisions
        int recheck_inc; // only check a literal again, if the number of binary clauses increased by X since the last check

        vec<Lit> usedLits;                 // Literals used during necessary assignments analysis
        vec<uint32_t> collectedPolarities; // hold polarity per tested literal
        vec<Lit> necessaryAssignments;     // set of literals to be enqueued and propagated next
        vec<uint32_t> lastCheckedBinaryClauses; // memorize number of binary clauses a literal has been present in last time we checked
        // stats
        uint64_t na_attempts, na_propagated_other_lits, na_other_conflicts, na_successful_attempts, na_implied_lits;

        NecessaryAssignments(Solver &solver, int maxBinaryClauses, int attemptEvery, int recheckBinInc);

        /// Check decision literal next on level 1 for necessary assignments in its clauses
        bool checkNecessaryAssignment(Lit next, CRef &confl);

        /// allow to set attemptEvery
        void setAttemptEvery(int attemptEvery) { attempt_every = attemptEvery; }
    };
    NecessaryAssignments nAssignment;

    // duplicate learnts version
    uint64_t VSIDS_props_limit, VSIDS_props_init_limit;
    bool switch_mode;

    // Resource contraints:
    //
    int64_t conflict_budget;    // -1 means no budget.
    int64_t propagation_budget; // -1 means no budget.
    bool asynch_interrupt;

    bool prefetch_assumptions; // assign all assumptions at once on the first levels
    void memorize_used_assumptions();
    int last_used_assumptions; // store how many assumptions have been assigned during last call to search before jumping back

    // Main internal methods:
    //
    void insertVarOrder(Var x);                         // Insert a variable in the decision order priority queue.
    Lit pickBranchLit(CRef &confl, bool autoLA = true); // Return the next decision variable.
    Lit pickLABranchLit(CRef &confl);                   // Return the next decision variable based on look ahead
    void newDecisionLevel();                            // Begins a new decision level.
    void uncheckedEnqueue(Lit p, int level, CRef from = CRef_Undef); // Enqueue a literal. Assumes value of literal is undefined.
    bool enqueue(Lit p, CRef from = CRef_Undef); // Test if fact 'p' contradicts current state, enqueue otherwise.
    CRef propagate();                            // Perform unit propagation. Returns possibly conflicting clause.
    void cancelUntil(int level, bool allow_trail_saving = false, bool updateHeuristics = true); // Backtrack until a certain level.
    void i_uip_analyze(vec<Lit> &out_learn, int i_level, vec<Lit> &analyze_toclear, int out_lbd, int out_btlevel);
    void analyze(CRef confl,
                 vec<Lit> &out_learnt,
                 int &out_btlevel,
                 int &out_lbd,
                 int learnAllUIPsLevel,
                 vec<Lit> &allUIPlearnts,
                 vec<OTFSS> &otfssInfo);              // (bt = backtrack)
    void analyzeFinal(Lit p, vec<Lit> &out_conflict); // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE
    void analyzeFinal(const CRef cr, vec<Lit> &out_conflict); // Find all assumptions that lead to the given conflict clause
    CRef createDecisionClause(Lit implied = lit_Undef); // Create the decision clause from the current decisions, add the implied lit if specified. Create in watchable order.
    template <class V>
    void analyzeFinal(const V &c, vec<Lit> &out_conflict); // Find all assumptions that lead to the conflict in the vector or clause
    bool litRedundant(Lit p, uint32_t abstract_levels); // (helper method for 'analyze()')
    bool litRedundant3(Lit p, uint32_t abstract_levels, vec<Lit> &new_analyze_toclear);
    lbool search(int &nof_conflicts); // Search for a given number of conflicts.
    lbool solve_();                   // Main solve method (assumptions given in 'assumptions').
    void reduceDB();                  // Reduce the set of learnt clauses.
    void reduceDB_Tier2();
    bool reduceDB_Core();                // Return true, if more than 5% have been deleted
    void removeSatisfied(vec<CRef> &cs); // Shrink 'cs' to contain only non-satisfied clauses.
    void safeRemoveSatisfied(vec<CRef> &cs, unsigned valid_mark);
    void rebuildOrderHeap();
    bool binResMinimize(vec<Lit> &out_learnt);     // Further learnt clause minimization by binary resolution.
    void toggle_decision_heuristic(bool to_VSIDS); // Switch between decision heuristic heaps.

    // Maintaining Variable/Clause activity:
    //
    void varDecayActivity(); // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    void varBumpActivity(Var v, double mult); // Increase a variable with the current 'bump' value.
    void claDecayActivity(); // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
    void claBumpActivity(Clause &c); // Increase a clause with the current 'bump' value.

    // Operations on clauses:
    //
    void attachClause(CRef cr);                                // Attach a clause to watcher lists.
    void detachClause(CRef cr, bool strict = false);           // Detach a clause to watcher lists.
    void removeClause(CRef cr, bool remove_from_proof = true); // Detach and free a clause.
    void removeSatisfiedClause(CRef cr, bool remove_from_proof = true);
    bool locked(const Clause &c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
    bool satisfied(const Clause &c) const; // Returns TRUE if a clause is satisfied in the current state.

    void relocWatches(ClauseAllocator &to, const Lit p); // relocated clauses from ca to to for literal p
    void relocAll(ClauseAllocator &to);                  // relocate all clauses

    // Misc:
    //
    int decisionLevel() const;           // Gives the current decisionlevel.
    uint32_t abstractLevel(Var x) const; // Used to represent an abstraction of sets of decision levels.
    CRef reason(Var x) const;

    ConflictData FindConflictLevel(CRef cind);

    public:
    int level(Var x) const;
    bool withinBudget() const;
    uint64_t counter_sum() const;
    virtual void printStats();

    protected:
    double progressEstimate() const; // DELETE THIS ?? IT'S NOT VERY USEFUL ...

    template <class V> int computeLBD(const V &c)
    {
        if (c.size() > max_lbd_calc_size) return c.size();

        int lbd = 0;
        const int assumption_level = assumptions.size(); // ignore anything below
        counter++;
        for (int i = 0; i < c.size(); i++) {
            int l = level(var(c[i]));
            if (l > assumption_level && seen2[l] != counter) {
                seen2[l] = counter;
                lbd++;
            }
        }

        return lbd;
    }

    template <class V> bool checkOnlineCheckerState(const V &d)
    {
        if (decisionLevel() != 0) return true; // lets not check other levels than 0!

        if (!proof.checkingEnabled()) return true;

        bool okay = true;
        for (int i = 0; i < trail.size(); ++i) {
            if (!proof.proofHasClause(trail[i]) && (d.size() != 1 || trail[i] != d[0])) {
                if (verbosity > 3) std::cout << "c online checker does not have clause " << trail[i] << std::endl;
                // okay = false;
            }
        }

        check_hit_vec.clear();
        check_hit_vec.growTo(2 * nVars() + 1, 0);
        for (int i = 0; i < d.size(); ++i) check_hit_vec[toInt(d[i])] = 1;

        for (int i = 0; i < 4; ++i) {
            const vec<CRef> &clause_set = i == 0 ? clauses : (i == 1 ? learnts_core : (i == 2 ? learnts_tier2 : learnts_local));

            for (int j = 0; j < clause_set.size(); ++j) {
                const Clause &c = ca[clause_set[j]];
                if (c.mark() == 1) continue;

                bool is_input = c.size() == d.size();
                if (is_input) {
                    for (int k = 0; k < c.size(); ++k) {
                        if (!check_hit_vec[toInt(c[k])]) {
                            is_input = false;
                            break;
                        }
                    }
                }

                if (!proof.proofHasClause(c) && !is_input) {
                    if (verbosity > 3) {
                        std::cout << "c online checker does not have non-unit clause [" << clause_set[j] << "] " << c << std::endl;
                    }
                    okay = false;
                }
            }
            assert(okay && "all present clauses should be present in the online checker as well");
        }

        if (verbosity > 3) std::cout << "c validated online checker state: " << okay << std::endl;
        return okay;
    }


    // Static helpers:
    //

    // Returns a random float 0 <= x < 1. Seed must never be 0.
    static inline double drand(double &seed)
    {
        seed *= 1389796;
        int q = (int)(seed / 2147483647);
        seed -= (double)q * 2147483647;
        return seed / 2147483647;
    }

    // Returns a random integer 0 <= x < size. Seed must never be 0.
    static inline int irand(double &seed, int size) { return (int)(drand(seed) * size); }

    int rand() { return Solver::irand(random_seed, INT32_MAX); }

    // simplify
    //
    public:
    bool simplifyAll();
    template <class C> void simplifyLearnt(C &c);
    bool isSimplifyDuplicate(CRef cr);
    /** simplify the learnt clauses in the given vector, move to learnt_core if is_tier2 is true*/
    bool simplifyLearnt(vec<CRef> &target_learnts, bool is_tier2 = false);
    int trailRecord;
    void litsEnqueue(int cutP, Clause &c);
    void cancelUntilTrailRecord();
    void simpleUncheckEnqueue(Lit p, CRef from = CRef_Undef);
    template <bool useAlwaysNegativeVar> CRef simplePropagate(Var alwaysFalseVar = var_Undef);
    uint64_t nbSimplifyAll;
    uint64_t simplified_length_record, original_length_record;
    uint64_t s_propagations, nr_lcm_duplicates;

    vec<Lit> simp_learnt_clause;
    vec<CRef> simp_reason_clause;
    void simpleAnalyze(CRef confl, vec<Lit> &out_learnt, vec<CRef> &reason_clause, bool True_confl);
    ClauseRingBuffer simplifyBuffer;

    // HordeSat Portfolio support
    bool share_parallel;                       // do send clauses for other parallel solvers
    int share_max_cls_size, share_max_cls_lbd; // allow to dynamicall control which clauses to consider for sharing
    bool receiveClauses;                       // do send clauses for other parallel solvers
    int share_clause_max_size;                 // max clause size for sharing
    int shared_unit_clauses;                   // count how many literals from level 0 of the trail we already shared
    uint64_t receivedCls;                      // count number of received clauses
    int sync_conflicts;                        // number of conflicts between to sync checks
    void (*learnedClsCallback)(const std::vector<int> &, int glueValue, void *issuer); // callback for clause learning
    void (*consumeSharedCls)(void *issuer); // get shared clauses from parallel solving and use them
    void *issuer;                           // used as the callback parameter
    int lastDecision;                       // the last decision made by the solver
    void shareUnitClauses();                // make sure all unit clauses from the trail have been shared
    template <class C>
    bool addLearnedClause(const C &cls, int lbd = -1, bool addToProof = false); // add a learned clause by hand
    template <class C> lbool prepareTopLevelClauseIntegration(C &ps);           // prepare top level clause addition
    void diversify(int rank, int size); // set parameters based on position in set, and set size
    bool importClause(const Clause &c, bool addToProof = false); // import 'input' (or formula) clause from another solver, and add to proof if required
    // parallel solving
    /// return true, if the current search should be stopped, and set the status variable for this solver
    bool (*external_sync_and_share)(void *issuer, lbool *status); // get shared clauses from parallel solving and use them
    bool sync_and_share(lbool *status_after_sync); // call external_sync_and_share, if set, otherwise, return default behavior, might write status value
    bool abort_search_after_sync; // indicate that we should not sync in this solve call anymore
    void initialize_parallel_solver(void *_issuer, bool (*_external_sync_and_share)(void *, lbool *)); // setup structures for parallel communication
    // allow accessing internal methods
    friend class Lookahead;
    friend class ParSolver;

    // Create clauses to partition the formula under the given assumptions into the given number of partitions
    lbool partitionFormula(vec<Lit> &baseAssumptions, int partitions, vec<Lit> &partitionClauses);

    // in redundant
    bool removed(CRef cr);
    // adjust simplifyAll occasion
    uint64_t curSimplify;
    uint64_t nbconfbeforesimplify;
    int incSimplify;
    bool lcm;
    bool reverse_LCM;
    bool lcm_core;         // apply LCM to generated conflict clause?
    bool lcm_core_success; // apply core simplification next time again?
    uint64_t LCM_total_tries, LCM_successful_tries, LCM_dropped_lits, LCM_dropped_reverse;

    bool collectFirstUIP(CRef confl);
    vec<double> var_iLevel, var_iLevel_tmp;
    uint64_t nbcollectfirstuip, nblearntclause, nbDoubleConflicts, nbTripleConflicts;
    int uip1, uip2;
    vec<int> pathCs;
    CRef propagateLits(vec<Lit> &lits);
    bool propagateLit(Lit l, vec<Lit> &implied); // propagate l, collect implied lits (without l), return if there has been a conflict
    double var_iLevel_inc;
    vec<Lit> involved_lits;
    double my_var_decay;

    void reset_old_trail();

    protected:
    bool use_ccnr, allow_rephasing;
    //  to avoid the init_soln of two LS too near.
    int restarts_gap;
    //  if trail.size() over c*nVars or p*max_trail, call ls.
    float conflict_ratio;
    float percent_ratio;
    //  control ls time total use.
    float up_time_ratio;
    //  control ls memory use per call.
    long long ls_mems_num;
    uint64_t state_change_time; // starts
    double state_change_time_inc, state_change_time_inc_inc;
    //  whether the mediation_soln is used as rephase, if not
    bool mediation_used;

    uint64_t switch_heristic_mod; // starts
    int last_switch_conflicts;

    int sls_var_lim, sls_clause_lim;

    // informations
    CCNR::ls_solver ccnr;
    int freeze_ls_restart_num = 0;
    double ls_used_time = 0;
    int ls_call_num = 0;
    int ls_best_unsat_num = INT_MAX;
    bool solved_by_ls = false, initial_sls, use_sls_phase = true;
    int max_trail = 0;
    bool called_initial_sls = false; // indicate, whether we called sls at all
    int solve_starts = 0;            // memorize how many restarts have been seen when starting another search


    // Phases
    // save the recent ls soln and best ls soln, need to call ls once.
    std::vector<char> ls_mediation_soln;
    // with the minimum unsat clauses num in LS.
    std::vector<char> ls_best_soln;
    // hold the soln with the best trail size.
    std::vector<char> top_trail_soln;

    // functions
    bool call_ls(bool use_up_build);
    void rand_based_rephase();
    void info_based_rephase();
};

// Method to update cli options from the environment variable MINISAT_RUNTIME_ARGS
bool updateOptions();

inline void Solver::addInputClause_(vec<Lit> &ps) { proof.addParsedclause(ps); }

//=================================================================================================
// Implementation of inline methods:

inline CRef Solver::reason(Var x) const { return vardata[x].reason; }
inline int Solver::level(Var x) const { return vardata[x].level; }

inline void Solver::insertVarOrder(Var x)
{
    if (!order_heap->inHeap(x) && decision[x]) order_heap->insert(x);
}

inline void Solver::varDecayActivity() { var_inc *= (1 / var_decay); }

inline void Solver::varBumpActivity(Var v, double mult)
{
    if ((activity_VSIDS[v] += var_inc * mult) > 1e100) {
        // Rescale:
        for (int i = 0; i < nVars(); i++) activity_VSIDS[i] *= 1e-100;
        var_inc *= 1e-100;
    }

    // Update order_heap with respect to new activity:
    if (usesVSIDS()) {
        if (order_heap->inHeap(v)) order_heap->decrease(v);
    }
}

inline void Solver::claDecayActivity() { cla_inc *= (1 / clause_decay); }
inline void Solver::claBumpActivity(Clause &c)
{
    if ((c.activity() += cla_inc) > 1e20) {
        // Rescale:
        for (int i = 0; i < learnts_local.size(); i++) ca[learnts_local[i]].activity() *= 1e-20;
        cla_inc *= 1e-20;
    }
}

inline void Solver::checkGarbage(void) { return checkGarbage(garbage_frac); }
inline void Solver::checkGarbage(double gf)
{
    if (ca.wasted() > ca.size() * gf) garbageCollect();
}

// NOTE: enqueue does not set the ok flag! (only public methods do)
inline bool Solver::enqueue(Lit p, CRef from)
{
    return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, decisionLevel(), from), true);
}
inline bool Solver::addClause(const vec<Lit> &ps)
{
    ps.copyTo(add_tmp);
    return addClause_(add_tmp);
}
inline bool Solver::addEmptyClause()
{
    add_tmp.clear();
    return addClause_(add_tmp);
}
inline bool Solver::addClause(Lit p)
{
    add_tmp.clear();
    add_tmp.push(p);
    return addClause_(add_tmp);
}
inline bool Solver::addClause(Lit p, Lit q)
{
    add_tmp.clear();
    add_tmp.push(p);
    add_tmp.push(q);
    return addClause_(add_tmp);
}
inline bool Solver::addClause(Lit p, Lit q, Lit r)
{
    add_tmp.clear();
    add_tmp.push(p);
    add_tmp.push(q);
    add_tmp.push(r);
    return addClause_(add_tmp);
}
inline bool Solver::locked(const Clause &c) const
{
    int i = c.size() != 2 ? 0 : (value(c[0]) == l_True ? 0 : 1);
    return value(c[i]) == l_True && reason(var(c[i])) != CRef_Undef && ca.lea(reason(var(c[i]))) == &c;
}
inline void Solver::newDecisionLevel() { trail_lim.push(trail.size()); }

inline int Solver::decisionLevel() const { return trail_lim.size(); }
inline uint32_t Solver::abstractLevel(Var x) const { return 1 << (level(x) & 31); }
inline lbool Solver::value(Var x) const { return assigns[x]; }
inline lbool Solver::value(Lit p) const { return assigns[var(p)] ^ sign(p); }
inline lbool Solver::modelValue(Var x) const { return model[x]; }
inline lbool Solver::modelValue(Lit p) const { return model[var(p)] ^ sign(p); }
inline int Solver::nAssigns() const { return trail.size(); }
inline size_t Solver::nClauses() const { return clauses.size(); }
inline size_t Solver::nUnits() const { return trail_lim.size() < 1 ? trail.size() : trail_lim[0]; }
inline const Lit Solver::getUnit(size_t unit_idx) const
{
    assert(unit_idx < nUnits() && "can only access valid unit clauses");
    return trail[unit_idx];
}
inline const Clause &Solver::getClause(size_t cls_idx) const
{
    assert(cls_idx < nClauses() && "can only access valid clauses");
    return ca[clauses[cls_idx]];
}
inline int Solver::nLearnts() const { return learnts_core.size() + learnts_tier2.size() + learnts_local.size(); }
inline int Solver::nVars() const { return vardata.size(); }
inline int Solver::nFreeVars() const { return (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]); }
inline void Solver::setPolarity(Var v, bool b) { polarity[v] = b; }
inline void Solver::setDecisionVar(Var v, bool b)
{
    if (b && !decision[v])
        dec_vars++;
    else if (!b && decision[v])
        dec_vars--;

    decision[v] = b;
    if (b && !order_heap->inHeap(v)) {
        order_heap->insert(v);
    }
}
inline void Solver::setConfBudget(int64_t x) { conflict_budget = conflicts + x; }
inline void Solver::setPropBudget(int64_t x) { propagation_budget = propagations + x; }
inline void Solver::interrupt() { asynch_interrupt = true; }
inline void Solver::clearInterrupt() { asynch_interrupt = false; }
inline void Solver::budgetOff() { conflict_budget = propagation_budget = -1; }
inline bool Solver::withinBudget() const
{
    return !asynch_interrupt && (conflict_budget < 0 || conflicts < (uint64_t)conflict_budget) &&
           (propagation_budget < 0 || propagations < (uint64_t)propagation_budget) &&
           (termCallback == 0 || 0 == termCallback(termCallbackState));
}
inline uint64_t Solver::counter_sum() const { return counter_access.sum(); }

// FIXME: after the introduction of asynchronous interrruptions the solve-versions that return a
// pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
// all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.
inline bool Solver::solve()
{
    budgetOff();
    assumptions.clear();
    return solve_() == l_True;
}
inline bool Solver::solve(Lit p)
{
    budgetOff();
    assumptions.clear();
    assumptions.push(p);
    return solve_() == l_True;
}
inline bool Solver::solve(Lit p, Lit q)
{
    budgetOff();
    assumptions.clear();
    assumptions.push(p);
    assumptions.push(q);
    return solve_() == l_True;
}
inline bool Solver::solve(Lit p, Lit q, Lit r)
{
    budgetOff();
    assumptions.clear();
    assumptions.push(p);
    assumptions.push(q);
    assumptions.push(r);
    return solve_() == l_True;
}
inline bool Solver::solve(const vec<Lit> &assumps)
{
    budgetOff();
    assumps.copyTo(assumptions);
    return solve_() == l_True;
}
inline lbool Solver::solveLimited(const vec<Lit> &assumps)
{
    assumps.copyTo(assumptions);
    return solve_();
}
inline bool Solver::okay() const { return ok; }

inline void Solver::set_not_okay() { ok = false; }

inline void Solver::toDimacs(const char *file)
{
    vec<Lit> as;
    toDimacs(file, as);
}
inline void Solver::toDimacs(const char *file, Lit p)
{
    vec<Lit> as;
    as.push(p);
    toDimacs(file, as);
}
inline void Solver::toDimacs(const char *file, Lit p, Lit q)
{
    vec<Lit> as;
    as.push(p);
    as.push(q);
    toDimacs(file, as);
}
inline void Solver::toDimacs(const char *file, Lit p, Lit q, Lit r)
{
    vec<Lit> as;
    as.push(p);
    as.push(q);
    as.push(r);
    toDimacs(file, as);
}

inline bool Solver::usesVSIDS() const { return current_heuristic == VSIDS_CHB || current_heuristic == VSIDS_DISTANCE; }

inline bool Solver::usesDISTANCE() const { return current_heuristic == DISTANCE; }

inline bool Solver::considersDISTANCE() const
{
    return current_heuristic == DISTANCE || current_heuristic == VSIDS_DISTANCE;
}

inline bool Solver::usesCHB() const { return current_heuristic == CHB; }

inline void Solver::setTermCallback(void *state, int (*termCallback)(void *))
{
    this->termCallbackState = state;
    this->termCallback = termCallback;
}

inline void Solver::setLearnCallback(void *state, int maxLength, void (*learn)(void *state, int *clause))
{
    this->learnCallbackState = state;
    this->learnCallbackLimit = maxLength;
    this->learnCallbackBuffer.resize(1 + maxLength);
    this->learnCallback = learn;
}

template <class V> inline void Solver::analyzeFinal(const V &c, vec<Lit> &out_conflict)
{
    out_conflict.clear();
    if (decisionLevel() == 0) return;
    for (int i = 0; i < c.size(); ++i) {
        if (level(var(c[i])) > 0) {
            seen[var(c[i])] = 1;
            printf("K\n");
        }
    }

    for (int i = trail.size() - 1; i >= trail_lim[0]; i--) {
        Var x = var(trail[i]);
        if (seen[x]) {
            if (reason(x) == CRef_Undef) {
                assert(level(x) > 0);
                out_conflict.push(~trail[i]);
            } else {
                const Clause &c = ca[reason(x)];
                for (int j = c.size() == 2 ? 0 : 1; j < c.size(); j++)
                    if (level(var(c[j])) > 0) seen[var(c[j])] = 1;
                statistics.solveSteps++;
            }
            seen[x] = 0;
        }
    }

    for (int i = 0; i < c.size(); ++i) seen[var(c[i])] = 0;
}

template <class C> inline void Solver::simplifyLearnt(C &c)
{
    ////
    original_length_record += c.size();

    trailRecord = trail.size(); // record the start pointer

    bool True_confl = false, reversed = false;
    int beforeSize = c.size(), preReserve = 0;
    int i, j;
    CRef confl = CRef_Undef;

    LCM_total_tries++;

    TRACE(std::cout << "c start simplifying clause " << c << std::endl);

    // try to simplify in reverse order, in case original succeeds
    for (size_t iteration = 0; iteration < (reverse_LCM ? 2 : 1); ++iteration) {
        True_confl = false;
        confl = CRef_Undef;
        statistics.solveSteps++;
        // reorder the current clause for next iteration?
        // (only useful if size changed in first iteration)
        if (iteration > 0) {
            if (c.size() == 1) break;
            c.reverse();
            reversed = !reversed;
            preReserve = c.size();
        }

        assert(decisionLevel() == 0 && "only run simplification on level 0");
        newDecisionLevel();
        assert(decisionLevel() == 1 && "only run simplification on level 0");

        for (i = 0, j = 0; i < c.size(); i++) {
            if (value(c[i]) == l_Undef) {
                // printf("///@@@ uncheckedEnqueue:index = %d. l_Undef\n", i);
                simpleUncheckEnqueue(~c[i]);
                c[j++] = c[i];
                confl = simplePropagate<false>();
                if (confl != CRef_Undef) {
                    break;
                }
            } else {
                if (value(c[i]) == l_True) {
                    // printf("///@@@ uncheckedEnqueue:index = %d. l_True\n", i);
                    c[j++] = c[i];
                    True_confl = true;
                    confl = reason(var(c[i]));
                    break;
                } else {
                    // printf("///@@@ uncheckedEnqueue:index = %d. l_False\n", i);
                }
            }
        }
        TRACE(if (c.size() - j > 0) {
            std::cout << "c shrink clause after propagation to " << c << " (dropping last " << c.size() - j
                      << " literals, reverse=" << reversed << ")" << std::endl;
        });
        c.shrink(c.size() - j);

        if (confl != CRef_Undef || True_confl == true) {
            simp_learnt_clause.clear();
            simp_reason_clause.clear();
            if (True_confl == true) {
                simp_learnt_clause.push(c.last());
            }
            simpleAnalyze(confl, simp_learnt_clause, simp_reason_clause, True_confl);

            if (simp_learnt_clause.size() < c.size()) {
                for (i = 0; i < simp_learnt_clause.size(); i++) {
                    c[i] = simp_learnt_clause[i];
                }
                TRACE(if (c.size() - i > 0) {
                    std::cout << "c shrink clause after analysis to " << c << " (dropping last " << c.size() - i
                              << " literals, reverse=" << reversed << ")" << std::endl;
                });
                c.shrink(c.size() - i);
            }
        }

        assert(decisionLevel() == 1);
        cancelUntil(0);
        cancelUntilTrailRecord();
        assert(decisionLevel() == 0);

        ////
        simplified_length_record += c.size();

        // printf("\nbefore : %d, after : %d ", beforeSize, afterSize);
        if (beforeSize == c.size() || c.size() <= 1) break;
        TRACE(std::cout << "c simplified clause to " << c << " (before size: " << beforeSize << ")" << std::endl);
        LCM_dropped_lits += (beforeSize - c.size());
        LCM_dropped_reverse = iteration == 0 ? LCM_dropped_reverse : LCM_dropped_reverse + (preReserve - c.size());
    }

    // make sure the original order is restored, in case we resorted
    if (reversed) c.reverse();

    LCM_successful_tries = beforeSize == c.size() ? LCM_successful_tries : LCM_successful_tries + 1;
}


// simplify All
template <bool useAlwaysNegativeVar> CRef Solver::simplePropagate(Var alwaysFalseVar)
{
    CRef confl = CRef_Undef;
    int num_props = 0;
    watches.cleanAll();
    watches_bin.cleanAll();
    while (qhead < trail.size()) {
        Lit p = trail[qhead++]; // 'p' is enqueued fact to propagate.
        vec<Watcher> &ws = watches[p];
        Watcher *i, *j, *end;
        num_props++;


        // First, Propagate binary clauses
        vec<Watcher> &wbin = watches_bin[p];

        for (int k = 0; k < wbin.size(); k++) {

            Lit imp = wbin[k].blocker;

            if (value(imp) == l_False) {
                return wbin[k].cref;
            }
            // when used, make sure the detected variable is always treated as false
            if ((useAlwaysNegativeVar) && var(imp) == alwaysFalseVar) {
                TRACE(std::cerr << "c simplePropagate conflict for literal " << p << " with clause " << ca[wbin[k].cref]
                                << std::endl);
                return wbin[k].cref;
            }

            if (value(imp) == l_Undef) {
                simpleUncheckEnqueue(imp, wbin[k].cref);
            }
        }
        for (i = j = (Watcher *)ws, end = i + ws.size(); i != end;) {
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            // Ignore the blocker, if it's the always false variable
            if (!(useAlwaysNegativeVar) || var(blocker) != alwaysFalseVar) {
                if (value(blocker) == l_True) {
                    *j++ = *i++;
                    continue;
                }
            }

            // Make sure the false literal is data[1]:
            CRef cr = i->cref;
            Clause &c = ca[cr];
            Lit false_lit = ~p;
            if (c[0] == false_lit) c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            //  i++;

            // If 0th watch is true, then clause is already satisfied.
            // However, 0th watch is not the blocker, make it blocker using a new watcher w
            // why not simply do i->blocker=first in this case?
            Lit first = c[0];
            //  Watcher w     = Watcher(cr, first);
            if (first != blocker && value(first) == l_True) {
                i->blocker = first;
                *j++ = *i++;
                continue;
            }

            for (int k = 2; k < c.size(); k++) {

                // Do not allow the alwaysFalseVar to satisfy the clause
                if ((useAlwaysNegativeVar) && var(c[k]) == alwaysFalseVar) continue;

                if (value(c[k]) != l_False) {
                    // watcher i is abandonned using i++, because cr watches now ~c[k] instead of p
                    // the blocker is first in the watcher. However,
                    // the blocker in the corresponding watcher in ~first is not c[1]
                    Watcher w = Watcher(cr, first);
                    i++;
                    c[1] = c[k];
                    c[k] = false_lit;
                    watches[~c[1]].push(w);
                    goto NextClause;
                }
            }

            // Did not find watch -- clause is unit under assignment:
            i->blocker = first;
            *j++ = *i++;
            if (value(first) == l_False || (useAlwaysNegativeVar && var(first) == alwaysFalseVar)) {
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end) *j++ = *i++;
            } else {
                simpleUncheckEnqueue(first, cr);
            }
        NextClause:;
        }
        ws.shrink(i - j);
    }

    s_propagations += num_props;

    return confl;
}


template <class C> inline lbool Solver::prepareTopLevelClauseIntegration(C &ps)
{
    assert(decisionLevel() == 0 && "Make sure we are allowed to modify the incoming clause");
    // Check if clause is satisfied and remove false/duplicate literals:
    MERGESAT_NSPACE::sort((Lit *)(&ps[0]), ps.size());
    Lit p;
    int i, j;
    bool somePositive = false;
    bool someNegative = false;

    TRACE(if (verbosity > 2) std::cout << "c adding clause " << ps << std::endl);

    for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
        if (value(ps[i]) == l_True || ps[i] == ~p)
            return l_True;
        else if (value(ps[i]) != l_False && ps[i] != p) {
            ps[j++] = p = ps[i];
            somePositive = somePositive || !sign(p);
            someNegative = someNegative || sign(p);
        } else if (value(ps[i]) == l_False) {
            // for polarity analysis, we ignore unit propagation
            somePositive = somePositive || !sign(ps[i]);
            someNegative = someNegative || sign(ps[i]);
        }
    ps.shrink(i - j);

    if (ps.size() > 1) {
        // memorize for the whole formula to feed polarity heuristic
        if (systematic_branching_state == 0) {
            posMissingInSome = somePositive ? posMissingInSome : posMissingInSome + 1;
            negMissingInSome = someNegative ? negMissingInSome : negMissingInSome + 1;
        }
    }

    // If there are no literals left, the clause is unsatisfiable
    return ps.size() > 0 ? l_Undef : l_False;
}


template <class C> inline bool Solver::addLearnedClause(const C &cls, int lbd, bool addToProof)
{
    /* allow to reject clause */
    if (!receiveClauses) return true;

    assert(!proof.enabled() && "parallel proof handling is not implemented");

    /* check the state of the initial clause */
    if (cls.size() == 0)
        return ok = false;
    else if (cls.size() == 1) {
        if (value(cls[0]) == l_True) return true;
        if (value(cls[0]) == l_False) return ok = false;
        uncheckedEnqueue(cls[0], 0);
        return ok = (propagate() == CRef_Undef);
    }

    assert(decisionLevel() == 0 && "only communicate on level 0");

    receivedCls++;
    CRef cr = ca.alloc(cls, true);
    Clause &imported = ca[cr];
    lbool integrated_successfully = prepareTopLevelClauseIntegration(imported);
    if (integrated_successfully == l_True) return true;

    if (addToProof) {
        assert(proof.enabled() && "activation needs to be consistent across layers");
        proof.addClause('a', imported);
    }

    if (integrated_successfully == l_False || imported.size() == 0)
        return ok = false;
    else if (imported.size() == 1) {
        if (value(imported[0]) == l_True) return true;
        uncheckedEnqueue(imported[0], 0);
        return ok = (propagate() == CRef_Undef);
    } else {
        /* we do not add shared clauses to core, due to duplicate restriction */
        if (lbd != -1 && lbd <= tier2_lbd_cut) {
            imported.set_lbd(lbd);
            learnts_tier2.push(cr);
            imported.mark(TIER2);
            imported.touched() = conflicts;
        } else {
            learnts_local.push(cr);
        }

        assert((value(imported[0]) != l_False || value(imported[1]) != l_False) && "cannot watch falsified literals");
        attachClause(cr);
        claBumpActivity(ca[cr]);
    }

    return true;
}

template <class V> inline void Solver::shareViaCallback(const V &v, int lbd)
{
    if (lbd > core_lbd_cut) return;
    if (v.size() > share_clause_max_size) return;

    bool filled_buffer = false;

    if (learnCallback != 0 && v.size() <= learnCallbackLimit) {
        learnCallbackBuffer.resize(v.size() + 1);
        for (int i = 0; i < v.size(); i++) {
            Lit lit = v[i];
            learnCallbackBuffer[i] = sign(lit) ? -(var(lit) + 1) : (var(lit) + 1);
        }
        learnCallbackBuffer[v.size()] = 0;
        filled_buffer = true;
        learnCallback(learnCallbackState, &(learnCallbackBuffer[0]));
    }

    /* share only limited clauses in parallel solving! */
    if (share_parallel && learnedClsCallback != 0 && (v.size() < share_max_cls_size || lbd <= share_max_cls_lbd)) {
        learnCallbackBuffer.resize(v.size());
        if (!filled_buffer) {
            for (int i = 0; i < v.size(); i++) {
                Lit lit = v[i];
                learnCallbackBuffer[i] = sign(lit) ? -(var(lit) + 1) : (var(lit) + 1);
            }
        }
        learnedClsCallback(learnCallbackBuffer, lbd, issuer);
    }
}

//=================================================================================================
// Debug etc:


//=================================================================================================
} // namespace MERGESAT_NSPACE

#endif
