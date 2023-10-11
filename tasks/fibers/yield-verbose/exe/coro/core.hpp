#pragma once

#include <sure/context.hpp>
#include <sure/stack.hpp>

#include <function2/function2.hpp>

#include <exception>
// #include "sure/stack/mmap.hpp"
#include <stack>

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/atomic.hpp>
#include "exe/coro/runnable.hpp"
#include "sure/trampoline.hpp"

using twist::ed::stdlike::mutex;
using twist::ed::stdlike::atomic;

namespace exe::coro {

void Ll(const char* format, ...);

enum Status {
  Runable = 0,
  Running = 1,
  Suspended = 2,
  Completed = 3
};

class Coroutine : private sure::ITrampoline {
 public:

  explicit Coroutine(
    wheels::MutableMemView stack,
    IRunnable* runnable
  );

  ~Coroutine();

  void Resume();

  // Suspend running coroutine
  static void Suspend();

  bool IsCompleted() const {
    return GetStatus() == Status::Completed;
  }

  void Ll(const char* format, ...);

  size_t GetId() {
    return id_;
  };

  void SetStatus(Status status) {
    status_.store(status);
  }

  Status GetStatus() const {
    return status_.load();
  }

 private:
  IRunnable* runnable_;

  sure::ExecutionContext context_;
  sure::ExecutionContext caller_context_;
  atomic<Status> status_{Status::Runable};
  std::exception_ptr eptr_;
  atomic<size_t> id_{0};

 private:
  // ITrampoline
  void Run() noexcept override;
  void SwitchToScheduler();
};

}  // namespace exe::coro
