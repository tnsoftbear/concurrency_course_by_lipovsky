#pragma once

#include <exe/coro/core.hpp>

namespace exe::coro {

// Simple stackful coroutine

class SimpleCoroutine {
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
  sure::Stack stack_;
  Coroutine impl_;

 private:
  sure::Stack AllocateStack();
  void ReleaseResources();
};

}  // namespace exe::coro
