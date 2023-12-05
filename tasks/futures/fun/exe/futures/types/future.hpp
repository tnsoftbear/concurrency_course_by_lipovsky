#pragma once

#include <cstdlib>
#include <utility>

#include <exe/futures/state/shared_state.hpp>
#include "exe/executors/inline.hpp"
using exe::futures::details::SharedState;

#include <exe/executors/executor.hpp>
#include <exe/futures/state/callback.hpp>
using exe::futures::details::Callback;

namespace exe::futures {

template <typename T>
struct [[nodiscard]] Future {
  using ValueType = T;

  // Non-copyable
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  // Non-copy-assignable
  Future& operator=(Future&) = delete;

  // Movable
  Future(Future&& f)
      : ss_(f.ss_) {
  }

  explicit Future(std::shared_ptr<SharedState<T>> ss)
      : ss_(ss)
  { }

  // Blocks current thread, One-shot await and consume future result.
  Result<T> GetResult() {
    return ss_->SyncGet();
  }

  // Asynchronous API

  // Set executor for asynchronous callback / continuation
  // Usage:: std::move(f).Via(e).Then(c)
  Future<T> Via(executors::IExecutor& executor) && {
    ss_->SetExecutor(&executor);
    return std::move(*this);
  }

  // Should be externally ordered with `Via` calls
  executors::IExecutor& GetExecutor() const {
    return ss_->GetExecutor();
  }

  // Consume future result with asynchronous callback
  // ? Post-condition: IsValid() == false
  void Subscribe(Callback<T>&& cb) {
    ss_->SetCallback(std::move(cb));
  }

  std::shared_ptr<SharedState<T>> ss_;
};

}  // namespace exe::futures
