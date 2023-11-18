#pragma once

#include "exe/support/msqueue.hpp"
#include "exe/fibers/core/fiber.cpp"
#include "exe/fibers/core/awaiter.hpp"
#include "exe/threads/spinlock.hpp"

// std::lock_guard and std::unique_lock
#include <mutex>

namespace exe::fibers {

using WaitQueue = exe::support::MSQueue<Fiber*>;

class Mutex {
 public:
  void Lock() {
    LockAwaiter awaiter{*this};
    Fiber::Self()->Suspend(&awaiter);
  }

  void Unlock() {
    UnlockAwaiter awaiter{*this};
    Fiber::Self()->Suspend(&awaiter);
  }

  // BasicLockable

  void lock() {  // NOLINT
    Lock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

 private:
  WaitQueue park_q_;
  enum Status {
    None = 0,
    LockedNoWaiter = 1,
    LockedWithWaiters = 2,
  };
  atomic<Status> st_{Status::None};
  exe::threads::SpinLock mtx_;

  class LockAwaiter : public IAwaiter {
    private:
      Mutex& mutex_;
    public:
      explicit LockAwaiter(Mutex& host) : mutex_(host) {};
      void AwaitSuspend(Fiber* acquire_lock_fiber) {
        while (true) {
          auto expected = Status::None;
          if (mutex_.st_.compare_exchange_strong(expected, Status::LockedNoWaiter)) {
            acquire_lock_fiber->Run();
            return;
          }

          expected = Status::LockedNoWaiter;
          mutex_.mtx_.Lock();
          if (
            mutex_.st_.load() == Status::LockedWithWaiters
            || mutex_.st_.compare_exchange_strong(expected, Status::LockedWithWaiters)
          ) {
            mutex_.park_q_.Put(std::move(acquire_lock_fiber));
            mutex_.mtx_.Unlock();
            return;
          }
          mutex_.mtx_.Unlock();
        }
      }
  };

  class UnlockAwaiter : public IAwaiter {
    private:
      Mutex& mutex_;
    public:
      explicit UnlockAwaiter(Mutex& host) : mutex_(host) {};
      void AwaitSuspend(Fiber* release_lock_fiber) {
        auto expected = Status::LockedNoWaiter;
        if (mutex_.st_.compare_exchange_strong(expected, Status::None)) {
          release_lock_fiber->Run();
          return;
        }
        
        mutex_.mtx_.Lock();
        std::optional<Fiber*> unparked_fiber_opt = mutex_.park_q_.Take();
        Fiber* unparked_fiber = std::move(unparked_fiber_opt.value());
        mutex_.st_.store(mutex_.park_q_.IsEmpty()
          ? Status::LockedNoWaiter
          : Status::LockedWithWaiters
        );
        mutex_.mtx_.Unlock();

        unparked_fiber->Schedule();
        
        release_lock_fiber->Run();
      }
  };

  void Ll(const char* format, ...) {
    const bool k_should_print = false;
    //const bool k_should_print = true;
    if (!k_should_print) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s Mutex::%s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};

}  // namespace exe::fibers
