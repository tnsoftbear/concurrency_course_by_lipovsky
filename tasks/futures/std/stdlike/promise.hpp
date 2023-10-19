#pragma once

#include <stdlike/future.hpp>
#include <stdlike/details/shared_state.hpp>

namespace stdlike {

using stdlike::details::SharedState;

template <typename T>
class Promise {
 public:
  Promise() {}

  // Non-copyable
  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  // Movable
  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  // One-shot
  Future<T> MakeFuture() {
    return Future<T>(ss_);
  }

  // One-shot
  // Fulfill promise with value
  void SetValue(T value) {
    ss_->SetValue(std::move(value));
  }

  T GetValue() {
    return ss_->Get();
  }

  // One-shot
  // Fulfill promise with exception
  void SetException(std::exception_ptr expt) {
    ss_->SetException(std::move(expt));
  }

 private:
  std::shared_ptr<SharedState<T>> ss_ = std::make_shared<SharedState<T>>();
};

}  // namespace stdlike
