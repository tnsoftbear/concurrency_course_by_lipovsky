#include <cstddef>
#include <exe/fibers/core/fiber.hpp>
#include <exe/coro/simple.hpp>
#include <exe/coro/core.hpp>

#include <memory>
#include <mutex>
#include <twist/ed/local/ptr.hpp>

#include <map>
#include <exe/fibers/core/scheduler.hpp>
#include "exe/tp/submit.hpp"

// #include <twist/ed/stdlike/thread.hpp>

// clippy target fibers_sched_unit_tests DebugASan --suite Fibers --test Yield1
// clippy target fibers_sched_unit_tests DebugASan --suite Fibers --test JustWorks
// clippy target fibers_sched_unit_tests DebugASan --suite Fibers --test RunInParallel
// clippy target fibers_sched_unit_tests DebugASan --suite Fibers --test TwoPools1
// clippy target fibers_sched_stress_tests FaultyThreadsTSan
// clippy target fibers_sched_stress_tests Debug

namespace exe::fibers {

static twist::ed::ThreadLocalPtr<Fiber> current_fiber = nullptr;

Fiber::Fiber(Scheduler& scheduler, Routine routine)
  : scheduler_(scheduler)
  , coro_(std::move(new coro::SimpleCoroutine(std::move(routine))))
{
}

Fiber::~Fiber() {
}

void Fiber::Run() {
  current_fiber = this;
  SetStatus(Status::Running);
  coro_->Resume();

  if (coro_->IsCompleted()) {
    delete coro_;
    delete this;
    return;
  }
}

Fiber* Fiber::Self() {
  return current_fiber;
}

void Fiber::Schedule() {
  exe::tp::Submit(scheduler_, [&]() { Run(); });
}

bool Fiber::IsSuspended() const {
  return GetStatus() == Status::Suspended;
}

void Fiber::Ll(const char* format, ...) {
  if (!exe::tp::kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Fiber::%s, this: %p, coro_: %p\n", pid.str().c_str(), format, (void*)this, (void*)coro_);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::fibers
