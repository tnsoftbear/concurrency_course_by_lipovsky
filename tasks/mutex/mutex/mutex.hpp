#pragma once

#include <atomic>
#include <cstdint>
#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>

#include <cstdlib>
#include "twist/rt/layer/facade/wait/futex.hpp"

#include <iostream>

namespace stdlike {

using twist::ed::futex::WakeKey;
using twist::ed::futex::Wait;
using twist::ed::futex::PrepareWake;
using twist::ed::stdlike::atomic;

class Mutex {
 public:
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




// class Mutex {
//  public:
//   void Lock() {
//     uint32_t c;
//     while ((c = m_.fetch_add(1)) != 0) {
//       m_.wait(c + 1);
//     }
//   }

//   void Unlock() {
//     m_.store(0);
//     m_.notify_one();
//   }

//  private:
//   std::atomic<uint32_t> m_{0};
// };




// using twist::ed::futex::WakeKey;

// class Mutex {
//  public:
//   void Lock() {
//     uint32_t c;
//     while ((c = m_.fetch_add(1, std::memory_order_acquire)) != 0) {
//       twist::ed::futex::Wait(m_, c + 1, std::memory_order_relaxed);
//     }
//   }

//   void Unlock() {
//     if (m_.load(std::memory_order_relaxed) > 0) {
//       WakeKey k = twist::ed::futex::PrepareWake(m_);
//       m_.store(0, std::memory_order_release);
//       twist::ed::futex::WakeOne(k);
//     }
//   }

//  private:
//   twist::ed::stdlike::atomic<uint32_t> m_{0};
// };




// class Mutex {
//  public:
//   void Lock() {
//     uint32_t c;
//     while ((c = m_.fetch_add(1)) != 0) {
//       std::cout << "Wait c: " << c << std::endl;
//       twist::ed::futex::Wait(m_, c + 1);
//     }
//   }

//   void Unlock() {
//     uint32_t c;
//     if ((c = m_.exchange(0)) > 1) {
//       std::cout << "Wake c: " << c << std::endl;
//       WakeKey k = twist::ed::futex::PrepareWake(m_);
//       twist::ed::futex::WakeOne(k);
//     }
//   }

//  private:
//   twist::ed::stdlike::atomic<uint32_t> m_{0};
// };