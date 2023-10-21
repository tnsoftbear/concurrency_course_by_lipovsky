#pragma once

#include <exe/executors/executor.hpp>

#include <exe/support/queue.hpp>
#include <memory>
#include <twist/ed/stdlike/atomic.hpp>
#include <exe/threads/spinlock.hpp>

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>
using twist::ed::stdlike::condition_variable;
using twist::ed::stdlike::mutex;

namespace exe::executors {

// Strand / serial executor / asynchronous mutex

using twist::ed::stdlike::atomic;
using exe::threads::SpinLock;

class Strand
  : public IExecutor
  , public std::enable_shared_from_this<Strand>
{
 public:
  explicit Strand(IExecutor& underlying);

  // Non-copyable
  Strand(const Strand&) = delete;
  Strand& operator=(const Strand&) = delete;

  // Non-movable
  Strand(Strand&&) = delete;
  Strand& operator=(Strand&&) = delete;

  ~Strand();

  // IExecutor
  void Submit(Task cs) override;
  
private:
  void Ll(const char* format, ...);

  // void operator()();

 private:
  IExecutor& underlying_;
  exe::support::UnboundedBlockingQueue<Task> tasks_;
  SpinLock lock_;
  atomic<bool> is_running_{false};
};

}  // namespace exe::executors
