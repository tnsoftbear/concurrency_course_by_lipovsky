#pragma once

#include <exe/futures/syntax/pipe.hpp>

#include <exe/futures/traits/value_of.hpp>

#include <type_traits>

namespace exe::futures {

namespace pipe {

/**

Сигнатура: `Future<T>` → (`T` → `Future<U>`) → `Future<U>`

Комбинация `Map` + `Flatten`. Планирует асинхронное продолжение цепочки задач.

```cpp
futures::Future<int> f = futures::Submit(pool, [] {
  return Ok(1);
}) | futures::FlatMap([&pool](int v) {
  return futures::Submit(pool, [v] {
    return result::Ok(v + 1);
  });
});   
```

 */

template <typename F>
struct [[nodiscard]] FlatMap {
  F fun;

  explicit FlatMap(F f)
      : fun(std::move(f)) {
  }

  template <typename T>
  using U = traits::ValueOf<std::invoke_result_t<F, T>>;

  template <typename T>
  Future<U<T>> Pipe(Future<T> input_future) {
    auto [f, p] = Contract<U<T>>();
    input_future.Subscribe([p = std::move(p), fun = std::forward<F>(fun)](Result<T> result) mutable {
      if (result) {
        Future<U<T>> included_future = fun(result.value());
        included_future.Subscribe([p = std::move(p)](Result<U<T>> included_future_result) mutable {
          std::move(p).Set(std::move(included_future_result.value()));
        });
      } else {
        std::move(p).SetError(result.error());
      }
    });
    return std::move(f).Via(input_future.GetExecutor());
  }
};

}  // namespace pipe

// Future<T> -> (T -> Future<U>) -> Future<U>

template <typename F>
auto FlatMap(F fun) {
  return pipe::FlatMap{std::move(fun)};
}

}  // namespace exe::futures
