/****************************************************************************************[Proofs.h]
Copyright (c) 2021, Norbert Manthey
**************************************************************************************************/

#ifndef Proof_h
#define Proof_h

#include "core/SolverTypes.h"
// check generation of DRUP/DRAT proof on the fly
#include "core/OnlineProofChecker.h"
#include "mtl/Vec.h"
#include "utils/System.h"

#include <mutex>

namespace MERGESAT_NSPACE
{

/** Class that handles DRUP/DRAT proof generation.
 * Note: for DRAT clauses only the very first literal will be used for checking RAT!
 */
class Proof
{
    /// File handle where the proof is written to, might be NULL
    FILE *drup_file;

    /// Print binary proofs?
    bool binary_format;
    int verbosity;

    /// Data structures for binary proof
    int buf_len;
    unsigned char *drup_buf;
    unsigned char *buf_ptr;

    /// permanent unit clause vec for adding unit clauses
    vec<Lit> unitClause;

    /// multi processing supports multiple proof generating sub-proofs
    Proof *parent;          // use resources of parent for writing to proof, cache locally
    vec<Proof *> subproofs; // child proofs that contribute to this proof (and have this object as parent)
    std::mutex *file_mutex; // mutex to log the drup_file

    /// constants used during proof generation
    const int flush_limit = 1048576;
    const int buffer_size = 2097152;

    // Don't allow copying (error prone)
    Proof &operator=(Proof &other) = delete;
    Proof(const Proof &other) = delete;
    Proof(Proof &&other) = delete;

    public:
    Proof()
      : drup_file(NULL)
      , binary_format(true)
      , verbosity(0)
      , buf_len(0)
      , drup_buf(nullptr)
      , buf_ptr(nullptr)
      , parent(nullptr)
      , file_mutex(nullptr)
      , onlineDratChecker(nullptr)
    {
    }
    ~Proof();

    /// Setup structures for given input, return index of proof in parent proof (or 0)
    unsigned init(const char *drup_file_name, bool binary_format, int check_proof, Proof *parentproof = nullptr, bool register_with_parent = false);

    /// Add clause to proof
    template <class V> void addClause(unsigned char op, const V &c);
    void addUnitClause(unsigned char op, Lit l);
    /// Add clauses to show (x1 \lor x2) with UP(x1) -> implied and UP(x2) -> implied
    void addNecessaryAssignment(Lit x1, Lit x2, Lit implied);

    /// Add the strengthened clause to the proof (after removing literal l)
    void strengthenClause(const Clause &c, Lit l);

    /// add a clause of the initial formula
    void addParsedclause(const vec<Lit> &cls);

    /// for removed locked clauses, add the unit clause to the proof checker
    bool addLockedUnitForChecking(Lit implied);

    /// indicate whether we create a proof
    bool enabled() const;
    bool checkingEnabled() const;
    bool use_binary_format() const;

    /// helper for checking
    template <class T> bool proofHasClause(const T &cls);
    bool proofHasClause(const Lit &cls);

    /// flush current proof to file, consider subproofs as well
    void flush(bool flush_subproofs = true, FILE *locked_write_file = nullptr);

    /// flush proof to file, in case we used a file, and close the file
    void finalize();

    /// Set proof generation verbosity level
    void setVerbosity(int v);

    /// register a subproof (returning index to hint at during deregistering)
    int register_subproof(Proof *sub_proof);

    /// deregister a subproof, allow to bring a hint for removal, and present a replacement sub proof
    int deregister_subproof(Proof *sub_proof, int id_hint = -1, Proof *new_sub_proof = nullptr);

    protected:
    /// for now, make public to allow moving code step by step
    OnlineProofChecker *onlineDratChecker;

    /// Add literal to proof
    void writeProofLiteral(Lit l);

    /// remove all subproofs
    void clear_subproofs();

    /// Lock the lock of this proof (or it's transitive parent (i.e. form a tree, lock the root))
    void lock_write_file();

    /// Unlock the lock of this proof (or it's transitive parent (i.e. form a tree, lock the root))
    void unlock_write_file();

    /// Return the file handle of this proof, after locking
    FILE *get_locked_write_file();
};

inline Proof::~Proof() {}

inline void Proof::finalize()
{
    flush();

    if (drup_buf) free(drup_buf);
    drup_buf = nullptr;
    buf_ptr = nullptr;

    if (onlineDratChecker) delete onlineDratChecker;
    onlineDratChecker = nullptr;

    clear_subproofs();

    if (file_mutex) {
        delete file_mutex;
        file_mutex = nullptr;
    };

    if (drup_file) fclose(drup_file);
    drup_file = NULL;
}

inline unsigned Proof::init(const char *drup_file_name, bool binary_format = true, int check_proof = 0, Proof *parentproof, bool register_with_parent)
{
    if (verbosity > 2)
        std::cout << "c init proof with file " << drup_file_name << " in binary format "
                  << (binary_format ? "yes" : "no") << std::endl;
    assert((!register_with_parent || parentproof) && "Cannot register at parent without a proof");
    // no proof?
    if (!drup_file_name && !parentproof) return true;

    // memorize proof format
    this->binary_format = binary_format;

    // init BIN DRUP
    buf_len = 0;
    drup_buf = (unsigned char *)xrealloc(NULL, sizeof(unsigned char) * buffer_size);
    if (!drup_buf) {
        printf("c ERROR: failed to allocate buffer for proof, abort\n");
        assert(false && "should be able to allocate proof buffer");
        exit(1);
    }
    buf_ptr = drup_buf;

    // for subproof, only register with parent
    if (parentproof) {
        parent = parentproof;
        if (!register_with_parent) return 0;
        return parent->register_subproof(this);
    }

    // for actual proof, setup file and oneline checker
    drup_file = binary_format ? fopen(drup_file_name, "wb") : fopen(drup_file_name, "w");
    if (!drup_file) {
        printf("c ERROR: failed to open proof file '%s', abort\n", drup_file_name);
        assert(false && "should be able to open drup file");
        exit(1);
    }

    if (check_proof != 0) {
        assert(!parent && "do not check sub proofs, if there is a global proof");
        onlineDratChecker = new OnlineProofChecker(drupProof);
        if (onlineDratChecker)
            onlineDratChecker->setVerbosity(check_proof);
        else {
            printf("c ERROR: failed to allocate online DRAT checker, aborting\n");
            assert(false && "should be to allocate online checker");
            exit(1);
        }
    }

    return 0;
}

inline void Proof::addParsedclause(const vec<Lit> &cls)
{
    if (onlineDratChecker || (parent && parent->onlineDratChecker)) {
        OnlineProofChecker *checker = parent ? parent->onlineDratChecker : onlineDratChecker;
        assert(checker && "one of the two checkers has to exist");
        checker->addParsedclause(cls, parent != nullptr);
    }
}

inline bool Proof::addLockedUnitForChecking(Lit implied)
{
    if (onlineDratChecker || (parent && parent->onlineDratChecker)) {
        OnlineProofChecker *checker = parent ? parent->onlineDratChecker : onlineDratChecker;
        assert(checker && "one of the two checkers has to exist");
        return checker->addClause(implied, parent != nullptr);
    }

    // if we do not check the proof, just accept the clause
    return true;
}

inline bool Proof::enabled() const { return drup_file != NULL || parent != nullptr; }

inline bool Proof::checkingEnabled() const
{
    return enabled() && (onlineDratChecker || (parent && parent->onlineDratChecker));
}

inline bool Proof::use_binary_format() const { return binary_format; }

template <class T> inline bool Proof::proofHasClause(const T &cls)
{
    if (onlineDratChecker || (parent && parent->onlineDratChecker)) {
        OnlineProofChecker *checker = parent ? parent->onlineDratChecker : onlineDratChecker;
        assert(checker && "one of the two checkers has to exist");
        return checker->hasClause(cls, parent != nullptr);
    }
    return true;
}

inline bool Proof::proofHasClause(const Lit &cls)
{
    if (onlineDratChecker || (parent && parent->onlineDratChecker)) {
        OnlineProofChecker *checker = parent ? parent->onlineDratChecker : onlineDratChecker;
        assert(checker && "one of the two checkers has to exist");
        return checker->hasClause(cls, parent != nullptr);
    }
    return true;
}

inline void Proof::writeProofLiteral(Lit l)
{
    if (binary_format) {
        unsigned int u = 2 * (var(l) + 1) + sign(l);
        do {
            *buf_ptr++ = (u & 0x7f) | 0x80;
            buf_len++;
            u = u >> 7;
        } while (u);
        *(buf_ptr - 1) &= 0x7f; // End marker of this unsigned number.
    } else {
        int lit = toFormal(l);
        int bytes = snprintf((char *)buf_ptr, 21, "%d ", lit);
        assert(bytes > 0 && "bytes need to be written to proof");
        buf_len += bytes;
        buf_ptr += bytes;
    }
}

inline void Proof::addUnitClause(unsigned char op, Lit l)
{
    unitClause.clear();
    unitClause.push(l);
    addClause(op, unitClause);
}

inline void Proof::addNecessaryAssignment(Lit x1, Lit x2, Lit implied)
{
    if (!enabled()) return;
    unitClause.clear();

    // add 2 auxiliary binary clauses for x1 -> implied and x2 -> implied
    unitClause.push(implied);
    unitClause.push(~x1);
    addClause('a', unitClause);
    unitClause[1] = ~x2;
    addClause('a', unitClause);

    // add actual necessary assignment
    unitClause.clear();
    unitClause.push(implied);
    addClause('a', unitClause);

    // remove 2 auxiliary clauses again
    unitClause.push(~x1);
    addClause('d', unitClause);
    unitClause[1] = ~x2;
    addClause('d', unitClause);
}

template <class V> inline void Proof::addClause(unsigned char op, const V &c)
{
    if (!enabled()) return;

    if (verbosity > 2) std::cout << "c proof op " << op << " with cls " << c << std::endl;

    assert(op == 'a' || op == 'd');

    if (onlineDratChecker || (parent && parent->onlineDratChecker)) {
        OnlineProofChecker *checker = parent ? parent->onlineDratChecker : onlineDratChecker;
        assert(checker && "one of the checkers has to exist");
        if (op == 'a') {
            if (!checker->addClause(c, lit_Undef, false, parent != nullptr)) {
                std::cout << "c ERROR: failed to add clause " << c << " to proof" << std::endl;
                exit(134);
            }
        } else {
            if (!checker->removeClause(c, parent != nullptr)) exit(134);
        }
    }

    /* TODO a (sub)proof local cache could be added here, to reduce number of clauses
     * that ever hit the proof. Note: that would increase run time of the solver.
     */

    if (binary_format) {
        *buf_ptr++ = op;
        buf_len++;
    } else {
        if (op == 'd') {
            *buf_ptr++ = op;
            *buf_ptr++ = ' ';
            buf_len += 2;
        }
    }

    // add plain, in case we do not hit the buffer limit yet
    if (buf_len + c.size() * 5 <= 2 * buffer_size) {
        for (int i = 0; i < c.size(); i++) writeProofLiteral(c[i]);
    } else {
        int offset = 0;
        int batch_size = flush_limit / 5;
        int total_added_lits = 0;
        while (offset < c.size()) {
            batch_size = batch_size + offset > c.size() ? c.size() - offset : batch_size;
            for (int i = 0; i < batch_size; i++) {
                writeProofLiteral(c[offset + i]);
                total_added_lits++;
            }
            flush();
            offset += batch_size;
        }
        assert(c.size() == total_added_lits && "add full clause");
    }

    // finalize clause
    if (binary_format) {
        *buf_ptr++ = 0;
        buf_len++;
    } else {
        *buf_ptr++ = '0';
        *buf_ptr++ = '\n';
        buf_len += 2;
    }

    if (buf_len > flush_limit) flush();
}


inline void Proof::strengthenClause(const Clause &c, Lit l)
{
    if (!enabled()) return;

    if (verbosity > 2) std::cout << "c proof strengthen clause " << c << " by dropping literal " << l << std::endl;

    /* check with the active checker for the clause */
    if (onlineDratChecker || (parent && parent->onlineDratChecker)) {
        OnlineProofChecker *checker = parent ? parent->onlineDratChecker : onlineDratChecker;
        assert(checker && "one of the two checkers has to exist");
        if (!checker->addStrengthenedClause(c, l, false, parent != nullptr)) exit(134);
    }

    /* TODO: could use caching here as well */

    if (binary_format) {
        *buf_ptr++ = 'a';
        buf_len++;
    }

    if (buf_len + c.size() * 5 <= 2 * buffer_size) {
        for (int i = 0; i < c.size(); i++)
            if (c[i] != l) writeProofLiteral(c[i]);
    } else {
        int offset = 0;
        int batch_size = flush_limit / 5;
        int total_added_lits = 0;
        while (offset < c.size()) {
            batch_size = batch_size + offset > c.size() ? c.size() - offset : batch_size;
            for (int i = 0; i < batch_size; i++) {
                if (c[offset + i] != l) {
                    writeProofLiteral(c[offset + i]);
                    total_added_lits++;
                }
            }
            flush();
            offset += batch_size;
        }
        assert(c.size() == total_added_lits && "add full clause");
    }

    // finalize clause
    if (binary_format) {
        *buf_ptr++ = 0;
        buf_len++;
    } else {
        *buf_ptr++ = '0';
        *buf_ptr++ = '\n';
        buf_len += 2;
    }
    if (buf_len > flush_limit) flush();
}

inline void Proof::flush(bool flush_subproofs, FILE *locked_write_file)
{
    if (!enabled()) return;

    bool locked = false;
    if (locked_write_file == nullptr) {
        locked_write_file = drup_file;
        if (parent) {
            locked_write_file = parent->get_locked_write_file();
            locked = true;
        } else if (subproofs.size()) {
            /* this is the root object, lock once for all subproofs */
            lock_write_file();
            locked = true;
        }
    } else {
        /* we already received a locked file from the parent call */
    }

    if (flush_subproofs) {
        /* also flush all subproofs, with the 1 locked file we have */
        for (int i = 0; i < subproofs.size(); ++i) subproofs[i]->flush(true, locked_write_file);
    }

    if (verbosity > 2) std::cout << "c proof flushing " << buf_len << " bytes to file" << std::endl;

        /* We could add a LRU cache here, that understands the data, and caches duplicates before writing to disk.
           Or rework the proof interface to first store native clause representation in memory, use the LRU on there,
           and only flush to buffer/disk in case the cache is full.
        */
#if defined(__linux__)
    fwrite_unlocked(drup_buf, sizeof(unsigned char), buf_len, locked_write_file);
#else
    fwrite(drup_buf, sizeof(unsigned char), buf_len, locked_write_file);
#endif
    buf_ptr = drup_buf;
    buf_len = 0;

    if (locked) unlock_write_file();
}

inline void Proof::setVerbosity(int v)
{
    verbosity = v;
    for (int i = 0; i < subproofs.size(); ++i) subproofs[i]->setVerbosity(v);
}

inline void Proof::clear_subproofs() { subproofs.clear(); }

inline int Proof::register_subproof(Proof *sub_proof)
{
    int idx = subproofs.size();
    assert(sub_proof != nullptr);
    subproofs.push(sub_proof);

    /* only create a lock, once we actually get sub proofs */
    if (!file_mutex) {
        file_mutex = new std::mutex();
        assert(file_mutex);
    }
    return idx;
}

inline int Proof::deregister_subproof(Proof *sub_proof, int id_hint, Proof *new_sub_proof)
{
    assert(subproofs.size() > 0);
    assert(sub_proof != nullptr);

    bool matched = false;
    if (id_hint < subproofs.size() && subproofs[id_hint] == sub_proof) {
        matched = true;
    } else {
        for (id_hint = 0; id_hint < subproofs.size(); ++id_hint) {
            if (subproofs[id_hint] == sub_proof) {
                matched = true;
                break;
            }
        }
        assert(id_hint >= subproofs.size() || subproofs[id_hint] == sub_proof);
    }

    int new_idx = 0;
    if (matched) {
        if (new_sub_proof != nullptr) {
            subproofs[id_hint] = new_sub_proof;
            new_idx = id_hint;
        }
    } else {
        new_idx = subproofs.size();
        subproofs.push(new_sub_proof);
    }
    return new_idx;
}

inline void Proof::lock_write_file()
{
    if (parent) {
        parent->lock_write_file();
    } else {
        assert(file_mutex && "this method should only be called if we have a lock");
        file_mutex->lock();
    }
}

inline void Proof::unlock_write_file()
{
    if (parent) {
        parent->unlock_write_file();
    } else {
        assert(file_mutex && "this method should only be called if we have a lock");
        file_mutex->unlock();
    }
}

inline FILE *Proof::get_locked_write_file()
{
    if (parent) {
        return parent->get_locked_write_file();
    } else {
        lock_write_file();
        return drup_file;
    }
}

} // namespace MERGESAT_NSPACE

#endif
