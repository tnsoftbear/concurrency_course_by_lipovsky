#pragma once

#include <exe/futures/types/future.hpp>
#include <exe/result/types/result.hpp>

#include <tuple>

#include <exe/futures/state/shared_state.hpp>
using exe::futures::details::SharedState;

namespace exe::futures {

template <typename T>
class Promise {
 public:
  void Set(Result<T> result) && {
    if (result.error()) {
      ss_->SetError(std::move(result.error()));
    } else {
      ss_->SetValue(std::move(result.value()));
    }
  }

  void SetValue(T value) && {
    ss_->SetValue(std::move(value));
  }

  void SetError(Error err) && {
    ss_->SetError(std::move(err));
  }

  Future<T> MakeFuture() {
    return Future<T>(ss_);
  }

 private:
  std::shared_ptr<SharedState<T>> ss_ = std::make_shared<SharedState<T>>();
};

template <typename T>
std::tuple<Future<T>, Promise<T>> Contract() {
  Promise<T> promise;
  Future<T> future = promise.MakeFuture();
  return std::make_tuple(std::move(future), std::move(promise));
}

}  // namespace exe::futures
