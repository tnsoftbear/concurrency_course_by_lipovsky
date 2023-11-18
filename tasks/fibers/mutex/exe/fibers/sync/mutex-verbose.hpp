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
    Fiber* fiber = Fiber::Self();
    auto id = fiber->GetId();
    Ll("Lock: starts, id: %lu", id);

    LockAwaiter awaiter{*this};
    fiber->Suspend(&awaiter);

    //Ll("Lock: ends, id: %lu", id);
  }

  void Unlock() {
    Fiber* fiber = Fiber::Self();
    UnlockAwaiter awaiter{*this};
    fiber->Suspend(&awaiter);
    // auto self_id = Fiber::Self()->GetId();
    // Ll("Unlock: starts by self-id: %lu", self_id);

    // mtx_.Lock();
    // auto expected = Status::LockedNoWaiter;
    // if (st_.compare_exchange_strong(expected, Status::None)) {
    //   Ll("Unlock: LockedNoWaiter -> None, return, self-id: %lu", self_id);
    //   mtx_.Unlock();
    //   return;
    // }
    
    // Ll("Unlock: Before Take(), self-id: %lu", self_id);
    // std::optional<Fiber*> fiber_opt = park_q_.Take();
    // Ll("Unlock: After Take(), self-id: %lu", self_id);
    // mtx_.Unlock();

    // if (!fiber_opt.has_value()) {
    //   Ll("Unlock: Unexpectedly empty queue (-> None), self-id: %lu", self_id);
    //   st_.store(Status::None);
    //   return;
    // }

    // Fiber* fiber = std::move(fiber_opt.value());
    // auto id = fiber->GetId();
    
    // mtx_.Lock();
    // if (park_q_.IsEmpty()) {
    //   st_.store(Status::LockedNoWaiter);
    //   Ll("Unlock: -> LockedNoWaiter, return, self-id: %lu, dequeued-id: %lu", self_id, id);
    // } else {
    //   st_.store(Status::LockedWithWaiters);
    //   Ll("Unlock: -> LockedWithWaiters, return, self-id: %lu, dequeued-id: %lu", self_id, id);
    // }
    // mtx_.Unlock();

    // Ll("Unlock: Before fiber->Schedule(), self-id: %lu, dequeded-id: %lu", self_id, id);
    // fiber->Schedule();
    // Ll("Unlock: ends, self-id: %lu, dequeded-id: %lu", self_id, id);
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
      void AwaitSuspend(Fiber* fiber) {
        auto id = fiber->GetId();
        while (true) {
          auto expected = Status::None;
          if (mutex_.st_.compare_exchange_strong(expected, Status::LockedNoWaiter)) {
            mutex_.Ll("LockAwaiter: acquired (None -> LockedNoWaiter), id: %lu", id);
            fiber->Run();
            return;
          }

          expected = Status::LockedNoWaiter;
          mutex_.mtx_.Lock();
          if (mutex_.st_.compare_exchange_strong(expected, Status::LockedWithWaiters)
            || mutex_.st_.load() == Status::LockedWithWaiters
          ) {
            mutex_.Ll("LockAwaiter: Put ( -> LockedWithWaiters), id: %lu", id);
            mutex_.park_q_.Put(std::move(fiber));
            mutex_.mtx_.Unlock();
            //mutex_.Ll("LockAwaiter: ends, id: %lu", id);
            return;
          }
          mutex_.mtx_.Unlock();
        }

        //fiber->Schedule();
      }
  };


  class UnlockAwaiter : public IAwaiter {
    private:
      Mutex& mutex_;
    public:
      explicit UnlockAwaiter(Mutex& host) : mutex_(host) {};
      void AwaitSuspend(Fiber* fiber) {
        auto self_id = Fiber::Self()->GetId();
        mutex_.Ll("Unlock: starts by self-id: %lu", self_id);

        mutex_.mtx_.Lock();
        auto expected = Status::LockedNoWaiter;
        if (mutex_.st_.compare_exchange_strong(expected, Status::None)) {
          mutex_.Ll("Unlock: LockedNoWaiter -> None, return, self-id: %lu", self_id);
          mutex_.mtx_.Unlock();
          fiber->Run();
          return;
        }
        
        mutex_.Ll("Unlock: Before Take(), self-id: %lu", self_id);
        std::optional<Fiber*> fiber_opt = mutex_.park_q_.Take();
        mutex_.Ll("Unlock: After Take(), self-id: %lu", self_id);
        mutex_.mtx_.Unlock();

        if (!fiber_opt.has_value()) {
          mutex_.Ll("Unlock: Unexpectedly empty queue (-> None), self-id: %lu", self_id);
          mutex_.st_.store(Status::None);
          fiber->Run();
          return;
        }

        Fiber* woken_fiber = std::move(fiber_opt.value());
        auto id = fiber->GetId();
        
        mutex_.mtx_.Lock();
        if (mutex_.park_q_.IsEmpty()) {
          mutex_.st_.store(Status::LockedNoWaiter);
          mutex_.Ll("Unlock: -> LockedNoWaiter, return, self-id: %lu, dequeued-id: %lu", self_id, id);
        } else {
          mutex_.st_.store(Status::LockedWithWaiters);
          mutex_.Ll("Unlock: -> LockedWithWaiters, return, self-id: %lu, dequeued-id: %lu", self_id, id);
        }
        mutex_.mtx_.Unlock();

        mutex_.Ll("Unlock: Before woken_fiber->Schedule(), self-id: %lu, dequeded-id: %lu", self_id, id);
        woken_fiber->Schedule();
        
        fiber->Run();
        //mutex_.Ll("Unlock: ends, self-id: %lu, dequeded-id: %lu", self_id, id);
      }
  };

  void Ll(const char* format, ...) {
    //const bool k_should_print = false;
    const bool k_should_print = true;
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
