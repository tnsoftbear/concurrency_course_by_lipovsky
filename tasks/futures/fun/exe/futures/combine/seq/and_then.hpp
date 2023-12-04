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
    printf("AndThen::Pipe() Starts\n");
    auto [f, p] = Contract<U<T>>();
    //std::move(p).Set(fun(input_future.GetResult().value()));
    input_future.Subscribe([p = std::move(p), fun = std::forward<F>(fun)](Result<T> result) mutable {
      if (result.has_value()) {
        printf("AndThen:: Routine starts\n");
        std::move(p).Set(fun(result.value()));
      } else {
        printf("AndThen::Routine skip\n");
        std::move(p).Set(result);
      }
    });

    return std::move(f).Via(input_future.GetExecutor());
  }
};

}  // namespace pipe

// Future<T> -> (T -> Result<U>) -> Future<U>

template <typename F>
auto AndThen(F fun) {
  return pipe::AndThen{std::move(fun)};
}

}  // namespace exe::futures
