#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/core/fiber.hpp>
#include "exe/fibers/core/scheduler.hpp"

namespace exe::fibers {

void Go(Scheduler& scheduler, Routine routine) {
  exe::fibers::SetCurrent(scheduler);
  fibers::Fiber* fiber = new fibers::Fiber(scheduler, std::move(routine));
  fiber->Schedule();
}

void Go(Routine routine) {
  Scheduler& scheduler = exe::fibers::GetCurrent();
  Go(scheduler, std::move(routine));
}

}  // namespace exe::fibers
