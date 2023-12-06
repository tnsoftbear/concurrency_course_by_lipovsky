#pragma once

#include <exe/executors/executor.hpp>

#include <exe/support/msqueue.hpp>
#include <exe/threads/spinlock.hpp>
#include "exe/support/ref_counter.hpp"

namespace exe::executors {

// Strand / serial executor / asynchronous mutex

using exe::threads::SpinLock;
using twist::ed::stdlike::atomic;
using TaskQueue = exe::support::MSQueue<Task>;

class Strand : public IExecutor {
 public:
  explicit Strand(IExecutor& underlying);

  // Non-copyable
  Strand(const Strand&) = delete;
  Strand& operator=(const Strand&) = delete;

  // Non-movable
  Strand(Strand&&) = delete;
  Strand& operator=(Strand&&) = delete;

  void Submit(Task cs) override;

 private:
  void Ll(const char* format, ...);
  void SubmitSelf();
  size_t RunTasks(TaskQueue& processing_tasks);
  void Run();

 private:
  IExecutor& underlying_;
  TaskQueue tasks_;
  SpinLock lock_;
  RefCounter* cnt_;
};

}  // namespace exe::executors
