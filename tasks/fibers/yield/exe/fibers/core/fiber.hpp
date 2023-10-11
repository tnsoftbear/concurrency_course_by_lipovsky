#pragma once

#include <exe/coro/simple.hpp>
#include <exe/fibers/core/routine.hpp>
#include <exe/fibers/core/scheduler.hpp>


namespace exe::fibers {

// Fiber = stackful coroutine + scheduler (thread pool)

class Fiber {
 public:
  Fiber(Scheduler& scheduler, Routine routine);

  ~Fiber();

  void Schedule();

  // Task
  void Run();

  static Fiber* Self();

  void Ll(const char* format, ...);
  Scheduler& GetScheduler() {
    return scheduler_;
  };
  bool IsSuspended() const;

 private:
  Scheduler& scheduler_;
  coro::SimpleCoroutine* coro_;
};

}  // namespace exe::fibers
