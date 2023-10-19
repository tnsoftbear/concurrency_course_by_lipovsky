#pragma once

#include <stdlike/future.hpp>
#include <memory>

#include <stdlike/shared_state.hpp>

namespace stdlike {

template <typename T>
class Promise {
 public:
  Promise() {
    //ss_ = std::make_shared<SharedState<T>>();
  }

  // Non-copyable
  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  // Movable
  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  // One-shot
  Future<T> MakeFuture() {
    Future<T> future(ss_);
    return future;
  }

  // One-shot
  // Fulfill promise with value
  void SetValue(T value) {
    ss_->SetValue(value);
  }

  T GetValue() {
    return ss_->Get();
  }

  // One-shot
  // Fulfill promise with exception
  void SetException(std::exception_ptr expt) {
    ss_->SetException(expt);
  }

 private:
  std::shared_ptr<SharedState<T>> ss_ = std::make_shared<SharedState<T>>();
};

}  // namespace stdlike
