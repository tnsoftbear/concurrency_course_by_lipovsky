#pragma once

#include <exe/coro/core.hpp>
#include <function2/function2.hpp>
#include "exe/coro/runnable.hpp"

namespace exe::coro {

// Simple stackful coroutine

// using SimpleCoroutine = Coroutine;

using Routine = fu2::unique_function<void()>;

class SimpleCoroutine : private IRunnable {
 public:
  explicit SimpleCoroutine(Routine routine);

  ~SimpleCoroutine();

  void Resume();

  // Suspend running coroutine
  static void Suspend();

  bool IsCompleted() const {
    return GetStatus() == Status::Completed;
  }

  //void ReleaseResources();
  void Ll(const char* format, ...);
  //void SwitchToScheduler();

  size_t GetId() {
    return impl_.GetId();
  };

  void SetStatus(Status status) {
    impl_.SetStatus(status);
  }

  Status GetStatus() const {
    return impl_.GetStatus();
  }

 private:
  Routine routine_;
  sure::Stack stack_;
  Coroutine impl_;

 private:
  void RunCoro();
  sure::Stack AllocateStack();
  void ReleaseResources();
};

}  // namespace exe::coro
