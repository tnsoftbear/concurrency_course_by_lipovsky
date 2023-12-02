#pragma once

#include <exe/futures/types/future.hpp>
#include <exe/result/types/unit.hpp>
#include "exe/futures/make/contract.hpp"

namespace exe::futures {

inline Future<Unit> Just() {
  auto [f, p] = Contract<Unit>();
  std::move(p).SetValue(Unit{});
  return std::move(f);
}

}  // namespace exe::futures
