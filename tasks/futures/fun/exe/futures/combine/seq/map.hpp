#pragma once

#include <exe/futures/syntax/pipe.hpp>

#include <type_traits>

namespace exe::futures {

namespace pipe {

template <typename F>
struct [[nodiscard]] Map {
  F fun;

  explicit Map(F f)
      : fun(std::move(f)) {
  }

  template <typename T>
  using U = std::invoke_result_t<F, T>;

  template <typename T>
  Future<U<T>> Pipe(Future<T> input_future) {
    printf("Map::Pipe(): starts\n");
    auto [f, p] = Contract<U<T>>();
    printf("Map::Pipe(): auto input_future_result = input_future.Get();\n");
    auto input_future_result = input_future.Get();
    if (input_future_result) {
      printf("Map::Pipe(): U<T> value = fun(input_future_result.value());\n");
      U<T> value = fun(input_future_result.value());
      printf("Map::Pipe(): std::move(p).SetValue(value);\n");
      std::move(p).SetValue(value);
    } else {
      printf("Map::Pipe(): std::move(p).SetError(input_future_result.error());\n");
      std::move(p).SetError(input_future_result.error());
    }
    printf("Map::Pipe(): return std::move(f);");
    return std::move(f);
  }
};

}  // namespace pipe

// Future<T> -> (T -> U) -> Future<U>

template <typename F>
auto Map(F fun) {
  printf("Map(F fun)\n");
  return pipe::Map{std::move(fun)};
}

}  // namespace exe::futures
