#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/core/fiber.hpp>

namespace exe::fibers {

void Go(Scheduler& scheduler, Routine routine) {
  fibers::Fiber* fiber = new fibers::Fiber(scheduler, std::move(routine));
  fiber->Schedule();
}

void Go(Routine routine) {
  Go(*Scheduler::Current(), std::move(routine));
}

}  // namespace exe::fibers
