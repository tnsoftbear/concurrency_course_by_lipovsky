#pragma once

#include <exe/support/msqueue.hpp>
#include <exe/fibers/core/fiber.cpp>
#include "exe/fibers/sched/yield.hpp"
#include "exe/threads/spinlock.hpp"

// std::lock_guard and std::unique_lock
#include <mutex>

namespace exe::fibers {

using WaitQueue = exe::support::MSQueue<Fiber*>;

class Mutex {
 public:
  void Lock() {
    if (!is_locked_.load()) {
      is_locked_.store(true);
      return;
    }
    Fiber* fiber = Fiber::Self();
    auto id = fiber->GetId();
    fiber->MarkSleep();
    Ll("Lock: Before Put(), id: %lu", id);
    lock_.Lock();
    wait_q_.Put(std::move(fiber));
    lock_.Unlock();
    Ll("Lock: Before Yield(), id: %lu", id);
    Yield();
    Ll("Lock: ends, id: %lu", id);
  }

  void Unlock() {
    lock_.Lock();
    if (wait_q_.IsEmpty()) {
      is_locked_.store(false);
      lock_.Unlock();
      return;
    }
    std::optional<Fiber*> fiber_opt = wait_q_.Take();
    lock_.Unlock();
    Fiber* fiber = std::move(fiber_opt.value());
    auto id = fiber->GetId();
    Ll("Unlock: Before WakeAndRun(), id: %lu", id);
    fiber->WakeAndRun();
    Ll("Unlock: ends, id: %lu", id);
  }

  // BasicLockable

  void lock() {  // NOLINT
    Lock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

 private:
  WaitQueue wait_q_;
  atomic<bool> is_locked_{false};
  exe::threads::SpinLock lock_;

  void Ll(const char* format, ...) {
    const bool k_should_print = true;
    if (!k_should_print) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s Event::%s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};

}  // namespace exe::fibers
