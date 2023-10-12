#include <cstddef>
#include <exe/fibers/core/fiber.hpp>
#include <exe/coro/core.hpp>

#include <memory>
#include <mutex>
#include <twist/ed/local/ptr.hpp>

#include <map>
#include <exe/fibers/core/scheduler.hpp>
#include "exe/tp/submit.hpp"

// 1] fibers::Go() -> new Fiber() + fiber->Schedule() 
// 2] -> ThreadPool::Submit({ fiber->Run() }) -> exits from fibers::Go() -> ThreadPool calls task() from queue in loop
// 3] fiber->Run() -> coroutine->Resume() // saves caller_context_
// 4] -> switch execution to context_ with trampoline entry-point: coroutine->Run() -> routine_()
// 5] routine_() does its job and calls fibers::Yield() -> fiber->Suspend() 
// 6] -> coroutine->Suspend() // saves execution in context_, restores caller_context_ from the point [3]
// 7] exits coroutine->Resume() -> exits to fiber->Run() 
// 8] -> fiber->Schedule(); -> 2] -> 3], coroutine->Resume() switches execution to saved context_ at the point [6] 
// 9] exits from coroutine->Suspend() -> exits from fibers::Yield() -> continue routine_()
// 10] routine_() ends and exits to Coroutine::Run() -> context_.ExitTo() -> exits to Coroutine::Resume()
// 11] -> exits to Fiber::Run() -> (IsCompleted == true) -> delete this; -> exits to ThreadPool loop.

// clippy target fibers_sched_unit_tests DebugASan --suite Fibers --test Yield3
// clippy target fibers_sched_stress_tests FaultyThreadsTSan
// clippy target fibers_sched_stress_tests Debug

namespace exe::fibers {

static twist::ed::ThreadLocalPtr<Fiber> current_fiber = nullptr;

Fiber::Fiber(Scheduler& scheduler, Routine routine)
  : scheduler_(scheduler)
  , stack_(sure::Stack::AllocateBytes(1024 * 64))
  , coro_(std::move(
    new coro::Coroutine(stack_.MutView(), std::move(routine))
  ))
{
}

void Fiber::Run() {
  current_fiber = this;
  coro_->Resume();
  
  if (coro_->IsCompleted()) {
    delete coro_;
    delete this;
    return;
  }
  
  // Schedule the next round of suspended execution after fibers::Yield() of routine
  Schedule();
}

Fiber* Fiber::Self() {
  return current_fiber;
}

void Fiber::Schedule() {
  exe::tp::Submit(scheduler_, [&]() { Run(); });
}

void Fiber::Suspend() {
  coro_->Suspend();
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
