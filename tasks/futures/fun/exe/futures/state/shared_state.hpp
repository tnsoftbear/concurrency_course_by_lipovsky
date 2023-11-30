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

namespace exe::futures {
namespace details {

using twist::ed::stdlike::condition_variable;
using twist::ed::stdlike::mutex;

template <typename T>
class SharedState {
 public:
  Result<T> Get() {
    if constexpr (std::is_same<T, Unit>::value) {
      return Unit{};
    }
    if (v_.index() == 1) {
      return Result<T>(std::move(std::get<1>(v_)));
    } else /* if (v_.index() == 2) */ {
      return tl::make_unexpected(std::get<2>(v_));
    }
  }

  // T Get() {
  //   std::unique_lock<mutex> lock(*mtx_);
  //   readiness_cv_.wait(lock, [this] {
  //     return v_.index() != 0;
  //   });

  //   if (v_.index() == 2) {
  //       auto eptr = std::move(std::get<2>(v_));
  //       return eptr;
  //   } else {
  //       return std::move(std::get<1>(v_));
  //   }
  // }

  // void SetResult(Result<T>&& result) {
  //   result_ = std::move(result);
  // }

  void SetValue(T&& value) {
    {
      std::lock_guard<mutex> guard(*mtx_);
      v_.template emplace<1>(std::forward<T>(value));
    }
    readiness_cv_.notify_one();
  }

  void SetError(Error&& error) {
    {
      std::lock_guard<mutex> guard(*mtx_);
      v_.template emplace<2>(std::forward<Error>(error));
    }
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
};

}  // namespace details
}  // namespace exe::futures
