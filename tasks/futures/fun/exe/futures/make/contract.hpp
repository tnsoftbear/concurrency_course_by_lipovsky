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
    if (result) {
      ss_->SetValue(std::move(result.value()));
    } else {
      ss_->SetError(std::move(result.error()));
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


  void Ll(const char* format, ...) {
    bool const k_should_print = true;
    if (!k_should_print) {
      return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s Promise::%s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};

template <typename T>
std::tuple<Future<T>, Promise<T>> Contract() {
  Promise<T> promise;
  Future<T> future = promise.MakeFuture();
  return std::make_tuple(std::move(future), std::move(promise));
}

}  // namespace exe::futures
