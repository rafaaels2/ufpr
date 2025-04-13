/*************************************************************************************[LockGuard.h]
Copyright (c) 2022, Norbert Manthey
**************************************************************************************************/

#ifndef LockGuard_h
#define LockGuard_h

#include "mtl/XAlloc.h"
#include <mutex>

namespace MERGESAT_NSPACE
{

/** Allow to easily grab a lock for the life time of a function or code block.*/
class LockGuard
{
    std::mutex &lock;
    bool doUnlock;

    public:
    LockGuard(std::mutex &externLock, bool doLock) : lock(externLock), doUnlock(doLock)
    {
        if (doLock) lock.lock();
    }
    ~LockGuard()
    {
        if (doUnlock) lock.unlock();
    }
};

} // namespace MERGESAT_NSPACE

#endif
