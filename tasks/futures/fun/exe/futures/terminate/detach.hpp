#pragma once

#include <exe/futures/syntax/pipe.hpp>
#include <variant>

namespace exe::futures {

namespace pipe {

/**
Терминатор `Detach` поглощает фьючу и игнорирует ее результат.
```
// Завершение задачи в пуле нас не интересует
futures::Submit(pool, [] {
  // ...
}) | futures::Detach();
```
*/

struct [[nodiscard]] Detach {
  template <typename T>
  void Pipe(Future<T> /* input_feature */) {
    // input_feature.Subscribe([](Result<T>){

    // });
    //f.ss_->Clear();
  }
};

}  // namespace pipe

inline auto Detach() {
  return pipe::Detach{};
}

}  // namespace exe::futures
