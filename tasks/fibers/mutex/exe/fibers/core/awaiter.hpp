#pragma once

#include <exe/fibers/core/handle.hpp>

namespace exe::fibers {

struct IAwaiter {
  virtual void AwaitSuspend(Fiber* fiber) = 0;
};

}  // namespace exe::fibers
