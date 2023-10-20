#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/spin.hpp>

namespace exe::threads {

using twist::ed::stdlike::atomic;

// Test-and-TAS spinlock

class SpinLock {
 public:
  void Lock() {
    while (locked_.exchange(true)) {
      twist::ed::CpuRelax();
    }
  }

  bool TryLock() {
    return !locked_.exchange(true);
  }

  void Unlock() {
    locked_.store(false);
  }

  // Lockable

  void lock() {  // NOLINT
    Lock();
  }

  bool try_lock() {  // NOLINT
    return TryLock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

 private:
  atomic<bool> locked_{false};
};

}  // namespace exe::threads
