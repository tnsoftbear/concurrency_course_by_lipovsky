#pragma once

#include <exe/futures/syntax/pipe.hpp>

namespace exe::futures {

namespace pipe {

template <typename F>
struct [[nodiscard]] OrElse {
  F fun;

  explicit OrElse(F f)
      : fun(std::move(f)) {
  }

  template <typename T>
  Future<T> Pipe(Future<T> input_future) {
    auto [f, p] = Contract<T>();
    // Result<T> input_future_result = input_future.Get();
    // Result<T> result = fun(input_future_result.error());
    // std::move(p).Set(result);
    std::move(p).Set(fun(input_future.Get().error()));
    return std::move(f);
  }
};

}  // namespace pipe

// Future<T> -> (Error -> Result<T>) -> Future<T>

template <typename F>
auto OrElse(F fun) {
  printf("OrElse\n");
  return pipe::OrElse{std::move(fun)};
}

}  // namespace exe::futures
