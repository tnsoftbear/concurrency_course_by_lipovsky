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
    printf("OrElse::Pipe(): starts\n");
    auto [f, p] = Contract<T>();
    //std::move(p).Set(fun(input_future.GetResult().error()));
    input_future.Subscribe([p = std::move(p), fun = std::forward<F>(fun)](Result<T> result) mutable {
      if (!result.has_value()) {
        printf("OrElse::Routine starts\n");
        std::move(p).Set(fun(result.error()));
      } else {
        printf("OrElse::Routine skip\n");
        std::move(p).Set(result);
      }
    });
    return std::move(f).Via(input_future.GetExecutor());
  }
};

}  // namespace pipe

// Future<T> -> (Error -> Result<T>) -> Future<T>

template <typename F>
auto OrElse(F fun) {
  return pipe::OrElse{std::move(fun)};
}

}  // namespace exe::futures
