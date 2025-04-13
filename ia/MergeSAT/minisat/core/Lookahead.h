/**************************************************************************************************
MiniSat -- Copyright (c) 2022, Norbert Manthey

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

#ifndef MergeSat_Lookahead_h
#define MergeSat_Lookahead_h

#include "core/Solver.h"

namespace MERGESAT_NSPACE
{

class Lookahead
{
    // private data structures to be used over multiple calls
    vec<Lit> learnt_clause_lits;

    uint64_t stats_dlas, stats_dlaFailedLits;

    Solver &solver; // handle to solver to be used durnig operations

    /** score a given variable, support sorting */
    class VarScore
    {
        Var var;
        double score;

        public:
        VarScore() {}
        VarScore(Var v, double s) : var(v), score(s) {}
        Var getVar() const { return var; }

        bool operator<(const VarScore &vs) const { return (score < vs.score); }
    };
    vec<VarScore> preScores;  // preselection scores per variable
    vec<int> numLitTerClause; // number of ternary clauses with a given literal

    vec<Lit> doubleQueue;      // Literals to be considered during double look ahead
    vec<char> doubleQueueSeen; // Flags to be used to de-duplicate literals on queue

    /** represent formula wrt current solver assignment */
    struct ClauseReduct {
        CRef cr;
        int size;
    };
    vec<ClauseReduct> reduct;
    bool computedPreScores; // indicate whether we precomputed scores at least once

    void computePreScores();
    void collectPreselectVars(vec<Var> &vars, int candidatePoolSize, bool recomputePreScores = false);

    /** create the reduct struct, is allowed to swap all unwatched literals (index != 0,1) */
    bool create_reduct(CRef cr, ClauseReduct &reduct);

    /** create a clause "(decisions) -> l", add to solver, use to imply 'l' */
    void fixViaEnqueueWithDecisionClause(Lit l);

    /** run analysis on 2nd level during double lookahead for literal q, for LA of p */
    CRef runDoubleLookahaed(Lit p, Lit q);

    /** run look-ahead for given literal p, return a conflict clause if p is a conflicting literal, or CRef_Undef */
    CRef localLookAhead(Lit p, double &score, vec<Lit> &lookaheadTrail, vec<Lit> &impliedLits, bool collectForDLA = false);

    /** fill doubleQueue with literals relevant for double-LA after propagating p, to be used on ~p for DLA! */
    void collectDoubleLACandidates(Lit p);

    /** clear DLA queue structure again*/
    void cleanDLAqueue(int start = 0);

    public:
    Lookahead(Solver &s);

    /** From the available variables, select a literal via look-ahead, or return a detected conflict
     *
     *  Returns lit_Error, in case of an error - sets confl to a conflict clause, or that not literal
     *                     could be detected.
     *          lit_Undef, LA failed to detect a suitable literal, because either none is available,
     *                     or recomputePreScores needs to be set to true
     *          literal    in case this literal can be used as decision literal
     *  Note: using double lookahead might introduce additional temporary clauses to the solver
     */
    Lit lookaheadDecision(CRef &confl, int candidatePoolSize = 5, bool recomputePreScores = false, bool useDLA = false);
};

//=================================================================================================
} // namespace MERGESAT_NSPACE

#endif
