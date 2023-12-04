#pragma once

#include <exe/executors/executor.hpp>

#include <exe/futures/syntax/pipe.hpp>

namespace exe::futures {

namespace pipe {

struct [[nodiscard]] Via {
  executors::IExecutor& executor;

  template <typename T>
  Future<T> Pipe(Future<T> input_future) {
    return std::move(input_future).Via(executor);
  }
};

}  // namespace pipe

// Future<T> -> Executor -> Future<T>

inline auto Via(executors::IExecutor& executor) {
  return pipe::Via{executor};
}

}  // namespace exe::futures
