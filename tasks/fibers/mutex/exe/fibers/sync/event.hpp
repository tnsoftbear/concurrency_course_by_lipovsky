#pragma once

#include <twist/ed/stdlike/atomic.hpp>

#include "exe/support/msqueue.hpp"
#include "exe/fibers/core/fiber.cpp"
#include "exe/fibers/core/awaiter.hpp"
#include "exe/threads/spinlock.hpp"

namespace exe::fibers {

using WaitQueue = exe::support::MSQueue<Fiber*>;
twist::ed::SpinWait spin_wait;
using twist::ed::stdlike::atomic;
// One-shot

class Event {

 public:
  void Wait() {
    /**
     * Awaiter - это способ выполнить действия не до вызова Suspend(), находясь в контексте пользовательской рутины,
     * а после высзова Suspend(), когда управление передаётся контексту коллера, который вызывает файбер Run()
     */
    EventAwaiter awaiter{*this};
    Fiber::Self()->Suspend(&awaiter);
  }

  void Fire() {
    lock_.Lock();
    is_fired_.store(true);
    bool is_empty = wait_q_.IsEmpty();
    lock_.Unlock();

    while (!is_empty) {
      std::optional<Fiber*> fiber_opt = wait_q_.Take();
      is_empty = wait_q_.IsEmpty();
      Fiber* fiber = std::move(fiber_opt.value());
      fiber->Schedule();
    }
  }

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
        event_.wait_q_.Put(std::move(fiber));
        event_.lock_.Unlock();
        return;
      }
      event_.lock_.Unlock();

      fiber->Schedule();
    }
  };
  
 private:
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
