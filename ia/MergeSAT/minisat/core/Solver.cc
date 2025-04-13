/***************************************************************************************[Solver.cc]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
           Copyright (c) 2007-2010, Niklas Sorensson

Chanseok Oh's MiniSat Patch Series -- Copyright (c) 2015, Chanseok Oh

Maple_LCM, Based on MapleCOMSPS_DRUP -- Copyright (c) 2017, Mao Luo, Chu-Min LI, Fan Xiao: implementing a learnt clause
minimisation approach Reference: M. Luo, C.-M. Li, F. Xiao, F. Manya, and Z. L. , “An effective learnt clause
minimization approach for cdcl sat solvers,” in IJCAI-2017, 2017, pp. to–appear.

Maple_LCM_Dist, Based on Maple_LCM -- Copyright (c) 2017, Fan Xiao, Chu-Min LI, Mao Luo: using a new branching heuristic
called Distance at the beginning of search MapleLCMDistChronoBT-DL, based on MapleLCMDistChronoBT -- Copyright (c),
Stepan Kochemazov, Oleg Zaikin, Victor Kondratiev, Alexander Semenov: The solver was augmented with heuristic that moves
duplicate learnt clauses into the core/tier2 tiers depending on a number of parameters.

Maple_LCM_Dist-alluip-trail -- Copyright (c) 2020, Randy Hickey and Fahiem Bacchus,
Based on Trail Saving on Backtrack SAT 2020 paper.

UWrMaxSat based on KP-MiniSat+ -- Copyright (c) 2019-2020 Marek Piotrów: avoid watching assumption literals
MapleLCMDistChronoBT, based on Maple_LCM_Dist -- Copyright (c) 2018, Alexander Nadel, Vadim Ryvchin: "Chronological Backtracking" in SAT-2018, pp. 111-121.

MapleLCMDistChronoBT-DL, based on MapleLCMDistChronoBT -- Copyright (c) 2019, Stepan Kochemazov, Oleg Zaikin, Victor Kondratiev,
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

#include <algorithm>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>

#include "core/Lookahead.h"
#include "core/Solver.h"
#include "mtl/Sort.h"

#include "utils/Options.h"
#include "utils/System.h"
#include "utils/ccnr.h"

using namespace MERGESAT_NSPACE;

//=================================================================================================
// Options:

static const char *_cat = "CORE";

static IntOption opt_stat_conflicts(_cat, "stat-cons", "Stats every X conflicts", 100000, IntRange(1, INT32_MAX), false);
static DoubleOption opt_step_size(_cat, "step-size", "Initial step size", 0.40, DoubleRange(0, false, 1, false));
static DoubleOption opt_step_size_dec(_cat, "step-size-dec", "Step size decrement", 0.000001, DoubleRange(0, false, 1, false));
static DoubleOption opt_min_step_size(_cat, "min-step-size", "Minimal step size", 0.06, DoubleRange(0, false, 1, false));
static DoubleOption opt_var_decay(_cat, "var-decay", "The variable activity decay factor", 0.80, DoubleRange(0, false, 1, false));
static IntOption
opt_var_decay_conflicts(_cat, "var-decay-conflicts", "Bump var decay after X conflicts", 5000, IntRange(1, INT32_MAX));
static DoubleOption opt_clause_decay(_cat, "cla-decay", "The clause activity decay factor", 0.999, DoubleRange(0, false, 1, false));
static DoubleOption opt_lbd_avg_compare_limit(_cat,
                                              "lbd-avg-compare-limit",
                                              "Constant used to force restart (higher == less restarts)",
                                              0.8,
                                              DoubleRange(0, false, 1, false));
static IntOption
opt_restart_queue_len(_cat, "lbd-restart-queue-len", "length of queue to trigger dynamic restarts", 50, IntRange(1, INT32_MAX));
static DoubleOption
opt_random_var_freq(_cat,
                    "rnd-freq",
                    "The frequency with which the decision heuristic tries to choose a random variable",
                    0,
                    DoubleRange(0, true, 1, true));
static DoubleOption
opt_random_seed(_cat, "rnd-seed", "Used by the random variable selection", 91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption opt_ccmin_mode(_cat,
                                "ccmin-mode",
                                "Controls conflict clause minimization (0=none, 1=basic, 2=deep, 3=deep+binMinimize)",
                                3,
                                IntRange(0, 3));
static IntOption
opt_phase_saving(_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static IntOption opt_init_act(_cat, "rnd-init", "Initial activity is 0:0, 1:random, 2:1000/v, 3:v", 0, IntRange(0, 3));
static IntOption opt_init_act_init(_cat, "rnd-init-init", "Initial activity for rnd-init=2", 1000, IntRange(1, INT32_MAX));
static IntOption opt_restart_first(_cat, "rfirst", "The base restart interval", 100, IntRange(1, INT32_MAX));
static IntOption opt_restart_strategy(_cat, "restart", "How to schedule restarts (split,luby,dynamic)", 1, IntRange(1, 3));
static DoubleOption opt_garbage_frac(_cat,
                                     "gc-frac",
                                     "The fraction of wasted memory allowed before a garbage collection is triggered",
                                     0.20,
                                     DoubleRange(0, false, HUGE_VAL, false));
static BoolOption opt_gc_order(_cat, "gc-order", "GC watches in order or assignment and decision heap", false);
static IntOption opt_chrono(_cat, "chrono", "Controls if to perform chrono backtrack", 100, IntRange(-1, INT32_MAX));
static IntOption
opt_conf_to_chrono(_cat, "confl-to-chrono", "Controls number of conflicts to perform chrono backtrack", 4000, IntRange(-1, INT32_MAX));
static IntOption opt_restart_select(_cat,
                                    "rtype",
                                    "How to select the restart level (0=0, 1=matching trail, 2=reused trail, 3=always "
                                    "partial, 4=random)",
                                    2,
                                    IntRange(0, 4));
static BoolOption opt_almost_pure(_cat, "almost-pure", "Try to optimize polarity by ignoring units", false);
static BoolOption opt_lcm(_cat, "lcm", "Use LCM", true);
static BoolOption opt_reverse_lcm(_cat, "lcm-reverse", "Try to continue LCM with reversed clause in case of success", true);
static BoolOption opt_lcm_core(_cat, "lcm-core", "Shrink the final conflict with LCM", true);
static IntOption opt_lcm_delay(_cat, "lcm-delay", "First number of conflicts before starting LCM", 1000, IntRange(0, INT32_MAX));
static IntOption opt_lcm_delay_inc(_cat,
                                   "lcm-delay-inc",
                                   "After first LCM, how many conflicts to see before running the next LCM",
                                   1000,
                                   IntRange(0, INT32_MAX));
static IntOption
opt_dup_buffer_size(_cat, "lcm-dup-buffer", "Number of clauses to keep for duplicate check", 16, IntRange(0, 1024));
static Int64Option
opt_vsids_c(_cat, "vsids-c", "conflicts after which we want to switch back to VSIDS (0=off)", 12000000, Int64Range(0, INT64_MAX));
static Int64Option
opt_vsids_p(_cat, "vsids-p", "propagations after which we want to switch back to VSIDS (0=off)", 3000000000, Int64Range(0, INT64_MAX));
static BoolOption opt_pref_assumpts(_cat, "pref-assumpts", "Assign all assumptions at once", true);

static Int64Option
opt_VSIDS_props_limit(_cat,
                      "VSIDS-lim",
                      "specifies the number of propagations after which the solver switches between LRB and VSIDS.",
                      30 * 1000000,
                      Int64Range(1, INT64_MAX));
static Int64Option opt_VSIDS_props_init_limit(_cat,
                                              "VSIDS-init-lim",
                                              "specifies the number of propagations before we start with LRB.",
                                              10000,
                                              Int64Range(1, INT64_MAX));
static IntOption opt_la_dec_level(_cat,
                                  "la-dec-level",
                                  "Perform look-ahead based decisions until this level (0 = off, -1 = always)",
                                  0,
                                  IntRange(-1, INT32_MAX));
static IntOption
opt_la_reprecompute(_cat,
                    "la-re-preselect",
                    "How often should we re-calculate preselected variables (-1 = always, 0 = on failure)",
                    0,
                    IntRange(-1, INT32_MAX));
static IntOption opt_la_every(_cat, "la-every", "Use lookahead every X-th attempt (1 = always)", 128, IntRange(1, INT32_MAX));
static BoolOption opt_la_dla(_cat, "la-dla", "Use DLA during LA decisions", true);
static IntOption opt_inprocessing_init_delay(_cat,
                                             "inprocess-init-delay",
                                             "Use this amount of iterations before using inprocessing (-1 == off)",
                                             -1,
                                             IntRange(-1, INT32_MAX));
static DoubleOption opt_inprocessing_inc(_cat,
                                         "inprocess-delay",
                                         "Use this factor to wait for next inprocessing (0=off)",
                                         2,
                                         DoubleRange(0, true, HUGE_VAL, false));
static Int64Option opt_inprocessing_penalty(_cat,
                                            "inprocess-penalty",
                                            "Add this amount, in case inprocessing did not simplify anything",
                                            2,
                                            Int64Range(0, INT64_MAX));
static IntOption opt_inprocess_learnt_level(_cat,
                                            "inprocess-learnt-level",
                                            "Which clauses to consider for inprocessing (1=core only, 3=all learnts)",
                                            2,
                                            IntRange(1, INT32_MAX));
static BoolOption opt_check_sat(_cat, "check-sat", "Store duplicate of formula and check SAT answers", false, false);
static BoolOption
opt_use_backuped_trail(_cat, "use-backup-trail", "Store trail during backtracking, and use it during propagation", true);
static IntOption opt_core_size_lim(_cat,
                                   "core-size-lim",
                                   "Start reducing core learnts, if we collected more than the given number",
                                   50000,
                                   IntRange(-1, INT32_MAX));
static DoubleOption opt_core_size_lim_inc(_cat,
                                          "core-size-lim-inc",
                                          "Percent to increase cycles between core clause reductions",
                                          0.1,
                                          DoubleRange(0.1, true, HUGE_VAL, false));

static BoolOption opt_use_ccnr("SLS", "use-ccnr", "Use SLS engine CCNR", true);
static BoolOption opt_allow_rephasing("SLS", "use-rephasing", "Use polarity rephasing", true);
static IntOption opt_ccnr_restarts_gap("SLS", "ccnr-restart-gap", "TBD", 300, IntRange(0, INT32_MAX));
static DoubleOption opt_ccnr_conflict_ratio("SLS", "ccnr-conflict-ratio", "TBD", 0.4, DoubleRange(0, true, 1, true));
static DoubleOption opt_ccnr_percent_ratio("SLS", "ccnr-percent-ratio", "TBD", 0.9, DoubleRange(0, true, 1, true));
static DoubleOption opt_ccnr_up_time_ratio("SLS", "ccnr-up-time-ratio", "TBD", 0.2, DoubleRange(0, true, 1, true));
static IntOption opt_ccnr_ls_mems_num("SLS", "ccnr-ls-mems", "TBD", 50 * 1000 * 1000, IntRange(0, INT32_MAX));
static IntOption opt_ccnr_state_change_time("SLS", "ccnr-change-time", "TBD", 2000, IntRange(0, INT32_MAX));
static IntOption
opt_ccnr_state_change_time_inc("SLS", "ccnr-change-time-inc", "increment rephasing distance after rephasing by", 1, IntRange(0, INT32_MAX));
static DoubleOption opt_ccnr_state_change_time_inc_inc("SLS",
                                                       "ccnr-change-time-inc-inc",
                                                       "increment rephasing increment distance by",
                                                       0.2,
                                                       DoubleRange(0, true, HUGE_VAL, true));
static BoolOption opt_ccnr_mediation_used("SLS", "ccnr-mediation", "TBD", false);
static IntOption opt_ccnr_switch_heristic_mod("SLS", "ccnr-switch-heuristic", "TBD", 500, IntRange(0, INT32_MAX));
static BoolOption opt_sls_initial("SLS", "ccnr-initial", "run CCNR right at start", false);
static IntOption
opt_sls_var_lim("SLS", "sls-var-lim", "Do not use SLS, if input variables exceed the given value", -1, IntRange(-1, INT32_MAX));
static IntOption
opt_sls_clause_lim("SLS", "sls-clause-lim", "Do not use SLS, if SLS input clauses exceed the given value", -1, IntRange(-1, INT32_MAX));

static IntOption
opt_max_lbd_calc_size(_cat, "max-lbd-calc", "Do not calculate LBD for clauses larger than X", 100, IntRange(0, INT32_MAX));

static IntOption
opt_share_max_cls_size("PARALLEL", "share-max-cls-size", "Do not share clauses greater than X", 3, IntRange(1, INT32_MAX));
static IntOption
opt_share_max_cls_lbd("PARALLEL", "share-max-cls-lbd", "Do not share clauses with LBD greater than X", 3, IntRange(1, INT32_MAX));

static IntOption opt_start_mini(_cat, "start-mini", "Conflicts when to use clause minimization", 0, IntRange(0, INT32_MAX));
static IntOption opt_start_litRed(_cat, "start-litRed", "Conflicts when to use litRedundant minimization", 0, IntRange(0, INT32_MAX));
static IntOption opt_start_lcm(_cat, "start-lcm", "Conflicts when to use LCM", 0, IntRange(0, INT32_MAX));
static IntOption
opt_max_vars_to_bump_per_analysis(_cat, "bumps-per-conflict", "Bump at most this nr of vars per conflict", INT32_MAX, IntRange(0, INT32_MAX));
static BoolOption opt_i_uip(_cat, "i-uip", "Use i-uip to reduce clause size", false);
static BoolOption opt_i_mini(_cat, "i-mini", "greedily choose shorter learned clause", false);
static IntOption
opt_auip_level(_cat, "aiup-learn-level", "For decision levels up to this level, learn all UIP clauses", 0, IntRange(0, INT32_MAX));
static IntOption opt_otfss(_cat, "otfss-lbd", "Run OTFSS for candidates with lbd less than this value", 0, IntRange(0, INT32_MAX));
static IntOption
opt_na_maxCls(_cat, "na-max-cls", "Test at most X binary clauses for necessary assignment", 4, IntRange(1, INT32_MAX));
static IntOption opt_na_attemptEvery(_cat,
                                     "na-attempt-every",
                                     "Attempt necessary assignment every X-th level 1 decision (0 == off)",
                                     0,
                                     IntRange(0, INT32_MAX));
static IntOption opt_na_recheckInc(_cat,
                                   "na-recheck-every",
                                   "Run NA check for a literal again, if number of binary clauses grew by X",
                                   8,
                                   IntRange(1, INT32_MAX));
static BoolOption opt_use_default_math(_cat, "lib-math", "Use default math", false);

//=================================================================================================
// Constructor/Destructor:

bool MERGESAT_NSPACE::updateOptions()
{
    if (getenv("MINISAT_RUNTIME_ARGS") == NULL) return false;

    char *args = strdup(getenv("MINISAT_RUNTIME_ARGS")); // make sure it's freed
    if (!args) return false;
    char *original_args = args;

    std::vector<char *> argv;
    argv.push_back(strdup("mergesat"));

    while (isspace(*args)) ++args;
    while (*args) {
        argv.push_back(args);                    // store current argument
        while (*args && !isspace(*args)) ++args; // skip current token
        if (!*args) break;
        *args = (char)0; // separate current token
        ++args;
    }

    argv.push_back(NULL);

    int count = argv.size() - 1;
    parseOptions(count, argv.data(), false);
    free(argv[0]);
    free(original_args);
    return false;
}

Solver::Solver()
  :

  // Parameters (user settable):
  //
  reparsed_options(updateOptions())
  , verbosity(0)
  , status_every(opt_stat_conflicts)
  , step_size(opt_step_size)
  , step_size_dec(opt_step_size_dec)
  , min_step_size(opt_min_step_size)
  , var_decay_timer(opt_var_decay_conflicts)
  , var_decay_timer_init(opt_var_decay_conflicts)
  , var_decay(opt_var_decay)
  , clause_decay(opt_clause_decay)
  , random_var_freq(opt_random_var_freq)
  , random_seed(opt_random_seed)
  , ccmin_mode(opt_ccmin_mode)
  , phase_saving(opt_phase_saving)
  , invert_pol(false)
  , init_act(opt_init_act)
  , init_act_init(opt_init_act_init)
  , garbage_frac(opt_garbage_frac)
  , gc_in_assign_order(opt_gc_order)
  , restart_first(opt_restart_first)

  // Parameters (the rest):
  //
  , learntsize_factor((double)1 / (double)3)
  , learntsize_inc(1.1)

  // Parameters (experimental):
  //
  , learntsize_adjust_start_confl(100)
  , learntsize_adjust_inc(1.5)

  // ALLUIP learning
  , uipMini(opt_i_uip, opt_i_mini)

  // Statistics: (formerly in 'SolverStats')
  //
  , solves(0)
  , starts(0)
  , decisions(0)
  , rnd_decisions(0)
  , propagations(0)
  , conflicts(0)
  , conflicts_VSIDS(0)
  , dec_vars(0)
  , clauses_literals(0)
  , learnts_literals(0)
  , max_literals(0)
  , tot_literals(0)
  , chrono_backtrack(0)
  , non_chrono_backtrack(0)
  , backuped_trail_lits(0)
  , used_backup_lits(0)

  , systematic_branching_state(0)
  , posMissingInSome(opt_almost_pure ? 0 : 1)
  , negMissingInSome(opt_almost_pure ? 0 : 1)

  , restart(opt_restart_select)
  , restart_strategy(opt_restart_strategy)
  , restartStrategy(opt_lbd_avg_compare_limit, opt_restart_queue_len)

  , VSIDS_conflicts(opt_vsids_c)
  , VSIDS_propagations(opt_vsids_p)
  , reactivate_VSIDS(false)

  , inprocessing_C(0)
  , inprocessing_L(0)
  , inprocess_mems(0)
  , inprocessings(0)

  , techniqueStartConflicts(opt_start_mini, opt_start_litRed, opt_start_lcm)
  , max_vars_to_bump_per_analysis(opt_max_vars_to_bump_per_analysis)

  , lookAhead(nullptr)
  , lookAheadDecisionLevel(opt_la_dec_level)
  , lookAheadRecompute(opt_la_reprecompute)
  , lookAheadRepeat(false)
  , dlaDecision(opt_la_dla)
  , lookAheadEvery(opt_la_every)
  , la_stat_conflicts(0)
  , la_stat_decisions(0)
  , la_stat_nodecisions(0)
  , la_stat_extra_recomputes(0)
  , la_stat_attempts(0)

  , ok(true)
  , cla_inc(1)
  , var_inc(1)
  , watches_bin(WatcherDeleted(ca), counter_access)
  , watches(WatcherDeleted(ca), counter_access)
  , qhead(0)
  , use_backuped_trail(opt_use_backuped_trail)
  , old_trail_qhead(0)
  , simpDB_assigns(-1)
  , simpDB_props(0)
  , constraint_active(false)
  , unsat_constraint(false)
  , current_heuristic(DISTANCE)
  , order_heap_VSIDS(VarOrderLt(activity_VSIDS))
  , order_heap_CHB(VarOrderLt(activity_CHB))
  , order_heap_DISTANCE(VarOrderLt(activity_distance))
  , order_heap(&order_heap_DISTANCE)
  , max_lbd_calc_size(opt_max_lbd_calc_size)
  , bumpPow(0.95, opt_use_default_math)
  , full_heap_size(-1)
  , remove_satisfied(true)
  , check_satisfiability(opt_check_sat)
  , check_satisfiability_simplified(false)

  , core_lbd_cut(3)
  , tier2_lbd_cut(6)
  , core_size_lim(opt_core_size_lim)
  , core_size_lim_inc(opt_core_size_lim_inc)
  , next_T2_reduce(10000)
  , next_L_reduce(15000)

  , counter_access()
  , ca(counter_access, (uint32_t)(1024 * 1024))

  , confl_to_chrono(opt_conf_to_chrono)
  , chrono(opt_chrono)

  , termCallbackState(0)
  , termCallback(0)
  , learnCallbackState(0)
  , learnCallbackLimit(0)
  , learnCallback(0)

  , counter(0)

  , T(0)
  , inprocess_attempts(0)
  , inprocess_next_lim(opt_inprocessing_init_delay)
  , L(0)
  , inprocess_learnt_level(opt_inprocess_learnt_level)

  , inprocess_inc(opt_inprocessing_inc)
  , inprocess_penalty(opt_inprocessing_penalty)

  , max_learnts(0)
  , learntsize_adjust_confl(0)
  , learntsize_adjust_cnt(0)
  , maxLearnAllUIPsLevel(opt_auip_level)
  , allUIPlearntClauses(0)
  , allUIPlearntClausesTotalSize(0)
  , allUIPlearntClausesTotalLBDs(0)

  , otfssMaxLBD(opt_otfss)
  , totalOtfssClauses(0)
  , totalOtfssClauseSizes(0)
  , otfssUnits(0)
  , otfssImplied(0)
  , otfssConflicts(0)

  , nAssignment(*this, opt_na_maxCls, opt_na_attemptEvery, opt_na_recheckInc)

  , VSIDS_props_limit(opt_VSIDS_props_limit)
  , VSIDS_props_init_limit(opt_VSIDS_props_init_limit)
  , switch_mode(false)

  // Resource constraints:
  //
  , conflict_budget(-1)
  , propagation_budget(-1)
  , asynch_interrupt(false)

  , prefetch_assumptions(opt_pref_assumpts)
  , last_used_assumptions(INT32_MAX)

  // simplfiy
  , trailRecord(0)
  , nbSimplifyAll(0)
  , simplified_length_record(0)
  , original_length_record(0)
  , s_propagations(0)
  , nr_lcm_duplicates(0)
  , simplifyBuffer(opt_dup_buffer_size)

  , share_parallel(true)
  , share_max_cls_size(opt_share_max_cls_size)
  , share_max_cls_lbd(opt_share_max_cls_lbd)
  , receiveClauses(true)
  , share_clause_max_size(64)
  , shared_unit_clauses(0)
  , receivedCls(0)
  , sync_conflicts(32)
  , learnedClsCallback(NULL)
  , consumeSharedCls(NULL)
  , issuer(NULL)
  , lastDecision(0)
  , external_sync_and_share(nullptr)
  , abort_search_after_sync(false)

  // simplifyAll adjust occasion
  , curSimplify(1)
  , nbconfbeforesimplify(opt_lcm_delay)
  , incSimplify(opt_lcm_delay_inc)
  , lcm(opt_lcm)
  , reverse_LCM(opt_reverse_lcm)
  , lcm_core(opt_lcm_core)
  , lcm_core_success(true) // start in the first round
  , LCM_total_tries(0)
  , LCM_successful_tries(0)
  , LCM_dropped_lits(0)
  , LCM_dropped_reverse(0)

  , nbcollectfirstuip(0)
  , nblearntclause(0)
  , nbDoubleConflicts(0)
  , nbTripleConflicts(0)
  , uip1(0)
  , uip2(0)

  , var_iLevel_inc(1)
  , my_var_decay(0.6)

  // for ccnr integration
  , use_ccnr(opt_use_ccnr)
  , allow_rephasing(opt_allow_rephasing)
  , restarts_gap(opt_ccnr_restarts_gap)
  , conflict_ratio(opt_ccnr_conflict_ratio)
  , percent_ratio(opt_ccnr_percent_ratio)
  , up_time_ratio(opt_ccnr_up_time_ratio)
  , ls_mems_num(opt_ccnr_ls_mems_num)
  , state_change_time(opt_ccnr_state_change_time)
  , state_change_time_inc(opt_ccnr_state_change_time_inc)
  , state_change_time_inc_inc(opt_ccnr_state_change_time_inc_inc)
  , mediation_used(opt_ccnr_mediation_used)
  , switch_heristic_mod(opt_ccnr_mediation_used)
  , last_switch_conflicts(0)
  , sls_var_lim(opt_sls_var_lim)
  , sls_clause_lim(opt_sls_clause_lim)

  , initial_sls(opt_sls_initial)
{
    lookAhead = new Lookahead(*this);
}


Solver::~Solver()
{
    if (lookAhead != nullptr) {
        delete lookAhead;
        lookAhead = nullptr;
    }
}

void Solver::simpleUncheckEnqueue(Lit p, CRef from)
{
    assert(value(p) == l_Undef);
    TRACE(if (from != CRef_Undef) {
        std::cerr << "c simpleEnqueue literal " << p << " with clause " << ca[from] << std::endl;
    } else { std::cerr << "c simpleEnqueue literal " << p << " with clause " << from << std::endl; });
    assigns[var(p)] = lbool(!sign(p)); // this makes a lbool object whose value is sign(p)
    vardata[var(p)].reason = from;
    vardata[var(p)].level = decisionLevel();
    trail.push_(p);
#ifndef NO_PREFETCHING
    __builtin_prefetch(&watches[p], 1, 0); // prefetch the watch, prepare for a write (1), the data is highly temoral (0)
#endif
}

void Solver::cancelUntilTrailRecord()
{
    for (int c = trail.size() - 1; c >= trailRecord; c--) {
        Var x = var(trail[c]);
        assigns[x] = l_Undef;
    }
    qhead = trailRecord;
    trail.shrink(trail.size() - trailRecord);
}

void Solver::litsEnqueue(int cutP, Clause &c)
{
    for (int i = cutP; i < c.size(); i++) {
        simpleUncheckEnqueue(~c[i]);
    }
}

bool Solver::removed(CRef cr) { return ca[cr].mark() == 1; }

void Solver::simpleAnalyze(CRef confl, vec<Lit> &out_learnt, vec<CRef> &reason_clause, bool True_confl)
{
    int pathC = 0;
    Lit p = lit_Undef;
    int index = trail.size() - 1;

    do {
        if (confl != CRef_Undef) {
            reason_clause.push(confl);
            Clause &c = ca[confl];
            // Special case for binary clauses
            // The first one has to be SAT
            if (p != lit_Undef && c.size() == 2 && value(c[0]) == l_False) {

                assert(value(c[1]) == l_True);
                Lit tmp = c[0];
                c[0] = c[1], c[1] = tmp;
            }
            // if True_confl==true, then choose p begin with the 1th index of c;
            for (int j = (p == lit_Undef && True_confl == false) ? 0 : 1; j < c.size(); j++) {
                Lit q = c[j];
                if (!seen[var(q)] && level(var(q)) > 0) { /* we will not touch level 0 variables */
                    seen[var(q)] = 1;
                    pathC++;
                }
            }
        } else if (confl == CRef_Undef) {
            out_learnt.push(~p);
        }
        // if not break, while() will come to the index of trail blow 0, and fatal error occur;
        if (pathC == 0) break;
        // Select next clause to look at:
        while (!seen[var(trail[index--])])
            ;
        // if the reason cr from the 0-level assigned var, we must break avoid move forth further;
        // but attention that maybe seen[x]=1 and never be clear. However makes no matter;
        if (trailRecord > index + 1) break;
        p = trail[index + 1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;

    } while (pathC >= 0);
}

bool Solver::isSimplifyDuplicate(CRef cr)
{
    // if there is no buffer, we do not have duplicates
    if (simplifyBuffer.size() == 0) {
        return false;
    }
    const Clause &c = ca[cr];
    int checkIndex = 0;
    // first, check on clause size
    for (; checkIndex < simplifyBuffer.size(); ++checkIndex) {
        const CRef bufferCRef = simplifyBuffer[checkIndex];
        if (bufferCRef == CRef_Undef) continue;
        if (bufferCRef == cr) continue;
        const Clause &d = ca[bufferCRef];
        if (c.size() == d.size()) break;
    }
    // no clause in buffer with the same size
    if (checkIndex == simplifyBuffer.size()) return false;

    // fill seen vector with literals of candidate clause
    counter++;
    for (int i = 0; i < c.size(); i++) {
        Lit l = c[i];
        seen2[toInt(l)] = counter;
    }
    // check for all remaining clauses, whether they hit all literals of c, and no others
    for (; checkIndex < simplifyBuffer.size(); ++checkIndex) {
        const CRef bufferCRef = simplifyBuffer[checkIndex];
        if (bufferCRef == CRef_Undef) continue;
        if (bufferCRef == cr) continue;
        const Clause &d = ca[bufferCRef];
        if (c.size() != d.size()) continue;
        int hits = 0;
        for (int i = 0; i < d.size(); ++i) {
            Lit l = d[i];
            if (seen2[toInt(l)] == counter)
                hits++;
            else
                break;
        }
        if (hits == d.size()) {
            return true;
        }
    }
    return false;
}

bool Solver::simplifyLearnt(vec<CRef> &target_learnts, bool is_tier2)
{
    int ci, cj, li, lj;
    bool sat, false_lit;
    int nblevels;
    ////
    ////
    int nbSimplified = 0;
    int nbSimplifing = 0;

    bool ret = true;

    for (ci = 0, cj = 0; ci < target_learnts.size(); ci++) {
        CRef cr = target_learnts[ci];
        Clause &c = ca[cr];

        if (removed(cr) || c.size() == 1)
            continue;
        else if (c.simplified()) {
            target_learnts[cj++] = target_learnts[ci];
            ////
            nbSimplified++;
        } else {
            int saved_size = c.size();
            /* DRUP: in case we want a correct proof, i.e. track all simplifications,
               we would need to keep track of the original clause here. Let's skip this
               for efficiency. */
            //         if (drup_file){
            //                 add_oc.clear();
            //                 for (int i = 0; i < c.size(); i++) add_oc.push(c[i]); }
            ////
            nbSimplifing++;
            sat = false_lit = false;
            for (int i = 0; i < c.size(); i++) {
                if (value(c[i]) == l_True) {
                    sat = true;
                    break;
                } else if (value(c[i]) == l_False) {
                    false_lit = true;
                }
            }
            if (sat) {
                removeSatisfiedClause(cr);
            } else {
                detachClause(cr, true);

                if (false_lit) {
                    for (li = lj = 0; li < c.size(); li++) {
                        if (value(c[li]) != l_False) {
                            c[lj++] = c[li];
                        }
                    }
                    c.shrink(li - lj);
                    TRACE(std::cout << "c dropped" << li - lj << " literals from clause [" << cr << "]: " << c << std::endl);
                    c.S(0); // this clause might subsume others now
                }

                assert(c.size() > 1);
                // simplify a learnt clause c
                TRACE(std::cout << "c LCM simplify clause[" << cr << "]: " << c << std::endl);
                simplifyLearnt(c);

                bool recentDuplicate = isSimplifyDuplicate(cr);

                if (!recentDuplicate) {

                    TRACE(std::cout << "c LCM keep (simplified?) clause[" << cr << "]: " << c << std::endl);

                    if (saved_size != c.size()) {
                        shareViaCallback(c, c.lbd()); // share via IPASIR?
                        proof.addClause('a', c); // TODO: could also drop previous clause, but would have to always copy the whole clause each time
                    }

                    if (c.size() == 0) {
                        ok = false;
                        ret = false;
                        ci++;
                        while (ci < target_learnts.size()) target_learnts[cj++] = target_learnts[ci++];
                        goto simplifyLearnt_out;
                    } else if (c.size() == 1) {
                        // when unit clause occur, enqueue and propagate
                        uncheckedEnqueue(c[0], 0);
                        c.mark(1);
                        if (propagate() != CRef_Undef) {
                            ok = false;
                            ret = false;
                            ci++;
                            while (ci < target_learnts.size()) target_learnts[cj++] = target_learnts[ci++];
                            goto simplifyLearnt_out;
                        }
                        // delete the clause memory in logic
                        ca.free(cr);
                        /* DRUP: in case we want a correct proof, i.e. track all simplifications,
                        we would need to keep track of the original clause here. Let's skip this
                        for efficiency. */
                        // proof.addClause('d', c);
                    } else {
                        attachClause(cr);
                        target_learnts[cj++] = target_learnts[ci];
                        simplifyBuffer.addNext(cr); /* store in duplicate buffer */

                        nblevels = computeLBD(c);
                        if (nblevels < c.lbd()) {
                            c.set_lbd(nblevels);
                        }

                        // in case we work on the tier2 set, a clause might move to core learnt clauses
                        if (is_tier2 && c.lbd() <= core_lbd_cut) {
                            cj--;
                            learnts_core.push(cr);
                            c.mark(CORE);
                        }

                        c.setSimplified(true);
                    }

                } else {
                    TRACE(std::cout << "c LCM: drop duplicate simplified clause[" << cr << "]: " << ca[cr] << std::endl);
                    // this clause is a duplicate now, hence, mark it accordingly
                    /* DRUP: do not delete this simplified clause from the proof, as we did not add it yet */
                    removeSatisfiedClause(cr, false);
                    c.mark(1);
                    nr_lcm_duplicates++;
                }
            }
        }
    }
simplifyLearnt_out:;
    target_learnts.shrink(ci - cj);

    return ret;
}

bool Solver::simplifyAll()
{
    reset_old_trail();

    ////
    simplified_length_record = original_length_record = 0;

    // make sure we have no decisions left due to partial restarts
    cancelUntil(0);

    if (!ok || propagate() != CRef_Undef) return ok = false;

    assert(decisionLevel() == 0 && "LCM works only on level 0");

    if (!simplifyLearnt(learnts_core, false)) return ok = false;
    if (!simplifyLearnt(learnts_tier2, true)) return ok = false;

    checkGarbage();

    ////
    //  printf("c size_reduce_ratio     : %4.2f%%\n",
    //         original_length_record == 0 ? 0 : (original_length_record - simplified_length_record) * 100 / (double)original_length_record);

    return true;
}
//=================================================================================================
// Minor methods:


/****************************************************************
 Set the incremental mode
****************************************************************/

// This function set the incremental mode to true.
// You can add special code for this mode here.

void Solver::setIncrementalMode()
{
    // TODO decide which features to enable as incremental mode (see glucose 3.0)
}

// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches_bin.init(mkLit(v, false));
    watches_bin.init(mkLit(v, true));
    watches.init(mkLit(v, false));
    watches.init(mkLit(v, true));
    assigns.push(l_Undef);
    vardata.push(mkVarData(CRef_Undef, 0));
    oldreasons.push(CRef_Undef);
    activity_CHB.push(0);
    float new_activity = 0;
    if (init_act == 1)
        new_activity = drand(random_seed) * 0.00001;
    else if (init_act == 2)
        new_activity = init_act_init / (v + 1);
    else if (init_act == 3)
        new_activity = v;
    activity_VSIDS.push(new_activity);
    activity_distance.push(0);

    picked.push(0);
    conflicted.push(0);
#ifdef ANTI_EXPLORATION
    canceled.push(0);
#endif

    seen.push(0);
    seen2.push(0);
    seen2.push(0);
    polarity.push(invert_pol ? !sign : sign);
    decision.push();
    trail.capacity(v + 1);
    old_trail.capacity(v + 1);

    var_iLevel.push(0);
    var_iLevel_tmp.push(0);
    pathCs.push(0);

    // TODO: make sure to add new data structures also to reserveVars below!

    setDecisionVar(v, dvar);
    return v;
}

void Solver::reserveVars(Var v)
{
    watches_bin.init(mkLit(v, false));
    watches_bin.init(mkLit(v, true));
    watches.init(mkLit(v, false));
    watches.init(mkLit(v, true));
    assigns.capacity(v + 1);
    vardata.capacity(v + 1);
    oldreasons.capacity(v + 1);
    activity_CHB.capacity(v + 1);
    activity_VSIDS.capacity(v + 1);
    activity_distance.capacity(v + 1);

    picked.capacity(v + 1);
    conflicted.capacity(v + 1);
#ifdef ANTI_EXPLORATION
    canceled.capacity(v + 1);
#endif

    seen.capacity(v + 1);
    seen2.capacity(2 * v + 1);
    polarity.capacity(v + 1);
    decision.capacity(v + 1);
    trail.capacity(v + 1);
    old_trail.capacity(v + 1);

    var_iLevel.capacity(v + 1);
    var_iLevel_tmp.capacity(v + 1);
    pathCs.capacity(v + 1);
}


bool Solver::addClause_(vec<Lit> &ps)
{
    assert(decisionLevel() == 0);
    if (!ok) return false;

    if (check_satisfiability) satChecker.addClause(ps); // add for SAT check tracking

    if (proof.enabled()) {
        add_oc.clear();
        for (int i = 0; i < ps.size(); i++) add_oc.push(ps[i]);
    }

    lbool integratable = prepareTopLevelClauseIntegration(ps);
    if (integratable == l_True) return true;

    if (proof.enabled() && add_oc.size() != ps.size()) {
        proof.addClause('a', ps);
        proof.addClause('d', add_oc);
    }

    /* if there are new clauses, we cannot claim that SLS solved the formula anymore */
    solved_by_ls = false;
    if (integratable == l_False || ps.size() == 0)
        return ok = false;
    else if (ps.size() == 1) {
        uncheckedEnqueue(ps[0], 0);
        return ok = (propagate() == CRef_Undef);
    } else {
        CRef cr = ca.alloc(ps, false);
        clauses.push(cr);
        attachClause(cr);
    }

    return true;
}

bool Solver::importClause(const Clause &c, bool addToProof)
{
    assert(decisionLevel() == 0 && "only import clauses on level 0");
    // make writable copy
    CRef cr = ca.alloc(c, false);
    Clause &copy = ca[cr];
    int i, j;
    Lit p;

    for (i = j = 0, p = lit_Undef; i < c.size(); i++) {
        assert(p != copy[i] && "imported clauses should not be redundant");
        assert(p != ~copy[i] && "imported clauses should not be trivial");
        if (value(copy[i]) == l_True) {
            copy.mark(1); // clause is satisfied, make as 'garbage'
            return true;
        } else if (value(copy[i]) != l_False) {
            copy[j++] = p = copy[i];
        }
    }
    copy.shrink(i - j);
    if (addToProof) {
        assert(proof.enabled() && "proof state should be consistent across layers");
        proof.addClause('a', copy);
    }

    /* if there are new clauses, we cannot claim that SLS solved the formula anymore */
    solved_by_ls = false;

    if (copy.size() == 0)
        return ok = false;
    else if (copy.size() == 1) {
        uncheckedEnqueue(copy[0], 0);
        return ok = (propagate() == CRef_Undef);
    } else {
        clauses.push(cr);
        attachClause(cr);
    }

    return true;
}

void Solver::attachClause(CRef cr)
{
    const Clause &c = ca[cr];
    statistics.solveSteps++;
    assert(c.size() > 1);
    OccLists<Lit, vec<Watcher>, WatcherDeleted> &ws = c.size() == 2 ? watches_bin : watches;
    ws[~c[0]].push(Watcher(cr, c[1]));
    ws[~c[1]].push(Watcher(cr, c[0]));
    if (c.learnt())
        learnts_literals += c.size();
    else
        clauses_literals += c.size();
}


void Solver::detachClause(CRef cr, bool strict)
{
    const Clause &c = ca[cr];
    assert(c.size() > 1);

    OccLists<Lit, vec<Watcher>, WatcherDeleted> &ws = c.size() == 2 ? watches_bin : watches;
    statistics.solveSteps++;

    // Strict or lazy detaching:
    if (strict) {
        remove(ws[~c[0]], Watcher(cr, c[1]));
        remove(ws[~c[1]], Watcher(cr, c[0]));
    } else {
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        ws.smudge(~c[0]);
        ws.smudge(~c[1]);
    }

    if (c.learnt())
        learnts_literals -= c.size();
    else
        clauses_literals -= c.size();
}


void Solver::removeClause(CRef cr, bool remove_from_proof)
{
    Clause &c = ca[cr];
    statistics.solveSteps++;

    detachClause(cr);
    // Don't leave pointers to free'd memory!
    if (remove_from_proof) {
        if (locked(c)) {
            Lit implied = c.size() != 2 ? c[0] : (value(c[0]) == l_True ? c[0] : c[1]);
            vardata[var(implied)].reason = CRef_Undef;
            if (level(var(implied)) == 0) { /* before we drop the reason, store a unit */
                if (!proof.addLockedUnitForChecking(mkLit(var(implied), value(var(implied)) == l_False))) exit(134);
            }
        }
        if (proof.enabled()) {
            if (c.mark() != 1) {
                proof.addClause('d', c);
            } else
                assert(false && "Clause for proofs should not be marked differently than with 1");
        }
    }

    c.mark(1);
    ca.free(cr);
}

void Solver::removeSatisfiedClause(CRef cr, bool remove_from_proof)
{
    Clause &c = ca[cr];

    if (proof.enabled() && locked(c)) {
        // The following line was copied from Solver::locked.
        int i = c.size() != 2 ? 0 : (value(c[0]) == l_True ? 0 : 1);
        proof.addUnitClause('a', c[i]);
    }

    removeClause(cr, remove_from_proof);
}


bool Solver::satisfied(const Clause &c) const
{
    if (assumptions.size()) // Check clauses with many selectors is too time consuming
        return (value(c[0]) == l_True) || (value(c[1]) == l_True);

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True) return true;
    return false;
}


// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int bLevel, bool allow_trail_saving, bool updateHeuristics)
{

    if (decisionLevel() > bLevel) {
        TRACE(std::cout << "c backtrack to " << bLevel << std::endl);

        reset_old_trail();

        bool savetrail = allow_trail_saving && use_backuped_trail && (decisionLevel() - bLevel > 1);

        add_tmp.clear();
        for (int c = trail.size() - 1; c >= trail_lim[bLevel]; c--) {
            Var x = var(trail[c]);

            if (level(x) <= bLevel) {
                add_tmp.push(trail[c]);
                continue;
            }
            if (savetrail) {
                old_trail.push_(trail[c]); /* we traverse trail in reverse order */
                oldreasons[x] = reason(x);
            }

            if (updateHeuristics) {
                if (!usesVSIDS()) {
                    uint32_t age = conflicts - picked[x];
                    if (age > 0) {
                        double adjusted_reward = ((double)(conflicted[x])) / ((double)age);
                        double old_activity = activity_CHB[x];
                        activity_CHB[x] = step_size * adjusted_reward + ((1 - step_size) * old_activity);
                        if (usesCHB() && order_heap->inHeap(x)) {
                            if (activity_CHB[x] > old_activity)
                                order_heap->decrease(x);
                            else
                                order_heap->increase(x);
                        }
                    }
                }
#ifdef ANTI_EXPLORATION
                canceled[x] = conflicts;
#endif

                if (phase_saving > 1 || ((phase_saving == 1) && c > trail_lim.last())) polarity[x] = sign(trail[c]);
            }
            assigns[x] = l_Undef;
            insertVarOrder(x);
        }
        qhead = trail_lim[bLevel];
        trail.shrink(trail.size() - trail_lim[bLevel]);
        trail_lim.shrink(trail_lim.size() - bLevel);
        for (int nLitId = add_tmp.size() - 1; nLitId >= 0; --nLitId) {
            trail.push_(add_tmp[nLitId]);
        }

        add_tmp.clear();

        /* reverse saved trail, as we added elements in reverse order as well */
        if (savetrail) {
            int i = 0, j = old_trail.size() - 1;
            while (i < j) {
                const Lit l = old_trail[i];
                old_trail[i++] = old_trail[j];
                old_trail[j--] = l;
            }
            backuped_trail_lits += old_trail.size();
        }
    }
}


//=================================================================================================
// Major methods:

Lit Solver::pickLABranchLit(CRef &confl)
{
    int postAssumptionDecisions = decisionLevel() - assumptions.size();
    // for now, statically use look ahead based on the current decision level - could be determined more dynamically
    if (lookAheadDecisionLevel == -1 || postAssumptionDecisions < lookAheadDecisionLevel) {
        CRef laConfl;
        la_stat_attempts++;

        // do not run look-ahead too often
        if (!lookAheadRepeat && la_stat_attempts % lookAheadEvery != 0) return lit_Undef;

        lookAheadRepeat = false; // reset post-conflict state
        bool recompute_now =
        lookAheadRecompute == -1 || (lookAheadRecompute != 0 && (la_stat_attempts % lookAheadRecompute) == 0);

        Lit laLit = lit_Undef;
        do {
            laLit = lookAhead->lookaheadDecision(laConfl, 100, recompute_now, dlaDecision);
            if (laLit == lit_Undef) {
                if (lookAheadRecompute == 0 && !recompute_now) {
                    // we did not find a literal, check whether we actually have one before re-checking
                    while (!order_heap->empty()) {
                        if (value(order_heap->min()) == l_Undef) break;
                        (void)order_heap->removeMin();
                    }
                    // we assigned all (relevant) decision variables
                    if (order_heap->empty()) break;
                    recompute_now = true;
                    la_stat_extra_recomputes++;
                    continue;
                }
            }
            break;
        } while (true);
        // if we found a conflict, use it
        if (laLit == lit_Error) {
            la_stat_conflicts++;
            confl = laConfl;
            // we found a conflict, proceed with conflict analysis - and use LA again next time
            lookAheadRepeat = true;
            return lit_Error;
        }

        if (verbosity > 2) std::cout << "c finished LA with lit " << laLit << std::endl;
        // we either found a decision literal, or none at all - but no conflict
        if (laLit != lit_Undef)
            la_stat_decisions++;
        else
            la_stat_nodecisions++;
        return laLit; // happy case
    }
    return lit_Undef;
}


Lit Solver::pickBranchLit(CRef &confl, bool autoLA)
{
    Var next = var_Undef;

    // Random decision:
    /*if (drand(random_seed) < random_var_freq && !order_heap.empty()){
        next = order_heap[irand(random_seed,order_heap.size())];
        if (value(next) == l_Undef && decision[next])
            rnd_decisions++; }*/

    // use LA branching? handle conflicts accordingly
    if (autoLA) {
        Lit laLit = pickLABranchLit(confl);
        if (laLit != lit_Undef) {
            assert(laLit != lit_Error || confl != CRef_Undef);
            return laLit;
        }
    }

    // Activity based decision:
    while (next == var_Undef || value(next) != l_Undef || !decision[next])
        if (order_heap->empty())
            return lit_Undef;
        else {
#ifdef ANTI_EXPLORATION
            if (usesCHB()) {
                Var v = (*order_heap)[0];
                uint32_t age = conflicts - canceled[v];
                while (age > 0) {
                    double decay = bumpPow.pow(age);
                    activity_CHB[v] *= decay;
                    if (order_heap->inHeap(v)) order_heap->increase(v);
                    canceled[v] = conflicts;
                    v = (*order_heap)[0];
                    age = conflicts - canceled[v];
                }
            }
#endif
            next = order_heap->removeMin();
        }

    // in case we found (almost) pure literals, disable phase-saving
    if (posMissingInSome == 0 || negMissingInSome == 0)
        return posMissingInSome == 0 ? mkLit(next, false) : mkLit(next, true);

    return mkLit(next, polarity[next]);
}

inline Solver::ConflictData Solver::FindConflictLevel(CRef cind)
{
    ConflictData data;
    Clause &conflCls = ca[cind];
    data.nHighestLevel = level(var(conflCls[0]));
    if (data.nHighestLevel == decisionLevel() && level(var(conflCls[1])) == decisionLevel()) {
        return data;
    }

    int highestId = 0;
    data.bOnlyOneLitFromHighest = true;
    data.secondHighestLevel = 0;
    // find the largest decision level in the clause
    for (int nLitId = 1; nLitId < conflCls.size(); ++nLitId) {
        int nLevel = level(var(conflCls[nLitId]));
        if (nLevel > data.nHighestLevel) {
            highestId = nLitId;
            data.secondHighestLevel = data.nHighestLevel;
            data.nHighestLevel = nLevel;
            data.bOnlyOneLitFromHighest = true;
        } else if (nLevel == data.nHighestLevel && data.bOnlyOneLitFromHighest == true) {
            data.bOnlyOneLitFromHighest = false;
        }
    }

    if (highestId != 0) {
        std::swap(conflCls[0], conflCls[highestId]);
        if (highestId > 1) {
            OccLists<Lit, vec<Watcher>, WatcherDeleted> &ws = conflCls.size() == 2 ? watches_bin : watches;
            // ws.smudge(~conflCls[highestId]);
            remove(ws[~conflCls[highestId]], Watcher(cind, conflCls[1]));
            ws[~conflCls[0]].push(Watcher(cind, conflCls[1]));
        }
    }

    return data;
}

void Solver::i_uip_analyze(vec<Lit> &out_learnt, int i_level, vec<Lit> &analyze_toclear, int out_lbd, int out_btlevel)
{
    int c_size = out_learnt.size();
    int gap_value = uipMini.i_uip_gap;
    if (c_size <= (out_lbd + gap_value)) return;
    TRACE(std::cout << "c" << std::endl << "c" << std::endl << "c UIP new analysis" << std::endl;)

    uipMini.i_uip_attempts++;
    uipMini.prepareNextAnalysis();

    uipMini.i_uip_pathC.growTo(decisionLevel(), 0);
    uipMini.new_out_learnt.push(out_learnt[0]);
    TRACE(std::cout << "c run uip analyze with clause " << out_learnt << std::endl;);

    // determine what is the lowest level to resolve according to i
    for (int i = out_learnt.size() - 1; i >= 1; i--) {
        Var v = var(out_learnt[i]);
        int v_level = level(v);
        uipMini.top_i.push_back(v_level);
        uipMini.i_uip_pathC[v_level - 1]++;
    }

    // FIXME: rewrite into a single method, using vec data structure
    std::sort(uipMini.top_i.begin(), uipMini.top_i.end());
    uipMini.top_i.erase(std::unique(uipMini.top_i.begin(), uipMini.top_i.end()), uipMini.top_i.end());
    std::reverse(uipMini.top_i.begin(), uipMini.top_i.end());
    int ith_index = ((unsigned)i_level >= uipMini.top_i.size()) ? uipMini.top_i.size() - 1 : i_level - 1;
    int lowest_level = uipMini.top_i[ith_index];

    counter++; // prepare for next round of using seen2 array
    // now mark all literals in the learnt clause except for the first literal seen
    for (int i = out_learnt.size() - 1; i >= 1; i--) {
        Var v = var(out_learnt[i]);
        seen2[v] = counter;
        uipMini.to_be_cleaned.push(out_learnt[i]);
        if (level(v) < lowest_level) {
            uipMini.new_out_learnt.push(out_learnt[i]);
        }
    }

    int grace_token = c_size - out_lbd;
    unsigned top_index = 0;
    int current_decision_level = uipMini.top_i[top_index];
    TRACE(std::cout << "c UIP-mini: minimize with current level " << current_decision_level
                    << " lowest: " << lowest_level << " and trail " << trail << std::endl;);
    while (current_decision_level >= lowest_level) {
        TRACE(std::cout << "c UIP-mini: analyze level " << current_decision_level << std::endl;);
        // we need to run over the full trail, as we might be using chronological backtracking sometimes
        int index = trail.size() - 1; // FIXME: scan clause, and update upper bounds per level!
        Lit p;
        bool should_res = true;
        int old_index = index;
        int oldPCcounter = uipMini.i_uip_pathC[current_decision_level - 1];
        uipMini.resolved_away.clear();
        uipMini.new_introduced.clear();
        int num_pushed = 0;
        while (uipMini.i_uip_pathC[current_decision_level - 1] > 1) {
            while (seen2[var(trail[index])] != counter || level(var(trail[index])) != current_decision_level) {
                TRACE(std::cout << "c UIP-mini skip var " << var(trail[index]) << " with level "
                                << level(var(trail[index])) << std::endl;);
                index--;
            }
            p = trail[index];
            CRef confl = reason(var(p));
            index--; // make sure we do not re-process this index
            TRACE(std::cout << "c UIP-mini select var " << var(p) + 1 << " with level " << level(var(p))
                            << " and reason " << confl << std::endl;);
            seen2[var(p)] = 0;
            if (should_res) uipMini.resolved_away.push(p);

            assert(level(var(p)) == current_decision_level);
            uipMini.i_uip_pathC[current_decision_level - 1]--;
            assert(confl != CRef_Undef);
            Clause &c = ca[confl];

            if (c.size() == 2 && value(c[0]) == l_False) {
                assert(value(c[1]) == l_True);
                Lit tmp = c[0];
                c[0] = c[1], c[1] = tmp;
            }

            // check if we should perform resolution, this is for PURE-I-UIP
            if (!should_res) {
                TRACE(std::cout << "c UIP add next literal " << ~p << " to new learnt clause via 'should-not-resolve''"
                                << std::endl;);
                uipMini.new_out_learnt.push(~p);
                seen2[var(~p)] = counter;
                uipMini.to_be_cleaned.push(~p);
                grace_token--;
                if (grace_token == 0 && uipMini.i_mini) {
                    TRACE(std::cout << "c UIP stop as there are no grace token anymore" << std::endl;);
                    // cleanup and stop
                    counter++;
                    return;
                }
            } else {
                TRACE(std::cout << "c UIP test resolve candidate for literal " << p << " ..." << std::endl;);
                for (int j = 1; j < c.size(); j++) {
                    Lit q = c[j];
                    if (seen2[var(q)] != counter && level(var(q)) > 0) {
                        if (uipMini.i_uip_pathC[level(var(q)) - 1] == 0) {
                            should_res = false;
                            TRACE(std::cout << "c UIP resolution would pull in new level, abort" << std::endl;);
                            // revert back to the old state
                            index = old_index;
                            for (int j = 0; j < uipMini.resolved_away.size(); j++) {
                                Lit toRecover = uipMini.resolved_away[j];
                                seen2[var(toRecover)] = counter;
                            }
                            for (int j = 0; j < uipMini.new_introduced.size(); j++) {
                                Lit newLit = uipMini.new_introduced[j];
                                Var newVar = var(newLit);
                                int q_level = level(newVar);
                                if (q_level >= lowest_level) {
                                    uipMini.i_uip_pathC[q_level - 1]--;
                                }
                                seen2[var(newLit)] = 0;
                            }
                            int retrack_counter = 0;
                            while (retrack_counter < num_pushed) {
                                uipMini.new_out_learnt.pop();
                            }
                            uipMini.i_uip_pathC[current_decision_level - 1] = oldPCcounter;
                            break;
                        } // end 'pull in new level'
                    }     // end 'handling unseen variable'
                }         // end resolving clause c
            }             // end 'should_resolve'

            // perform resolution
            if (should_res) {
                TRACE(std::cout << "c UIP resolve with reason for literal " << p << " which is " << c << std::endl;);
                for (int j = 1; j < c.size(); j++) {
                    Lit q = c[j];
                    int q_level = level(var(q));
                    if (seen2[var(q)] != counter && q_level > 0) {
                        seen2[var(q)] = counter;
                        uipMini.new_introduced.push(q);
                        uipMini.to_be_cleaned.push(q);
                        if (q_level >= lowest_level) {
                            uipMini.i_uip_pathC[q_level - 1]++;
                        } else {
                            TRACE(std::cout << "c UIP add next literal " << q << " to new learnt clause via resolution"
                                            << std::endl;);
                            uipMini.new_out_learnt.push(q);
                            num_pushed++;
                        }
                    }
                }
            }
        } // end iterating over variables of current decision level
        if (uipMini.i_uip_pathC[current_decision_level - 1] > 0) {
            while (seen2[var(trail[index])] != counter || level(var(trail[index])) != current_decision_level) {
                index--;
            }
            p = trail[index];
            index--; // make sure we do not re-process this index
            assert(level(var(p)) == current_decision_level &&
                   "there has to be at least 1 more literal of this level on the clause, which we removed before");
            uipMini.new_out_learnt.push(~p);
            TRACE(std::cout << "c UIP add literal " << ~p << " to the minimized clause (now: " << uipMini.new_out_learnt
                            << std::endl;);
            seen2[var(p)] = counter;
            uipMini.to_be_cleaned.push(p);
        }

        // update to next decision level
        top_index++;
        if (top_index == uipMini.top_i.size()) break;
        current_decision_level = uipMini.top_i[top_index];
    }

    int i, j;
    uipMini.new_out_learnt.copyTo(uipMini.new_analyze_toclear);
    if (ccmin_mode == 2) {
        uint32_t abstract_level = 0;
        for (i = 1; i < uipMini.new_out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(uipMini.new_out_learnt[i])); // (maintain an abstraction of levels involved in conflict)
        for (i = j = 1; i < uipMini.new_out_learnt.size(); i++)
            if (reason(var(uipMini.new_out_learnt[i])) == CRef_Undef ||
                !litRedundant3(uipMini.new_out_learnt[i], abstract_level, uipMini.new_analyze_toclear))
                uipMini.new_out_learnt[j++] = uipMini.new_out_learnt[i];

    } else if (ccmin_mode == 1) { // FIXME: refactor-merge with default minimization code!
        for (i = j = 1; i < uipMini.new_out_learnt.size(); i++) {
            Var x = var(uipMini.new_out_learnt[i]);

            if (reason(x) == CRef_Undef)
                uipMini.new_out_learnt[j++] = uipMini.new_out_learnt[i];
            else {
                Clause &c = ca[reason(var(uipMini.new_out_learnt[i]))];
                for (int k = c.size() == 2 ? 0 : 1; k < c.size(); k++)
                    if (!seen[var(c[k])] && seen2[var(c[k])] != counter && level(var(c[k])) > 0) {
                        uipMini.new_out_learnt[j++] = uipMini.new_out_learnt[i];
                        break;
                    }
            }
        }
    } else
        i = j = uipMini.new_out_learnt.size();

    TRACE(std::cout << "c UIP minimized new clause by removing " << i - j << " literals, resulting in "
                    << uipMini.new_out_learnt << std::endl;);
    uipMini.new_out_learnt.shrink(i - j);

    // clean up, revert back to initial state for seen if we don't intend to change activity
    counter++;

    if (out_learnt.size() > uipMini.new_out_learnt.size()) {
        uipMini.i_uip_decisions++;
        uipMini.i_uip_global_dropped_lits += out_learnt.size() - uipMini.new_out_learnt.size();
        uipMini.new_out_learnt.swap(out_learnt);
    }
    uipMini.new_out_learnt.clear();
}

/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|
|  Description:
|    Analyze conflict and produce a reason clause.
|
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
|        rest of literals. There may be others from the same level though.
|
|________________________________________________________________________________________________@*/
void Solver::analyze(CRef confl, vec<Lit> &out_learnt, int &out_btlevel, int &out_lbd, int learnAllUIPsLevel, vec<Lit> &allUIPlearnts, vec<OTFSS> &otfssInfo)
{
    int pathC = 0;
    Lit p = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push(); // (leave room for the asserting literal)
    int index = trail.size() - 1;
    int nDecisionLevel = level(var(ca[confl][0]));
    int varsToBump = max_vars_to_bump_per_analysis;
    int backjumpLevel = 0; // level where we would jump to when learning the current clause
    assert(nDecisionLevel == level(var(ca[confl][0])));
    add_tmp.clear();
    do {
        assert(confl != CRef_Undef); // (otherwise should be UIP)
        Clause &c = ca[confl];
        statistics.solveSteps++;
        TRACE(std::cout << "c analyze: resolve with " << c << std::endl;)

        // For binary clauses, we don't rearrange literals in propagate(), so check and make sure the first is an implied lit.
        if (p != lit_Undef && c.size() == 2 && value(c[0]) == l_False) {
            assert(value(c[1]) == l_True);
            Lit tmp = c[0];
            c[0] = c[1], c[1] = tmp;
        }

        // Update LBD if improved.
        if (c.learnt()) {
            if (c.mark() == CORE) {
                c.touched() = conflicts;
            } else {
                int lbd = computeLBD(c);
                if (lbd < c.lbd()) {
                    if (c.lbd() <= 30) c.removable(false); // Protect once from reduction.
                    c.set_lbd(lbd);
                    if (lbd <= core_lbd_cut) {
                        learnts_core.push(confl);
                        c.mark(CORE);
                    } else if (lbd <= tier2_lbd_cut && c.mark() == LOCAL) {
                        // Bug: 'cr' may already be in 'learnts_tier2', e.g., if 'cr' was demoted from TIER2
                        // to LOCAL previously and if that 'cr' is not cleaned from 'learnts_tier2' yet.
                        learnts_tier2.push(confl);
                        c.mark(TIER2);
                    }
                }

                if (c.mark() == TIER2)
                    c.touched() = conflicts;
                else if (c.mark() == LOCAL) {
                    if (--varsToBump > 0) claBumpActivity(c);
                }
            }
        }

        bool isOtfss = true;
        int matchingLits = 0;
        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++) {
            Lit q = c[j];
            const int varLevel = level(var(q));
            if (varLevel > 0) { // only work on reduct
                if (!seen[var(q)]) {
                    isOtfss = false;
                    if (usesVSIDS()) {
                        if (--varsToBump > 0) varBumpActivity(var(q), .5);
                        add_tmp.push(q);
                    } else
                        conflicted[var(q)]++;
                    seen[var(q)] = 1;
                    if (varLevel >= nDecisionLevel) {
                        pathC++;
                    } else {
                        out_learnt.push(q);
                        backjumpLevel = backjumpLevel > varLevel ? backjumpLevel : varLevel;
                    }
                } else {
                    matchingLits++;
                }
            }
        }
        // exactly all literals of C need to be level(0) or in resolvent - except the resolving literal
        // conditions: 1: C (except level 0) has to have same size as resolvent (except level 0) BEFORE resolution: out_learnt.size() + pathC + 1
        //             2: all literals of C need to be present in resolvent, except first literal
        //             3: all literals of resolvent need to be present in C, and ~p
        // |C| == matchingLits + 1; |D| = out_learnt.size() - 1 + pathC
        TRACE(std::cout << "c finish resolution with pathC: " << pathC << " out_learnt: " << out_learnt << std::endl;)
        if (isOtfss && matchingLits + 1 == out_learnt.size() + pathC) {
            TRACE(std::cout << "c OTFSS add candidate (pathC: " << pathC << " out_learnt: " << out_learnt
                            << ") when resolving " << c << " with literal " << p << std::endl;)
            otfssInfo.push(OTFSS(confl, backjumpLevel, p));
        }

        // Select next clause to look at:
        do {
            while (!seen[var(trail[index--])])
                ;
            p = trail[index + 1];
        } while (level(var(p)) < nDecisionLevel);

        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;

    } while (pathC > 0);
    out_learnt[0] = ~p;

    assert(backjumpLevel < nDecisionLevel && "The learned clause should not stay on the current decision level");
    // only check for more UIP learned clauses, if we did not reach the decision literal yet!
    if (nDecisionLevel <= learnAllUIPsLevel && reason(var(p)) != CRef_Undef) {
        // backup 1st UIP clause, to continue with it later
        analyze_stack.clear();
        const Lit learningP = p;
        const CRef preConfl = confl;
        out_learnt.copyTo(analyze_stack);
        while (reason(var(p)) != CRef_Undef) {
            bool multipleLits = false;
            do {
                assert(confl != CRef_Undef); // (otherwise should be UIP)
                Clause &d = ca[confl];
                statistics.solveSteps++;
                // For binary clauses, we don't rearrange literals in propagate(), so check and make sure the first is an implied lit.
                if (p != lit_Undef && d.size() == 2 && value(d[0]) == l_False) d.swapLits(0, 1);
                const Clause &c = ca[confl];
                for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++) {
                    const Lit q = c[j];
                    if (!seen[var(q)] && level(var(q)) > 0) {
                        seen[var(q)] = 1;
                        if (level(var(q)) >= nDecisionLevel) {
                            pathC++;
                            multipleLits = true;
                        } else
                            out_learnt.push(q);
                    }
                }
                // Select next clause to look at:
                do {
                    while (!seen[var(trail[index--])]) {
                    }
                    p = trail[index + 1];
                } while (level(var(p)) < nDecisionLevel);
                confl = reason(var(p));
                seen[var(p)] = 0;
                pathC--;
            } while (pathC > 0);
            out_learnt[0] = ~p;
            TRACE(std::cout << "c analysis: found another UIP clause " << out_learnt << std::endl;);
            // only store interesting allUIPlearnt clauses
            if (multipleLits) {
                for (int j = 0; j < out_learnt.size(); j++) allUIPlearnts.push(out_learnt[j]);
                allUIPlearnts.push(lit_Undef);
            }
            multipleLits = false;
        }
        // cleanup and switch back
        for (int j = 0; j < out_learnt.size(); j++) seen[var(out_learnt[j])] = 0;
        p = learningP;
        confl = preConfl;
        analyze_stack.swap(out_learnt);
        for (int j = 0; j < out_learnt.size(); j++) seen[var(out_learnt[j])] = 1;
        analyze_stack.clear();
    }

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    if (ccmin_mode >= 2 && techniqueStartConflicts.allowMinimize()) {
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];

    } else if (ccmin_mode == 1 && techniqueStartConflicts.allowMinimize()) {
        for (i = j = 1; i < out_learnt.size(); i++) {
            Var x = var(out_learnt[i]);

            if (reason(x) == CRef_Undef)
                out_learnt[j++] = out_learnt[i];
            else {
                Clause &c = ca[reason(var(out_learnt[i]))];

                statistics.solveSteps++;
                for (int k = c.size() == 2 ? 0 : 1; k < c.size(); k++)
                    if (!seen[var(c[k])] && level(var(c[k])) > 0) {
                        out_learnt[j++] = out_learnt[i];
                        break;
                    }
            }
        }
    } else
        i = j = out_learnt.size();

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();

    out_lbd = computeLBD(out_learnt);
    if (ccmin_mode >= 3 && out_lbd <= tier2_lbd_cut && techniqueStartConflicts.allowLitRedundant() && out_learnt.size() <= 30) // Try further minimization?
        if (binResMinimize(out_learnt)) out_lbd = computeLBD(out_learnt); // Recompute LBD if minimized.

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else {
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i]))) max_i = i;
        // Swap-in this literal at index 1:
        Lit p = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1] = p;
        out_btlevel = level(var(p));
    }

    if (usesVSIDS()) {
        for (int i = 0; i < add_tmp.size(); i++) {
            Var v = var(add_tmp[i]);
            if (level(v) >= out_btlevel - 1) {
                if (--varsToBump > 0) varBumpActivity(v, 1);
            }
        }
        add_tmp.clear();
    } else {
        seen[var(p)] = true;
        for (int i = out_learnt.size() - 1; i >= 0; i--) {
            Var v = var(out_learnt[i]);
            CRef rea = reason(v);
            if (rea != CRef_Undef) {
                const Clause &reaC = ca[rea];
                for (int i = 0; i < reaC.size(); i++) {
                    Lit l = reaC[i];
                    if (!seen[var(l)]) {
                        seen[var(l)] = true;
                        conflicted[var(l)]++;
                        analyze_toclear.push(l);
                    }
                }
            }
        }
    }

    if (uipMini.i_uip) {
        i_uip_analyze(out_learnt, decisionLevel(), analyze_toclear, out_lbd, out_btlevel);
        if (out_learnt.size() == 1)
            out_btlevel = 0;
        else {
            int max_i = 1;
            // Find the first literal assigned at the next-highest level:
            for (int i = 2; i < out_learnt.size(); i++)
                if (level(var(out_learnt[i])) > level(var(out_learnt[max_i]))) max_i = i;
            // Swap-in this literal at index 1:
            Lit p = out_learnt[max_i];
            out_learnt[max_i] = out_learnt[1];
            out_learnt[1] = p;
            out_btlevel = level(var(p));
        }
    }
    for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0; // ('seen[]' is now cleared)
}


// Try further learnt clause minimization by means of binary clause resolution.
bool Solver::binResMinimize(vec<Lit> &out_learnt)
{
    // Preparation: remember which false variables we have in 'out_learnt'.
    counter++;
    for (int i = 1; i < out_learnt.size(); i++) seen2[var(out_learnt[i])] = counter;

    // Get the list of binary clauses containing 'out_learnt[0]'.
    const vec<Watcher> &ws = watches_bin[~out_learnt[0]];
    statistics.solveSteps++;

    int to_remove = 0;
    for (int i = 0; i < ws.size(); i++) {
        Lit the_other = ws[i].blocker;
        // Does 'the_other' appear negatively in 'out_learnt'?
        if (seen2[var(the_other)] == counter && value(the_other) == l_True) {
            to_remove++;
            seen2[var(the_other)] = counter - 1; // Remember to remove this variable.
        }
    }

    // Shrink.
    if (to_remove > 0) {
        int last = out_learnt.size() - 1;
        for (int i = 1; i < out_learnt.size() - to_remove; i++)
            if (seen2[var(out_learnt[i])] != counter) out_learnt[i--] = out_learnt[last--];
        out_learnt.shrink(to_remove);
    }
    return to_remove != 0;
}


// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
    analyze_stack.clear();
    analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0) {
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause &c = ca[reason(var(analyze_stack.last()))];
        analyze_stack.pop();

        // Special handling for binary clauses like in 'analyze()'.
        if (c.size() == 2 && value(c[0]) == l_False) {
            assert(value(c[1]) == l_True);
            Lit tmp = c[0];
            c[0] = c[1], c[1] = tmp;
        }

        for (int i = 1; i < c.size(); i++) {
            Lit p = c[i];
            if (!seen[var(p)] && level(var(p)) > 0) {
                if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0) {
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                    statistics.solveSteps++;
                } else {
                    for (int j = top; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant3(Lit p, uint32_t abstract_levels, vec<Lit> &new_analyze_toclear)
{
    analyze_stack.clear();
    analyze_stack.push(p);
    int top = new_analyze_toclear.size();
    while (analyze_stack.size() > 0) {
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause &c = ca[reason(var(analyze_stack.last()))];
        analyze_stack.pop();

        // Special handling for binary clauses like in 'analyze()'.
        if (c.size() == 2 && value(c[0]) == l_False) {
            assert(value(c[1]) == l_True);
            Lit tmp = c[0];
            c[0] = c[1], c[1] = tmp;
        }

        for (int i = 1; i < c.size(); i++) {
            Lit p = c[i];
            if (seen2[var(p)] != counter && level(var(p)) > 0) {
                if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0) {
                    seen2[var(p)] = counter;
                    analyze_stack.push(p);
                    new_analyze_toclear.push(p);
                } else {
                    for (int j = top; j < new_analyze_toclear.size(); j++) seen2[var(new_analyze_toclear[j])] = 0;
                    new_analyze_toclear.shrink(new_analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit> &out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0) return;

    seen[var(p)] = 1;

    for (int i = trail.size() - 1; i >= trail_lim[0]; i--) {
        Var x = var(trail[i]);
        if (seen[x]) {
            if (reason(x) == CRef_Undef) {
                // assert(level(x) > 0); // chronological backtracking can make that happen
                if (level(x) > 0) out_conflict.push(~trail[i]);
            } else {
                Clause &c = ca[reason(x)];
                for (int j = c.size() == 2 ? 0 : 1; j < c.size(); j++)
                    if (level(var(c[j])) > 0) seen[var(c[j])] = 1;
                statistics.solveSteps++;
            }
            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (cr : CRef)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions, or
|    decisions. Calculates the (possibly empty) set of assumptions that led to the assignment
|    of 'cr', and stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(const CRef cr, vec<Lit> &out_conflict)
{
    const Clause &c = ca[cr];
    analyzeFinal(c, out_conflict);
}

CRef Solver::createDecisionClause(Lit implied)
{
    add_tmp.clear();

    // clause should represent {decisions} -> implied
    if (implied != lit_Undef) add_tmp.push(implied);

    // collect all negated decisions
    for (int i = trail_lim.size() - 1; i >= 0; i--) {
        Lit l = trail[trail_lim[i]];
        add_tmp.push(~l);
    }

    // we cannot create a clause from no decisions
    if (add_tmp.size() == 0) return CRef_Undef;
    CRef cr = ca.alloc(add_tmp, true);
    ca[cr].set_lbd(trail_lim.size());
    return cr;
}


void Solver::uncheckedEnqueue(Lit p, int level, CRef from)
{
    assert(value(p) == l_Undef);
    assert(level <= decisionLevel() && "do not enqueue literals on non-existing levels");
    assert((from == CRef_Undef || from < ca.size()) && "do not use reasons that are not located in the allocator");
    Var x = var(p);
    if (!usesVSIDS()) {
        picked[x] = conflicts;
        conflicted[x] = 0;
#ifdef ANTI_EXPLORATION
        uint32_t age = conflicts - canceled[var(p)];
        if (age > 0) {
            double decay = bumpPow.pow(age);
            activity_CHB[var(p)] *= decay;
            if (usesCHB() && order_heap->inHeap(var(p))) order_heap->increase(var(p));
        }
#endif
    }

    assigns[x] = lbool(!sign(p));
    vardata[x] = mkVarData(from, level);
#ifndef NO_PREFETCHING
    __builtin_prefetch(&watches[p], 1, 0); // prefetch the watch, prepare for a write (1), the data is highly temoral (0)
#endif
    trail.push_(p);
}


/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
CRef Solver::propagate()
{
    CRef confl = CRef_Undef;
    int num_props = 0;
    lazySATwatch.clear();
    Lit old_trail_top = lit_Undef;
    CRef old_reason = CRef_Undef;
    watches.cleanAll();
    watches_bin.cleanAll();

    while (qhead < trail.size()) {
        Lit p = trail[qhead++]; // 'p' is enqueued fact to propagate.
        int currLevel = level(var(p));
        vec<Watcher> &ws = watches[p];
        Watcher *i, *j, *end;
        num_props++;

        /* begin old trail reconstruction */
        if (use_backuped_trail) {
            if (old_trail_qhead < old_trail.size()) {
                old_trail_top = old_trail[old_trail_qhead];
                old_reason = oldreasons[var(old_trail_top)];
            }
            if (old_trail_top != lit_Undef) {
                if (p == old_trail_top) {
                    while (old_trail_qhead < old_trail.size() - 1) {
                        old_trail_qhead++;
                        old_trail_top = old_trail[old_trail_qhead];
                        old_reason = oldreasons[var(old_trail_top)];
                        if (old_reason == CRef_Undef) {
                            break;
                        } else if (value(old_trail_top) == l_False) {
                            const Clause &new_conflict = ca[old_reason];
                            if (new_conflict.size() != 2 && new_conflict[0] != old_trail_top) {
                                /* We will hit a conflict here, the clause is just not structured correctly yet,
                                nor watched correctly. */
                                reset_old_trail();
                                break;
                            }
                            confl = old_reason;
                            used_backup_lits++;
                            TRACE(std::cout
                                  << "c prop: hit conflict during trail restoring, when trying to propagate literal "
                                  << old_trail_top << " with reason[" << old_reason << "] " << new_conflict << std::endl;);
                            assert((new_conflict.size() == 2 || new_conflict[0] == old_trail_top) &&
                                   "asserting literal is at position 1");
                            /* No need to touch watch lists, we will backtrack this level anyways! */
                            goto propagation_out;
                        } else if (value(old_trail_top) == l_Undef) {
                            Clause &new_conflict = ca[old_reason];
                            if (new_conflict.size() != 2 && new_conflict[0] != old_trail_top) {
                                /* We will use this unit clause, the clause is just not structured correctly yet */
                                reset_old_trail();
                                break;
                            }
                            used_backup_lits++;
                            TRACE(std::cout << "c prop: enqueue literal " << old_trail_top << " with reason["
                                            << old_reason << "] " << ca[old_reason] << std::endl;);
                            assert((ca[old_reason].size() == 2 || ca[old_reason][0] == old_trail_top) &&
                                   "asserting literal is at position 1");
                            uncheckedEnqueue(old_trail_top, decisionLevel(), old_reason);
                        }
                    }
                } else if (var(p) == var(old_trail_top) || value(old_trail_top) == l_False) {
                    reset_old_trail();
                }
            }
        }
        /* end old trail reconstruction */

        vec<Watcher> &ws_bin = watches_bin[p]; // Propagate binary clauses first.
        for (int k = 0; k < ws_bin.size(); k++) {
            Lit the_other = ws_bin[k].blocker;
            if (value(the_other) == l_False) {
                confl = ws_bin[k].cref;
                goto propagation_out;
            } else if (value(the_other) == l_Undef) {
                uncheckedEnqueue(the_other, currLevel, ws_bin[k].cref);
            }
        }

        for (i = j = (Watcher *)ws, end = i + ws.size(); i != end;) {
            // Try to avoid inspecting the clause:
            const Lit blocker = i->blocker;
            if (value(blocker) == l_True) {
                *j++ = *i++;
                continue;
            }

            // Make sure the false literal is data[1]:
            const CRef cr = i->cref;
            Clause &c = ca[cr];
            statistics.solveSteps++;
            const Lit false_lit = ~p;
            if (c[0] == false_lit) c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            i++;

            // If 0th watch is true, then clause is already satisfied.
            const Lit first = c[0];
            const Watcher w = Watcher(cr, first);
            if (first != blocker && value(first) == l_True) {
                *j++ = w;
                continue;
            }

            // Look for new watch:
            int watchPos = 0, avoidLevel = assumptions.size();
            for (int k = 2; k < c.size(); k++) {
                if (value(c[k]) != l_False) {
                    watchPos = k; /* memorize that we found one literal we can watch */
                    if (level(var(c[k])) > avoidLevel) break;
                }
            }

            /* found a position to watch, watch the clause */
            if (watchPos != 0) {
                c[1] = c[watchPos];
                c[watchPos] = false_lit;
                if (value(c[1]) == l_True) {
                    lazySATwatch.push(watchItem(w, ~c[1]));
                } else {
                    watches[~c[1]].push(w);
                }
                goto NextClause;
            }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if (value(first) == l_False) {
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end) *j++ = *i++;
            } else {
                if (currLevel == decisionLevel()) {
                    uncheckedEnqueue(first, currLevel, cr);
                } else {
                    int nMaxLevel = currLevel;
                    int nMaxInd = 1;
                    // pass over all the literals in the clause and find the one with the biggest level
                    for (int nInd = 2; nInd < c.size(); ++nInd) {
                        int nLevel = level(var(c[nInd]));
                        if (nLevel > nMaxLevel) {
                            nMaxLevel = nLevel;
                            nMaxInd = nInd;
                        }
                    }

                    if (nMaxInd != 1) {
                        std::swap(c[1], c[nMaxInd]);
                        j--; // undo last watch
                        if (value(c[1]) == l_True) {
                            lazySATwatch.push(watchItem(w, ~c[1]));
                        } else {
                            watches[~c[1]].push(w);
                        }
                    }

                    uncheckedEnqueue(first, nMaxLevel, cr);
                }
            }

        NextClause:;
        }
        ws.shrink(i - j);
    }

propagation_out:;
    /* we still need to re-add the satisfied clauses to their respective watch lists - in order! */
    for (int k = 0; k < lazySATwatch.size(); ++k) {
        watches[lazySATwatch[k].l].push(lazySATwatch[k].w);
    }

    propagations += num_props;
    simpDB_props -= num_props;

    return confl;
}


/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt {
    ClauseAllocator &ca;
    reduceDB_lt(ClauseAllocator &ca_) : ca(ca_) {}
    bool operator()(CRef x, CRef y) const { return ca[x].activity() < ca[y].activity(); }
};

struct reduceDB_tch {
    ClauseAllocator &ca;
    reduceDB_tch(ClauseAllocator &ca_) : ca(ca_) {}
    bool operator()(CRef x, CRef y) const { return ca[x].touched() < ca[y].touched(); }
};

struct reduceDB_c {
    ClauseAllocator &ca;
    reduceDB_c(ClauseAllocator &ca_) : ca(ca_) {}
    bool operator()(CRef x, CRef y) const
    {
        return ((ca[x].lbd() != ca[y].lbd()) && (ca[x].lbd() > ca[y].lbd())) ||
               ((ca[x].lbd() == ca[y].lbd()) && (ca[x].size() > ca[y].size()));
    }
};

bool Solver::reduceDB_Core()
{
    if (verbosity > 0) printf("c Core size before reduce: %i\n", learnts_core.size());
    int i, j;
    bool ret = false;
    sort(learnts_core, reduceDB_c(ca));
    int limit = learnts_core.size() / 2;

    for (i = j = 0; i < learnts_core.size(); i++) {
        Clause &c = ca[learnts_core[i]];
        if (c.mark() == CORE) {
            if (c.lbd() > 2 && !locked(c) && (c.touched() + 100000 < conflicts) && i < limit) {
                learnts_tier2.push(learnts_core[i]);
                c.mark(TIER2);
                // c.removable(true);
                // c.activity() = 0;
                c.touched() = conflicts;
                // claBumpActivity(c);
            } else {
                learnts_core[j++] = learnts_core[i];
                if (locked(c) || (c.touched() + 50000 < conflicts) || c.lbd() <= 2) {
                    limit++;
                }
            }
        }
    }
    ret = j < learnts_core.size() * 0.95;
    learnts_core.shrink(i - j);
    if (verbosity > 0)
        printf("c Core size after reduce: %i, dropped more than 5 percent: %d\n", learnts_core.size(), ret);

    return ret;
}

void Solver::reduceDB()
{
    int i, j;
    TRACE(std::cout << "c run reduceDB on level " << decisionLevel() << std::endl);
    // if (local_learnts_dirty) cleanLearnts(learnts_local, LOCAL);
    // local_learnts_dirty = false;
    reset_old_trail();

    sort(learnts_local, reduceDB_lt(ca));

    int limit = learnts_local.size() / 2;
    for (i = j = 0; i < learnts_local.size(); i++) {
        Clause &c = ca[learnts_local[i]];
        if (c.mark() == LOCAL) {
            if (c.removable() && !locked(c) && i < limit) {
                removeClause(learnts_local[i]);
            } else {
                if (!c.removable()) limit++;
                c.removable(true);
                learnts_local[j++] = learnts_local[i];
            }
        }
    }
    statistics.solveSteps += learnts_local.size();
    learnts_local.shrink(i - j);
    checkGarbage();
    TRACE(std::cout << "c done running reduceDB on level " << decisionLevel() << std::endl);
}

void Solver::reduceDB_Tier2()
{
    TRACE(std::cout << "c run reduceDB_tier2 on level " << decisionLevel() << std::endl);
    reset_old_trail();
    int i, j;
    sort(learnts_tier2, reduceDB_tch(ca));
    int limit = learnts_tier2.size() / 2;

    for (i = j = 0; i < learnts_tier2.size(); i++) {
        Clause &c = ca[learnts_tier2[i]];
        if (c.mark() == TIER2) {
            if (!locked(c) && i < limit) {
                learnts_local.push(learnts_tier2[i]);
                c.mark(LOCAL);
                // c.removable(true);
                c.activity() = 0;
                c.touched() = conflicts;
                claBumpActivity(c);
            } else {
                learnts_tier2[j++] = learnts_tier2[i];
                if (locked(c)) {
                    limit++;
                }
            }
        }
    }
    learnts_tier2.shrink(i - j);
    statistics.solveSteps += learnts_tier2.size();
    TRACE(std::cout << "c done running reduceDB_tier2 on level " << decisionLevel() << std::endl);
}


int Solver::getRestartLevel()
{
    // stay on the current level?
    if (restart.selection_type == 3) return decisionLevel();
    if (restart.selection_type == 4) return decisionLevel() == 0 ? 0 : rand() % decisionLevel();

    if (restart.selection_type >= 1) {

        bool repeatReusedTrail = false;
        Var next = var_Undef;
        int restartLevel = 0;

        const vec<double> &restart_activity = usesVSIDS() ? activity_VSIDS : (usesCHB() ? activity_CHB : activity_distance);

        do {
            repeatReusedTrail = false; // get it right this time?

            // Activity based selection
            while (next == var_Undef || value(next) != l_Undef ||
                   !decision[next]) // found a yet unassigned variable with the highest activity among the unassigned variables
                if (order_heap->empty()) {
                    // we cannot compare to any other variable, hence, we have SAT already
                    return 0;
                } else {
                    next = order_heap->removeMin(); // get next element
                }

            // based on variable next, either check for reusedTrail, or matching Trail!
            // activity of the next decision literal
            restartLevel = 0;
            for (int i = 0; i < decisionLevel(); ++i) {
                if (restart_activity[var(trail[trail_lim[i]])] < restart_activity[next]) {
                    restartLevel = i;
                    break;
                }
            }
            // put the decision literal back, so that it can be used for the next decision
            order_heap->insert(next);

            // reused trail
            if (restart.selection_type > 1 && restartLevel > 0) { // check whether jumping higher would be "more correct"
                cancelUntil(restartLevel);
                Var more = var_Undef;
                while (more == var_Undef || value(more) != l_Undef || !decision[more])
                    if (order_heap->empty()) {
                        more = var_Undef;
                        break;
                    } else {
                        more = order_heap->removeMin();
                    }

                // actually, would have to jump higher than the current level!
                if (more != var_Undef && restart_activity[more] > var(trail[trail_lim[restartLevel - 1]])) {
                    repeatReusedTrail = true;
                    next = more; // no need to insert, and get back afterwards again!
                } else {
                    order_heap->insert(more);
                }
            }
        } while (repeatReusedTrail);

        // stats
        if (restartLevel > 0) { // if a partial restart is done
            restart.savedDecisions += restartLevel;
            const int thisPropSize = restartLevel == decisionLevel() ? trail.size() : trail_lim[restartLevel];
            restart.savedPropagations += (thisPropSize - trail_lim[0]); // number of literals that do not need to be propagated
            restart.partialRestarts++;
        }

        // return restart level
        return restartLevel;
    }
    return 0;
}

void Solver::removeSatisfied(vec<CRef> &cs)
{
    int i, j;
    for (i = j = 0; i < cs.size(); i++) {
        Clause &c = ca[cs[i]];
        if (c.mark() == 1) continue;
        if (satisfied(c))
            removeSatisfiedClause(cs[i]);
        else
            cs[j++] = cs[i];
    }
    statistics.solveSteps += cs.size();
    cs.shrink(i - j);
}

void Solver::safeRemoveSatisfied(vec<CRef> &cs, unsigned valid_mark)
{
    int i, j;
    for (i = j = 0; i < cs.size(); i++) {
        Clause &c = ca[cs[i]];
        if (c.mark() == valid_mark) {
            if (satisfied(c)) {
                removeSatisfiedClause(cs[i]);
            } else {
                cs[j++] = cs[i];
            }
        }
    }
    cs.shrink(i - j);
}

void Solver::rebuildOrderHeap()
{
    /* all unassigned variables present, no need to rebuild */
    if (decisionLevel() == 0 && (order_heap->size() + trail.size() >= nVars())) {
        TRACE(for (Var v = 0; v < nVars(); v++) {
            assert((!decision[v] || value(v) != l_Undef || order_heap->inHeap(v)) &&
                   "unassigned variables have to be present in the heap");
        });
        return;
    }

    decision_rebuild_vars.clear();
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef) decision_rebuild_vars.push(v);

    order_heap->build(decision_rebuild_vars);
    full_heap_size = order_heap->size();
}


/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
    assert(decisionLevel() == 0);

    reset_old_trail();

    if (!ok || propagate() != CRef_Undef) return ok = false;

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0)) return true;
    TRACE(std::cout << "c run simplify on level " << decisionLevel() << std::endl);

    // Remove satisfied clauses:
    removeSatisfied(learnts_core); // Should clean core first.
    safeRemoveSatisfied(learnts_tier2, TIER2);
    safeRemoveSatisfied(learnts_local, LOCAL);
    if (remove_satisfied) // Can be turned off.
        removeSatisfied(clauses);
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props = clauses_literals + learnts_literals; // (shouldn't depend on stats really, but it will do for now)

    TRACE(std::cout << "c finished simplify on level " << decisionLevel() << std::endl);
    return true;
}

// pathCs[k] is the number of variables assigned at level k,
// it is initialized to 0 at the begining and reset to 0 after the function execution
bool Solver::collectFirstUIP(CRef confl)
{
    involved_lits.clear();
    int max_level = 1;
    Clause &c = ca[confl];
    int minLevel = decisionLevel();
    for (int i = 0; i < c.size(); i++) {
        Var v = var(c[i]);
        //        assert(!seen[v]);
        if (level(v) > 0) {
            seen[v] = 1;
            var_iLevel_tmp[v] = 1;
            pathCs[level(v)]++;
            if (minLevel > level(v)) {
                minLevel = level(v);
                assert(minLevel > 0);
            }
            //    varBumpActivity(v);
        }
    }

    int limit = trail_lim[minLevel - 1];
    for (int i = trail.size() - 1; i >= limit; i--) {
        Lit p = trail[i];
        Var v = var(p);
        if (seen[v]) {
            int currentDecLevel = level(v);
            //      if (currentDecLevel==decisionLevel())
            //      	varBumpActivity(v);
            seen[v] = 0;
            if (--pathCs[currentDecLevel] != 0) {
                int reasonVarLevel = var_iLevel_tmp[v] + 1;
                if (reasonVarLevel > max_level) max_level = reasonVarLevel;

                if (reason(v) != CRef_Undef) {
                    Clause &rc = ca[reason(v)];
                    if (rc.size() == 2 && value(rc[0]) == l_False) {
                        // Special case for binary clauses
                        // The first one has to be SAT
                        assert(value(rc[1]) != l_False);
                        Lit tmp = rc[0];
                        rc[0] = rc[1], rc[1] = tmp;
                    }
                    for (int j = 1; j < rc.size(); j++) {
                        Lit q = rc[j];
                        Var v1 = var(q);
                        if (level(v1) > 0) {
                            if (minLevel > level(v1)) {
                                minLevel = level(v1);
                                limit = trail_lim[minLevel - 1];
                                assert(minLevel > 0);
                            }
                            if (seen[v1]) {
                                if (var_iLevel_tmp[v1] < reasonVarLevel) var_iLevel_tmp[v1] = reasonVarLevel;
                            } else {
                                var_iLevel_tmp[v1] = reasonVarLevel;
                                //   varBumpActivity(v1);
                                seen[v1] = 1;
                                pathCs[level(v1)]++;
                            }
                        }
                    }
                }
            }
            involved_lits.push(p);
        }
    }

    /* TODO: check whether we can skip this once we do not use DISTANCE decision heuristic anymore */
    double inc = var_iLevel_inc;
    distance_level_incs.clear();
    for (int i = 0; i < max_level; i++) {
        distance_level_incs.push(inc);
        inc = inc / my_var_decay;
    }

    for (int i = 0; i < involved_lits.size(); i++) {
        Var v = var(involved_lits[i]);
        //        double old_act=activity_distance[v];
        //        activity_distance[v] +=var_iLevel_inc * var_iLevel_tmp[v];
        activity_distance[v] += var_iLevel_tmp[v] * distance_level_incs[var_iLevel_tmp[v] - 1];

        if (activity_distance[v] > 1e100) {
            for (int vv = 0; vv < nVars(); vv++) activity_distance[vv] *= 1e-100;
            var_iLevel_inc *= 1e-100;
            for (int j = 0; j < max_level; j++) distance_level_incs[j] *= 1e-100;
        }
        if (usesDISTANCE()) {
            if (order_heap->inHeap(v)) order_heap->decrease(v); /* TODO: increase? */
        }
    }
    var_iLevel_inc = distance_level_incs[distance_level_incs.size() - 1];
    return true;
}

struct UIPOrderByILevel_Lt {
    Solver &solver;
    const vec<double> &var_iLevel;
    bool operator()(Lit x, Lit y) const
    {
        return var_iLevel[var(x)] < var_iLevel[var(y)] ||
               (var_iLevel[var(x)] == var_iLevel[var(y)] && solver.level(var(x)) > solver.level(var(y)));
    }
    UIPOrderByILevel_Lt(const vec<double> &iLevel, Solver &para_solver) : solver(para_solver), var_iLevel(iLevel) {}
};

CRef Solver::propagateLits(vec<Lit> &lits)
{
    Lit lit;
    int i;

    for (i = lits.size() - 1; i >= 0; i--) {
        lit = lits[i];
        if (value(lit) == l_Undef) {
            newDecisionLevel();
            uncheckedEnqueue(lit, decisionLevel());
            CRef confl = propagate();
            if (confl != CRef_Undef) {
                return confl;
            }
        }
    }
    return CRef_Undef;
}

/// expose propagation (e.g. for Open-WBO)
bool Solver::propagateLit(Lit l, vec<Lit> &implied)
{
    cancelUntil(0);
    implied.clear();
    bool conflict = false;

    // literal is a unit clause
    if (value(l) != l_Undef) {
        return value(l) == l_False;
    }
    assert(value(l) == l_Undef);

    // propagate on a new decision level, to be able to roll back
    newDecisionLevel();
    uncheckedEnqueue(l, decisionLevel(), CRef_Undef);

    // collect trail literals
    int pre_size = trail.size();
    CRef cr = propagate();
    if (cr != CRef_Undef) conflict = true;
    for (int i = pre_size; i < trail.size(); i++) {
        implied.push(trail[i]);
    }
    cancelUntil(0);

    return conflict;
}

lbool Solver::prefetchAssumptions()
{
    if (prefetch_assumptions && decisionLevel() == 0 && assumptions.size() > 0) {
        while (decisionLevel() < assumptions.size() && decisionLevel() < last_used_assumptions) {
            // Perform user provided assumption:
            Lit p = assumptions[decisionLevel()];

            if (value(p) == l_False) {
                // TODO: write proper conflict handling
                cancelUntil(0);
                break;
            }

            newDecisionLevel();
            if (value(p) == l_Undef) uncheckedEnqueue(p, decisionLevel(), CRef_Undef);
        }

        assert((decisionLevel() == 0 || decisionLevel() == assumptions.size() || decisionLevel() == last_used_assumptions) &&
               "we propagated all assumptions by now");

        // TODO: write proper conflict handling
        CRef confl = propagate();
        if (confl != CRef_Undef) {
            cancelUntil(0);
        }
    }

    return l_Undef; // for now, we just work with the generic case
}

bool Solver::check_invariants()
{
    TRACE(printf("c check solver invariants\n");)

    bool pass = true;
    bool fatal_on_watch_removed = false;

    /* ensure that each assigned literal has a proper reason clause as well */
    for (int i = 0; i < trail.size(); ++i) {
        Var v = var(trail[i]);
        int l = level(v);
        if (!(l == 0 || reason(v) != CRef_Undef || trail_lim[l - 1] == i)) {
            std::cout << "c trail literal " << trail[i] << " at level " << l << " (pos: " << i
                      << " has no proper reason clause" << std::endl;
            pass = false;
        }
    }

    // check whether clause is in solver in the right watch lists
    for (int p = 0; p < 4; ++p) {

        const vec<CRef> &clause_list = (p == 0 ? clauses : (p == 1 ? learnts_core : (p == 2 ? learnts_tier2 : learnts_local)));
        for (int i = 0; i < clause_list.size(); ++i) {
            const CRef cr = clause_list[i];
            const Clause &c = ca[cr];
            if (c.mark() == 1) {
                continue;
            }

            if (c.size() == 1) {
                std::cout << "c there should not be unit clauses! [" << cr << "]" << c << std::endl;
                pass = false;
            } else {
                if (c.size() > 2) {
                    for (int j = 0; j < 2; ++j) {
                        const Lit l = ~c[j];
                        vec<Watcher> &ws = watches[l];
                        int didFind = 0;
                        for (int j = 0; j < ws.size(); ++j) {
                            CRef wcr = ws[j].cref;
                            if (wcr == cr) {
                                didFind++;
                                break;
                            }
                        }
                        if (didFind != 1) {
                            std::cout << "c could not find clause[" << cr << "] " << c << " in watcher for lit [" << j
                                      << "]" << l << " 1 time, but " << didFind << " times" << std::endl;
                            pass = false;
                        }
                    }
                } else {
                    for (int j = 0; j < 2; ++j) {
                        const Lit l = ~c[j];
                        vec<Watcher> &ws = watches_bin[l];
                        int didFind = 0;
                        for (int j = 0; j < ws.size(); ++j) {
                            CRef wcr = ws[j].cref;
                            if (wcr == cr) {
                                didFind++;
                                break;
                            }
                        }
                        if (didFind != 1) {
                            std::cout << "c could not find clause[" << cr << "] " << c << " in watcher for lit [" << j
                                      << "]" << l << " 1 time, but " << didFind << " times" << std::endl;
                            pass = false;
                        }
                    }
                }
            }
        }
    }

    for (Var v = 0; v < nVars(); ++v) {
        for (int p = 0; p < 2; ++p) {
            const Lit l = mkLit(v, p == 1);
            vec<Watcher> &ws = watches[l];
            for (int j = 0; j < ws.size(); ++j) {
                CRef wcr = ws[j].cref;
                const Clause &c = ca[wcr];

                for (int k = j + 1; k < ws.size(); ++k) {
                    CRef inner_cr = ws[k].cref;
                    if (inner_cr == wcr) {
                        std::cout << "c found clause [" << wcr << "] " << c
                                  << " multiple times in watch lists of literal " << l << std::endl;
                        if (fatal_on_watch_removed) pass = false;
                    }
                }

                if (c.mark() == 1) {
                    std::cout << "c found deleted clause [" << wcr << "]" << c << " in watch lists of literal " << l << std::endl;
                    if (fatal_on_watch_removed) pass = false;
                }
                if (c.size() <= 2) {
                    std::cout << "c found binary or smaller clause [" << wcr << "]" << c << " in watch list of literal "
                              << l << std::endl;
                    pass = false;
                }
                if (c[0] != ~l && c[1] != ~l) {
                    std::cout << "c wrong literals for clause [" << wcr << "] " << c
                              << " are watched. Found in list for " << l << std::endl;
                    pass = false;
                }
            }
            vec<Watcher> &ws_bin = watches_bin[l];
            for (int j = 0; j < ws_bin.size(); ++j) {
                CRef wcr = ws_bin[j].cref;
                const Clause &c = ca[wcr];

                for (int k = j + 1; k < ws_bin.size(); ++k) {
                    CRef inner_cr = ws_bin[k].cref;
                    if (inner_cr == wcr) {
                        std::cout << "c found clause [" << wcr << "] " << c
                                  << " multiple times in watch lists of literal " << l << std::endl;
                        if (fatal_on_watch_removed) pass = false;
                    }
                }

                if (c.mark() == 1) {
                    std::cout << "c found deleted clause [" << wcr << "]" << c << " in watch lists of literal " << l << std::endl;
                    if (fatal_on_watch_removed) pass = false;
                }
                if (c.size() != 2) {
                    std::cout << "c found non-binary clause [" << wcr << "]" << c << " in binary watch list of literal "
                              << l << std::endl;
                    pass = false;
                }
                if (c[0] != ~l && c[1] != ~l) {
                    std::cout << "c wrong literals for clause [" << wcr << "] " << c
                              << " are watched. Found in list for " << l << std::endl;
                    pass = false;
                }
            }
        }
        if (seen[v] != 0) {
            std::cout << "c seen for variable " << v << " is not 0, but " << (int)seen[v] << std::endl;
            pass = false;
        }
    }

    for (int i = 0; i < old_trail.size(); ++i) {
        const Lit old_trail_top = old_trail[old_trail_qhead];
        const CRef old_reason = oldreasons[var(old_trail_top)];
        if (old_reason == CRef_Undef) continue;
        const Clause &c = ca[old_reason];
        assert((c.size() == 2 || c[0] == old_trail_top) && "assert literal has to be at first position");
        if (!(c.size() == 2 || c[0] == old_trail_top)) pass = false;
    }


    assert(pass && "some solver invariant check failed");
    return pass;
}

void Solver::info_based_rephase()
{
    int var_nums = nVars();
    for (int i = 0; i < var_nums; ++i) polarity[i] = !ls_mediation_soln[i];
    if (!considersDISTANCE() && ccnr.conflict_ct.size() > 0 && ccnr._step > 0) {
        for (int i = 0; i < var_nums; ++i) {
            if (ccnr.conflict_ct[i + 1] > 0) {
                if (usesVSIDS()) {
                    varBumpActivity(i, ccnr.conflict_ct[i + 1] * 100 / ccnr._step);
                } else {
                    conflicted[i] += std::max((long long int)1, ccnr.conflict_ct[i + 1] * 100 / ccnr._step);
                }
            }
        }
    }
}


void Solver::rand_based_rephase()
{
    int var_nums = nVars();
    int pick_rand = rand() % 1000;

    // local search
    if ((pick_rand -= 100) < 0) {
        for (int i = 0; i < var_nums; ++i) polarity[i] = !ls_best_soln[i];
    } else if ((pick_rand -= 300) < 0) {
        for (int i = 0; i < var_nums; ++i) polarity[i] = !ls_mediation_soln[i];
        mediation_used = true;
    }
    // top_trail 200
    else if ((pick_rand -= 300) < 0) {
        for (int i = 0; i < var_nums; ++i) polarity[i] = !top_trail_soln[i];
    }
    // reverse
    else if ((pick_rand -= 50) < 0) {
        for (int i = 0; i < var_nums; ++i) polarity[i] = !polarity[i];
    } else if ((pick_rand -= 25) < 0) {
        for (int i = 0; i < var_nums; ++i) polarity[i] = ls_best_soln[i];
    } else if ((pick_rand -= 25) < 0) {
        for (int i = 0; i < var_nums; ++i) polarity[i] = top_trail_soln[i];
    }
    // 150
    else if ((pick_rand -= 140) < 0) {
        for (int i = 0; i < var_nums; ++i) polarity[i] = rand() % 2 == 0 ? 1 : 0;
    } else if ((pick_rand -= 5) < 0) {
        for (int i = 0; i < var_nums; ++i) polarity[i] = 1;
    } else if ((pick_rand -= 5) < 0) {
        for (int i = 0; i < var_nums; ++i) polarity[i] = 0;
    }
    // 50
    else {
        // do nothing
    }
}

/// Share all unit clauses on trail after propagation
void Solver::shareUnitClauses()
{
    if (decisionLevel() != 0) return;

    add_tmp.clear();
    add_tmp.push(lit_Undef);
    while (shared_unit_clauses < trail.size()) {
        assert(level(var(trail[shared_unit_clauses])) == 0 && "shared unit clauses are level 0");
        add_tmp[0] = trail[shared_unit_clauses];
        shareViaCallback(add_tmp, 1);
        ++shared_unit_clauses;
    }
    add_tmp.clear();
}

/// Store minimum of assumptions and current level, to forward assumptions again
void Solver::memorize_used_assumptions()
{
    last_used_assumptions = assumptions.size() > decisionLevel() ? decisionLevel() : assumptions.size();
}

/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|
|  Description:
|    Search for a model the specified number of conflicts.
|
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int &nof_conflicts)
{
    TRACE(std::cout << "c start search at level " << decisionLevel() << std::endl);
    assert(ok);
    int backtrack_level;
    int lbd;
    learnt_clause.clear();
    starts++;

    // make sure that all unassigned variables are in the heap
    assert(trail.size() + order_heap->size() >= full_heap_size);

    freeze_ls_restart_num--;
    bool can_call_ls = true;

    // get clauses from parallel solving, if we want to receive
    if (consumeSharedCls != NULL && receiveClauses && decisionLevel() == 0) consumeSharedCls(issuer);

    if (allow_rephasing && solve_starts + starts > state_change_time) {

        if (!called_initial_sls) call_ls(false);

        /* grow limit after each rephasing */
        state_change_time = state_change_time + state_change_time_inc;
        state_change_time_inc *= state_change_time_inc_inc;

        /* actually rephase */
        if (rand() % 100 < 50)
            info_based_rephase();
        else
            rand_based_rephase();
    }


    // simplify
    //
    if (lcm && conflicts >= curSimplify * nbconfbeforesimplify && techniqueStartConflicts.allowLcm()) {
        TRACE(printf("c ### simplifyAll on conflict : %" PRId64 "\n", conflicts);)
        if (verbosity >= 1)
            printf("c schedule LCM with: nbClauses: %d, nbLearnts_core: %d, nbLearnts_tier2: %d, nbLearnts_local: %d, "
                   "nbLearnts: %d\n",
                   clauses.size(), learnts_core.size(), learnts_tier2.size(), learnts_local.size(),
                   learnts_core.size() + learnts_tier2.size() + learnts_local.size());
        nbSimplifyAll++;
        if (!simplifyAll()) {
            return l_False;
        }
        curSimplify = (conflicts / nbconfbeforesimplify) + 1;
        nbconfbeforesimplify += incSimplify;
    }

    if (!okay()) return l_False;

    // check whether we want to do inprocessing
    inprocessing();

    prefetchAssumptions();
    CRef this_confl = CRef_Undef, prev_confl = CRef_Undef;
    TRACE(check_invariants();)
    CRef confl = CRef_Undef;

    for (;;) {

        // only propagate, if nobody else detected a conflict already
        if (confl == CRef_Undef) {
            TRACE(printf("c propagate literals on level %d with trail size %d\n", decisionLevel(), trail.size());)
            confl = propagate();
        }

        if (confl != CRef_Undef) {
            prev_confl = this_confl;
            this_confl = confl;
            confl = CRef_Undef;

            // CONFLICT
            if (usesVSIDS()) {
                if (--var_decay_timer == 0 && var_decay < 0.95)
                    var_decay_timer = var_decay_timer_init, var_decay += 0.01;
            } else if (step_size > min_step_size)
                step_size -= step_size_dec;

            conflicts++;
            nof_conflicts--;
            techniqueStartConflicts.nextConflicts();
            TRACE(printf("c hit conflict %" PRId64 "\n", conflicts);)
            if (conflicts == 100000 && learnts_core.size() < 100) core_lbd_cut = 5;
            ConflictData data = FindConflictLevel(this_confl);
            if (data.nHighestLevel == 0) return l_False;
            // assert(prev_confl != this_confl && "we should not have duplicate conflicts in a row");
            if (data.bOnlyOneLitFromHighest) { //  && prev_confl != this_confl) {
                int btLevel = (prev_confl != this_confl) ? data.nHighestLevel - 1 : data.secondHighestLevel - 1;
                btLevel = btLevel >= 0 ? btLevel : 0;
                TRACE(std::cout << "c chronological backtracking, backtrack until level " << data.nHighestLevel - 1 << std::endl;
                      for (int i = 0; i < ca[this_confl].size(); ++i) {
                          Lit tl = ca[this_confl][i];
                          std::cout << "c     " << tl << "@" << level(var(tl)) << " with reason " << reason(var(tl)) << std::endl;
                      })
                cancelUntil(btLevel, false);
                continue;
            }

            learnt_clause.clear();
            learntUIPclauses.clear(); // store all additional learnt clauses, lit_Undef separated
            otfsss.clear();           // store information about potential on-the-fly-self-subsumption clauses
            if (conflicts > 50000) {
                if (considersDISTANCE()) {
                    if (verbosity) printf("c set DISTANCE to 0\n");
                    disableDISTANCEheuristic();
                }
            } else {
                if (!considersDISTANCE()) {
                    if (verbosity) printf("c set DISTANCE to 1\n");
                    enableDISTANCEheuristic();
                }
            }
            if (current_heuristic == VSIDS_DISTANCE) collectFirstUIP(this_confl);

            TRACE(std::cout << "c run conflict analysis on conflict clause [" << this_confl << "]: " << ca[this_confl] << std::endl);
            analyze(this_confl, learnt_clause, backtrack_level, lbd, maxLearnAllUIPsLevel, learntUIPclauses, otfsss);
            TRACE(std::cout << "c retrieved learnt clause " << learnt_clause << std::endl);

            // check chrono backtrack condition
            if ((confl_to_chrono < 0 || confl_to_chrono <= (int64_t)conflicts) && chrono > -1 &&
                (decisionLevel() - backtrack_level) >= chrono) {
                ++chrono_backtrack;
                TRACE(std::cout << "c chronological backtracking until level " << data.nHighestLevel - 1 << std::endl);
                assert((level(var(learnt_clause[0])) == 0 || level(var(learnt_clause[0])) > data.nHighestLevel - 1) &&
                       "learnt clause is asserting");
                cancelUntil(data.nHighestLevel - 1, false);
            } else // default behavior
            {
                ++non_chrono_backtrack;
                TRACE(std::cout << "c non-chrono backtracking until level " << backtrack_level << std::endl);
                cancelUntil(backtrack_level, true);
            }

            lbd--;
            restartStrategy.notifyConflict(usesVSIDS(), lbd);

            // share clause with interfaces (ipasir, hordesat)
            shareViaCallback(learnt_clause, lbd);

            if (learntUIPclauses.size() > 0) {
                add_tmp.clear();
                int i = 0;
                int highestFalseLevel = 0;
                int undefLits = 0;
                for (; i < learntUIPclauses.size(); ++i) {
                    const Lit l = learntUIPclauses[i];
                    if (l != lit_Undef) {
                        add_tmp.push(l);
                        assert(value(l) != l_True && "Reolvent after analysis has to be falsified");
                        if (value(l) == l_Undef)
                            undefLits++;
                        else {
                            if (level(var(l)) > highestFalseLevel) highestFalseLevel = level(var(l));
                        }
                        continue;
                    }
                    allUIPlearntClauses++;
                    allUIPlearntClausesTotalSize += add_tmp.size();
                    allUIPlearntClausesTotalLBDs += lbd;
                    TRACE(std::cout << "c found additional UIP learned clause" << add_tmp << std::endl;)
                    assert(undefLits > 0 && "At least one literal has to be undefined after backjumping");
                    assert(highestFalseLevel >= backtrack_level && "Final learned clause cannot have less levels");
                    // TODO: attach and share clause here!
                    proof.addClause('a', add_tmp);
                    if (add_tmp.size() == 1) {
                        uncheckedEnqueue(add_tmp[0], 0);
                    } else {
                        CRef cr = ca.alloc(add_tmp, true);
                        ca[cr].set_lbd(lbd);
                        learnts_local.push(cr);
                        attachClause(cr);
                        if (undefLits == 1) {
                            uncheckedEnqueue(add_tmp[0], backtrack_level, cr);
                        }
                    }
                    add_tmp.clear();
                }
            }

            if (learnt_clause.size() == 1) {
                uncheckedEnqueue(learnt_clause[0], 0);
            } else {
                CRef cr = ca.alloc(learnt_clause, true);
                TRACE(std::cout << "c allocate learnt clause " << learnt_clause << " with cref= " << cr << std::endl);
                ca[cr].set_lbd(lbd);
                if (lbd <= core_lbd_cut) {
                    learnts_core.push(cr);
                    ca[cr].mark(CORE);
                    ca[cr].touched() = conflicts;
                } else if (lbd <= tier2_lbd_cut) {
                    learnts_tier2.push(cr);
                    ca[cr].mark(TIER2);
                    ca[cr].touched() = conflicts;
                } else {
                    learnts_local.push(cr);
                    claBumpActivity(ca[cr]);
                }
                attachClause(cr);
                statistics.solveSteps++;

                uncheckedEnqueue(learnt_clause[0], backtrack_level, cr);
            }
            proof.addClause('a', learnt_clause);
            if (usesVSIDS()) varDecayActivity();
            claDecayActivity();

            // process otfss candidates, set conflict in case a conflict is detected
            confl = processOTFSScandidates(lbd);
            otfsss.clear();

            if (verbosity >= 1 && (conflicts % status_every) == 0) {
                printf("c | %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |\n", (int)conflicts,
                       (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), (int)nClauses(), (int)clauses_literals,
                       (int)max_learnts, nLearnts(), (double)learnts_literals / nLearnts(), progressEstimate() * 100);
                printf("c accesses: %" PRIu64 " cpu-time: %lf\n", counter_access.sum(), cpuTime());
            }

            // the top_trail_soln should be update after each conflict
            if (trail.size() > max_trail) {
                max_trail = trail.size();

                int var_nums = nVars();
                for (int idx_i = 0; idx_i < var_nums; ++idx_i) {
                    lbool value_i = value(idx_i);
                    if (value_i == l_Undef)
                        top_trail_soln[idx_i] = !polarity[idx_i];
                    else {
                        top_trail_soln[idx_i] = value_i == l_True ? 1 : 0;
                    }
                }
            }

        } else {

            shareUnitClauses();

            // NO CONFLICT
            if (solve_starts + starts > state_change_time) {

                if (can_call_ls && freeze_ls_restart_num < 1 && mediation_used &&
                    (trail.size() > (int)(conflict_ratio * nVars()) || trail.size() > (int)(percent_ratio * max_trail)) //&& up_time_ratio * search_start_cpu_time > ls_used_time
                ) {

                    can_call_ls = false;
                    mediation_used = false;
                    freeze_ls_restart_num = restarts_gap;
                    bool res = call_ls(true);

                    if (res) {
                        solved_by_ls = true;
                        memorize_used_assumptions();
                        return l_True;
                    }
                }
            }

            bool restart = restartStrategy.triggerRestart(usesVSIDS(), nof_conflicts);
            if (restart || !withinBudget()) {
                TRACE(std::cout << "c interrupt search due to restart or budget" << std::endl);
                restartStrategy.performRestart();

                int restartLevel = getRestartLevel();
                if (verbosity > 3)
                    printf("c trigger restart with target level %d from %d\n", restartLevel, decisionLevel());
                restartLevel = (assumptions.size() && restartLevel <= assumptions.size()) ? assumptions.size() : restartLevel;
                TRACE(std::cout << "c jump to level " << restartLevel << " for restart" << std::endl);
                cancelUntil(restartLevel);
                memorize_used_assumptions();
                uipMini.restartUpdate();
                return l_Undef;
            }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify()) return l_False;

            if (core_size_lim != -1 && learnts_core.size() > core_size_lim) {
                TRACE(std::cout << "c reduce core learnt clauses" << std::endl);
                bool successful_reduced = reduceDB_Core();
                core_size_lim += core_size_lim * core_size_lim_inc;
                /* add extra penalty, if no success */
                if (!successful_reduced) core_size_lim += core_size_lim * core_size_lim_inc;
            }

            if (learnts_tier2.size() > 7000) {
                TRACE(std::cout << "c reduce tier 2 clauses" << std::endl);
                reduceDB_Tier2();
            }

            if (conflicts >= next_L_reduce) {
                next_L_reduce = conflicts + 15000;
                TRACE(std::cout << "c reduce learnt clauses" << std::endl);
                reduceDB();
            }

            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size()) {
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True) {
                    // Dummy decision level:
                    newDecisionLevel();
                } else if (value(p) == l_False) {
                    analyzeFinal(~p, conflict);
                    memorize_used_assumptions();
                    return l_False;
                } else {
                    next = p;
                    break;
                }
            }

            if (next == lit_Undef && constraint_active && decisionLevel() == assumptions.size() && constrain_clause.size() > 0) {
                bool satisfied_constraint = false;
                for (int i = 0; i < constrain_clause.size(); ++i) {
                    const lbool tmp = value(constrain_clause[i]);
                    if (tmp == l_False) {
                        /* just skip this literal, as it's falsified already */
                    } else {
                        satisfied_constraint = true;
                        if (tmp == l_True) {
                            break;
                        } else if (tmp == l_Undef) {
                            next = constrain_clause[i];
                        }
                    }
                }
                if (!satisfied_constraint) {
                    analyzeFinal(constrain_clause, conflict);
                    memorize_used_assumptions();
                    return l_False;
                }
            }

            if (next == lit_Undef) {
                // New variable decision:
                decisions++;
                next = pickBranchLit(confl);
                if (next == lit_Error) {
                    assert(confl != CRef_Undef);
                    continue;
                }

                if (next == lit_Undef) {
                    // Model found:
                    memorize_used_assumptions();
                    return l_True;
                }

                lastDecision = var(next);
            }

            if (!nAssignment.checkNecessaryAssignment(next, confl)) {
                // Increase decision level and enqueue 'next'
                newDecisionLevel();
                TRACE(std::cout << "c use literal " << next << " as decision literal on level " << decisionLevel() << std::endl);
                uncheckedEnqueue(next, decisionLevel());
            }
        }


        if ((conflicts & (sync_conflicts - 1)) == 0) {
            lbool sync_status = l_Undef;
            sync_and_share(&sync_status);
            if (sync_status != l_Undef) {
                if (verbosity > 2)
                    std::cout << "c make sure we stop search with a new status (" << sync_status << ")" << std::endl;
                return sync_status; /* make sure we consume the status */
            }
            if (abort_search_after_sync) { /* TODO: make sure we run into the same amount of syncs. if we abort here, do not check solve! */
                if (verbosity > 2)
                    printf("c abort search based on sync after %" PRIu64 " conflicts with status %d\n", conflicts, toInt(sync_status));
                return l_Undef; /* we do not have any proof*/
            }
        }
    }

    memorize_used_assumptions();
    return l_Undef;
}

Solver::NecessaryAssignments::NecessaryAssignments(Solver &solver, int maxBinaryClauses, int attemptEvery, int recheckBinInc)
  : s(solver)
  , verbosity(solver.verbosity)
  , max_necc_binary(maxBinaryClauses)
  , attempt_every(attemptEvery)
  , recheck_inc(recheckBinInc)
  , na_attempts(0)
  , na_propagated_other_lits(0)
  , na_other_conflicts(0)
  , na_successful_attempts(0)
  , na_implied_lits(0)
{
}

bool Solver::NecessaryAssignments::checkNecessaryAssignment(Lit next, CRef &confl)
{
    // reject, in case we should not use this
    if (next == lit_Undef || next == lit_Error || s.decisionLevel() != 0) return false;
    if (attempt_every < 1) return false;
    if (s.decisions % attempt_every != 0) return false;
    assert(s.value(next) == l_Undef && "no need to analyze assigned variables");
    assert(confl == CRef_Undef && "do not check necessary assignments during an active conflict");

    const vec<Watcher> &ws = s.watches_bin[~next]; // binary watch list with clauses containing next
    lastCheckedBinaryClauses.growTo(toInt(next) + 1, 0);
    int nextLastChecked = lastCheckedBinaryClauses[toInt(next)];

    // do not re-check same literal each time
    if (nextLastChecked != 0 && ws.size() + recheck_inc < nextLastChecked) return false;

    verbosity = s.verbosity;

    necessaryAssignments.clear();
    collectedPolarities.clear();
    collectedPolarities.growTo(s.nVars() + 1, 0);


    int litsToProbe = ws.size() > max_necc_binary ? max_necc_binary : ws.size();
    TRACE(std::cout << "c NA check literal " << next << " with " << litsToProbe << " literals, and " << ws.size()
                    << " binary clauses" << std::endl;)
    na_attempts++;

    int wsIndex = 0;
    int round = 0;
    usedLits.clear();
    for (; round < litsToProbe; ++round, ++wsIndex) {
        if (wsIndex >= ws.size()) break; // no more clauses to analyze
        TRACE(std::cout << "c NA (" << next << ") round " << round << " for index " << wsIndex << "/" << ws.size() << std::endl;)
        const Clause &c = s.ca[ws[wsIndex].cref];
        Lit otherLit = c[0] == next ? c[1] : c[0];
        // skip satisfied clauses
        if (c.mark() == 1 || s.value(c[0]) == l_True || s.value(c[1]) == l_True) {
            round--;
            continue;
        }

        na_propagated_other_lits++;
        usedLits.push(otherLit); // store used literal to create proof
        TRACE(std::cout << "c NA use literal " << otherLit << " during necessary assignment analysis for literal "
                        << next << " based on clause " << c << std::endl);
        const int preProbeTrailSize = s.trail.size();
        s.newDecisionLevel();
        assert(s.decisionLevel() == 1);
        s.uncheckedEnqueue(otherLit, s.decisionLevel());
        confl = s.propagate();

        if (confl != CRef_Undef) {
            na_other_conflicts++;
            TRACE(std::cout << "c NA found conflict " << s.ca[confl] << " when propagating other literal " << otherLit << std::endl;)
            return true; // found a conflict, keep state as is
        }

        for (int i = preProbeTrailSize; i < s.trail.size(); ++i) {
            Var v = var(s.trail[i]);
            // use 2 bits to represent state, shift by 2 bits per round
            collectedPolarities[v] |= ((sign(s.trail[i]) ? 2 : 1) << (2 * round));
        }
        s.cancelUntil(0, false, false); // backtrack without keeping state
    }

    if (round == 0) return false; // we did not do anything, perform a decision as usual

    // perform actual selected decision, and propagate to detect state changes
    s.newDecisionLevel();
    TRACE(std::cout << "c NA use literal " << next << " as decision literal on level " << s.decisionLevel()
                    << " during necessary assignment analysis" << std::endl);
    const int preProbeTrailSize = s.trail.size();
    s.uncheckedEnqueue(next, s.decisionLevel());
    confl = s.propagate();
    if (confl != CRef_Undef) {
        TRACE(std::cout << "c NA found conflict after initial decision" << std::endl;)
        return true; // we found a conflict, keep the state
    }

    // check literals implied by current decision for matches with previously collected literals
    for (int i = preProbeTrailSize; i < s.trail.size(); ++i) {
        Lit necessaryCandidate = s.trail[i];
        Var v = var(necessaryCandidate);
        // test for necessary assignment for all performed rounds
        for (int j = 0; j < round; ++j) {
            Lit otherLit = usedLits[j];
            assert(otherLit != lit_Undef);
            // use 2 bits to represent state, shift by 2 bits per round
            const int signCode = (collectedPolarities[v] >> (2 * j)) & 3; // shifted 2 bits
            if ((signCode == 2 && sign(necessaryCandidate)) || (signCode == 1 && !sign(necessaryCandidate))) {
                necessaryAssignments.push(necessaryCandidate);
                // add to proof here already, as we need the other lit as well
                s.proof.addNecessaryAssignment(next, otherLit, necessaryCandidate);
                break; // done checking this literal
            }
        }
    }

    // no assignments found, perform decision outside of this function
    if (necessaryAssignments.size() == 0) {
        TRACE(std::cout << "c NA no necessary assignments found, continue with search" << std::endl;)
        return true;
    }

    na_successful_attempts++;

    // jump to root and assign all necessary assignments, and propagate
    s.cancelUntil(0, false, false);
    for (int i = 0; i < necessaryAssignments.size(); ++i) {
        Lit necessary = necessaryAssignments[i];
        assert(s.value(necessary) == l_Undef && "Literal has to be unassigned during backjumping");
        s.uncheckedEnqueue(necessary, 0);
    }
    na_implied_lits += necessaryAssignments.size();

    TRACE(std::cout << "c NA enqueues necessary assignments on level 0, keep state" << std::endl);
    return true;
}

CRef Solver::processOTFSScandidates(int learnedLBD)
{
    CRef otfssConflict = CRef_Undef;
    if (otfssMaxLBD > 0 && learnedLBD <= otfssMaxLBD) {
        for (int i = 0; i < otfsss.size(); ++i) {
            OTFSS &otfss = otfsss[i];
            Clause &c = ca[otfss.cr];
            assert(c.size() > 1 && "there cannot be unit clauses in OTFSS candidates");
            totalOtfssClauses++;
            totalOtfssClauseSizes += c.size();
            // drop c from watch list of l, detach, treat binary clauses special, as they become units
            if (c.size() == 2) {
                Lit unit = c[0] == otfss.l ? c[1] : c[0];
                assert(value(unit) != l_False);
                if (value(unit) != l_Undef) continue;
                uncheckedEnqueue(unit, 0);
                add_tmp.clear();
                add_tmp.push(unit);
                proof.addClause('a', add_tmp);
                otfssUnits++;
                otfssImplied++;
                continue;
            } else {
                // TODO: (1) drop watched otfss literal, (2) replace with 2nd highest level (3) if unit, enqueue
                //
                assert(c[0] == otfss.l && "first literal is the one to remove via OTFSS");
                // check whether we found another conflict, or have literals to still propagate
                int nonFalseLitPos = 0;
                int highestLevelLitPos = 1;
                bool multipleHighestLevel = false;
                int highestLevel = 0; // level to jump to in case of unit or conflict (==future unit)
                int nonFalseLits = 0;
                bool isSat = false;
                for (int i = 1; i < c.size(); ++i) {
                    const int vLevel = level(var(c[i]));
                    if (vLevel == 0) {
                        if (value(c[i]) == l_True) {
                            isSat = true;
                            break;
                        }
                        continue; // only work on reduct
                    }
                    if (value(c[i]) != l_False) {
                        nonFalseLits++;
                        if (nonFalseLits > 1) break; // fully watchable clause
                        nonFalseLitPos = i;
                        continue;
                    }
                    // check what other literal should be watched
                    if (vLevel > highestLevel) {
                        highestLevelLitPos = i;
                        highestLevel = vLevel;
                        multipleHighestLevel = false;
                    } else if (vLevel == highestLevel) {
                        multipleHighestLevel =
                        true; // the reduced clause qualifies for a conflict, multiple lits from the highest level, all l_False
                    }
                }
                if (isSat) continue;
                TRACE(std::cout << "c OTFSS found candidate " << c << " with " << nonFalseLits
                                << " nonFalseLits and highest level " << highestLevel << " at pos: " << highestLevelLitPos
                                << "(multiple: " << multipleHighestLevel << std::endl;)
                assert((nonFalseLits < 2 || value(c[1]) != l_False) &&
                       "either the clause is unit/conflict, or the other watched literal has to be unassigned");
                // update 1st, literal, and re-watch clause
                detachClause(otfss.cr, true);
                proof.strengthenClause(c, c[0]);
                int swapPos = nonFalseLits < 2 ? highestLevelLitPos : nonFalseLitPos;
                c.swapLits(0, swapPos);            // move OTFSS literal to back
                c.swapLits(swapPos, c.size() - 1); // drop literal by removing to end
                c.shrink(1);                       // and actually dropping
                TRACE(std::cout << "c OTFSS after removing literal " << otfss.l << ", remaining clause: " << c << std::endl;);
                // process the clause accordingly
                if (nonFalseLits == 0) {
                    if (highestLevel == 0)
                        multipleHighestLevel = true; // formula is UNSAT, we found the corresponding conflict
                    if (multipleHighestLevel) {
                        cancelUntil(highestLevel, false); // caching might actually be possible here
                        otfssConflicts++;
                        // TODO: memorize that this will be a conflict for now, and might have to be processed accordingly
                        otfssConflict = otfss.cr;
                        TRACE(std::cout << "c OTFSS: memorize conflict " << ca[otfssConflict] << " on level "
                                        << highestLevel << std::endl;)
                    } else {
                        assert(highestLevelLitPos == 1 &&
                               "literal with highest level has to be the previously other watched literal");
                        assert(level(var(c[0])) == highestLevel &&
                               "the literal with the highest level was swapped to front");
                        int secondHighestLevelLitPos = 1;
                        int secondHighestLevel = level(var(c[secondHighestLevelLitPos]));
                        for (int i = 2; i < c.size(); ++i) {
                            const int vLevel = level(var(c[i]));
                            if (vLevel == 0) continue; // only work on reduct
                            // check what other literal should be watched
                            if (vLevel > secondHighestLevel) {
                                secondHighestLevelLitPos = i;
                                secondHighestLevel = vLevel;
                            }
                        }
                        if (secondHighestLevelLitPos != 1) c.swapLits(1, secondHighestLevelLitPos);
                        TRACE(std::cout << "c OTFSS detect 2nd highest level " << secondHighestLevel << " to use unit" << std::endl;)
                        cancelUntil(secondHighestLevel, false); // caching might actually be possible here
                        otfssConflict = CRef_Undef;
                        otfssImplied++;
                        assert(value(c[0]) == l_Undef &&
                               "the OTFSSed clause needs to be unit after freeing the final highest-level variable");
                        uncheckedEnqueue(c[0], secondHighestLevel, otfss.cr); // we turned the clause into a unit
                    }
                } else if (nonFalseLits == 1) {
                    otfssImplied++;
                    if (value(c[0]) == l_Undef)
                        uncheckedEnqueue(c[0], highestLevel, otfss.cr); // we turned the clause into a unit
                }
                // attach very late, as we might swap watched literals
                attachClause(otfss.cr);
            }
        }
    } // end handling OTFSS

    assert((otfssConflict == CRef_Undef || value(ca[otfssConflict][0]) == l_False) &&
           "an indicated conflict has to be a conflict");
    return otfssConflict;
}


void Solver::diversify(int rank, int size)
{
    /* rank ranges from 0 to size-1 */

    /* keep first 2 configurations as is, and do not receive clause! */
    if (rank == 0 || (rank == 1 && size > 2)) {
        receiveClauses = false;
    }
    /* use previous release configurations as first configs */
    if (rank == 1) {
        /* initialize activity as in v3 */
        init_act = 2;
    }
    if (rank == 2) {
        /* do not use SLS, and initialize activities as in v3 */
        init_act = 0;
        use_ccnr = false;
        state_change_time = 1000000000;
    }
    if (rank < 3) return;

    /* allow many combinations of configurations for large ranks! */
    if (rank % 3 == 2) invert_pol = true;
    if (rank % 5 == 2) restart = Restart(0);
    if (rank % 5 == 3) restart = Restart(1);
    if (rank % 7 == 3) core_lbd_cut = 4;
    if (rank % 7 == 6) nAssignment.setAttemptEvery(4);
    if (rank % 11 == 4) init_act = 0;
    if (rank % 11 == 7) init_act = 3;
    if (rank % 13 == 8) {
        var_decay_timer = 100000;
        var_decay_timer_init = 100000;
    }
    if (rank % 17 == 4) var_decay = 0.999;
    if (rank % 19 == 6) {
        inprocess_next_lim = 2000;
        inprocess_learnt_level = 1;
    }
    if (rank % 19 == 9) {
        inprocess_next_lim = 3000;
        inprocess_learnt_level = 2;
    }
    if (rank % 23 == 5) chrono = 10;
    if (rank % 23 == 6) chrono = 5;
    if (rank % 29 == 6) state_change_time = 1000;
    if (rank % 29 == 8) state_change_time = 3000;
    if (rank % 29 == 10) state_change_time = 5000;
}


double Solver::progressEstimate() const
{
    double progress = 0;
    const double F = 1.0 / nVars();
    double current_f = 1;
    for (int i = 0; i <= decisionLevel(); i++) {
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += current_f * (end - beg);
        current_f *= F;
    }

    return progress / nVars();
}

void Solver::toggle_decision_heuristic(bool to_VSIDS)
{
    if (to_VSIDS) { // initialize VSIDS heap again?
        assert(!usesVSIDS() && "should not from VSIDS to itself");
        Heap<VarOrderLt> &currentHeap = usesDISTANCE() ? order_heap_DISTANCE : order_heap_CHB;
        order_heap_VSIDS.growTo(currentHeap);
        order_heap_VSIDS.build(currentHeap.elements());
        order_heap = &order_heap_VSIDS;
        current_heuristic = current_heuristic == CHB ? VSIDS_CHB : VSIDS_DISTANCE;
    } else {
        assert(usesVSIDS() && "should come from VSIDS");
        if (current_heuristic == VSIDS_CHB) {
            order_heap_CHB.growTo(order_heap_VSIDS);
            order_heap_CHB.build(order_heap_VSIDS.elements());
            order_heap = &order_heap_CHB;
            current_heuristic = CHB;
        } else {
            order_heap_DISTANCE.growTo(order_heap_VSIDS);
            order_heap_DISTANCE.build(order_heap_VSIDS.elements());
            order_heap = &order_heap_DISTANCE;
            current_heuristic = DISTANCE;
        }
    }
    assert((trail.size() + order_heap->size()) >= full_heap_size);
}

void Solver::disableDISTANCEheuristic()
{
    switch (current_heuristic) {
    case VSIDS_DISTANCE:
        /* currently VSIDS, just change swap partner */
        current_heuristic = VSIDS_CHB;
        break;
    case DISTANCE:
        current_heuristic = CHB;
        order_heap_CHB.growTo(order_heap_DISTANCE);
        order_heap_CHB.build(order_heap_DISTANCE.elements());
        order_heap = &order_heap_CHB;
        break;
    case VSIDS_CHB:
    case CHB:
        break;
    }
    assert(!considersDISTANCE() && "we should have disabled DISTANCE heuristic");
    assert((trail.size() + order_heap->size()) >= full_heap_size);
}

void Solver::enableDISTANCEheuristic()
{
    switch (current_heuristic) {
    case VSIDS_CHB:
        /* currently VSIDS, just change swap partner */
        current_heuristic = VSIDS_DISTANCE;
        break;
    case CHB:
        current_heuristic = DISTANCE;
        order_heap_DISTANCE.growTo(order_heap_CHB);
        order_heap_DISTANCE.build(order_heap_CHB.elements());
        order_heap = &order_heap_DISTANCE;
        break;
    case VSIDS_DISTANCE:
    case DISTANCE:
        break;
    }
    assert(considersDISTANCE() && "we should have enabled DISTANCE heuristic");
    assert((trail.size() + order_heap->size()) >= full_heap_size);
}


// NOTE: assumptions passed in member-variable 'assumptions'.
lbool Solver::solve_()
{
    model.clear();
    conflict.clear();
    if (!ok) {
        deactivate_constrain_clause(l_False);
        TRACE(std::cout << "c leave solve_ early with status: " << l_False << std::endl;)
        return l_False;
    }
    if (!constraint_active) reset_constrain_clause();

    reset_old_trail();
    solves++;
    solve_starts = starts;
    TRACE(std::cout << "c start " << solves << " solve call with " << clauses.size() << " clauses and " << nVars()
                    << " variables" << std::endl);

    double solve_start = cpuTime();
    systematic_branching_state = 1;
    techniqueStartConflicts.reset();

    max_learnts = nClauses() * learntsize_factor;
    learntsize_adjust_confl = learntsize_adjust_start_confl;
    learntsize_adjust_cnt = (int)learntsize_adjust_confl;
    lbool status = l_Undef;
    abort_search_after_sync = false; // initially, we have to sync at least once, or find the solution ourselves

    ls_mediation_soln.resize(nVars());
    ls_best_soln.resize(nVars());
    top_trail_soln.resize(nVars());

    if (verbosity >= 1) {
        printf("c ============================[ Search Statistics ]==============================\n");
        printf("c | Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        printf("c |           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        printf("c ===============================================================================\n");
    }

    add_tmp.clear();

    /* disable a potential SLS solution in case of solving with assumptions */
    if (assumptions.size() > 0 || constraint_active) solved_by_ls = false;

    /* allow to disable SLS for larger clauses */
    if ((sls_var_lim != -1 && nVars() > sls_var_lim) || (sls_clause_lim != -1 && nClauses() > (size_t)sls_clause_lim)) {
        use_ccnr = false;
    }

    /* do not start with SLS, in case we have assumptions, or solve incrementally */
    if (assumptions.size() == 0 && solves == 1 && initial_sls) {
        int fls_res = call_ls(false);
        if (fls_res) {
            status = l_True;
        }
    }

    // toggle back to VSIDS, in case we run the initialization here
    if (solves == 1) {
        if (!usesVSIDS()) toggle_decision_heuristic(true);
        int init = VSIDS_props_init_limit;
        int inner_init = init / 5 > 0 ? init / 5 : 1;
        int inner_init_step = inner_init;
        assert(inner_init_step > 0 && "only steps greater 0");
        while (status == l_Undef && init > 0 && withinBudget() && !abort_search_after_sync) {
            assert(inner_init > 0 && "only steps greater 0");
            while (status == l_Undef && inner_init > 0 && withinBudget() && !abort_search_after_sync)
                status = search(inner_init);
            init = init - inner_init_step > 0 ? init - inner_init_step : 0;
            inner_init = inner_init_step > init ? inner_init_step : init;

            // check with parallel co-workers
            sync_and_share(&status);
        }

        // do not use VSIDS now
        toggle_decision_heuristic(false);
    }

    // Search:
    uint64_t curr_props = 0;
    restartStrategy.notifyNewSearch(restart_strategy, usesVSIDS());
    last_switch_conflicts = starts;
    while (status == l_Undef && withinBudget() && !abort_search_after_sync) {
        if (propagations - curr_props > VSIDS_props_limit) {
            curr_props = propagations;
            switch_mode = true;
            VSIDS_props_limit = VSIDS_props_limit + VSIDS_props_limit / 10;
        }
        int nof_conflicts = restartStrategy.nextRestartConflicts(usesVSIDS(), restart_first);
        status = search(nof_conflicts);

        // check with parallel co-workers
        if (sync_and_share(&status))
            break;
        else {
            if (verbosity > 2) std::cout << "c syncing did not result in abort " << status << std::endl;
        }

        // toggle VSIDS?
        // if (switch_mode) {
        if (starts - last_switch_conflicts > switch_heristic_mod) {
            switch_mode = false;
            toggle_decision_heuristic(!usesVSIDS()); // switch to VSIDS
            if (verbosity >= 1) {
                if (usesVSIDS()) {
                    if (verbosity > 1) printf("c Switched to VSIDS.\n");
                } else {
                    if (verbosity > 1) printf("c Switched to LRB/DISTANCE.\n");
                }
            }
        }
    }

    TRACE(check_invariants();)

    if (verbosity >= 1) printf("c ===============================================================================\n");
    if (status == l_False) proof.flush();

    if (status == l_True) {
        // Extend & copy model:
        model.growTo(nVars());
        if (solved_by_ls)
            for (int i = 0; i < nVars(); i++) model[i] = ls_mediation_soln[i] ? l_True : l_False;
        else
            for (int i = 0; i < nVars(); i++) model[i] = value(i);

        if (check_satisfiability && !check_satisfiability_simplified) {
            if (!satChecker.checkModel(model)) {
                assert(false && "model should satisfy full input formula");
                throw("ERROR: detected model that does not satisfy input formula, abort");
                exit(1);
            } else if (verbosity)
                printf("c validated SAT answer\n");
        }

    } else if (status == l_False) {
        if (conflict.size() == 0 && !constraint_active) ok = false;
    }

    cancelUntil(0);

    if (status == l_False && conflict.size() && lcm_core) {
        if (lcm_core_success) {
            int pre_conflict_size = conflict.size();
            simplifyLearnt(conflict);
            lcm_core_success = pre_conflict_size > conflict.size();
        } else
            lcm_core_success = true;
    }

    systematic_branching_state = 0;
    statistics.solveSeconds += cpuTime() - solve_start; // stop timer and record time consumed until now

    deactivate_constrain_clause(status);
    TRACE(std::cout << "c leave solve_ with status: " << status << std::endl;)
    return status;
}

bool Solver::sync_and_share(lbool *status_after_sync)
{
    /* Implement synchronization and sharing
     * 1: check whether we should sync with others based on counter
     * 2: while a thread waits, increase a counter
     * 3: share and receive clauses in a deterministic way, adjust sharing bounds in case of too many/few clauses
     * 4: calculate numbers per thread until next rendezvous
     * 5: clean structures
     * 6: continue
     */

    if (external_sync_and_share && issuer) {
        if (!abort_search_after_sync) {
            abort_search_after_sync = external_sync_and_share(issuer, status_after_sync);
            if (verbosity > 1 && *status_after_sync != l_Undef)
                std::cout << "c update solver status to " << status_after_sync << std::endl;
        } else {
            if (verbosity > 2) printf("c skip sync, as sync is already set to true\n");
        }
        return abort_search_after_sync;
    }
    return false;
}

void Solver::initialize_parallel_solver(void *_issuer, bool (*_external_sync_and_share)(void *, lbool *))
{
    assert((issuer == nullptr || issuer == _issuer) && "cannot set issuer multiple times");
    assert((external_sync_and_share == nullptr || external_sync_and_share == _external_sync_and_share) &&
           "cannot set sync function multiple times");

    /* set values */
    issuer = _issuer;
    external_sync_and_share = _external_sync_and_share;
}

//=================================================================================================
// Writing CNF to DIMACS:
//
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var> &map, Var &max)
{
    if (map.size() <= x || map[x] == -1) {
        map.growTo(x + 1, -1);
        map[x] = max++;
    }
    return map[x];
}


void Solver::toDimacs(FILE *f, Clause &c, vec<Var> &map, Var &max)
{
    if (satisfied(c)) return;

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) != l_False) fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max) + 1);
    fprintf(f, "0\n");
}


void Solver::toDimacs(const char *file, const vec<Lit> &assumps)
{
    FILE *f = fopen(file, "wr");
    if (f == NULL) fprintf(stderr, "could not open file %s\n", file), exit(1);
    toDimacs(f, assumps);
    fclose(f);
}


void Solver::toDimacs(FILE *f, const vec<Lit> &assumps)
{
    // Handle case when solver is in contradictory state:
    if (!ok) {
        fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
        return;
    }

    vec<Var> map;
    Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]])) cnt++;

    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]])) {
            Clause &c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (value(c[j]) != l_False) mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumptions.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for (int i = 0; i < assumptions.size(); i++) {
        assert(value(assumptions[i]) != l_False);
        fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "", mapVar(var(assumptions[i]), map, max) + 1);
    }

    for (int i = 0; i < clauses.size(); i++) toDimacs(f, ca[clauses[i]], map, max);

    if (verbosity > 0) printf("c Wrote DIMACS with %d variables and %d clauses.\n", max, cnt);
}


bool Solver::inprocessing()
{
    if (inprocess_next_lim != 0 && solves && inprocess_attempts++ >= inprocess_next_lim && inprocess_inc != (double)0) {
        L = 60; // clauses with lbd higher than 60 are not considered (and rather large anyways)
        inprocess_next_lim = (uint64_t)((double)inprocess_next_lim * inprocess_inc);
        inprocessings++;
        int Z = 0, i, j, k, l = -1, p;

        if (verbosity > 0)
            printf("c inprocessing simplify at try %" PRId64 ", next limit: %" PRId64 "\n", inprocess_attempts, inprocess_next_lim);
        // fill occurrence data structure
        O.resize(2 * nVars());

        add_tmp.clear();

        for (i = 0; i < 1 + inprocess_learnt_level; ++i) {
            vec<CRef> &V = i == 0 ? clauses : (i == 1 ? learnts_core : (i == 2 ? learnts_tier2 : learnts_local));
            for (j = 0; j < V.size(); ++j) {
                CRef R = V[j];
                Clause &c = ca[R];
                if (c.mark() == 1 || R == reason(var(c[0])) || R == reason(var(c[1]))) continue;
                for (k = 0; k < c.size(); ++k) O[toInt(c[k])].push_back(R);
                inprocess_mems++;
            }
        }

        // clean marker structure
        M.shrink_(M.size());
        M.growTo(2 * nVars());
        T = 0;

        // there are multiple "learnt" vectors, hence, consider all of them
        for (int select = 0; select < inprocess_learnt_level; select++) {
            vec<CRef> &learnts = (select == 0 ? learnts_core : (select == 1 ? learnts_tier2 : learnts_local));
            for (i = 0; i < learnts.size(); ++i) {
                T++;
                CRef s = learnts[i];
                Clause &c = ca[s];
                inprocess_mems++;
                if (c.mark() == 1 || c.S() || c.lbd() > 12) continue; // run this check for each clause exactly once!
                Lit m = c[0];

                // get least frequent literal from clause, and mark all lits of clause in array
                for (j = 0; j < c.size(); ++j) {
                    k = toInt(c[j]);
                    M[k] = T;                                        // make array for current literal
                    m = O[k].size() < O[toInt(m)].size() ? c[j] : m; // get least frequent literal
                }
                // printf ("c for clause %d, mark %d lits\n", s, c.size());

                std::vector<CRef> &V = O[toInt(m)];
                for (size_t j = 0; j < V.size(); ++j) {
                    CRef r = V[j];
                    if (r == s) continue; // do not subsume the same clause
                    Clause &d = ca[r];    // get the actual clause
                    if (d.size() < c.size() || d.mark() == 1 || (d.learnt() && d.lbd() > L) || d.size() == 2 ||
                        r == reason(var(d[0])))
                        continue; // smaller clauses cannot be (self-)subsumed

                    inprocess_mems++;
                    l = -1;
                    p = 0;
                    for (k = 0; k < d.size(); ++k) {
                        // the current literal is not present in clause 'c', so 'd' is not subsumed
                        if (M[toInt(d[k])] == T)
                            p++;
                        else {
                            // are there 2 literals that do not match, or we'd remove a watched literal, then do not consider self subsuming resolution
                            if (l >= 0 || k < 2) {
                                l = d.size();
                            } else if (M[toInt(~d[k])] == T) {
                                l = k;
                                p++; // count this literal as matching
                            }
                            // could break, if d.size() - k < c.size() - p (might be more expensive than just running through, even after reformulating statement)
                        }
                    }
                    // subsume or self-subsume (we matched all c literals, and did not hit a break statement)
                    // printf ("c for clause %d, hit %d out o %d lits\n", r, p, c.size());
                    if (p == c.size()) { // && k==d.size() ) // in case there is a break statment in the above loop, we need to make sure we processed all literals in d
                        if (l < 0 && (d.learnt() || !c.learnt())) {
                            removeClause(r); // subsume, if learnt status matches, hence drop
                            Z++;
                            ++inprocessing_C;
                        } else if (l >= 0 && l < d.size()) {

                            // drop proof file
                            if (proof.enabled()) {
                                proof.strengthenClause(d, d[l]);
                                proof.addClause('d', d);
                            }

                            // drop the one literal, whose complement is in clause 'c'
                            if (l < 2 || d.size() == 3) detachClause(r, true);
                            d[l] = d.last();
                            d.pop();
                            d.S(0); // allow this clause for simplification again!
                            if (l < 2 || d.size() == 2) {
                                if (d.size() == 1)
                                    add_tmp.push(d[0]);
                                else
                                    attachClause(r);
                            }
                            Z++;
                            ++inprocessing_L;
                        }
                    }
                }
                c.S(1); // memorize that we will not repeat the analysis with this clause
            }

            if (!Z)
                inprocess_next_lim += inprocess_penalty; // in case we did not modify anything, skip a few more relocs before trying again
            for (size_t i = 0; i < O.size(); ++i) O[i].clear(); // do not free, just drop elements
        }

        /* in case we found unit clauses, make sure we find them fast */
        if (add_tmp.size()) {
            cancelUntil(0, false);
            for (int i = 0; i < add_tmp.size(); ++i) {
                if (value(add_tmp[i]) == l_False) { /* we found a contradicting unit clause */
                    ok = false;
                    return false;
                }
                uncheckedEnqueue(add_tmp[l], decisionLevel());
            }
        }
    }

    return true;
}

//=================================================================================================
// Garbage Collection methods:

void Solver::relocWatches(ClauseAllocator &to, Lit p)
{
    vec<Watcher> &ws = watches[p];
    for (int j = 0; j < ws.size(); j++) ca.reloc(ws[j].cref, to);
    vec<Watcher> &ws_bin = watches_bin[p];
    for (int j = 0; j < ws_bin.size(); j++) ca.reloc(ws_bin[j].cref, to);
}

void Solver::relocAll(ClauseAllocator &to)
{
    TRACE(std::cout << "c relocing ..." << std::endl);

    // All watchers:
    //
    // for (int i = 0; i < watches.size(); i++)
    watches.cleanAll();
    watches_bin.cleanAll();

    if (gc_in_assign_order) {
        for (int i = 0; i < trail.size(); ++i) {
            relocWatches(to, trail[i]);
            relocWatches(to, ~trail[i]);
            assert(seen[var(trail[i])] == 0);
            seen[var(trail[i])] = 1;
        }
        for (int i = 0; i < order_heap->size(); ++i) {
            const Var v = order_heap->at(i);
            if (seen[v]) continue;
            for (int s = 0; s < 2; s++) relocWatches(to, mkLit(v, s));
            seen[v] = 1;
        }
    }

    for (int v = 0; v < nVars(); v++) {
        if (!seen[v]) {
            for (int s = 0; s < 2; s++) relocWatches(to, mkLit(v, s));
        } else {
            seen[v] = 0;
        }
    }

    // All reasons:
    //
    for (int i = 0; i < trail.size(); i++) {
        Var v = var(trail[i]);

        // Note: it is not safe to call 'locked()' on a relocated clause. This is why we keep
        // 'dangling' reasons here. It is safe and does not hurt.
        if (reason(v) != CRef_Undef && statistics.solveSteps++ && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
            ca.reloc(vardata[v].reason, to);
    }

    for (int i = 0; i < old_trail.size(); i++) {
        Var v = var(old_trail[i]);

        if (oldreasons[v] != CRef_Undef && (ca[oldreasons[v]].reloced())) ca.reloc(oldreasons[v], to);
    }

    // All learnt:
    //
    for (int i = 0; i < learnts_core.size(); i++) ca.reloc(learnts_core[i], to);
    for (int i = 0; i < learnts_tier2.size(); i++) ca.reloc(learnts_tier2[i], to);
    for (int i = 0; i < learnts_local.size(); i++) ca.reloc(learnts_local[i], to);

    // All original:
    //
    int i, j;
    for (i = j = 0; i < clauses.size(); i++)
        if (ca[clauses[i]].mark() != 1) {
            ca.reloc(clauses[i], to);
            clauses[j++] = clauses[i];
        }
    clauses.shrink(i - j);

    for (int i = 0; i < simplifyBuffer.size(); i++) {
        if (simplifyBuffer[i] != CRef_Undef) ca.reloc(simplifyBuffer[i], to);
    }
}


void Solver::garbageCollect()
{
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(counter_access, ca.size() - ca.wasted());

    relocAll(to);
    if (verbosity >= 2)
        printf("c |  Garbage collection:   %12d bytes => %12d bytes             |\n",
               ca.size() * ClauseAllocator::Unit_Size, to.size() * ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}

void Solver::reset_old_trail()
{
    TRACE(std::cout << "c reset old trail" << std::endl);
    for (int i = 0; i < old_trail.size(); i++) {
        oldreasons[var(old_trail[i])] = CRef_Undef;
    }
    old_trail.clear();
    old_trail_qhead = 0;
}

void Solver::printStats()
{
    double cpu_time = cpuTime();
    printf("c restarts              : %" PRIu64 "\n", starts);
    printf("c conflicts             : %-12" PRIu64 "   (%.0f /sec)\n", conflicts, conflicts / cpu_time);
    printf("c decisions             : %-12" PRIu64 "   (%4.2f %% random) (%.0f /sec)\n", decisions,
           (float)rnd_decisions * 100 / (float)decisions, decisions / cpu_time);
    printf("c propagations          : %-12" PRIu64 "   (%.0f /sec)\n", propagations, propagations / cpu_time);
    printf("c conflict literals     : %-12" PRIu64 "   (%4.2f %% deleted)\n", tot_literals,
           (max_literals - tot_literals) * 100 / (double)max_literals);
    printf("c i-uip                 : %-12" PRIu64 "   (%g %%, minimized: %" PRIu64 ", %g %%, %" PRIu64 " lits) \n",
           uipMini.i_uip_global_attempts, 100.0 * (double)uipMini.i_uip_global_attempts / (double)conflicts,
           uipMini.i_uip_global_decisions, 100.0 * (double)uipMini.i_uip_global_decisions / (double)(double)uipMini.i_uip_global_attempts,
           uipMini.i_uip_global_dropped_lits);
    printf("c all-uip-learned       : %-12" PRIu64 "   learned (%g avg.size, %g avg.lbd) \n", allUIPlearntClauses,
           (double)allUIPlearntClausesTotalSize / (double)allUIPlearntClauses,
           (double)allUIPlearntClausesTotalLBDs / (double)allUIPlearntClauses);
    printf("c OTFSS:                : %-12" PRIu64 "   (%" PRIu64 " units, %" PRIu64 " implied, %" PRIu64
           " conflicts, %g avg.size)\n",
           totalOtfssClauses, otfssUnits, otfssImplied, otfssConflicts, (double)totalOtfssClauseSizes / (double)totalOtfssClauses);
    printf("c necessary.assignments : %-12" PRIu64 "   (%" PRIu64 " attempts - %" PRIu64 " successful, %" PRIu64
           " partnerLits, %" PRIu64 " extraConflicts)\n",
           nAssignment.na_implied_lits, nAssignment.na_attempts, nAssignment.na_successful_attempts,
           nAssignment.na_propagated_other_lits, nAssignment.na_other_conflicts);
    printf("c backtracks            : %-12" PRIu64 "   (NCB %0.f%% , CB %0.f%%)\n", non_chrono_backtrack + chrono_backtrack,
           (non_chrono_backtrack * 100) / (double)(non_chrono_backtrack + chrono_backtrack),
           (chrono_backtrack * 100) / (double)(non_chrono_backtrack + chrono_backtrack));
    printf("c lookahead             : %-12" PRIu64 "   (decisions: %" PRIu64 " conflicts: %" PRIu64
           " no-decisions: %" PRIu64 " extra-recompute: %" PRIu64 ")\n",
           la_stat_attempts, la_stat_decisions, la_stat_conflicts, la_stat_nodecisions, la_stat_extra_recomputes);
    printf("c partial restarts      : %-12" PRIu64 "   (partial: %" PRIu64 "  savedD: %" PRIu64 " savedP: %" PRIu64
           " (%.2lf %%))\n",
           starts, restart.partialRestarts, restart.savedDecisions, restart.savedPropagations,
           ((double)restart.savedPropagations * 100.0) / (double)propagations);
    printf("c polarity              : %u pos, %u neg\n", posMissingInSome, negMissingInSome);
    printf("c LCM                   : %" PRIu64 " runs, %" PRIu64 " Ctried, %" PRIu64 " Cshrinked (%" PRIu64
           " known duplicates), %" PRIu64 " Ldeleted, %" PRIu64 " Lrev-deleted\n",
           nbSimplifyAll, LCM_total_tries, LCM_successful_tries, nr_lcm_duplicates, LCM_dropped_lits, LCM_dropped_reverse);
    printf("c Inprocessing          : %" PRIu64 " subsumed, %" PRIu64 " dropped lits, %" PRIu64 " attempts, %" PRIu64 " mems\n",
           inprocessing_C, inprocessing_L, inprocessings, inprocess_mems);
    printf("c Stats:                : %lf solve, %" PRIu64 " steps, %lf simp, %" PRIu64 " steps, %d var, budget: %d\n",
           statistics.solveSeconds, statistics.solveSteps, statistics.simpSeconds, statistics.simpSteps, nVars(), withinBudget());
    printf("c backup trail: stored: %" PRIu64 " used successfully: %" PRIu64 "\n", backuped_trail_lits, used_backup_lits);
    printf("c accesses:               clauses: %" PRIu64 " occurrences: %" PRIu64 " sum: %" PRIu64 "\n",
           counter_access.clause(), counter_access.occurrence(), counter_access.sum());
    printf("c CPU time              : %g s\n", cpu_time);
}

void Solver::addConstrainClause(vec<Lit> &newConstrainClause)
{
    if (decisionLevel() > 0) cancelUntil(0);
    bool satisfied_constraint = false;

    /* reset data about constrain_clause */
    unsat_constraint = false;
    assert(!constraint_active && "Should not override constrain clause with an active constrain clause");

    for (int i = 0; i < newConstrainClause.size(); ++i) {
        const Lit l = newConstrainClause[i];
        /* ignore satified constrain clauses */
        if (seen2[toInt(~l)] != 0 || value(l) == l_True) {
            satisfied_constraint = true;
            break;
        }
        /* saw this literal already */
        if (seen2[toInt(l)] == 1) continue;
        seen2[toInt(l)] = 1;
        constrain_clause.push(l);
    }

    TRACE(std::cout << "c add contraint clause " << constrain_clause << std::endl;)
    constraint_active = true;
    for (int i = 0; i < constrain_clause.size(); ++i) seen2[toInt(constrain_clause[i])] = 0;
    if (satisfied_constraint) {
        constrain_clause.clear();
    } else if (constrain_clause.size() == 0) {
        unsat_constraint = true;
    }
}

bool Solver::failed_constraint()
{
    TRACE(std::cout << "c checking for failed constraint, return " << unsat_constraint << std::endl;)
    return unsat_constraint;
}

void Solver::deactivate_constrain_clause(lbool status)
{
    if (status == l_False && constraint_active) {
        TRACE(std::cout << "c set unsat constraint with status " << status << std::endl;)
        unsat_constraint = true; // always indicate failure in case of unsat
    }
    constraint_active = false;
}

void Solver::reset_constrain_clause()
{
    TRACE(std::cout << "c reset constraint" << std::endl;)
    constrain_clause.clear();
    deactivate_constrain_clause(l_Undef);
    unsat_constraint = false;
}

/** Create clauses to partition the formula under the given assumptions into the given number of partitions
 *  Clauses are terminated with l_Undef, partition formulas are terminated with l_Error.
 *
 * Partitioning invariants: (1) F = \lor_i F_i and (2) F_i \land F_j = unsat
 *
 *  Return - l_False if the formula is unsat under the assumptions (will add relevant clauses to proof)
 *         - l_True if a split has been generated successfully
 *         - l_Undef if no split has been generated successfully
 */
lbool Solver::partitionFormula(vec<Lit> &baseAssumptions, int partitions, vec<Lit> &partitionClauses)
{
    // We need to partition starting at level 0
    if (decisionLevel() != 0) return l_Undef;

    // Unroll assumptions
    TRACE(std::cout << "c partitioning in " << partitions << " partitions, with " << baseAssumptions.size()
                    << " base assumptions" << std::endl);
    for (int i = 0; i < baseAssumptions.size(); ++i) {
        Lit next = baseAssumptions[i];
        if (value(next) == l_True) continue;
        if (value(next) == l_False) {
            cancelUntil(0);
            return l_False; // F falsifies assumptions
        }
        newDecisionLevel();
        uncheckedEnqueue(next, decisionLevel());
    }

    CRef confl = propagate();
    if (confl != CRef_Undef) {
        cancelUntil(0);
        TRACE(std::cout << "c partitioning with " << baseAssumptions.size() << " resulted in unsat" << std::endl);
        return l_False; // assumptions propagated on F result in unsat
    }

    // For now, implement a simple, assumption-based partitioning
    const int varsPerSplit = log2_64(partitions);
    partitions = (1 << varsPerSplit); /* use 2^n partitions  */
    TRACE(std::cout << "c based on " << varsPerSplit << " partition variables, create " << partitions << " partitions" << std::endl);
    vec<Lit> partitionAssumptions; /* current assumptions for a partition */
    vec<bool> isDecision;          /* if a given literal is a decision, it has to be backtracked */
    for (int partition = 0; partition < partitions; ++partition) {
        Lit next = lit_Undef;

        TRACE(std::cout << "c construct partition " << partition << " (state: lits: " << partitionAssumptions
                        << " decisions: " << isDecision << ")" << std::endl);
        /* undo all picks that have been used due to backtracking */
        if (isDecision.size() > 0) {
            while (isDecision.size() > 0 && isDecision.last() != true) {
                TRACE(std::cout << "c rolling back partitioning literal " << partitionAssumptions.last() << std::endl);
                cancelUntil(decisionLevel() - 1);
                isDecision.pop();
                partitionAssumptions.pop();
            }
            if (isDecision.size() == 0) {
                /* was non-empty before, hence, we performed a full roll-back.*/
                break;
            }
        }
        /* actually flip the most recent decision */
        if (isDecision.size() > 0) {
            assert(decisionLevel() > 0 && "The previous literals had to be assumed");
            Lit lastLit = partitionAssumptions.last();
            partitionAssumptions.pop();
            isDecision.pop();
            cancelUntil(decisionLevel() - 1);
            next = ~lastLit;
        }
        TRACE(std::cout << "c looking for " << varsPerSplit << " variables, starting with [" << partitionAssumptions
                        << "]" << std::endl);

        for (int i = partitionAssumptions.size(); i < varsPerSplit; ++i) {
            assert(partitionAssumptions.size() == isDecision.size());
            // Select the next free variable from the decision heap
            bool partitionDecision = next == lit_Undef;
            if (next == lit_Undef) {
                next = pickBranchLit(confl, false); // TODO: allow to use LA here as well, e.g. via parameter
                TRACE(std::cout << "c selected literal " << next << std::endl);
            }

            if (next == lit_Error) { /* partition is unsat */
                // dropping this unsatisfiable partition is fine; partitioning invariants are still valid
                assert(confl != CRef_Undef && "there has to be a conflict on failing decision picks");
                break;
            }
            if (next == lit_Undef) {
                // Found a solution, handle this case
                break;
            }
            partitionAssumptions.push(next);
            isDecision.push(partitionDecision);
            newDecisionLevel();
            uncheckedEnqueue(next, decisionLevel(), CRef_Undef);
            // propagate state, stop on unsat partition
            confl = propagate();
            if (confl != CRef_Undef) break; /* partition is unsat */
            next = lit_Undef;               // reset decision literal selection
        }

        TRACE(std::cout << "c post selection next=" << next << " confl=" << confl
                        << " selectedLits=" << partitionAssumptions << std::endl);

        /* if the partition is not unsat, add it here! */
        if (confl != CRef_Undef) {
            /* learn a conflict clause based on the assumptions, add it to the proof */
            if (next == lit_Error) {
                analyzeFinal(confl, conflict);
                proof.addClause('a', conflict);
                shareViaCallback(conflict, 1); /* relevant clause, as found during partitioning */
                conflict.clear();
            }
            continue;
        }

        TRACE(std::cout << "c storing partition " << partition << " with literals: " << partitionAssumptions << std::endl);
        for (int i = 0; i < partitionAssumptions.size(); ++i) {
            partitionClauses.push(partitionAssumptions[i]);
            partitionClauses.push(lit_Undef); /* finalize current clause */
        }
        partitionClauses.push(lit_Error); /* finalize current partition */
    }
    cancelUntil(0); /* cleanup state */

    /* indicate that the provided partitioning constraints should be used */
    return l_True;
}


bool Solver::call_ls(bool use_up_build)
{
    if (!use_ccnr) return false;

    called_initial_sls = true;

    ccnr = CCNR::ls_solver();
    int ls_var_nums = nVars();
    int ls_cls_nums = nClauses() + learnts_core.size() + learnts_tier2.size();

    /* allow to disable SLS for larger clauses */
    if ((sls_var_lim != -1 && nVars() > sls_var_lim) || (sls_clause_lim != -1 && ls_cls_nums > sls_clause_lim)) {
        use_ccnr = false;
        return false;
    }

    if (trail_lim.size() > 0)
        ls_cls_nums += trail_lim[0];
    else
        ls_cls_nums += trail.size();

    ls_cls_nums += assumptions.size();

    if (nVars() == 0 || ls_cls_nums == 0) return false;

    ccnr._num_vars = ls_var_nums;
    ccnr._num_clauses = ls_cls_nums;
    ccnr._max_mems = ls_mems_num;
    if (!ccnr.make_space()) {
        std::cout << "c ls solver make space error." << std::endl;
        return false;
    }


    // build_instance
    int ct = 0;
    for (int idx = 0; idx < 3; ++idx) {
        vec<CRef> &vs = (idx == 0) ? clauses : (idx == 1 ? learnts_core : (idx == 2 ? learnts_tier2 : learnts_local));
        int vs_sz = vs.size();
        for (int i = 0; i < vs_sz; i++) {
            CRef &cr = vs[i];
            Clause &c = ca[cr];
            int cls_sz = c.size();
            for (int j = 0; j < cls_sz; j++) {
                int cur_lit = toFormal(c[j]);
                ccnr._clauses[ct].literals.push_back(CCNR::lit(cur_lit, ct));
            }
            ct++;
        }
    }
    if (trail_lim.size() > 0) {
        int cls_sz = trail_lim[0];
        for (int i = 0; i < cls_sz; i++) {
            ccnr._clauses[ct].literals.push_back(CCNR::lit(toFormal(trail[i]), ct));
            ct++;
        }
    } else if (trail_lim.size() == 0) {
        int trl_sz = trail.size();
        for (int i = 0; i < trl_sz; i++) {
            ccnr._clauses[ct].literals.push_back(CCNR::lit(toFormal(trail[i]), ct));
            ct++;
        }
    }

    for (int i = 0; i < assumptions.size(); i++) {
        ccnr._clauses[ct].literals.push_back(CCNR::lit(toFormal(assumptions[i]), ct));
        ct++;
    }

    // set top level literals in SLS engine
    int topLevel = trail_lim.size() > 0 ? trail_lim[0] : trail.size();
    for (int i = 0; i < topLevel; ++i) {
        Var v = var(trail[i]);
        ls_mediation_soln[v] = (value(v) == l_True) ? 1 : 0;
    }
    // set assignment variables
    for (int i = 0; i < assumptions.size(); i++) {
        Var v = var(assumptions[i]);
        ls_mediation_soln[v] = (value(v) == l_True) ? 1 : 0;
    }

    for (size_t c = 0; c < ccnr._num_clauses; c++) {
        for (CCNR::lit item : ccnr._clauses[c].literals) {
            int v = item.var_num;
            ccnr._vars[v].literals.push_back(item);
        }
    }
    ccnr.build_neighborhood();


    bool res = false;
    if (use_up_build) { // do unit propagate negalate conflicts.

        // load init_soln use UP
        int var_nums = nVars();
        int t_sz = trail.size();
        int idx = qhead;

        int viewList_sz = t_sz - qhead;
        std::vector<Lit> viewList(var_nums + 2);
        for (int i = qhead; i < t_sz; ++i) viewList[i] = trail[i];

        int undef_nums = 0;
        std::vector<int> undef_vars(var_nums - t_sz + 2);
        std::vector<int> idx_undef_vars(var_nums + 1, -1); // undef_vars' idx is not -1
        for (int i = 0; i < var_nums; ++i)
            if (value(i) == l_Undef) {
                idx_undef_vars[i] = undef_nums;
                undef_vars[undef_nums++] = i;
            } else {
                ls_mediation_soln[i] = (value(i) == l_True) ? 1 : 0;
            }

        while (undef_nums > 0) {
            while (idx < viewList_sz && undef_nums > 0) {
                Lit p = viewList[idx++];

                vec<Watcher> &ws_bin = watches_bin[p];
                int ws_bin_sz = ws_bin.size();
                for (int k = 0; k < ws_bin_sz; k++) {
                    Lit the_other = ws_bin[k].blocker;
                    Var the_other_var = var(the_other);
                    if (idx_undef_vars[the_other_var] > -1) {
                        // no conflict and can decide.
                        ls_mediation_soln[the_other_var] = sign(the_other) ? 0 : 1;
                        viewList[viewList_sz++] = the_other;

                        int end_var = undef_vars[--undef_nums];
                        int idx_end_var = idx_undef_vars[the_other_var];
                        undef_vars[idx_end_var] = end_var;
                        idx_undef_vars[end_var] = idx_end_var;
                        idx_undef_vars[the_other_var] = -1;
                    }
                }
                if (undef_nums == 0) break;

                vec<Watcher> &ws = watches[p];
                Watcher *i, *j, *end;
                for (i = j = (Watcher *)ws, end = i + ws.size(); i != end;) {
                    // Make sure the false literal is data[1]:
                    CRef cr = i->cref;
                    Clause &c = ca[cr];
                    Lit false_lit = ~p;
                    if (c[0] == false_lit) c[0] = c[1], c[1] = false_lit;
                    i++;

                    // If 0th watch is true, then clause is already satisfied.
                    Lit first = c[0];
                    Var first_var = var(first);
                    Watcher w = Watcher(cr, first);
                    if (idx_undef_vars[first_var] == -1 && ls_mediation_soln[first_var] == (!sign(first))) {
                        *j++ = w;
                        continue;
                    }

                    int c_sz = c.size();
                    for (int k = 2; k < c_sz; ++k) {
                        Lit tmp_lit = c[k];
                        Var tmp_var = var(tmp_lit);
                        if (idx_undef_vars[tmp_var] == -1 && ls_mediation_soln[tmp_var] == sign(tmp_lit)) {
                        } else {
                            c[1] = c[k];
                            c[k] = false_lit;
                            watches[~c[1]].push(w);
                            // next clause
                            goto check_next_clause;
                        }
                    }
                    *j++ = w;
                    if (idx_undef_vars[first_var] == -1 && ls_mediation_soln[first_var] == sign(first)) {
                        // confliction bump
                        //?need to break or go on ?
                        continue;
                    } else {
                        // unit can assign
                        ls_mediation_soln[first_var] = sign(first) ? 0 : 1;
                        viewList[viewList_sz++] = first;

                        int end_var = undef_vars[--undef_nums];
                        int idx_end_var = idx_undef_vars[first_var];
                        undef_vars[idx_end_var] = end_var;
                        idx_undef_vars[end_var] = idx_end_var;
                        idx_undef_vars[first_var] = -1;
                    }
                check_next_clause:;
                }
                ws.shrink(i - j);
            }

            if (undef_nums == 0) break;

            // pick and assign
            // method 1: rand pick and rand assign
            int choosevar_idx = rand() % undef_nums;
            Var choosevar = undef_vars[choosevar_idx];
            Lit choose = mkLit(choosevar, polarity[choosevar]);

            ls_mediation_soln[choosevar] = sign(choose) ? 0 : 1;
            viewList[viewList_sz++] = choose;

            int end_var = undef_vars[--undef_nums];
            int idx_end_var = idx_undef_vars[choosevar];
            undef_vars[idx_end_var] = end_var;
            idx_undef_vars[end_var] = idx_end_var;
            idx_undef_vars[choosevar] = -1;
        }
        // call ccanr
        res = ccnr.local_search(&ls_mediation_soln);

    } else {
        // use total rand mod, however, respect top level literals and assumptions!
        // call ccanr use rand assign
        res = ccnr.local_search(&ls_mediation_soln);
    }


    // reload mediation soln
    for (int i = 0; i < ls_var_nums; ++i) {
        ls_mediation_soln[i] = ccnr._best_solution[i + 1];
    }

    int ls_unsat_back_num = ccnr._best_found_cost;
    if (ls_unsat_back_num <= ls_best_unsat_num) {
        for (int i = 0; i < ls_var_nums; ++i) ls_best_soln[i] = ls_mediation_soln[i];
        ls_best_unsat_num = ls_unsat_back_num;
    }

    if (res == true) {
        solved_by_ls = true;
    }
    return res;
}

Solver::UIPmini::UIPmini(bool uipAnalyze, bool uipMinimization)
  : i_uip(uipAnalyze)
  , i_mini(uipMinimization)
  , i_uip_decisions(0)
  , i_uip_attempts(0)
  , i_uip_global_decisions(0)
  , i_uip_global_dropped_lits(0)
  , i_uip_global_attempts(0)
  , i_uip_gap(0)
{
}


void Solver::UIPmini::restartUpdate()
{
    if (i_uip_attempts > 0 && ((double)i_uip_decisions / (double)i_uip_attempts) < 0.8) {
        i_uip_gap += 1;
    } else {
        i_uip_gap = (i_uip_gap == 0) ? 0 : i_uip_gap - 1;
    }
    i_uip_global_decisions += i_uip_decisions;
    i_uip_global_attempts += i_uip_attempts;
    i_uip_attempts = 0;
    i_uip_decisions = 0;
}

void Solver::UIPmini::prepareNextAnalysis()
{
    to_be_bumped.clear();
    to_be_cleaned.clear();
    clause_to_be_bumped.clear();
    resolved_away.clear();
    new_introduced.clear();
    new_analyze_toclear.clear();
    new_out_learnt.clear();
    i_uip_pathC.clear();
    top_i.clear();
}