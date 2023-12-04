#pragma once

#include <exe/futures/types/future.hpp>
#include <exe/result/types/result.hpp>

#include <exe/result/make/err.hpp>
#include "exe/executors/executor.hpp"
#include "exe/executors/inline.hpp"
using exe::result::Err;
#include <exe/result/make/ok.hpp>
using exe::result::Ok;

#include <tuple>

#include <exe/futures/state/shared_state.hpp>
using exe::futures::details::SharedState;

namespace exe::futures {

template <typename T>
class Promise {
 public:
  void Set(Result<T> result) && {
    ss_->SetResult(std::move(result));
  }

  void SetValue(T value) && {
    ss_->SetResult(Ok(std::move(value)));
  }

  void SetError(Error err) && {
    ss_->SetResult(Err(std::move(err)));
  }

  Future<T> MakeFuture() {
    ss_ = std::make_shared<SharedState<T>>();
    return Future<T>(ss_);
  }

 private:
  std::shared_ptr<SharedState<T>> ss_; // = std::make_shared<SharedState<T>>();

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

#include <typeinfo>

template <typename T>
std::tuple<Future<T>, Promise<T>> Contract() {
  Promise<T> promise;
  Future<T> future = promise.MakeFuture();
  return std::make_tuple(std::move(future), std::move(promise));
}

}  // namespace exe::futures
