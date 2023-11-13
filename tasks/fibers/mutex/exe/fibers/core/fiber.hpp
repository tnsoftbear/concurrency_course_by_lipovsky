#pragma once

#include <exe/coro/core.hpp>
#include <exe/fibers/core/routine.hpp>
#include <exe/fibers/core/scheduler.hpp>
#include "exe/fibers/core/awaiter.hpp"

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
  void Switch();
  void Suspend(IAwaiter* awaiter);

  void Ll(const char* format, ...);
  size_t GetId() { return id_; }
 
  Scheduler& GetScheduler() {
    return scheduler_;
  }

 public:
  std::string name;
  
 private:
  Scheduler& scheduler_;
  sure::Stack stack_;
  coro::Coroutine* coro_;
  size_t id_{0};
  IAwaiter* awaiter_{nullptr};
};

}  // namespace exe::fibers
