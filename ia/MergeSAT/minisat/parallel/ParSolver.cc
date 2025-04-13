/************************************************************************************[ParSolver.cc]
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

#include "parallel/ParSolver.h"
#include "mtl/Sort.h"
#include "parallel/JobQueue.h"
#include "utils/Options.h"
#include "utils/System.h"

using namespace MERGESAT_NSPACE;

//=================================================================================================
// Options:

#define DEFAULT_CORES 2

static const char *_cat = "PAR";

static IntOption
opt_cores(_cat, "cores", "Number of solvers to use, 0 means each CPU, < 0 use CPU/-X", DEFAULT_CORES, IntRange(INT32_MIN, INT32_MAX));
static IntOption
opt_min_cores(_cat, "min-auto-cores", "Only use parallelism if more than the given number of cores are available", 4, IntRange(1, INT32_MAX));
static BoolOption opt_use_diversify(_cat, "diversify", "Diversify parallel configurations", true);
static BoolOption opt_global_receive(_cat, "share-receive", "Do receive shared clauses (global)", true);
static IntOption
opt_sync_mode(_cat, "sync-mode", "How to sync threads (1=deterministic static, 2=deterministic dynamic)", 1, IntRange(1, 2));
static BoolOption opt_untouch_thread0(_cat, "untouch-thread0", "Do not receive clauses for thread 0", true);
static BoolOption opt_primary_controlled_sync(_cat, "primary-sync", "sync after primary thread", false);
static IntOption
opt_base_sync_accesses(_cat, "init-accesses", "Number of 'memory accesses' before first sync", 32000000, IntRange(0, INT32_MAX));
static IntOption
opt_thread0_extra_steps(_cat, "thread0-extra", "Number of extra 'memory accesses' for thread 0", 1000, IntRange(0, INT32_MAX));
static IntOption
opt_sync_step_update_mode(_cat, "sync-method", "Dynamic update sync step limit method: 0=linear to avg", 0, IntRange(0, 0));
static BoolOption opt_check_par_sat(_cat, "check-par-sat", "Store duplicate of formula and check SAT answers", false, false);

//=================================================================================================
// Constructor/Destructor:

ParSolver::ParSolver(int cores_override, ParSolver::SIMPLIFICATION_TYPE use_simp)
  : par_reparsed_options(updateOptions())
  , sync_mode(opt_sync_mode == 1 ? DETERMINISTIC_STATIC : DETERMINISTIC_DYNAMIC)
  , cores(cores_override == INT32_MAX ? opt_cores : cores_override)
  , min_cores(opt_min_cores)
  , verbosity(0)
  , initialized(false)
  , use_diversify(opt_use_diversify)
  , replaced_primary_solver(false)
  , base_sync_increase(opt_base_sync_accesses)
  , sync_step_update_mode(opt_sync_step_update_mode == 0 ? SYNC_STEPS_LINEAR_AVG_ADAPTION : SYNC_STEPS_LINEAR_AVG_ADAPTION)
  , solved_current_call(0)
  , sync_by_primary(false)
  , synced_clauses(0)
  , synced_units(0)
  , global_receive_enabled(opt_global_receive)
  , thread0_extra_steps(opt_thread0_extra_steps)
  , untouch_thread0(opt_untouch_thread0)
  , primary_controlled_sync(opt_primary_controlled_sync)
  , jobqueue(nullptr)
  , solvingBarrier(nullptr)
  , simplification_seconds(0)
  , check_satisfiability(opt_check_par_sat)
{
    if (use_simp == SIMPLIFICATION_TYPE::NONE) {
        use_simplification = false;
    } else if (use_simp == SIMPLIFICATION_TYPE::EQUIV)
        use_elim = false;

    // Get number of cores, and allocate arrays
    init_solvers();
}

ParSolver::~ParSolver()
{
    tear_down_solvers();
    proof_finalize(false);
}

bool ParSolver::set_cores(int new_cores)
{
    if (initialized) return false;
    cores = new_cores;
    return true;
}

void ParSolver::set_verbosity(int verb)
{
    verbosity = verb;
    for (int i = 0; i < solvers.size(); ++i)
        if (verbosity > 2) solvers[i]->verbosity = verbosity - 3;
}

int ParSolver::nVars() const
{
    assert(solvers[0] != nullptr && "there has to be one working solver");
    return solvers[0]->nVars();
}

int ParSolver::nClauses() const
{
    assert(solvers[0] != nullptr && "there has to be one working solver");
    return solvers[0]->nClauses();
}

Var ParSolver::newVar(bool polarity, bool dvar)
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    Var v = 0;
    for (int i = 0; i < solvers.size(); ++i) {
        Var newV = solvers[i]->newVar(polarity, dvar);
        if (v == 0) {
            v = newV;
        } else {
            assert(v == newV);
        }
    }
    return v;
}

void ParSolver::reserveVars(Var vars)
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    for (int i = 0; i < solvers.size(); ++i) solvers[i]->reserveVars(vars);
}

bool ParSolver::addClause_(vec<Lit> &ps)
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    if (check_satisfiability) satChecker.addClause(ps); // add for SAT check tracking
    bool all_okay = true;
    for (int i = 0; i < solvers.size(); ++i) {
        all_okay = all_okay && solvers[i]->addClause_(ps); // only continue if still true
    }
    return all_okay;
}

void ParSolver::addInputClause_(vec<Lit> &ps)
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    for (int i = 0; i < solvers.size(); ++i) {
        solvers[i]->addInputClause_(ps);
    }
}

void ParSolver::setTermCallback(void *state, int (*termCallback)(void *))
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    solvers[0]->termCallbackState = state;
    solvers[0]->termCallback = termCallback;
}

void ParSolver::setLearnCallback(void *state, int maxLength, void (*learn)(void *state, int *clause))
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    solvers[0]->learnCallbackState = state;
    solvers[0]->learnCallbackLimit = maxLength;
    solvers[0]->learnCallbackBuffer.resize(1 + maxLength);
    solvers[0]->learnCallback = learn;
}

int ParSolver::nLearnts() const
{
    if (solvers.size() < 1) return 0;
    assert(solvers[0] != nullptr && "there has to be one working solver");
    return solvers[0]->nLearnts();
}

void ParSolver::addConstrainClause(vec<Lit> &newConstrainClause) { throw "not yet implemented"; }

bool ParSolver::failed_constraint() { throw "not yet implemented"; }

void ParSolver::deactivate_constrain_clause(lbool status) { throw "not yet implemented"; }

void ParSolver::reset_constrain_clause() { throw "not yet implemented"; }

// Variable mode:
//
void ParSolver::setFrozen(Var v, bool b)
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    for (int i = 0; i < solvers.size(); ++i) solvers[i]->setFrozen(v, b);
}

bool ParSolver::getFrozen(Var v)
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    return solvers[0]->getFrozen(v);
}

bool ParSolver::isEliminated(Var v) const
{
    assert(solvers[0] != nullptr && "there has to be one working solver");
    return solvers[0]->isEliminated(v);
}

bool ParSolver::eliminate(bool turn_off_elim)
{
    if (turn_off_elim) return disable_simplification();
    return true;
}

bool ParSolver::disable_simplification(bool keep_equiv)
{
    use_simplification = false;

    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    if (verbosity > 1) std::cout << "c primary elimination" << std::endl;
    bool ret = true;
    if (!keep_equiv) {
        for (int i = 0; i < solvers.size(); ++i) {
            if (verbosity > 1)
                std::cout << "c simplifying for solver " << i << " with clauses: " << solvers[i]->nClauses() << std::endl;
            ret = solvers[i]->eliminate(true) && ret;
        }
    } else {
        for (int i = 0; i < solvers.size(); ++i) ret = solvers[i]->use_elim = false;
    }
    return ret;
}

bool ParSolver::set_configuration(const char *config_name)
{
    if (strncmp("cbmc", config_name, 5)) {
        for (int i = 0; i < solvers.size(); ++i) {
            solvers[i]->grow_iterations = false;
            solvers[i]->nr_max_simp_cls = 1000000;
        }
    } else {
        // no known configuration found
        return false;
    }
    return true;
}

void ParSolver::setPolarity(Var v, bool b)
{
    init_solvers();
    for (int i = 0; i < solvers.size(); ++i) {
        if (solvers[i] && solvers[i]->nVars() > v) solvers[i]->setPolarity(v, b);
    }
}

void ParSolver::clearInterrupt()
{
    init_solvers();
    for (int i = 0; i < solvers.size(); ++i) {
        if (solvers[i]) solvers[i]->clearInterrupt();
    }
}

#define SUM_STATS(sum_variable, member)                                                                                \
    do {                                                                                                               \
        sum_variable = 0;                                                                                              \
        for (int i = 0; i < solvers.size(); ++i) sum_variable += solvers[i]->member;                                   \
    } while (false)

#define PRINT_DETAILED_STATS(member, name)                                                                             \
    do {                                                                                                               \
        printf("c DETAILS stats %s : ", name);                                                                         \
        for (int i = 0; i < solvers.size(); ++i) printf(" %d:%" PRIu64 "", i, solvers[i]->member);                     \
        printf("\n");                                                                                                  \
    } while (false)

void ParSolver::printStats()
{
    init_solvers();
    printf("c used cores:                    : %d\n", cores);
    printf("c simplification wall time:      : %g s\n", simplification_seconds);

    const double cpu_time = cpuTime();
    const double wall_time = wallClockTime();
    printf("c CPU time                       : %g s\n", cpu_time);

    double theoretical_max_wall = (wallClockTime() - simplification_seconds) * cores + simplification_seconds;
    printf("c theor. Max CPU time:           : %g s\n", theoretical_max_wall);

    double total_idle_time = 0;
    for (int i = 0; i < solverData.size(); ++i) total_idle_time += solverData[i]->_idle_s;
    const double efficiency = 1.0 - (total_idle_time / (wall_time * solverData.size()));
    printf("c idle wall search time (sum):   : %g s  (efficiency: %g)\n", total_idle_time, efficiency);

    uint64_t total_conflicts = 0, total_decisions = 0, total_restarts = 0;
    SUM_STATS(total_conflicts, conflicts);
    SUM_STATS(total_decisions, decisions);
    SUM_STATS(total_restarts, starts);
    printf("c SUM stats conflicts:           : %" PRIu64 "\n", total_conflicts);
    if (verbosity > 1) PRINT_DETAILED_STATS(conflicts, "conflicts");
    printf("c SUM stats decisions:           : %" PRIu64 "\n", total_decisions);
    if (verbosity > 1) PRINT_DETAILED_STATS(decisions, "decisions");
    printf("c SUM stats restarts:            : %" PRIu64 "\n", total_restarts);
    if (verbosity > 1) PRINT_DETAILED_STATS(starts, "restarts");
    printf("c threads sync attempts:");
    for (int i = 0; i < solverData.size(); ++i) {
        printf(" %d:%" PRIu64 "", i, solverData[i]->_attempted_to_sync);
    }
    printf("\n");
    printf("c idle seconds:");
    for (int i = 0; i < solverData.size(); ++i) {
        printf(" %d:%lf", i, solverData[i]->_idle_s);
    }
    printf("\n");
    printf("c winning threads:");
    for (int i = 0; i < solverData.size(); ++i) {
        if (solverData[i]->_winning > 0) printf(" %d:%d", i, solverData[i]->_winning);
    }
    printf("\n");
    printf("c barrier data:");
    for (int i = 0; i < solverData.size(); ++i) {
        printf(" %d:%u/%u", i, solverData[i]->_entered_barrier, solverData[i]->_blocked_by_barrier);
    }
    printf("\n");
    printf("c received clauses:");
    for (int i = 0; i < solverData.size(); ++i) {
        printf(" %d:%" PRIu64 "", i, solverData[i]->received_clauses);
    }
    printf("\n");

    printf("c sync-info (last,current,steps,rank):");
    for (int i = 0; i < solverData.size(); ++i) {
        printf(" %d:%" PRIu64 ":%" PRIu64 ":%" PRIu64 "", i, solverData[i]->_next_sync_previous_limit,
               solverData[i]->_next_sync_counter_limit, solverData[i]->_next_sync_current_accesses);
    }
    printf("\n");
}

uint64_t ParSolver::simp_counter_sum() const { return solvers.size() > 0 ? solvers[0]->counter_sum() : 0; }

void ParSolver::toDimacs(const char *file)
{
    vec<Lit> as;
    if (solvers.size() > 0) solvers[0]->toDimacs(file, as);
}

void ParSolver::set_parsing(bool is_parsing)
{
    for (int i = 0; i < solvers.size(); ++i) {
        solvers[i]->parsing = is_parsing;
    }
    parsing = is_parsing;
}

void ParSolver::setConfBudget(int64_t x)
{
    if (x >= 0) {
        if (solvers.size() == 0 || solvers[0] == nullptr) {
            solvers[0]->setConfBudget(x);
        }
    } else {
        budgetOff();
    }
}

void ParSolver::budgetOff()
{
    conflict_budget = -1;
    for (int i = 0; i < solvers.size(); ++i) {
        solvers[i]->budgetOff();
    }
}

bool ParSolver::diversify(int rank, int size)
{
    if (solvers.size() == 0 || solvers[0] == nullptr) return false;
    solvers[0]->diversify(rank, size);
    return true;
}

// Solving:
//
bool ParSolver::solve(const vec<Lit> &assumps, bool do_simp, bool turn_off_simp)
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    assert(false && "support parallelism here!"); // TODO: implement simple wrapper properly
    return solvers[0]->solve(assumps, do_simp, turn_off_simp);
}

lbool ParSolver::solveLimited(const vec<Lit> &assumps, bool do_simp, bool turn_off_simp)
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");

    /* prepare next search iteration, local and object global state */
    lbool ret = l_Undef;
    solved_current_call = 0;
    conflict.clear();
    model.clear();

    for (int i = 0; i < solvers.size(); ++i) {
        assert((i >= solverData.size() || solverData[i]->_status == l_Undef) && "Start fresh");
    }

    if (!solvers[0]->okay()) {
        // no need to run solvers if we already have UNSAT
        ret = l_False;
    } else if (sequential()) {
        assert(solvers.size() == 1 && "actually implement parallel case");
        assert((!proof.enabled() || solvers[0]->proof.enabled()) && "proofs need to be enabled at the same time");
        ret = solvers[0]->solveLimited(assumps, do_simp, turn_off_simp);
        solvers[0]->conflict.moveTo(conflict);
        solvers[0]->model.moveTo(model);
    } else {
        assert(jobqueue && "jobqueue should be initialized");
        assert(solvingBarrier && "solvingBarrier should be initialized");

        /* allow to use the barrier for ALL parallel solvers before they start solving in parallel */
        if (!solvingBarrier->grow(cores)) {
            assert(false && "growing to requested number of cores has to work");
        }

        assumps.copyTo(assumptions); // copy to shared assumptions object
        jobqueue->setState(JobQueue::SLEEP);
        for (int t = 1; t < cores; ++t) { // all except master now
            JobQueue::Job job;
            job.function = &(ParSolver::thread_entrypoint); // function that controls how a solver runs
            job.argument = (void *)solverData[t];
            jobqueue->addJob(job); // TODO: instead of queue, use a slot based data structure, to make use of core pinning
        }

        // parallel execution will start
        jobqueue->setState(JobQueue::WORKING);

        // also run the primary solver
        if (!use_diversify && !replaced_primary_solver) {
            if (verbosity > 0) printf("c re-initialize first solver\n");
            solverData[0]->reset(this, 0);
            SimpSolver *replaceSolver = newSolverObject(0);
            /* do not add clauses to proof one more time, as they are present in the solver we delete already */
            sync_solver_from_other(replaceSolver, solvers[0], true, false);
            replaceSolver->swapEliminationStack(solvers[0]->elimclauses);
            SimpSolver *oldSolver0 = solvers[0];
            solvers[0] = replaceSolver;
            if (proof.enabled()) {
                solvers[0]->proof.init(nullptr, proof.use_binary_format(), false, &proof, false);
                proof.deregister_subproof(&oldSolver0->proof, 0, &replaceSolver->proof); // keep proofs useful
            }
            delete oldSolver0;
            replaced_primary_solver = true;
            assert(solvers[0]->counter_sum() == solvers[1]->counter_sum() &&
                   "after initialization, all should be the same");
        }
        // also run the primary solver
        thread_run_solve(0);
        // prepare to sync from the state of the primary solver for incremental solving
        synced_clauses = solvers[0]->nClauses();
        synced_units = solvers[0]->nUnits();

        assert(solvers.size() > 1 && "sequential code should not reach here");
        assert((use_diversify || solvers[0]->nVars() == solvers[1]->nVars()) && "solver variables should be in sync");
        assert((use_diversify || solvers[0]->nClauses() == solvers[1]->nClauses()) &&
               "solver clauses should be in sync");
        assert((use_diversify || solvers[0]->nUnits() == solvers[1]->nUnits()) && "solver units should be in sync");

        // when returning from this, all parallel solvers are 'done' as well, i.e. do not modify relevant state anymore

        ret = collect_solvers_results();
        assert((use_diversify || solvers.size() < 2 || solvers[0]->conflicts == solvers[1]->conflicts) && "in sync");
        assert((use_diversify || solvers.size() < 2 || solvers[0]->decisions == solvers[1]->decisions) && "in sync");
        // allow new call to solve method
        if (verbosity > 1) std::cout << "c terminate ParSolver::solveLimited()" << std::endl;
        assert(solvingBarrier->empty() && "all job functions should terminate themselves now");
    }

    /* set status back to undef for the next run */
    for (int threadnr = 0; threadnr < solvers.size(); ++threadnr) {
        if (threadnr >= solverData.size()) break;
        if (solvers[threadnr]->okay()) {
            solverData[threadnr]->_status = l_Undef;
        } else {
            assert(!solvers[0]->okay() || solverData[threadnr]->_status == l_False);
        }
    }

    if (ret == l_True && check_satisfiability) {
        if (!satChecker.checkModel(model)) {
            assert(false && "model should satisfy full input formula");
            throw("ERROR: detected model that does not satisfy input formula, abort");
            exit(1);
        } else if (verbosity)
            printf("c validated SAT answer\n");
    }

    solved_current_call.store(0);
    return ret;
}

lbool ParSolver::collect_solvers_results()
{
    if (verbosity > 1) std::cout << "collect solver results ..." << std::endl;
    lbool status = l_Undef;
    size_t smallest_conflict = ~0UL;
    int smallest_conflict_idx = -1, sat_solver_idx = -1;
    for (int t = 0; t < cores; ++t) { // all except master now
        lbool r = solverData[t]->_status;
        assert((status == l_Undef || r == l_Undef || status == r) && "solvers have to have same result");
        assert(solverData[t]->_blocked_by_barrier == solverData[0]->_blocked_by_barrier &&
               "barrier blocks have to match");
        assert(solverData[t]->_entered_barrier == solverData[0]->_entered_barrier && "entered barriers have to match");

        /* pick 'winning solver' */
        if (r != l_Undef) {
            /* for conflicts, heuristicalls select the smallest conflict */
            if (r == l_False && (size_t)solvers[t]->conflict.size() < smallest_conflict) {
                smallest_conflict_idx = t;
            } else if (r == l_True) {
                /* select the first solver that won */
                sat_solver_idx = sat_solver_idx >= 0 ? sat_solver_idx : t;
            }

            if (status != l_Undef && r != status) {
                throw "c detected unsound parallel behavior when collecting results, aborting";
            }
            status = r;
        }
    }

    /* update model or conflict */
    if (status == l_True) {
        if (verbosity > 1)
            std::cout << "c return model from solver with ID " << sat_solver_idx << " with size "
                      << solvers[sat_solver_idx]->model.size() << std::endl;
        solvers[sat_solver_idx]->extendModel();
        solvers[sat_solver_idx]->model.moveTo(model);
    } else if (status == l_False) {
        if (verbosity > 1)
            std::cout << "c return result from conflicting solver with ID " << smallest_conflict_idx << std::endl;
        assert(smallest_conflict_idx >= 0 && "at least one index has been found");
        solvers[smallest_conflict_idx]->conflict.moveTo(conflict);
    }

    return status;
}

void ParSolver::interrupt()
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    for (int i = 0; i < solvers.size(); ++i) solvers[i]->interrupt();
}

bool ParSolver::okay() const
{
    assert(solvers[0] != nullptr && "there has to be one working solver");
    for (int i = 0; i < solvers.size(); ++i) {
        if (!solvers[i]->okay()) return false;
    }
    return ok;
}

int ParSolver::max_simp_cls()
{
    init_solvers();
    assert(solvers[0] != nullptr && "there has to be one working solver");
    return solvers[0]->max_simp_cls();
}

bool ParSolver::sharingSendFilterAccept(const std::vector<int> &c, int glueValue, size_t threadnr)
{
    /* TODO: here, filtering functions for keeping clauses can be implemented */
    return true;
}

void ParSolver::learnedClsCallback(const std::vector<int> &c, int glueValue, size_t threadnr)
{
    if (sharingSendFilterAccept(c, glueValue, threadnr)) {
        /* Just adding a learned clause to the shared pool does no require it to be on the proof
         * because we receive clauses from the pool in a synchronized fashion.
         */
        solverData[threadnr]->pool.add_shared_clause(c, glueValue);
    }
    return;
}

void ParSolver::solver_learnedClsCallback(const std::vector<int> &c, int glueValue, void *issuer)
{
    if (!issuer) return;
    SolverData *s = (SolverData *)issuer;
    (s->_parent)->learnedClsCallback(c, glueValue, s->_threadnr);
}

SimpSolver *ParSolver::newSolverObject(uint32_t threadnr)
{
    SimpSolver *solver = new SimpSolver();
    if (!use_simplification) solver->use_simplification = false;
    if (verbosity > 2) solver->verbosity = verbosity - 3;
    if (use_diversify) solver->diversify(threadnr, 32);
    /* setup non-primary solvers specially */
    if (cores > 1) {
        /* setup the handler to share learned clauses */
        solver->learnedClsCallback = ParSolver::solver_learnedClsCallback;
        /* initialize communication for solver */
        solver->initialize_parallel_solver(solverData[threadnr], ParSolver::sync_and_share);
    }
    if (!solver->use_simplification) {
        solver->eliminate(true);
    }
    if (!use_elim) solver->use_elim = false;
    return solver;
}

void ParSolver::init_solvers()
{
    if (initialized) return; // abort, in case this function was called already
    assert(solvers.size() == 0 && "do not allocate solvers multiple times");

    // auto detect cores
    if (cores <= 0) {
        min_cores = min_cores < 1 ? 1 : min_cores;
        if (cores == 0) {
            cores = nrCores();
        } else {
            cores = nrCores() / (-cores);
        }
        /* reset cores, use sequential mode */
        if (cores < min_cores) cores = 1;
    }

    /* make sure we have at least one solver */
    cores = cores <= 1 ? 1 : cores;
    if (verbosity > 1) std::cout << "c initialize solver for " << cores << " cores" << std::endl;

    solvers.growTo(cores, nullptr);

    if (cores > 1) {
        /* prepare structures for parallelism */
        assert(!jobqueue && "do not override the jobqueue");
        if (jobqueue) delete jobqueue;
        if (verbosity > 1)
            std::cout << "c initialize thread pool for " << cores - 1 << " non-primary threads" << std::endl;
        jobqueue = new JobQueue(cores - 1);  // all except the main core
        jobqueue->setState(JobQueue::SLEEP); // set all to sleep

        solvingBarrier = new Barrier(0); // setup a dummy barrier for now
        solverData.growTo(cores, 0);
    }

    /* initialize each solver object */
    for (int i = 0; i < solvers.size(); ++i) {
        if (cores > 1 && !solverData[i]) solverData[i] = new SolverData(this, i);
        solvers[i] = newSolverObject(i);
    }

    assert(solvers[0] != nullptr && "there has to be one working solver");

    /* in case outside parameters decided against simplification, disable it */
    if (!use_simplification) {
        solvers[0]->eliminate(true);
    }

    initialized = true;
}

void ParSolver::tear_down_solvers()
{
    if (solvers.size() > 0) {
        for (int i = 0; i < solvers.size(); ++i) {
            if (solvers[i] != nullptr) delete solvers[i];
            solvers[i] = nullptr;
        }
    }
    solvers.clear();
    solverData.clear();
    if (solvingBarrier) delete solvingBarrier;
    solvingBarrier = nullptr;
    if (jobqueue) delete jobqueue;
    jobqueue = nullptr;

    initialized = false;
}

void ParSolver::solver_start_measure_idling(size_t threadnr)
{
    assert(threadnr < (size_t)solverData.size() && "only existing solvers can idle");
    solverData[threadnr]->_idle_s = wallClockTime() - solverData[threadnr]->_idle_s;
}

void ParSolver::solver_stop_measure_idling(size_t threadnr)
{
    assert(threadnr < (size_t)solverData.size() && "only existing solvers can idle");
    solverData[threadnr]->_idle_s = wallClockTime() - solverData[threadnr]->_idle_s;
    ;
    assert(solverData[threadnr]->_idle_s >= 0 && "idling cannot become negative");
}

void ParSolver::thread_run_solve(size_t threadnr)
{
    if (verbosity > 1) std::cout << "c started thread " << threadnr << std::endl;

    assert(solvers.size() == solverData.size() && "number of solvers and data should match");
    assert(threadnr < (size_t)solverData.size() && "cannot run threads beyond initialized cores");
    assert(solvers[threadnr]->issuer != nullptr && "Parallel solver requires a sync mechanism issuer");
    assert(solvers[threadnr]->external_sync_and_share != nullptr &&
           "Parallel solver requires a sync mechanism share method");
    if (threadnr >= (size_t)solverData.size()) {
        return; // do not interrupt too aggressively, just ignore the ask
    }

    /* make sure we all start at the same step */
    solvingBarrier->wait();

    // stop early, in case solver is in a bad state initially already
    if (!solvers[threadnr]->okay()) {
        solverData[threadnr]->_status = l_False;
        // Do not return early, as that might fail the global syncing
    }
    solverData[threadnr]->_status = l_Undef;
    assert((!proof.enabled() || solvers[0]->proof.enabled()) && "proofs need to be enabled at the same time");
    solverData[threadnr]->_status = solvers[threadnr]->solveLimited(assumptions);

    if (verbosity > 1)
        std::cout << "c thread " << threadnr << " finished solving with " << solverData[threadnr]->_status
                  << " and accesses:" << solvers[threadnr]->counter_access.sum() << std::endl;
    solver_start_measure_idling(threadnr);
    /* final sync, enforce it (e.g. do not respect limits) */
    synchronize(threadnr, true);
    solver_stop_measure_idling(threadnr);

    /* wait until all solvers enter here */
    solvingBarrier->wait();
}

void *ParSolver::thread_entrypoint(void *argument)
{
    SolverData *s = (SolverData *)argument;

    (s->_parent)->thread_run_solve(s->_threadnr);
    return 0;
}

bool ParSolver::sync_solver_from_other(SimpSolver *const dest_solver, SimpSolver *const src_solver, bool force_sync, bool addToProof)
{
    if (!force_sync) return false;
    src_solver->proof.flush(); // make sure, global proof is valid

    if (verbosity > 1)
        std::cout << "c sync from primary solver ... (clauses: " << src_solver->nClauses() << ")" << std::endl;

    // this assumption might change later, but holds for now.
    assert(src_solver->nLearnts() == 0 &&
           "only sync at a time where no learned clauses are present (or implement syncing!)");

    // propagate limits to sub solvers
    if (src_solver->conflict_budget) dest_solver->conflict_budget = conflict_budget;
    if (src_solver->propagation_budget) dest_solver->propagation_budget = propagation_budget;

    // sync variables
    if (dest_solver->nVars() < src_solver->nVars()) {
        if (verbosity > 1)
            std::cout << "c resolve variable diff: " << src_solver->nVars() - dest_solver->nVars() << std::endl;
        dest_solver->reserveVars(src_solver->nVars());
        while (dest_solver->nVars() < src_solver->nVars()) {
            // ignore eliminated variables for decisions
            Var next = dest_solver->nVars();
            dest_solver->newVar(true, !src_solver->isEliminated(next));
        }
    }
    // sync unit clauses
    bool succeed_adding_clauses = true;
    if (verbosity > 1) std::cout << "c resolve unit diff: " << src_solver->nUnits() - synced_units << std::endl;
    for (size_t unit_idx = synced_units; unit_idx < src_solver->nUnits(); ++unit_idx) {
        const Lit l = src_solver->getUnit(unit_idx);
        if (succeed_adding_clauses) {
            succeed_adding_clauses = dest_solver->addClause(l); /* units are always added to the proof */
        }
    }

    // sync clauses (after simplification, this will only sync the simplified clauses)
    if (verbosity > 1) std::cout << "c resolve clause diff: " << src_solver->nClauses() - synced_clauses << std::endl;
    for (size_t cls_idx = synced_clauses; cls_idx < src_solver->nClauses(); ++cls_idx) {
        const Clause &c = src_solver->getClause(cls_idx);
        if (c.mark() == 1) continue; // skip satisfied clauses
        if (succeed_adding_clauses) {
            succeed_adding_clauses = dest_solver->importClause(c, addToProof);
        }
    }

    // propagate inconsistencies
    if (!src_solver->okay()) dest_solver->set_not_okay();

    // sub solver object is not unsat
    return succeed_adding_clauses && dest_solver->okay();
}

bool ParSolver::sync_and_share(void *issuer, lbool *status)
{
    // no communication set, just return
    if (issuer == nullptr) return false;

    SolverData *solverData = (SolverData *)issuer;

    // use status that has been handed in!
    if (status != nullptr && *status != l_Undef) {
        assert((solverData->_status == l_Undef || solverData->_status == *status) && "status has to match");
        solverData->_status = *status;
    }

    /* actuall sync all solvers with the portfolio sharing strategy */
    bool stop_search = solverData->_parent->synchronize(solverData->_threadnr);

    // forward status of solver after sharing
    if (status != nullptr && *status == l_Undef) {
        *status = solverData->_status;
    }

    return stop_search;
}

/// have solver in thread threadnr assess all clauses shared by other solvers, and pick
bool ParSolver::assess_and_consume_shared_clauses(size_t threadnr)
{
    /* do not receive clauses for thread0 (if enabled) */
    if (untouch_thread0 && threadnr == 0) return true;

    Solver *thread_solver = solvers[threadnr];
    bool receive_causes_successful = true;
    bool backjumped_to_toplevel = false;

    assert(solverData[threadnr]->_status != l_True && "do not receive clauses if we have a solution already");
    if (solverData[threadnr]->_status == l_True) return true;

    int received_clauses = 0;
    /* iterate over all pools, in order */
    for (size_t target_pool = 0; target_pool < (size_t)solverData.size(); ++target_pool) {
        /* do not consume from own pool */
        if (target_pool == threadnr) continue;
        const ClausePool &pool = solverData[target_pool]->pool;
        /* check whether each clause in the pool would be a fit */
        for (int idx = 0; idx < pool.size(); ++idx) {
            const Clause &c = pool.getClause(idx);
            /* add the clause to the thread's solver, and check for conflicts via propagation */
            if (sharingReceiveFilterAccept(c, threadnr)) {
                if (!backjumped_to_toplevel) {
                    thread_solver->cancelUntil(0);
                    backjumped_to_toplevel = true;
                }
                receive_causes_successful = thread_solver->addLearnedClause(c, c.lbd(), proof.enabled()) && receive_causes_successful;
                received_clauses++; // TODO: could also collect stats per pair of solvers
            }
        }
    }

    solverData[threadnr]->received_clauses += received_clauses;

    /* return status of this solver after sharing */
    return receive_causes_successful;
}

bool ParSolver::sharingReceiveFilterAccept(const Clause &c, size_t threadnr)
{
    /* TODO implement actual heuristic here! */
    return global_receive_enabled;
}


bool ParSolver::assignExtraTask(size_t threadnr)
{
    /*  Currently, this method can only be run by threadnr 0.
     *  This method should only be called between two
     *  barrier->wait() calls.
     */
    assert(threadnr == 0 && "assigning extra tasks should be done by thread 0 only");
    return true;
}

bool ParSolver::evaluateExtraTask(size_t threadnr)
{
    /*  Currently, this method can only be run by threadnr 0.
     *  This method should only be called between two
     *  barrier->wait() calls.
     */
    assert(threadnr == 0 && "evaluating extra tasks should be done by thread 0 only");
    return true;
}

bool ParSolver::runAssignedExtraTask(size_t threadnr)
{
    // execute tasks in order
    for (int i = 0; i < solverData[threadnr]->tasks.size(); ++i) {
        TaskType tt = solverData[threadnr]->tasks[i];
        switch (tt) {
        case TASKS_SPLIT_PARTITION:
            assert(false && "to be implemented");
        default:
            assert(false && "to be implemented");
        }
    }
    solverData[threadnr]->tasks.clear();

    // if we reach this point, we are done
    assert(solverData[threadnr]->tasks.size() == 0);
    return true;
}

bool ParSolver::synchronize(size_t threadnr, bool caller_has_solution)
{
    /* TODO: make use of the solvingBarrier here to make solving deterministic */
    assert(solvingBarrier && "in case of parallel solving, there needs to be a barrier");
    assert((!caller_has_solution || solved_current_call != 0 || solverData[threadnr]->_status != l_Undef) &&
           "we have to know the solution with this flag set, either ourselves, or another solver in an earlier sync");

    // count how often a thread attempts to sync - to better estimate sync granularity
    solverData[threadnr]->_attempted_to_sync++;

    /* run extra task, if present, e.g. partitioning current formula */
    if (!caller_has_solution && !primary_controlled_sync) {
        if (!runAssignedExtraTask(threadnr)) {
            assert(false && "handle this case");
        }
    }

    /* ignore this call, in case we did not reach the solver internal step limit */
    if (!caller_has_solution) {
        if (!primary_controlled_sync) {
            /* all threads need to reach access limit */
            if (solverData[threadnr]->_next_sync_counter_limit >= solvers[threadnr]->counter_access.sum()) return false;
        } else {
            /* primary thread controls */
            if (threadnr == 0) {
                if (solverData[threadnr]->_next_sync_counter_limit >= solvers[threadnr]->counter_access.sum())
                    return false; /* same as above, reorder ifs */
                if (verbosity > 0) std::cout << "c thread " << threadnr << " activates sync_by_primary" << std::endl;
                sync_by_primary.store(true);
            } else {
                if (!sync_by_primary.load()) return false; /* master not yet in sync mode */
                if (verbosity > 0) std::cout << "c thread " << threadnr << " sync based on primary" << std::endl;
            }
        }
    } else {
        /* caller already has solution, ignore */
    }

    /* store number of accesses of this thread when entering a successful sync */
    uint64_t sync_entering_accesses = solvers[threadnr]->counter_access.sum();
    solverData[threadnr]->_next_sync_current_accesses = sync_entering_accesses;

    bool ret = false;
    switch (sync_mode) {
    case DETERMINISTIC_STATIC:
    case DETERMINISTIC_DYNAMIC:
        /* this method handles dynamic vs static internally */
        ret = sync_deterministic(threadnr, caller_has_solution);
        break;
    default:
        throw "Not Implemented Yet";
    }

    /* update number of accesses performed in this sync for this solver */
    uint64_t sync_levaing_accesses = solvers[threadnr]->counter_access.sum();
    solverData[threadnr]->_sync_accesses += sync_levaing_accesses - sync_entering_accesses;

    return ret;
}

int64_t ParSolver::get_next_linear_sync_limit_diff(size_t threadnr) const
{
    uint64_t sum_steps_since_last_sync = 0;
    double avg_update_factor = 0.5; // allow to adjust adoption - TODO: measure whether we need to fully adapt, or even over adapt!

    // get sum of all steps since last sync
    int64_t min_steps = solverData[0]->_next_sync_current_accesses - solverData[0]->_next_sync_previous_limit;
    for (int thread = 0; thread < solvers.size(); ++thread) {
        int64_t since_last_sync = solverData[thread]->_next_sync_current_accesses - solverData[thread]->_next_sync_previous_limit;
        min_steps = min_steps < since_last_sync ? min_step_size : since_last_sync;
        sum_steps_since_last_sync += since_last_sync;
    }
    if (verbosity > 1) {
        std::cout << "c thread " << threadnr << " sum all steps: " << sum_steps_since_last_sync << std::endl;
    }

    // add more steps, if we had less steps since last attempt, or substract steps in case we had more this time
    int64_t avg_steps_since_last_sync = sum_steps_since_last_sync / solvers.size();
    assert(min_steps <= avg_steps_since_last_sync && "minimum has to be lower than average");
    uint64_t steps_since_last_sync = solverData[threadnr]->_next_sync_current_accesses - solverData[threadnr]->_next_sync_previous_limit;
    int64_t my_diff_to_avg = avg_steps_since_last_sync - steps_since_last_sync;

    // make sure we do not decrease the number of steps
    int64_t sync_step_offset = avg_steps_since_last_sync - min_steps;

    // return steps difference to add for current solver - threads with more steps will grow less
    int64_t returned_value = my_diff_to_avg * (avg_update_factor * (my_diff_to_avg > 0 ? avg_update_factor : 1)) + sync_step_offset;
    if (verbosity > 1) {
        std::cout << "c thread " << threadnr << " obtains " << returned_value
                  << " extra steps (steps: " << solverData[threadnr]->_next_sync_current_accesses
                  << ", prev-limit: " << solverData[threadnr]->_next_sync_previous_limit
                  << ", avg-diff: " << avg_steps_since_last_sync << ")" << std::endl;
    }

    /* do not return negative values */
    return returned_value < 0 ? 0 : returned_value;
}

uint64_t ParSolver::get_next_sync_limit(size_t threadnr) const
{
    const SolverData &data = *(solverData[threadnr]);

    /* use this variable to determine how to treat difficiencies in synchronizing */
    int64_t sync_diff = base_sync_increase; // as a start, allow 10k more clause accesses

    /* We assume that all threads currently halt their solving. Hence, accessing the
       number of accesses per thread is safe.
       To satisfy this condition, this function has to be called between two
       solvingBarrier->wait() calls. */

    // TODO: make this a strategy, using 'static', 'linear-avg', ...
    if (sync_mode == DETERMINISTIC_DYNAMIC) {
        int64_t extra_steps = 0;
        switch (sync_step_update_mode) {
        case SYNC_STEPS_LINEAR_AVG_ADAPTION:
            extra_steps = get_next_linear_sync_limit_diff(threadnr);
            break;
        default:
            break;
        }
        sync_diff += extra_steps;
    }
    sync_diff += thread0_extra_steps;
    if (verbosity > 1) std::cout << "c sync step updates " << sync_diff << " for thread " << threadnr << std::endl;

    // grant less cycles extra, if precision of a given thread is poor - better this 1 waits than 'all' others
    // TODO: to be implemented!
    return data._next_sync_current_accesses + sync_diff;
}


bool ParSolver::sync_prepare_deterministic(size_t threadnr, bool caller_has_solution)
{
    /* do not block, in case some solver already found the solution */
    if (solved_current_call.load() != 0) {
        if (verbosity > 1)
            std::cout << "c thread " << threadnr << " leaves sync early due to solve call state "
                      << solved_current_call.load() << std::endl;
        return true;
    }

    solverData[threadnr]->_entered_barrier++;
    if (verbosity > 0 && threadnr == 0) {
        std::cout << "c sync " << solverData[threadnr]->_entered_barrier << " confl[0] " << solvers[0]->conflicts
                  << " solvers: " << solvers.size() << " accesses: " << solverData[threadnr]->_next_sync_current_accesses
                  << " limit: " << solverData[threadnr]->_next_sync_counter_limit << std::endl;
    }

    /* block on the barrier */
    if (verbosity > 1) std::cout << "c synchronize barrier wait 1 by thread " << threadnr << std::endl;
    return false;
}

bool ParSolver::sync_evaluate_state_deterministic(size_t threadnr, bool caller_has_solution, uint64_t &new_sync_limit)
{
    if (!solvers[0]->withinBudget()) return true; /* we reached the budget limit, abort search with l_Undef */
    if (primary_controlled_sync && threadnr == 0) sync_by_primary.store(false); // prepare for next sync

    /* From here, do not modify our own state for deterministic syncing */
    new_sync_limit = get_next_sync_limit(threadnr);

    if (solverData[threadnr]->_status != l_Undef) {
        solverData[threadnr]->_winning++;
        /* indicate we solved the current formula */
        if (solverData[threadnr]->_status != l_Undef) {
            solved_current_call.store(solverData[threadnr]->_status == l_False ? 20 : 10);
            if (verbosity > 1)
                std::cout << "c thread " << threadnr << " sets global current call status to "
                          << solverData[threadnr]->_status << " equals: " << solved_current_call << std::endl;
        }
    }

    if (threadnr == 0) {
        if (!evaluateExtraTask(threadnr)) {
            assert(false && "implement failure of evaluating task");
        }
    }

    /* block on the barrier */
    if (verbosity > 1) std::cout << "c synchronize barrier wait 2 by thread " << threadnr << std::endl;
    return false;
}

bool ParSolver::sync_consume_shared_data_deterministic(size_t threadnr, bool caller_has_solution, uint64_t new_sync_limit)
{
    /* Being allowed to write own data again */
    solverData[threadnr]->_next_sync_previous_sync_attempts = solverData[threadnr]->_attempted_to_sync;
    solverData[threadnr]->_next_sync_previous_limit = solverData[threadnr]->_next_sync_counter_limit;
    assert(new_sync_limit >= solverData[threadnr]->_next_sync_counter_limit && "do not decrease sync diff value");
    assert((caller_has_solution || solverData[threadnr]->_next_sync_counter_limit <= solverData[threadnr]->_next_sync_current_accesses) &&
           "current steps have to be beyond limit");
    solverData[threadnr]->_next_sync_counter_limit = new_sync_limit;

    /* check whether any solver indicated success above */
    if (solved_current_call.load() != 0) {
        if (verbosity > 2)
            std::cout << "c solver [" << threadnr << "] status: " << solverData[threadnr]->_status
                      << " current-call: " << solved_current_call << std::endl;
        assert((solverData[threadnr]->_status == l_Undef || (solverData[threadnr]->_status == l_True && solved_current_call == 10) ||
                (solverData[threadnr]->_status == l_False && solved_current_call == 20)) &&
               "need to agree on status");
        if (verbosity > 1)
            std::cout << "c sync clauses with thread " << threadnr << " exits early with current call status "
                      << solved_current_call << std::endl;
        return true;
    }

    if (verbosity > 1)
        std::cout << "c sync clauses with thread " << threadnr
                  << " and access limit: " << solvers[threadnr]->counter_access.sum() << std::endl;

    /* consume clauses shared by others, can change status of portfolio */
    bool receive_ret_success = assess_and_consume_shared_clauses(threadnr);
    if (!receive_ret_success) {
        if (verbosity > 1) std::cout << "c receiving clauses resulted in UNSAT for thread " << threadnr << std::endl;
        solverData[threadnr]->_status = l_False;
        solved_current_call.store(20);
    }

    if (threadnr == 0) {
        if (!assignExtraTask(threadnr)) {
            assert(false && "implement failure of assigning extra task");
        }
    }
    /* block on the barrier */
    if (verbosity > 1) std::cout << "c synchronize barrier wait 3 by thread " << threadnr << std::endl;
    return false;
}

bool ParSolver::sync_reset_shared_data_deterministic(size_t threadnr, bool caller_has_solution)
{
    /* free the space used in the pool of this thread again for the next iteration */
    if (verbosity > 1)
        std::cout << "c thread " << threadnr << " resetting its shared pool from size "
                  << solverData[threadnr]->pool.size() << " and status " << solved_current_call << std::endl;
    solverData[threadnr]->pool.reset();

    if (solverData[threadnr]->_status != l_Undef) {
        solverData[threadnr]->_winning++;
        assert((solved_current_call.load() != 0) && "must propagate state");
        assert(((solverData[threadnr]->_status == l_True && solved_current_call == 10) ||
                (solverData[threadnr]->_status == l_False && solved_current_call == 20)) &&
               "states must match");
    }

    if (solved_current_call == 0) solverData[threadnr]->_blocked_by_barrier++;
    if (verbosity > 1)
        std::cout << "c thread " << threadnr << " leaves sync at the end with call status "
                  << solved_current_call.load() << std::endl;
    return false;
}

bool ParSolver::sync_deterministic(size_t threadnr, bool caller_has_solution)
{
    /* keep evaluation and early aborts below limitation check, to maintain deterministic behavior */
    if (sync_prepare_deterministic(threadnr, caller_has_solution)) return true;
    /* in case we do not have the solution already, count waiting time as idle time */
    if (!caller_has_solution) solver_start_measure_idling(threadnr);
    solvingBarrier->wait();
    if (!caller_has_solution) solver_stop_measure_idling(threadnr);

    uint64_t new_sync_limit = 0;
    if (sync_evaluate_state_deterministic(threadnr, caller_has_solution, new_sync_limit)) return true;

    /* prepare clauses to share */
    solvingBarrier->wait();
    if (sync_consume_shared_data_deterministic(threadnr, caller_has_solution, new_sync_limit)) return true;
    solvingBarrier->wait();

    if (sync_reset_shared_data_deterministic(threadnr, caller_has_solution)) return true;

    /* stop search in case we found a solution */
    return solved_current_call != 0;
}

void ParSolver::proof_init(const char *drup_file_name, bool binary_format, int check_proof)
{
    if (proof.enabled()) {
        printf("c ERROR: initialize proof while another proof is active, aborting.");
        assert(false && "a proof should not be initialized multiple times");
        exit(1);
    }
    assert(initialized && "When initializing the proof, solvers should be up and running");

    proof.init(drup_file_name, binary_format, check_proof);

    for (int i = 0; i < solvers.size(); ++i) {
        solvers[i]->proof.init(nullptr, binary_format, check_proof, &proof, true);
    }
}

void ParSolver::proof_setVerbosity(int verb)
{
    if (!proof.enabled()) return;
    proof.setVerbosity(verb);
}

void ParSolver::proof_finalize(bool add_empty_clause)
{
    if (!proof.enabled()) return;
    if (add_empty_clause) proof.addClause('a', vec<Lit>());
    proof.finalize();
}