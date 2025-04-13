/*************************************************************************************[ParSolver.h]
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

#ifndef MergeSat_ParSolver_h
#define MergeSat_ParSolver_h

#include "mtl/Queue.h"
#include "simp/SimpSolver.h"

#include "parallel/Sharing.h"

#include <atomic>

namespace MERGESAT_NSPACE
{

//=================================================================================================

class JobQueue;
class Barrier;

class ParSolver : protected SimpSolver
{
    bool par_reparsed_options; // Indicate whether the update parameter method has been used

    enum TaskType {
        TASKS_SPLIT_PARTITION = 0,
        TASKS_END,
    };

    /* structure, that holds relevant data for parallel solving */
    class SolverData
    {
        // Don't allow copying (error prone):
        SolverData &operator=(SolverData &other) = delete;
        SolverData(const SolverData &other) = delete;
        SolverData(SolverData &&other) = delete;

        public:
        ParSolver *_parent = nullptr;                   // pointer to the coordinating ParSolver object
        int _threadnr = 0;                              // number of the solver among all solvers
        lbool _status = l_Undef;                        // status of the associated SAT solver
        double _idle_s = 0;                             // seconds this thread idled while waiting
        uint64_t _attempted_to_sync = 0;                // number of attempts to enter a sync
        uint64_t _next_sync_counter_limit = 0;          // indicate when to actually sync next
        uint64_t _next_sync_current_accesses = 0;       // stores number of accesses when entring successful sync
        uint64_t _next_sync_previous_limit = 0;         // stores the previous limit we have seen in a given sync
        uint64_t _next_sync_previous_sync_attempts = 0; // stores the number of sync attempts during the last event
        uint64_t _sync_accesses = 0;                    // stores number of accesses performed during sync (in total)
        uint32_t _winning = 0;                          // indicate number of times this thread won
        uint32_t _entered_barrier = 0;                  // count how often we entered the sync barrier
        uint32_t _blocked_by_barrier = 0;               // count how often we had to wait in the sync barrier
        ClausePool pool;                                // store for learned clauses that are shared by this thread
        uint64_t received_clauses = 0;                  // number of clauses a thread received

        vec<TaskType> tasks; // task to be executed before next sync test

        SolverData(ParSolver *parent, int threadnr) : _parent(parent), _threadnr(threadnr) {}
        SolverData() {}

        void reset(ParSolver *parent, int threadnrs)
        {
            _parent = parent;
            _threadnr = threadnrs;
            _status = l_Undef;
            _idle_s = 0;
            _attempted_to_sync = 0;
            _next_sync_counter_limit = 0;
            _next_sync_current_accesses = 0;
            _next_sync_previous_limit = 0;
            _next_sync_previous_sync_attempts = 0;
            _sync_accesses = 0;
            _winning = 0;
            _entered_barrier = 0;
            _blocked_by_barrier = 0;
            received_clauses = 0;
        }
    };

    public:
    enum SIMPLIFICATION_TYPE { NONE = 0, EQUIV = 1, ELIM = 2 };

    // Constructor/Destructor:
    //
    ParSolver(int cores_override = INT32_MAX, SIMPLIFICATION_TYPE use_simp = ELIM);
    ~ParSolver();
    bool set_cores(int cores); // set number of cores, only works right after object initialization. returns success.

    // Problem specification:
    //
    int nVars() const;    // The current number of variables.
    int nClauses() const; // The current number of original clauses.
    Var newVar(bool polarity = true, bool dvar = true);
    void reserveVars(Var vars);    // Reserve space for given amount of variables
    bool addClause_(vec<Lit> &ps); // Add a clause to the solver without making superflous internal copy. Will
    // change the passed vector 'ps'.
    bool addClause(const vec<Lit> &ps);  // Add a clause to the solver.
    bool addEmptyClause();               // Add the empty clause, making the solver contradictory.
    bool addClause(Lit p);               // Add a unit clause to the solver.
    bool addClause(Lit p, Lit q);        // Add a binary clause to the solver.
    bool addClause(Lit p, Lit q, Lit r); // Add a ternary clause to the solver.
    void addInputClause_(vec<Lit> &ps);  // Add a clause to the online proof checker

    // Variable mode:
    //
    void setFrozen(Var v, bool b); // If a variable is frozen it will not be eliminated.
    bool getFrozen(Var v);         // Return frozen status of variable
    bool isEliminated(Var v) const;
    bool eliminate(bool turn_off_elim = false);           // Perform variable elimination based simplification.
    bool disable_simplification(bool keep_equiv = false); // Disable simplification properly, works also after
                                                          // initialization Can also only disable elimination, but keep
                                                          // strengthening Note: has to be called before adding clauses
    void setPolarity(Var v, bool b);                      // Forward implementation from Solver base class
    void clearInterrupt();                                // Forward implementation from Solver base class

    // Solving:
    //
    bool solve(const vec<Lit> &assumps, bool do_simp = true, bool turn_off_simp = false);
    lbool solveLimited(const vec<Lit> &assumps, bool do_simp = true, bool turn_off_simp = false);
    void interrupt();  // Trigger a (potentially asynchronous) interruption of the solver.
    bool okay() const; // FALSE means solver is in a conflicting state

    int modelSize() const { return model.size(); }
    lbool modelValue(Var x) const { return SimpSolver::modelValue(x); }
    lbool modelValue(Lit p) const { return SimpSolver::modelValue(p); }
    lbool value(Var x) const; // The current value of a variable, in case any solver already has a value
    lbool value(Lit p) const; // The current value of a literal, in case any solver already has a value

    int max_simp_cls(); // Return number of clauses when we do not perform simplification anymore

    void printStats();
    uint64_t simp_counter_sum() const; /* return access counter of simplifying solver */
    void toDimacs(const char *file);   /* print CNF for convenience */

    // Mode of operation:
    //
    using SimpSolver::parsing;
    void set_parsing(bool is_parsing = true);
    void set_verbosity(int verb);
    int verbose() const { return verbosity; }
    void setConfBudget(int64_t x);
    void budgetOff();
    bool diversify(int rank, int size); // set parameters for first solver based on position in set, and set size; return success
    enum SYNC_MODE { NON_DETERMINISTIC = 0, DETERMINISTIC_STATIC = 1, DETERMINISTIC_DYNAMIC = 2 };
    SYNC_MODE sync_mode;
    enum SYNC_UPDATE_METHOD { SYNC_STEPS_LINEAR_AVG_ADAPTION = 0 };

    /// synchronize threads deterministically (static or dynamic). return true, if search should be stopped
    bool sync_deterministic(size_t threadnr, bool enforce_waiting = false);
    /// check whether we need to actually sync, or can abort early
    bool sync_prepare_deterministic(size_t threadnr, bool caller_has_solution);
    /// evaluate own state and prepare for sharing, check whether we can abort early, calculate next limit to sync
    bool sync_evaluate_state_deterministic(size_t threadnr, bool caller_has_solution, uint64_t &new_sync_limit);
    /// visit shared data of others, and import into private solver of own thread
    bool sync_consume_shared_data_deterministic(size_t threadnr, bool caller_has_solution, uint64_t new_sync_limit);
    /// reset currently shared data to not duplicate sharing
    bool sync_reset_shared_data_deterministic(size_t threadnr, bool caller_has_solution);

    void proof_init(const char *drup_file_name, bool binary_format, int check_proof);
    void proof_setVerbosity(int verb);
    void proof_finalize(bool add_empty_clause);

    // IPASIR and incremental solving interface:
    //
    int nLearnts() const; // The current number of learnt clauses.
    void setTermCallback(void *state, int (*termCallback)(void *));
    void setLearnCallback(void *state, int maxLength, void (*learn)(void *state, int *clause));
    template <class V> void shareViaCallback(const V &v, int lbd = -1);

    void addConstrainClause(vec<Lit> &newConstrainClause); // At least one literal of the given clause has to be satisfied in the next call to solve
    bool failed_constraint();                              // Indicate whether the constrain clause was respected
    void deactivate_constrain_clause(lbool status); // Memorize that we do not use the current constrain clause in the next iteration, but allow cleanup
    void reset_constrain_clause(); // Reset information about constrain_clause

    bool set_configuration(const char *config_name); // set presets configuration for users of MergeSat

    protected:
    using SimpSolver::use_elim;
    using SimpSolver::use_simplification;

    // Solver state:
    //
    int cores;     /// number of cores available to this parallel solver
    int min_cores; /// number of cores to actually start parallel execution
    int verbosity;
    bool initialized;
    bool use_diversify;
    bool replaced_primary_solver;
    vec<SimpSolver *> solvers;
    vec<SolverData *> solverData;
    Proof proof; // Unsatisfiability proof
    vec<Lit> assumptions;
    uint64_t base_sync_increase;              // how much to bump number of accesses before next sync
    SYNC_UPDATE_METHOD sync_step_update_mode; // how to update step limits for next sync

    std::atomic<int> solved_current_call; // indicate that the current solving task has been solved (10 or 20), if not 0
    std::atomic<bool> sync_by_primary;    // false, if true, search will be non-deterministic
    size_t synced_clauses;                // store number of clauses in primary solver after last sync (after solving)
    size_t synced_units;         // store number of unit clauses in primary solver after last sync (after solving)
    bool global_receive_enabled; // allow to receive clauses
    uint64_t thread0_extra_steps; // allow first thread more steps, to not slow it down too much compared to sequential execution
    bool untouch_thread0;         // do not modify search of thread0? (allows to limit impact of parallel search)
    bool primary_controlled_sync; // should syncs happen based on primary thread

    JobQueue *jobqueue;      /// hold jobs for parallel items
    Barrier *solvingBarrier; /// sync execution after parallel solveing (prt for simpler integration)
    static void *thread_entrypoint(void *argument);
    void thread_run_solve(size_t threadnr);
    bool sync_solver_from_other(SimpSolver *const dest_solver,
                                SimpSolver *const src_solver,
                                bool force_sync = false,
                                bool addToProof = false); /// sync from primary to parallel solver
    lbool collect_solvers_results();

    // Iternal helper methods:
    //
    SimpSolver *newSolverObject(uint32_t threadnr);
    void init_solvers();
    void tear_down_solvers();
    bool sequential() const { return cores == 1; }
    void solver_start_measure_idling(size_t threadnr);
    void solver_stop_measure_idling(size_t threadnr);

    // Handle synchronization
    static bool sync_and_share(void *issuer, lbool *status); // this is called from a thread when a solver needs to sync (each restart)
    bool synchronize(size_t threadnr, bool enforce_waiting = false); // synchronize threads at this point. return true, if search should be stopped

    // Learned clause sharing
    /// callback to be registered in solver
    static void solver_learnedClsCallback(const std::vector<int> &c, int glueValue, void *issuer);
    /// function that executes the callback in the scope of the parallel sharing entity
    void learnedClsCallback(const std::vector<int> &c, int glueValue, size_t threadnr);
    /// check whether a thread should actually keep a clause for sharing
    bool sharingSendFilterAccept(const std::vector<int> &c, int glueValue, size_t threadnr);

    /// have solver in thread threadnr assess all clauses shared by other solvers, and pick
    bool assess_and_consume_shared_clauses(size_t threadnr);
    /// check whether a thread should actually take a clause from sharing
    bool sharingReceiveFilterAccept(const Clause &c, size_t threadnr);

    /// assign (new) extra tasks, return false on UNSAT
    bool assignExtraTask(size_t threadnr);
    /// evaluate the results of the previously assigned tasks, return false on UNSAT
    bool evaluateExtraTask(size_t threadnr);
    /// check, and execute, pending task for a given extra thread, return false on UNSAT
    bool runAssignedExtraTask(size_t threadnr);

    /// return new limit to be set next
    uint64_t get_next_sync_limit(size_t threadnr) const;
    /// return additional increment for steps until next sync
    int64_t get_next_linear_sync_limit_diff(size_t threadnr) const;

    // Extra stats
    double simplification_seconds; // seconds of sequential core spend during simplification

    // Check solver output
    bool check_satisfiability;
    Solver::SATchecker satChecker;
};

inline bool ParSolver::addClause(const vec<Lit> &ps)
{
    ps.copyTo(add_tmp);
    return addClause_(add_tmp);
}
inline bool ParSolver::addEmptyClause()
{
    add_tmp.clear();
    return addClause_(add_tmp);
}
inline bool ParSolver::addClause(Lit p)
{
    add_tmp.clear();
    add_tmp.push(p);
    return addClause_(add_tmp);
}
inline bool ParSolver::addClause(Lit p, Lit q)
{
    add_tmp.clear();
    add_tmp.push(p);
    add_tmp.push(q);
    return addClause_(add_tmp);
}
inline bool ParSolver::addClause(Lit p, Lit q, Lit r)
{
    add_tmp.clear();
    add_tmp.push(p);
    add_tmp.push(q);
    add_tmp.push(r);
    return addClause_(add_tmp);
}

inline lbool ParSolver::value(Var x) const
{
    for (int i = 0; i < solvers.size(); ++i) {
        lbool y = solvers[i]->value(x);
        if (y != l_Undef) return y;
    }
    return l_Undef;
}

inline lbool ParSolver::value(Lit p) const
{
    for (int i = 0; i < solvers.size(); ++i) {
        lbool y = solvers[i]->value(p);
        if (y != l_Undef) return y;
    }
    return l_Undef;
}

//=================================================================================================
} // namespace MERGESAT_NSPACE

#endif
