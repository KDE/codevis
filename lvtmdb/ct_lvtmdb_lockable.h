// ct_lvtmdb_lockable.h                                               -*-C++-*-

/*
// Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#ifndef INCLUDED_CT_LVTMDB_LOCKABLE
#define INCLUDED_CT_LVTMDB_LOCKABLE

//@PURPOSE: Shared implementation of per-object reader-writer locking

#include <lvtmdb_export.h>

#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>

#ifndef NDEBUG
#define LVTMDB_LOCK_DEBUGGING
#endif

namespace Codethink::lvtmdb {

// ==========================
// class Lockable
// ==========================

class LVTMDB_EXPORT Lockable {
  private:
    // DATA
    std::unique_ptr<std::shared_mutex> d_mutex_p;
    // Read access through shared or exclusive lock
    // Write access *only* through exclusive lock
    // this applies to derived classes too
    // When locking two objects, the DbObject with the lowest address
    // should be locked first (see the dining philosopher's problem).

    // debugging stuff
#ifdef LVTMDB_LOCK_DEBUGGING
    std::atomic_int d_lockTracker = 0;
    //  n < 0: locked for writing
    // n == 0: unlocked
    //  n > 0: there are n readers

    void readLock()
    {
        int res = ++d_lockTracker;
        assert(res > 0);
        (void) res;
    }

    void readUnlock()
    {
        int res = --d_lockTracker;
        assert(res >= 0);
        (void) res;
    }

    void writeLock()
    {
        int res = --d_lockTracker;
        assert(res == -1);
        (void) res;
    }

    void writeUnlock()
    {
        int res = ++d_lockTracker;
        assert(res == 0);
        (void) res;
    }
#endif

  public:
    // TYPES
    class ROLock {
        std::shared_lock<std::shared_mutex> d_lock;
#ifdef LVTMDB_LOCK_DEBUGGING
        Lockable *d_source_p = nullptr;
#endif

      public:
        ROLock() = delete;
        // Don't create locks which do not own a mutex

        ROLock(std::shared_mutex& mutex, Lockable *source):
            d_lock(mutex) // mutex locked here
#ifdef LVTMDB_LOCK_DEBUGGING
            ,
            d_source_p(source)
#endif
        {
            (void) source;
#ifdef LVTMDB_LOCK_DEBUGGING
            if (d_source_p) {
                d_source_p->readLock();
            }
#endif
        }

        ROLock(ROLock&& other) noexcept:
            d_lock(std::move(other.d_lock))
#ifdef LVTMDB_LOCK_DEBUGGING
            ,
            d_source_p(other.d_source_p)
#endif
        {
#ifdef LVTMDB_LOCK_DEBUGGING
            // make sure we don't run the destructor twice
            other.d_source_p = nullptr;

            assert(d_lock.owns_lock());
            assert(!other.d_lock.owns_lock());
#endif
        }

        ~ROLock()
        {
#ifdef LVTMDB_LOCK_DEBUGGING
            if (d_source_p) {
                d_source_p->readUnlock();
            }
#endif
        } // mutex unlocked here

        ROLock& operator=(ROLock&& other) = delete;
        // We don't need this. Make it = delete because the default
        // implementation doesn't set other.d_souce_p = nullptr
    };

    class RWLock {
        std::unique_lock<std::shared_mutex> d_lock;
#ifdef LVTMDB_LOCK_DEBUGGING
        Lockable *d_source_p = nullptr;
#endif

      public:
        RWLock() = delete;
        // Don't create locks which do not own a mutex

        RWLock(std::shared_mutex& mutex, Lockable *source):
            d_lock(mutex) // mutex locked here
#ifdef LVTMDB_LOCK_DEBUGGING
            ,
            d_source_p(source)
#endif
        {
            (void) source;
#ifdef LVTMDB_LOCK_DEBUGGING
            if (d_source_p) {
                d_source_p->writeLock();
            }
#endif
        }

        RWLock(RWLock&& other) noexcept:
            d_lock(std::move(other.d_lock))
#ifdef LVTMDB_LOCK_DEBUGGING
            ,
            d_source_p(other.d_source_p)
#endif
        {
#ifdef LVTMDB_LOCK_DEBUGGING
            // make sure we don't run the destructor twice
            other.d_source_p = nullptr;

            assert(d_lock.owns_lock());
            assert(!other.d_lock.owns_lock());
#endif
        }

        ~RWLock()
        {
#ifdef LVTMDB_LOCK_DEBUGGING
            if (d_source_p) {
                d_source_p->writeUnlock();
            }
#endif
        } // mutex unlocked here

        RWLock& operator=(RWLock&& other) = delete;
        // We don't need this. Make it = delete because the default
        // implementation doesn't set other.d_source_p = nullptr
    };

    // CREATORS
    Lockable(): d_mutex_p(std::make_unique<std::shared_mutex>())
    {
    }

    virtual ~Lockable() noexcept = default;

    Lockable(Lockable&& other) noexcept: d_mutex_p(std::move(other.d_mutex_p))
    {
#ifdef LVTMDB_LOCK_DEBUGGING
        d_lockTracker.store(other.d_lockTracker.load());
#endif
    }

    Lockable& operator=(Lockable&& other) noexcept
    {
        d_mutex_p = std::move(other.d_mutex_p);
#ifdef LVTMDB_LOCK_DEBUGGING
        d_lockTracker.store(other.d_lockTracker.load());
#endif
        return *this;
    }

    // ACCESSORS
    void assertReadable() const
    {
#ifdef LVTMDB_LOCK_DEBUGGING
        assert(d_lockTracker.load() != 0);
#endif
    }

    void assertWritable() const
    {
#ifdef LVTMDB_LOCK_DEBUGGING
        assert(d_lockTracker.load() < 0);
#endif
    }

    // MODIFIERS
    [[nodiscard]] ROLock readOnlyLock()
    {
        return ROLock{*d_mutex_p, this};
    }

    [[nodiscard]] RWLock rwLock()
    {
        return RWLock{*d_mutex_p, this};
    }

    template<class LOCK>
    void withLock(std::function<void(void)>& fn)
    // Execute fn while holding the lock
    // e.g. foo.withLock<ROLock>([] { bar(); });
    {
        auto lock = LOCK{*d_mutex_p, this};
        (void) lock; // cppcheck
        fn();
    }

    inline void withROLock(std::function<void(void)> fn)
    {
        withLock<ROLock>(fn);
    }

    inline void withRWLock(std::function<void(void)> fn)
    {
        withLock<RWLock>(fn);
    }

    // CLASS METHODS
    [[nodiscard]] static std::pair<ROLock, ROLock> roLockTwo(Lockable *a, Lockable *b);
    // Lock two objects in a consistent order so as to avoid the dining
    // philosopher's problem

    [[nodiscard]] static std::pair<RWLock, RWLock> rwLockTwo(Lockable *a, Lockable *b);
    // Lock two objects in a consistent order so as to avoid the dining
    // philosopher's problem
};

} // namespace Codethink::lvtmdb

#endif // INCLUDED_CT_LVTMDB_LOCKABLE
