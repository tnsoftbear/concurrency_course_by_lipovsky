#pragma once

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

#include <exe/result/types/result.hpp>
#include <utility>
#include <variant>
#include "exe/result/types/error.hpp"
#include "exe/result/types/status.hpp"
#include "tl/expected.hpp"
#include <type_traits>

#include <iostream>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

namespace exe::futures {
namespace details {

using twist::ed::stdlike::condition_variable;
using twist::ed::stdlike::mutex;

template <typename T>
class SharedState {
 public:
  Result<T> Get() {
    Ll("Before readiness_cv_.wait");
    std::unique_lock<mutex> lock(*mtx_);
    readiness_cv_.wait(lock, [this] {
      return v_.index() != 0;
    });
    Ll("After readiness_cv_.wait");

    if (v_.index() == 1) {
      Ll("Get: return Result<T>(std::move(std::get<1>(v_)));");
      return Result<T>(std::move(std::get<1>(v_)));
    } else /* if (v_.index() == 2) */ {
      Ll("Get: return tl::make_unexpected(std::get<2>(v_));");
      return tl::make_unexpected(std::get<2>(v_));
    } 
  }

  void SetValue(T&& value) {
    {
      std::lock_guard<mutex> guard(*mtx_);
      v_.template emplace<1>(std::forward<T>(value));
    }
    Ll("SetValue: readiness_cv_.notify_one();");
    readiness_cv_.notify_one();
  }

  void SetError(Error&& error) {
    {
      std::lock_guard<mutex> guard(*mtx_);
      v_.template emplace<2>(std::forward<Error>(error));
    }
    Ll("SetError: readiness_cv_.notify_one();");
    readiness_cv_.notify_one();
  }

  void Clear() {
    v_.template emplace<0>();
  }

  // void Submit(traits::SubmitT<T> fun) {

  // }

 private:
  // Result<T> result_;
  std::variant<std::monostate, T, Error> v_;
  std::unique_ptr<mutex> mtx_ = std::make_unique<mutex>();
  condition_variable readiness_cv_;

void Ll(const char* format, ...) {
  bool const k_should_print = true;
  if (!k_should_print) {
    return;
  }

  char buf[250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s SharedState::%s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}
};

}  // namespace details
}  // namespace exe::futures
