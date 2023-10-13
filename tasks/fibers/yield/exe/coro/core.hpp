#pragma once

#include <sure/context.hpp>
#include <sure/stack.hpp>

#include <function2/function2.hpp>

#include <exception>
#include <stack>

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/atomic.hpp>
#include "sure/trampoline.hpp"

using twist::ed::stdlike::mutex;
using twist::ed::stdlike::atomic;

namespace exe::coro {

using Routine = fu2::unique_function<void()>;

class Coroutine : private sure::ITrampoline {
 public:

  explicit Coroutine(
    wheels::MutableMemView stack,
    Routine routine
  );

  void Resume();

  // Suspend running coroutine
  void Suspend();

  bool IsCompleted() const {
    return completed_;
  }

  void Ll(const char* format, ...);

 private:
  //IRunnable* runnable_;
  Routine routine_;
  sure::ExecutionContext context_;
  sure::ExecutionContext caller_context_;
  std::exception_ptr eptr_{nullptr};
  bool completed_{false};

 private:
  // ITrampoline
  void Run() noexcept override;
};

}  // namespace exe::coro
