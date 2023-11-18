#pragma once

#include "exe/support/msqueue.hpp"
#include "exe/fibers/core/fiber.cpp"
#include "exe/fibers/core/awaiter.hpp"
#include "exe/fibers/sched/yield.hpp"
#include "exe/threads/spinlock.hpp"

// std::lock_guard and std::unique_lock
#include <atomic>
#include <mutex>

namespace exe::fibers {

// Это решение не работает (падает на Stress_2 тесте)
// Оно здесь, чтобы остался классический алгоритм в Lock(), скопированный из обычного мьютекса.

using WaitQueue = exe::support::MSQueue<Fiber*>;

class Mutex {
 public:
  void Lock() {
    Fiber* fiber = Fiber::Self();
    auto id = fiber->GetId();
    Ll("Lock: starts, id: %lu", id);

    Status c;
    lock_.Lock();
    if ((c = Cmpxchg(Status::None, Status::LockedNoWaiter)) != Status::None) {
      do {
        if (c == Status::LockedWithWaiters
          || Cmpxchg(Status::LockedNoWaiter, Status::LockedWithWaiters) != Status::None
        ) {
          park_q_.Put(std::move(fiber));
          lock_.Unlock();
          DummyAwaiter awaiter{};
          fiber->Suspend(&awaiter);
          return;
        }
      } while ((c = Cmpxchg(Status::None, Status::LockedWithWaiters)) != Status::None);
    }
    lock_.Unlock();
  }

  void Unlock() {
    auto self_id = Fiber::Self()->GetId();
    // Ll("Unlock: starts by self-id: %lu", self_id);

    lock_.Lock();
    auto expected = Status::LockedNoWaiter;
    if (st_.compare_exchange_strong(expected, Status::None)) {
      // Ll("Unlock: LockedNoWaiter -> None, return, self-id: %lu", self_id);
      lock_.Unlock();
      return;
    }
    
    //Ll("Unlock: Before Take(), self-id: %lu", self_id);
    std::optional<Fiber*> fiber_opt = park_q_.Take();
    //Ll("Unlock: After Take(), self-id: %lu", self_id);
    lock_.Unlock();

    if (!fiber_opt.has_value()) {
      printf("Unlock: Unexpectedly empty queue (-> None), self-id: %lu", self_id);
      st_.store(Status::None);
      return;
    }

    Fiber* fiber = std::move(fiber_opt.value());
    //auto id = fiber->GetId();
    
    lock_.Lock();
    if (park_q_.IsEmpty()) {
      st_.store(Status::LockedNoWaiter);
      //Ll("Unlock: -> LockedNoWaiter, return, self-id: %lu, dequeued-id: %lu", self_id, id);
    } else {
      st_.store(Status::LockedWithWaiters);
      //Ll("Unlock: -> LockedWithWaiters, return, self-id: %lu, dequeued-id: %lu", self_id, id);
    }
    lock_.Unlock();

    //Ll("Unlock: Before fiber->Schedule(), self-id: %lu, dequeded-id: %lu", self_id, id);
    fiber->Schedule();
    //Ll("Unlock: ends, self-id: %lu, dequeded-id: %lu", self_id, id);
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
  exe::threads::SpinLock lock_;

  class DummyAwaiter : public IAwaiter {
    public:
      void AwaitSuspend(Fiber* fiber) {
        (void)fiber;
      }
  };

  Status Cmpxchg(Status old_st, Status new_st, std::memory_order mo = std::memory_order_seq_cst) {
    st_.compare_exchange_strong(old_st, new_st, mo);
    return old_st;
  }

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
