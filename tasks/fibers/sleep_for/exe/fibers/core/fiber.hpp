#pragma once

#include <exe/coro/core.hpp>
#include <exe/fibers/core/routine.hpp>
#include <exe/fibers/core/scheduler.hpp>

using Routine = fu2::unique_function<void()>;

namespace exe::fibers {

// Fiber = stackful coroutine + scheduler (thread pool)

class Fiber {
 public:
  Fiber(Scheduler& scheduler, Routine routine);
  ~Fiber();

  void Schedule();
  void Run();
  static Fiber* Self();

  void MarkSleep();
  void WakeAndRun();
  void Suspend();
  void Ll(const char* format, ...);

  Scheduler& GetScheduler() {
    return scheduler_;
  }

 public:
   enum Status {
    Runnable = 0,
    Running = 1,
    Sleeping = 2,
  };

 private:
  Scheduler& scheduler_;
  sure::Stack stack_;
  coro::Coroutine* coro_;
  Status status_{Status::Runnable};
  size_t id_{0};
};

}  // namespace exe::fibers
