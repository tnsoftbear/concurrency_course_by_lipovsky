#pragma once

#include <exe/coro/core.hpp>
#include <function2/function2.hpp>
#include "exe/coro/runnable.hpp"

namespace exe::coro {

// Simple stackful coroutine

using Routine = fu2::unique_function<void()>;

class SimpleCoroutine : private IRunnable {
 public:
  explicit SimpleCoroutine(Routine routine);

  ~SimpleCoroutine();

  void Resume();

  // Suspend running coroutine
  static void Suspend();

  bool IsCompleted() const {
    return impl_.IsCompleted();
  }

  void Ll(const char* format, ...);

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
