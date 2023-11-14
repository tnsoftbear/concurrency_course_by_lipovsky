#include <exe/fibers/sched/sleep_for.hpp>
#include <exe/fibers/core/scheduler.hpp>

#include <asio/steady_timer.hpp>
#include "asio/detail/chrono.hpp"
#include "exe/fibers/sched/yield.hpp"
#include <exe/fibers/core/fiber.hpp>
#include <asio/defer.hpp>

namespace exe::fibers {

void SleepFor(Millis delay) {
  auto fiber = Fiber::Self();
  fiber->MarkSleep();
  asio::steady_timer t(
    fiber->GetScheduler(),
    asio::chrono::milliseconds(delay)
  );
  asio::defer(fiber->GetScheduler(), [fiber, &t]() {
    t.async_wait([fiber](const asio::error_code) {
      fiber->WakeAndRun();
    });
  });
  exe::fibers::Yield();
}

}  // namespace exe::fibers
