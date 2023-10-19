#pragma once

#include <exception>
#include <memory>
#include <cassert>

#include <stdlike/shared_state.hpp>

namespace stdlike {

template <typename T>
class Future {
  template <typename U>
  friend class Promise;

 public:
  // Non-copyable
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  // Movable
  Future(Future&&) = default;
  Future& operator=(Future&&) = default;

  // One-shot
  // Wait for result (value or exception)
  T Get() {
    return ss_->Get();
  }

 private:
  explicit Future(std::shared_ptr<SharedState<T>> ss) : ss_(ss) {
  }

 private:
  std::shared_ptr<SharedState<T>> ss_;
};

}  // namespace stdlike
