#pragma once

#include <exe/futures/syntax/pipe.hpp>

namespace exe::futures {

namespace pipe {

struct [[nodiscard]] Flatten {
  template <typename T>
  Future<T> Pipe(Future<Future<T>> input_future) {
    auto [output_future, output_promise] = Contract<T>();
    input_future.Subscribe([output_promise = std::move(output_promise)](Result<Future<T>> input_future_result) mutable {
      if (input_future_result) {
        auto included_future = std::move(input_future_result.value());
        included_future.Subscribe([output_promise = std::move(output_promise)](Result<T> included_future_result) mutable {
          std::move(output_promise).Set(std::move(included_future_result.value()));
        });
      } else {
        std::move(output_promise).SetError(input_future_result.error());
      }
    });
    return std::move(output_future);
  }
};

}  // namespace pipe

// Future<Future<T>> -> Future<T>

inline auto Flatten() {
  return pipe::Flatten{};
}

}  // namespace exe::futures
