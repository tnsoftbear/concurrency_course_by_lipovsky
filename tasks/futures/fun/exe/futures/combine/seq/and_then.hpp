#pragma once

#include <exe/futures/syntax/pipe.hpp>

#include <exe/result/traits/value_of.hpp>

#include <type_traits>

namespace exe::futures {

namespace pipe {

template <typename F>
struct [[nodiscard]] AndThen {
  F fun;

  explicit AndThen(F f)
      : fun(std::move(f)) {
  }

  // Non-copyable
  AndThen(AndThen&) = delete;

  template <typename T>
  using U = result::traits::ValueOf<std::invoke_result_t<F, T>>;

  template <typename T>
  Future<U<T>> Pipe(Future<T> input_future) {
    auto [f, p] = Contract<U<T>>();
    // Result<T> input_future_result = input_future.Get();
    // Result<T> result = fun(input_future_result.value());
    // std::move(p).Set(result);
    std::move(p).Set(fun(input_future.Get().value()));
    return std::move(f);
  }
};

}  // namespace pipe

// Future<T> -> (T -> Result<U>) -> Future<U>

template <typename F>
auto AndThen(F fun) {
  return pipe::AndThen{std::move(fun)};
}

}  // namespace exe::futures
