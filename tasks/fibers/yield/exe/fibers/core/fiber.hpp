#pragma once

#include <exe/coro/simple.hpp>
#include <exe/fibers/core/routine.hpp>
#include <exe/fibers/core/scheduler.hpp>


namespace exe::fibers {

// Fiber = stackful coroutine + scheduler (thread pool)

enum Status {
  Runable = 0,
  Running = 1,
  Suspended = 2,
  Completed = 3
};

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

  void SetStatus(Status status) {
    status_.store(status);
  }

  Status GetStatus() const {
    return status_.load();
  }

 private:
  Scheduler& scheduler_;
  coro::SimpleCoroutine* coro_;
  atomic<Status> status_{Status::Runable};

};

}  // namespace exe::fibers
