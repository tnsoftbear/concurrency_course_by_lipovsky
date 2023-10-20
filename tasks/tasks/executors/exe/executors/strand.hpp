#pragma once

#include <exe/executors/executor.hpp>

#include <exe/support/queue.hpp>
#include <twist/ed/stdlike/atomic.hpp>
#include <exe/threads/spinlock.hpp>

namespace exe::executors {

// Strand / serial executor / asynchronous mutex

using twist::ed::stdlike::atomic;
using exe::threads::SpinLock;

class Strand : public IExecutor 
{
 public:
  explicit Strand(IExecutor& underlying);

  // Non-copyable
  Strand(const Strand&) = delete;
  Strand& operator=(const Strand&) = delete;

  // Non-movable
  Strand(Strand&&) = delete;
  Strand& operator=(Strand&&) = delete;

  // IExecutor
  void Submit(Task cs) override;
  
private:
  void Ll(const char* format, ...);

  // void operator()();

 private:
  IExecutor& underlying_;
  //Task all_in_one_;
  // atomic<bool> is_completed_{false};
  // int completed_no_{-1};
  // int submitted_no_{-1};
  exe::support::UnboundedBlockingQueue<Task> tasks_;
  SpinLock lock_;
  SpinLock lock2_;
  atomic<bool> is_running_{false};
};

}  // namespace exe::executors
