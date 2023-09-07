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

class Mutex {
 public:
  void Lock() {
    uint32_t c = Status::Unlocked;
    m_.compare_exchange_strong(c, Status::LockedNoWaiters);
    if (c != Status::Unlocked) {
      do {
        if (c == Status::LockedWithWaiters) {
          twist::ed::futex::Wait(m_, Status::LockedWithWaiters);
        } else {
          uint32_t cc = Status::LockedNoWaiters;
          m_.compare_exchange_strong(cc, Status::LockedWithWaiters);
          if (cc != Status::Unlocked) {
            twist::ed::futex::Wait(m_, Status::LockedWithWaiters);
          }
        }

        c = Status::Unlocked;
        m_.compare_exchange_strong(c, Status::LockedWithWaiters);
      } while (c != Status::Unlocked);
    }
  }

  void Unlock() {
    if (m_.fetch_sub(1) != Status::LockedNoWaiters) {
      m_.store(Status::Unlocked);
      WakeKey k = twist::ed::futex::PrepareWake(m_);
      twist::ed::futex::WakeOne(k);
    }
  }

 private:
  twist::ed::stdlike::atomic<uint32_t> m_{Status::Unlocked};
  enum Status {
    Unlocked = 0,
    LockedNoWaiters = 1,
    LockedWithWaiters = 2
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