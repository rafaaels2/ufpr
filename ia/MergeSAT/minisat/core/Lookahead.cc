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

#include "core/Lookahead.h"
#include "utils/Options.h"

#include <cassert>

using namespace MERGESAT_NSPACE;

// TODO: turn into object members in constructor
static const char *_cat = "LookAhead";
static IntOption opt_hscore_accuracy(_cat, "h-acc", "hScore accuracy; number of iterations", 3, IntRange(1, 32));
static IntOption opt_hscore_maxclause(_cat, "h-maxcl", "hScore max clause size", 7, IntRange(1, 32));
static IntOption opt_hscore_clause_weight(_cat, "h-cl-wg", "hScore clause weight", 5, IntRange(1, 32));
static DoubleOption
opt_hscore_clause_upperbound(_cat, "h-upper", "Upper bound for hscore of a literal", 10900, DoubleRange(0, false, INT32_MAX, true));
static DoubleOption
opt_hscore_clause_lowerbound(_cat, "h-lower", "lower bound for hscore of a literal", 0.1, DoubleRange(0, false, INT32_MAX, true));
static BoolOption opt_learn_lookahead_conflict(_cat, "la-learn", "Analyze conflicts of look ahead", true);


/** create the reduct struct, is allowed to swap all unwatched literals (index != 0,1) */
bool MERGESAT_NSPACE::Lookahead::create_reduct(CRef cr, ClauseReduct &reduct)
{
    Clause &c = solver.ca[cr];
    int i = 0;
    int j = c.size();
    bool checked_sat = false;
    if (solver.verbosity > 3) std::cout << "c create reduct for clause " << solver.ca[cr] << std::endl;
    for (; i < j; ++i) {
        if (solver.value(c[i]) == l_True) return false; // satisfied clause, ignore
        if (solver.value(c[i]) == l_False) {
            // check remaining literals for being true, once!
            if (!checked_sat) {
                for (int k = i + 1; k < j; ++k)
                    if (solver.value(c[k]) == l_True) return false;
            }
            checked_sat = true;
            do {
                --j;
            } while (solver.value(c[j]) == l_False && j > i + 1);

            Lit tmp = c[j];
            c[j] = c[i];
            c[i] = tmp;
        }
        // UP should have been executed until completion, without conflict!
        assert(i > 0 || solver.value(c[i]) != l_False);
    }
    assert(i > 1);
    reduct.size = i;
    reduct.cr = cr;
    if (solver.verbosity > 3)
        std::cout << "c reduct result (new size: " << reduct.size << ") " << solver.ca[cr] << std::endl;
    return true;
}

void MERGESAT_NSPACE::Lookahead::computePreScores()
{
    computedPreScores = true;
    vec<double> negHScore;
    vec<double> posHScore;
    vec<double> prevNegHScore;
    vec<double> prevPosHScore;

    if (solver.verbosity > 3) std::cout << "c LA compute pre scores" << std::endl;
    const Var nVars = solver.nVars();

    // init scores to 0 for all literals
    numLitTerClause.growTo(2 * nVars);
    for (Var v = 0; v < nVars; v++) {
        numLitTerClause[toInt(mkLit(v, true))] = 0;
        numLitTerClause[toInt(mkLit(v, false))] = 0;
    }

    // create initial counter and reduct
    reduct.clear();
    reduct.capacity(solver.clauses.size());
    for (int i = 0; i < solver.clauses.size(); i++) {
        ClauseReduct clause_reduct;
        if (!create_reduct(solver.clauses[i], clause_reduct)) continue; // clause is satisfied with current assignment
        reduct.push(clause_reduct);                                     // construct current reduct
        if (clause_reduct.size == 3) {
            Clause &c = solver.ca[clause_reduct.cr];
            numLitTerClause[toInt(c[0])]++;
            numLitTerClause[toInt(c[1])]++;
            numLitTerClause[toInt(c[2])]++;
        }
    }
    if (solver.verbosity > 1) std::cout << "c LA reduct size: " << reduct.size() << " clauses" << std::endl;

    // compute score per variable from formula
    prevNegHScore.growTo(nVars, 1.0f);
    prevPosHScore.growTo(nVars, 1.0f);
    double sum = 2 * nVars;
    vec<double> constant;
    for (int i = 1; i <= opt_hscore_accuracy; i++) {
        negHScore.clear();
        posHScore.clear();
        negHScore.growTo(nVars, 0.0f);
        posHScore.growTo(nVars, 0.0f);
        double avg = sum / (2 * nVars);
        constant.clear();
        constant.growTo(opt_hscore_maxclause + 1, 0.0f);
        for (int j = 2; j <= opt_hscore_maxclause; j++) {
            constant[j] = pow((int)opt_hscore_clause_weight, (int)opt_hscore_maxclause - j) / (pow(avg, (double)j - 1));
        }
        for (int k = 0; k < reduct.size(); k++) {
            const ClauseReduct clause_reduct = reduct[k];
            if (clause_reduct.size <= opt_hscore_maxclause) {
                double product = constant[clause_reduct.size];
                const Clause &c = solver.ca[clause_reduct.cr];
                for (int l = 0; l < clause_reduct.size; l++) {
                    product = product * (sign(c[l]) ? prevPosHScore[var(c[l])] : prevNegHScore[var(c[l])]);
                }
                for (int l = 0; l < clause_reduct.size; l++) {
                    if (sign(c[l])) {
                        negHScore[var(c[l])] += (product / prevNegHScore[var(c[l])]);
                    } else {
                        posHScore[var(c[l])] += (product / prevPosHScore[var(c[l])]);
                    }
                }
            }
        }
        if (i == opt_hscore_accuracy) { // skip the normalization in the last iteration
            break;
        }

        // normalize weights, and combine sum of all all scores, TODO: do this live above!
        sum = 0;
        for (Var k = 0; k < nVars; k++) {
            if (negHScore[k] < opt_hscore_clause_lowerbound) {
                negHScore[k] = opt_hscore_clause_lowerbound;
            }
            if (negHScore[k] > opt_hscore_clause_upperbound) {
                negHScore[k] = opt_hscore_clause_upperbound;
            }
            if (posHScore[k] < opt_hscore_clause_lowerbound) {
                posHScore[k] = opt_hscore_clause_lowerbound;
            }
            if (posHScore[k] > opt_hscore_clause_upperbound) {
                posHScore[k] = opt_hscore_clause_upperbound;
            }
            sum += negHScore[k] + posHScore[k];
        }
        // prepare next iteration
        negHScore.swap(prevNegHScore);
        posHScore.swap(prevPosHScore);
    }

    // collect all variables that are free, and have some score
    preScores.clear();
    for (Var i = 0; i < nVars; i++) {
        if (solver.value(i) != l_Undef || !solver.decision[i]) continue;
        if (solver.verbosity > 3)
            std::cout << "c LA pre-score check variable " << i << " with score p: " << posHScore[i]
                      << " n: " << negHScore[i] << std::endl;
        if (negHScore[i] >= 0 && posHScore[i] >= 0) {
            preScores.push(VarScore(i, negHScore[i] * posHScore[i] + log(negHScore[i] + 1) + log(posHScore[i] + 1)));
        }
    }
    sort(preScores);
    if (solver.verbosity > 3)
        std::cout << "c LA selected " << preScores.size() << " variables with pre scores" << std::endl;
}

MERGESAT_NSPACE::Lookahead::Lookahead(Solver &s)
  : stats_dlas(0), stats_dlaFailedLits(0), solver(s), computedPreScores(false)
{
}

void MERGESAT_NSPACE::Lookahead::collectPreselectVars(vec<Var> &vars, int candidatePoolSize, bool recomputePreScores)
{
    if (recomputePreScores || !computedPreScores) computePreScores();

    vars.clear();
    unsigned maxElements = candidatePoolSize > preScores.size() ? preScores.size() : candidatePoolSize;
    if (solver.verbosity > 3)
        std::cout << "c LA filter " << preScores.size() << " preselected variables, consider at most " << maxElements << std::endl;
    for (unsigned i = 0; i < maxElements; ++i) {
        Var v = preScores[i].getVar();
        if (solver.value(v) != l_Undef) continue;
        vars.push(v);
    }
    if (solver.verbosity > 3)
        std::cout << "c LA collected " << vars.size() << " pre-select variables (" << vars << ")" << std::endl;
}

void MERGESAT_NSPACE::Lookahead::collectDoubleLACandidates(Lit p)
{
    doubleQueue.clear();                   // clean member
    vec<Watcher> &ws = solver.watches[~p]; // Get clauses with 'p' in them
    Watcher *i, *end;
    assert(solver.value(p) != l_Undef);
    Lit freeLits[2];
    for (i = (Watcher *)ws, end = i + ws.size(); i != end; i++) {
        // Try to avoid inspecting the clause:
        const Lit blocker = i->blocker;
        if (solver.value(blocker) == l_True) continue;
        // Only use
        Clause &c = solver.ca[i->cref];
        if (c.size() != 3) continue;
        int freeLitIdx = 0;
        for (int j = 0; j < 3; j++) {
            if (solver.value(c[j]) == l_Undef) {
                if (freeLitIdx >= 2) break;
                freeLits[freeLitIdx++] = c[j];
            }
        }
        if (freeLitIdx == 2) { // only consider clauses whose reduct is binary
            for (int j = 0; j < freeLitIdx; j++) {
                if (doubleQueueSeen[var(freeLits[j])] == 0) {
                    doubleQueue.push(freeLits[j]);         // run look ahead with this literal
                    doubleQueueSeen[var(freeLits[j])] = 1; // set back to 0
                }
            }
        }
    }
}

/** create a clause "(decisions) -> l", add to solver, use to imply 'l' */
void MERGESAT_NSPACE::Lookahead::fixViaEnqueueWithDecisionClause(Lit l)
{
    assert(solver.value(l) == l_Undef);
    CRef implyReason = solver.createDecisionClause(l);
    solver.learnts_local.push(implyReason);
    solver.attachClause(implyReason);
    solver.proof.addClause('a', solver.ca[implyReason]);
    solver.uncheckedEnqueue(l, solver.decisionLevel(), implyReason);
}

CRef MERGESAT_NSPACE::Lookahead::runDoubleLookahaed(Lit p, Lit q)
{
    /** run DLA, might introduce clauses, imply literals, introduce non-chrono trail */

    // skip literals that have been enqueued by now
    if (solver.value(q) != l_Undef) return CRef_Undef;

    stats_dlas++;
    const int preLevel = solver.decisionLevel();
    for (int s = 0; s < 2; ++s) { // propagate both polarities
        Lit l = s == 0 ? q : ~q;
        assert(solver.decisionLevel() == preLevel);
        solver.newDecisionLevel();

        solver.newDecisionLevel();
        solver.uncheckedEnqueue(q, solver.decisionLevel(), CRef_Undef);
        CRef confl = solver.propagate();
        if (confl != CRef_Undef) {
            // TODO: could analyze confl, and use a proper reason + implied literal
            stats_dlaFailedLits++; // count failures due to double lookahaed
            solver.cancelUntil(preLevel, false, false);
            fixViaEnqueueWithDecisionClause(~l); // properly enqueue negation
            confl = solver.propagate();          // fix state on trail
            if (confl != CRef_Undef) return confl;
            return CRef_Undef;
        }
        // TODO: could collect trail for s==0, to run necessary literal detection

        solver.cancelUntil(preLevel, false, false);
    }

    return CRef_Undef; // we did not find an issue with LA p and q, return 'success'
}

void MERGESAT_NSPACE::Lookahead::cleanDLAqueue(int start)
{
    // finish to reset the queue
    for (int i = start; i < doubleQueue.size(); ++i) {
        // actually run double look ahead
        doubleQueueSeen[var(doubleQueue[i])] = 0;
    }
    doubleQueue.clear();
}


CRef MERGESAT_NSPACE::Lookahead::localLookAhead(Lit p, double &score, vec<Lit> &lookaheadTrail, vec<Lit> &implied, bool collectForDLA)
{
    const int trailStart = solver.trail.size();
    const int preLevel = solver.decisionLevel();
    solver.newDecisionLevel();
    solver.uncheckedEnqueue(p, solver.decisionLevel(), CRef_Undef);
    CRef confl = solver.propagate();
    if (confl != CRef_Undef) return confl; // look-ahead step failed, return detected conflict

    /** Are there literals already that should be used for double look ahead checking? */
    if (doubleQueue.size()) {
        int i = 0;
        for (; i < doubleQueue.size() && confl == CRef_Undef; ++i) {
            Lit q = doubleQueue[i];
            assert(doubleQueueSeen[var(q)] != 0);
            // on both polarities of the variable in case of success
            // create temporary reason clauses
            confl = runDoubleLookahaed(p, q);
        }
        cleanDLAqueue(i); // clean the queue
    }
    if (confl != CRef_Undef) return confl; // look-ahead step failed, return detected conflict

    /** collect for the other polarity? */
    if (collectForDLA) {
        doubleQueueSeen.growTo(solver.nVars(), 0); // initialize per literal
        collectDoubleLACandidates(p);              // writes to member doubleQueue and doubleQueueSeen
    }

    // copying the trail
    for (int j = trailStart; j < solver.trail.size(); j++) {
        lookaheadTrail.push(solver.trail[j]);
    }

    // TODO: check here for necessary assignments and equivalences, hand in other trail as additional parameter

    solver.cancelUntil(preLevel, false, false);
    return CRef_Undef;
}

Lit MERGESAT_NSPACE::Lookahead::lookaheadDecision(CRef &confl, int candidatePoolSize, bool recomputePreScores, bool useDLA)
{
    // parameters
    const double sizeScoreWeight = 0.3;
    const double binaryScoreWeight = 0.7;
    const int scoreMultipleFactor = 1024;

    if (solver.verbosity > 2) std::cout << "c perform LA decision with trail " << solver.trail << std::endl;

    vec<Var> candidateVars;
    collectPreselectVars(candidateVars, candidatePoolSize, recomputePreScores);
    if (solver.verbosity > 2)
        std::cout << "c collected " << candidateVars.size() << " candidate variables " << std::endl;

    double bestScore = 0;
    confl = CRef_Undef; // initially, we do not know about a conflict clause

    class LiteralScore
    {
        Lit lit;
        double score;

        public:
        LiteralScore() {}
        LiteralScore(Lit l, double s) : lit(l), score(s) {}
        Lit getLit() const { return lit; }
        double getScore() const { return score; }

        bool operator<(const LiteralScore &vs) const { return (score < vs.score); }
    };
    vec<LiteralScore> laScores; // keep all scores, in case we assign variables to fall back

    // keep these data structures around across all iterations
    vec<Lit> lookaheadTrail[2];
    vec<Lit> implied[2];
    Lit bestLit = lit_Error;
    double literalScores[2] = { 0 };
    for (int i = 0; i < candidateVars.size(); ++i) {
        Var v = candidateVars[i];
        if (solver.value(v) != l_Undef) continue;
        if (solver.verbosity > 2) std::cout << "c LA analyze variable " << mkLit(v, false) << std::endl;

        confl = CRef_Undef;
        for (int polarity = 0; polarity < 2; ++polarity) {
            double score = 0;
            lookaheadTrail[polarity].clear();
            implied[polarity].clear();
            Lit p = mkLit(v, polarity == 1); // Literal to test wrt look-ahead
            assert(solver.value(p) == l_Undef && "Can only check free literals");
            assert((polarity != 0 || doubleQueue.size() == 0) && "empty DLA queue before first attempt");
            // TODO: implement to use DLA on both polarities!
            confl = localLookAhead(p, score, lookaheadTrail[polarity], implied[polarity], useDLA && polarity == 0);
            if (solver.verbosity > 3) {
                std::cout << "c LA la with lit " << p << " results in conflict clause?: " << confl << std::endl;
            }
            if (confl != CRef_Undef) {
                cleanDLAqueue();
                return lit_Error;
            }

            // calculate score for the currently tested polarity
            const vec<Lit> &laTrail = lookaheadTrail[polarity];
            double binClauseLookahead = 0;
            for (int j = 0; j < laTrail.size(); j++) {
                binClauseLookahead += numLitTerClause[toInt(~laTrail[j])];
            }
            literalScores[polarity] = (sizeScoreWeight * laTrail.size()) + (binaryScoreWeight * binClauseLookahead);
        }

        // actually compute score for LA on the current variable v
        double score = scoreMultipleFactor * literalScores[0] * literalScores[1] + literalScores[0] + literalScores[1];
        // go after polarity that will cause more simplification
        Lit selectLit = mkLit(v, literalScores[1] >= literalScores[0]);
        laScores.push(LiteralScore(selectLit, score));
        if (score > bestScore) {
            bestLit = selectLit;
            bestScore = score;
        }
        if (solver.verbosity > 3) {
            std::cout << "c LA pushed literal " << selectLit << " with score " << score << " (best lit: " << bestLit
                      << " score: " << bestScore << ")" << std::endl;
        }
    }
    cleanDLAqueue();

    // return the best literal we detected
    if (bestLit != lit_Error && solver.value(bestLit) != l_Undef) return bestLit;

    // try to select another backup literal
    for (int i = 0; i < laScores.size(); ++i) {
        const Lit l = laScores[i].getLit();
        if (solver.value(l) != l_Undef) continue;
        if (bestLit == lit_Undef || laScores[i].getScore() > bestScore) {
            bestLit = l;
            bestScore = laScores[i].getScore();
        }
    }
    if (bestLit != lit_Error) return bestLit;

    // we failed to detect a literal, indicate rerun with re-precompute
    return lit_Undef;
}