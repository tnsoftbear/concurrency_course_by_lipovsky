#include <cstddef>
#include <exe/fibers/core/fiber.hpp>
#include <exe/coro/core.hpp>

#include <memory>
#include <mutex>
#include <twist/ed/local/ptr.hpp>

#include <map>
#include <exe/fibers/core/scheduler.hpp>
#include "exe/tp/submit.hpp"

namespace exe::fibers {

const bool kShouldPrint = false;

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
  //exe::tp::Submit(scheduler_, [&]() { Run(); });
}

void Fiber::Suspend() {
  coro_->Suspend();
}

void Fiber::Ll(const char* format, ...) {
  if (!kShouldPrint) {
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
