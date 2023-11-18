#include <exe/fibers/sched/suspend.hpp>
#include "exe/fibers/core/fiber.hpp"
#include "exe/fibers/core/awaiter.hpp"
#include "exe/fibers/core/fwd.hpp"

namespace exe::fibers {

void Suspend(IAwaiter& awaiter) {
  Fiber::Self()->Suspend(&awaiter);
}

}  // namespace exe::fibers
