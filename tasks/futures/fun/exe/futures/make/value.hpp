#pragma once

#include <exe/futures/types/future.hpp>
#include <memory>

namespace exe::futures {

template <typename T>
Future<T> Value(T v) {
  auto [f, p] = Contract<T>();
  std::move(p).SetValue(v);
  return std::move(f);
}

}  // namespace exe::futures


// std::shared_ptr<SharedState<T>> ss = std::make_shared<SharedState<T>>();
// ss->SetValue(std::move(v));
// return Future<T>(ss);
