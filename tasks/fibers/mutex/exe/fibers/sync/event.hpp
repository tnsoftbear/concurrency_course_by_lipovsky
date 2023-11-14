#pragma once

#include <exe/support/queue.hpp>
#include <exe/support/msqueue.hpp>
#include <exe/fibers/core/fiber.cpp>
#include "exe/fibers/core/awaiter.hpp"
#include "exe/fibers/sched/yield.hpp"
#include "exe/fibers/sched/go.hpp"
#include "exe/threads/spinlock.hpp"
#include <twist/ed/wait/spin.hpp>
#include <twist/ed/stdlike/atomic.hpp>

namespace exe::fibers {

using WaitQueue = exe::support::MSQueue<Fiber*>;
twist::ed::SpinWait spin_wait;
using twist::ed::stdlike::atomic;
// One-shot

class Event {
 private:
  WaitQueue wait_q_;
  exe::threads::SpinLock lock_;
  atomic<bool> is_fired_{false};

  class EventAwaiter : public IAwaiter {
    private:
      Event& event_;
    public:
    explicit EventAwaiter(Event& host) : event_(host) {}
    void AwaitSuspend(Fiber* fiber) {
      event_.lock_.Lock();
      if (!event_.is_fired_.load()) {
        event_.Ll("AwaitSuspend: Before Put(), id: %lu", fiber->GetId());
        event_.wait_q_.Put(std::move(fiber));
        event_.lock_.Unlock();
        return;
      }
      event_.lock_.Unlock();

      fiber->Schedule();
    }
  };

 public:
  void Wait() {
    if (is_fired_.load()) {
      Ll("Wait: Don't sleep anymore");
      return;
    }

    Fiber* fiber = Fiber::Self();
    size_t id = fiber->GetId();
    Ll("Wait: Before Suspend(), id: %lu", id);
    EventAwaiter awaiter{*this};
    fiber->Suspend(&awaiter);
    Ll("Wait: ends, id: %lu", id);
  }

  void Fire() {
    lock_.Lock();
    is_fired_.store(true);
    lock_.Unlock();

    Ll("Fire: Before while, wait_q_.size: %lu", wait_q_.Count());
    while (!wait_q_.IsEmpty()) {
      std::optional<Fiber*> fiber_opt = wait_q_.Take();
      Fiber* fiber = std::move(fiber_opt.value());
      auto id = fiber->GetId();
      Ll("Fire: Before Schedule(), id: %lu", id);
      fiber->Schedule();
      Ll("Fire: Complete loop iteration, id: %lu", id);
    }
    Ll("Fire: ends");
  }

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
