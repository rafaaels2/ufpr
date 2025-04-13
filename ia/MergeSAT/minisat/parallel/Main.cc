/*****************************************************************************************[Main.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007,      Niklas Sorensson

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

#include <errno.h>

#include <signal.h>
#include <sys/resource.h>
#ifdef USE_LIBZ
#include <zlib.h>
#endif

#include "core/Dimacs.h"
#include "parallel/ParSolver.h"
#include "utils/Options.h"
#include "utils/ParseUtils.h"
#include "utils/System.h"

#include <sstream>

using namespace MERGESAT_NSPACE;

//=================================================================================================

void printStats(ParSolver &solver)
{
    solver.printStats();
    double wall_clock = wallClockTime();
    printf("c Wall time             : %g s\n", wall_clock);

    double mem_used = memUsedPeak();
    /* access memory access functions as part of CLI files only */
    if (mem_used != 0) printf("c Memory used           : %.2f MB\n", mem_used);
}


static ParSolver *solver = nullptr;
// Terminate by notifying the solver and back out gracefully. This is mainly to have a test-case
// for this feature of the Solver as it may take longer than an immediate call to '_exit()'.
static void default_signal_handler(int signalnr)
{
    static bool interrupted = false;
    solver->interrupt();

    int ret = 0;
    if (signalnr == SIGXCPU) ret = 124;
    if (signalnr == SIGSEGV) ret = 139;
    if (interrupted || ret != 0) {
        if (solver->verbose() > 0) {
            printf("c\nc *** caught signal %d ***\nc\n", signalnr);
            printStats(*solver);
            printf("c\n");
            printf("c *** INTERRUPTED ***\n");
        }
        _exit(ret);
    }

    interrupted = true;
}


// Note that '_exit()' rather than 'exit()' has to be used. The reason is that 'exit()' calls
// destructors and may cause deadlocks if a malloc/free function happens to be running (these
// functions are guarded by locks for multithreaded use).
static void SIGINT_exit(int)
{
    printf("\n");
    printf("c *** INTERRUPTED ***\n");
    if (solver->verbose() > 0) {
        printStats(*solver);
        printf("c\n");
        printf("c *** INTERRUPTED ***\n");
    }
    _exit(1);
}


// print pcs information into file
void print_pcs_file(const char *output_file_name)
{
    if (0 != (const char *)output_file_name) {
        FILE *pcsFile = fopen((const char *)output_file_name, "wb"); // open file
        if (!pcsFile) {
            printf("c error: could not open parameter file %s\n", output_file_name);
            exit(1);
        }
        fprintf(pcsFile, "# PCS Information for MergeSat\n#\n#\n# Parameters\n#\n#\n");
        ::printOptions(pcsFile);
        fprintf(pcsFile, "\n\n#\n#\n# Dependencies \n#\n#\n");
        ::printOptionsDependencies(pcsFile);
        fclose(pcsFile);
        exit(0);
    }
}

//=================================================================================================
// Main:

int main(int argc, char **argv)
{
    try {
        setUsageHelp("USAGE: %s [options] <input-file> <result-output-file>\n\n  where input may be either in plain or "
                     "gzipped DIMACS.\n");
        printf("c MergeSAT - Norbert Manthey, 2023.\n");

        // Extra options:
        //
        IntOption verb("MAIN", "verb", "Verbosity level (0=silent, 1=some, 2=more).", 0, IntRange(0, 7), false);
        BoolOption pre("MAIN", "pre", "Completely turn on/off any preprocessing.", true);
        BoolOption s_model("MAIN", "model", "Do report a model if the formula is satisfiable.", true, false);
        IntOption opt_model_width("MAIN", "model-width", "How many variables should on one model output line", 20,
                                  IntRange(0, INT32_MAX));
        StringOption dimacs("MAIN", "dimacs", "If given, stop after preprocessing and write the result to this file.");
        IntOption cpu_lim("MAIN", "cpu-lim", "Limit on CPU time allowed in seconds.\n", INT32_MAX, IntRange(0, INT32_MAX), false);
        IntOption mem_lim("MAIN", "mem-lim", "Limit on memory usage in megabytes.\n", INT32_MAX, IntRange(0, INT32_MAX), false);
        IntOption con_lim("MAIN", "con-lim", "Limit the number of conflicts for the search.\n", -1, IntRange(-1, INT32_MAX), false);
        BoolOption drup_binary("MAIN", "binary-proof", "Write proof in binary format.", true, false);
        StringOption drup_file("MAIN", "drup-file", "DRUP UNSAT proof ouput file.", "", false);
        IntOption opt_checkProofOnline("MAIN", "check-proof", "Check proof during run time", 0, IntRange(0, 10), false);
        StringOption pcs_file("MAIN", "pcs-file", "Print solver parameter configuration to this file.", "", false);

        IntOption opt_diversify_rank("MAIN", "diversify-rank", "Select a diversification rank to quickly test another configuration",
                                     0, IntRange(-1, INT32_MAX), false);
        IntOption opt_diversify_size("MAIN", "diversify-size", "Select a diversification size to quickly test another configuration",
                                     32, IntRange(1, INT32_MAX), false);

        parseOptions(argc, argv, true);

        if (!pcs_file.is_empty()) print_pcs_file(pcs_file);

        // Use signal handlers that forcibly quit until the solver will be able to respond to
        // interrupts:
        signal(SIGINT, SIGINT_exit);
        signal(SIGTERM, SIGINT_exit);
        signal(SIGXCPU, SIGINT_exit);

        // Set limit on CPU-time:
        if (cpu_lim != INT32_MAX) {
            rlimit rl;
            getrlimit(RLIMIT_CPU, &rl);
            if (rl.rlim_max == RLIM_INFINITY || (rlim_t)cpu_lim < rl.rlim_max) {
                rl.rlim_cur = cpu_lim;
                if (setrlimit(RLIMIT_CPU, &rl) == -1) printf("c WARNING! Could not set resource limit: CPU-time.\n");
            }
        }

        // Set limit on virtual memory:
        if (mem_lim != INT32_MAX) {
            rlim_t new_mem_lim = (rlim_t)mem_lim * 1024 * 1024;
            rlimit rl;
            getrlimit(RLIMIT_AS, &rl);
            if (rl.rlim_max == RLIM_INFINITY || new_mem_lim < rl.rlim_max) {
                rl.rlim_cur = new_mem_lim;
                if (setrlimit(RLIMIT_AS, &rl) == -1)
                    printf("c WARNING! Could not set resource limit: Virtual memory.\n");
            }
        }

        ParSolver S;
        S.set_verbosity(verb);
        double initial_time = cpuTime();

        if (!pre) S.eliminate(true);
        S.set_parsing(true);
        if (!pre) S.disable_simplification();
        if (strlen(drup_file)) {
            S.proof_init(drup_file, drup_binary, opt_checkProofOnline);
            S.proof_setVerbosity(verb);
        }
        solver = &S;

        if (argc == 1) printf("c Reading from standard input... Use '--help' for help.\n");

#ifdef USE_LIBZ
        gzFile in = (argc == 1) ? gzdopen(0, "rb") : gzopen(argv[1], "rb");
#else
        FILE *in = (argc == 1) ? stdin : open_to_read_file(argv[1]);
#endif
        if (in == NULL) printf("c ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]), exit(1);

        if (verb > 0) {
            printf("c ============================[ Problem Statistics ]=============================\n");
            printf("c |                                                                             |\n");
        }

        if (opt_diversify_rank >= 0) {
            if (verb > 0)
                printf("c |  Diversify with rank:  %12d size:  %12d                     |\n",
                       opt_diversify_rank % opt_diversify_size, (int)opt_diversify_size);
            S.diversify(opt_diversify_rank % opt_diversify_size, opt_diversify_size);
        }

        parse_DIMACS(in, S);
        fclose(in);
        FILE *res = (argc >= 3) ? fopen(argv[2], "wb") : NULL;

        if (verb > 0) {
            printf("c |  Number of variables:  %12d                                         |\n", S.nVars());
            printf("c |  Number of clauses:    %12d                                         |\n", (int)S.nClauses());
        }

        double parsed_time = cpuTime();
        if (verb > 0)
            printf("c |  Parse time:           %12.2f s                                       |\n", parsed_time - initial_time);

        // Change to signal-handlers that will only notify the solver and allow it to terminate
        // voluntarily:
        signal(SIGINT, default_signal_handler);
        signal(SIGTERM, default_signal_handler);
        signal(SIGXCPU, default_signal_handler);
        signal(SIGSEGV, default_signal_handler); /* we still want to understand the state */

        S.set_parsing(false);
        S.eliminate(true);
        double simplified_time = cpuTime();
        if (verb > 0) {
            printf("c |  Simplification time:  %12.2f s                                       |\n", simplified_time - parsed_time);
            printf("c |  Simplification accesses: %12" PRId64 "                                      |\n", S.simp_counter_sum());
            printf("c |                                                                             |\n");
        }

        if (!S.okay()) {
            if (res != NULL) fprintf(res, "s UNSATISFIABLE\n"), fclose(res);
            if (verb > 0) {
                printf("c ===============================================================================\n");
                printf("c Solved by simplification\n");
                printStats(S);
                printf("\n");
            }
            printf("s UNSATISFIABLE\n");
            S.proof_finalize(true);
            exit(20);
        }

        if (dimacs) {
            if (verb > 0) printf("c ==============================[ Writing DIMACS ]===============================\n");
            S.toDimacs((const char *)dimacs);
            if (verb > 0) printStats(S);
            exit(0);
        }

        if (con_lim != -1) {
            S.setConfBudget(con_lim);
        }
        vec<Lit> dummy;
        lbool ret = S.solveLimited(dummy);

        if (verb > 0) {
            printStats(S);
            printf("\n");
        }
        printf(ret == l_True ? "s SATISFIABLE\n" : ret == l_False ? "s UNSATISFIABLE\n" : "s UNKNOWN\n");
        if (ret == l_True && s_model) {
            std::stringstream model_stream;
            int vars_on_last_line = 0;
            printf("v");
            for (int i = 0; i < S.nVars(); i++) {
                if (S.modelValue(i) != l_Undef)
                    model_stream << " " << ((S.modelValue(i) == l_True) ? "" : "-") << (i + 1);
                // add a new line, if too many variables have been printed
                vars_on_last_line++;
                if (opt_model_width > 0 && vars_on_last_line >= opt_model_width) {
                    model_stream << std::endl << "v";
                    vars_on_last_line = 0;
                }
                if (model_stream.str().size() > 1024 * 1024 * 2) {
                    // print current part, and continue with cleared buffer
                    printf("%s", model_stream.str().c_str());
                    model_stream.str(std::string());
                }
            }
            printf("%s 0\n", model_stream.str().c_str());
        }

        S.proof_finalize(ret == l_False);

        if (res != NULL) {
            if (ret == l_True) {
                fprintf(res, "s SATISFIABLE\nv ");
                for (int i = 0; i < S.nVars(); i++)
                    if (S.modelValue(i) != l_Undef)
                        fprintf(res, "%s%s%d", (i == 0) ? "" : " ", (S.modelValue(i) == l_True) ? "" : "-", i + 1);
                fprintf(res, " 0\n");
            } else if (ret == l_False)
                fprintf(res, "s UNSATISFIABLE\n");
            else
                fprintf(res, "s UNKNOWN\n");
            fclose(res);
        }

#ifdef NDEBUG
        exit(ret == l_True ? 10 : ret == l_False ? 20 : 0); // (faster than "return", which will invoke the destructor for 'Solver')
#else
        return (ret == l_True ? 10 : ret == l_False ? 20 : 0);
#endif
    } catch (OutOfMemoryException &) {
        printf("c ===============================================================================\n");
        printf("c Out of memory\n");
        printf("s UNKNOWN\n");
        exit(0);
    }
}
