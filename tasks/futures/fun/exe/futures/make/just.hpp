#pragma once

#include <exe/futures/types/future.hpp>
#include <exe/result/types/unit.hpp>

namespace exe::futures {

inline Future<Unit> Just() {
  std::shared_ptr<SharedState<Unit>> ss = std::make_shared<SharedState<Unit>>();
  ss->Clear();
  return Future<Unit>(ss);
}

}  // namespace exe::futures
