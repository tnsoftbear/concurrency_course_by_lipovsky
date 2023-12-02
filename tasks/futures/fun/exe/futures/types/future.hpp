#pragma once

#include <cstdlib>
#include <utility>

#include <exe/futures/state/shared_state.hpp>
using exe::futures::details::SharedState;

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
      : ss(f.ss) {
  }

  explicit Future(std::shared_ptr<SharedState<T>> ss)
      : ss(ss) {
  }

  Result<T> Get() {
    printf("Future::Get()\n");
    return ss->Get();
  }

  std::shared_ptr<SharedState<T>> ss;
};

}  // namespace exe::futures
