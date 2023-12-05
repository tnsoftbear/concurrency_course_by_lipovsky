#pragma once

#include <exe/futures/syntax/pipe.hpp>

#include <exe/result/types/result.hpp>

namespace exe::futures {

namespace pipe {

/**
Терминатор `Get` блокирует текущий поток до готовности результата:
```
// Планируем задачу в пул потоков и дожидаемся результата
Result<int> r = futures::Submit(pool, [] {
  return result::Ok(7);
}) | futures::Get();
```
Иначе говоря, `Get` синхронно "распаковывает" `Future` в `Result`.
*/

struct [[nodiscard]] Get {
  template <typename T>
  Result<T> Pipe(Future<T> f) {
    return std::move(f).GetResult();
  }
};

}  // namespace pipe

inline auto Get() {
  return pipe::Get{};
}

}  // namespace exe::futures
