#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>

#include <cstdint>

namespace stdlike {

using twist::ed::futex::WakeKey;
using twist::ed::futex::Wait;
using twist::ed::futex::PrepareWake;
using twist::ed::stdlike::atomic;

class Mutex {
 public:

  void lock() {  // NOLINT
    Lock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

  void Lock() {
    uint32_t c;
    if ((c = Cmpxchg(Status::Unlocked, Status::Locked)) != Status::Unlocked) {
      do {
        if (c == Status::Sleeping
          || Cmpxchg(Status::Locked, Status::Sleeping) != Status::Unlocked
        ) {
          Wait(m_, Status::Sleeping);
        }
      } while ((c = Cmpxchg(Status::Unlocked, Status::Sleeping)) != Status::Unlocked);
    }
  }

  void Unlock() {
    if (m_.exchange(Status::Unlocked) != Status::Locked) {
      WakeKey k = PrepareWake(m_);
      WakeOne(k);
    }
  }

 private:
  uint32_t Cmpxchg(uint32_t old_st, uint32_t new_st) {
    m_.compare_exchange_strong(old_st, new_st);
    return old_st;
  }

 private:
  atomic<uint32_t> m_{Status::Unlocked};
  enum Status {
    Unlocked = 0,
    Locked = 1,   // Mutex is locked without waiters
    Sleeping = 2  // Mutex is locked with waiters
  };
};

}  // namespace stdlike
