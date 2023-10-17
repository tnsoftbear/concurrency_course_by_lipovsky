#include <exe/fibers/sched/sleep_for.hpp>
#include <exe/fibers/core/scheduler.hpp>

#include <asio/steady_timer.hpp>
#include "asio/detail/chrono.hpp"
#include "exe/fibers/sched/yield.hpp"
#include <exe/fibers/core/fiber.hpp>

namespace exe::fibers {

void SleepFor(Millis delay) {
  auto fiber = Fiber::Self();
  fiber->Sleep();
  asio::steady_timer t(
    exe::fibers::GetCurrent(),
    asio::chrono::milliseconds(delay)
  );
  t.async_wait([fiber](const asio::error_code) {
    fiber->Wake();
  });
  exe::fibers::Yield();
}

}  // namespace exe::fibers
