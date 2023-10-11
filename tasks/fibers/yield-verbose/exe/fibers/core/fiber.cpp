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
static atomic<size_t> fiber_id_counter{0};

Fiber::Fiber(Scheduler& scheduler, Routine routine)
  : scheduler_(scheduler)
  , coro_(std::move(new coro::SimpleCoroutine(std::move(routine))))
{
  id_.store(fiber_id_counter.fetch_add(1) + 1);
  Ll("Construct::Fiber(Scheduler& scheduler, Routine routine)");
}

Fiber::~Fiber() {
  //printf("Destruct");
}

void Fiber::Run() {
  Ll("Run: Start");

  current_fiber = this;
  Ll("Run: Before Resume, coro_->IsCompleted(): %lu", (int)coro_->IsCompleted());
  coro_->Resume();
  Ll("Run: After Resume, coro_->IsCompleted(): %lu", (int)coro_->IsCompleted());

  if (coro_->IsCompleted()) {
    Ll("Run: IsCompleted before delete coro_");
    delete coro_;
    delete this;
    //printf("Fiber::Run: before return\n");
    return;
  }

  Ll("Run: End");
}

Fiber* Fiber::Self() {
  return current_fiber;
}

void Fiber::Schedule() {
  Ll("Schedule: Start");
  exe::tp::Submit(scheduler_, [&]() { Run(); });
  Ll("Schedule: End");
}

bool Fiber::IsSuspended() const {
  return coro_->GetStatus() == coro::Status::Suspended;
}

void Fiber::Ll(const char* format, ...) {
  if (!exe::tp::kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Fiber::%s, id: %lu, this: %p, coro_: %p (id: %lu)\n", pid.str().c_str(), format, GetId(), (void*)this, (void*)coro_, coro_->GetId());
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::fibers
