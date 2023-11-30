#pragma once

#include <exe/futures/types/future.hpp>

#include <exe/result/types/error.hpp>

namespace exe::futures {

template <typename T>
Future<T> Failure(Error with) {
  std::shared_ptr<SharedState<T>> ss = std::make_shared<SharedState<T>>();
  ss->SetError(std::move(with));
  return Future<T>(ss);
}

}  // namespace exe::futures
