#pragma once

#include <exe/executors/executor.hpp>

#include <exe/support/msqueue.hpp>

#include <exe/support/refer/ref_counted.hpp>

#include <twist/ed/stdlike/atomic.hpp>

namespace exe::executors {

// Strand / serial executor / asynchronous mutex

using twist::ed::stdlike::atomic;
using TaskQueue = exe::support::MSQueue<Task>;

class Strand : public IExecutor
{
 private:
  class Impl : public refer::RefCounted<Impl> {
   public:
    explicit Impl(IExecutor& executor);
    void Submit(Task task);
    void Run() noexcept;
   private:
    void SubmitSelf();
    static size_t RunTasks(TaskQueue& processing_tasks);
    void Ll(const char* format, ...);

   private:
    atomic<size_t> scheduled_{0};
    IExecutor& underlying_;
    TaskQueue tasks_;
  };

 public:
  explicit Strand(IExecutor& executor)
    : impl_(refer::New<Impl>(executor)) {};

  // Non-copyable
  Strand(const Strand&) = delete;
  Strand& operator=(const Strand&) = delete;

  // Non-movable
  Strand(Strand&&) = delete;
  Strand& operator=(Strand&&) = delete;

  void Submit(Task task) override {
    return impl_->Submit(std::move(task));
  }  
  
 private:
  refer::Ref<Impl> impl_;

};

}  // namespace exe::executors
