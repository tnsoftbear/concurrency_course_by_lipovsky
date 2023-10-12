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

  void Schedule();

  // Task
  void Run();

  static Fiber* Self();

  void Ll(const char* format, ...);
  void Suspend();

 private:
  Scheduler& scheduler_;
  sure::Stack stack_;
  coro::Coroutine* coro_;
  //Routine routine_;
};

}  // namespace exe::fibers
