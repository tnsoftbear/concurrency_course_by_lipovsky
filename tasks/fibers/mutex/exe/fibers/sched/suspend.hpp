#pragma once

#include "exe/fibers/core/awaiter.hpp"

namespace exe::fibers {

void Suspend(IAwaiter& awaiter);

}  // namespace exe::fibers
