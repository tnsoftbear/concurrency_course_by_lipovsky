#include <cstddef>
#include <exe/fibers/core/fiber.hpp>

#include <memory>
#include <mutex>
#include <twist/ed/local/ptr.hpp>

#include <map>
#include <exe/fibers/core/scheduler.hpp>
#include "exe/tp/submit.hpp"

namespace exe::fibers {

//const bool kShouldPrint = false;
const bool kShouldPrint = true;

static twist::ed::ThreadLocalPtr<Fiber> current_fiber = nullptr;
static size_t fiber_id = 0;

Fiber::Fiber(Scheduler& scheduler, Routine routine)
  : scheduler_(scheduler)
  , stack_(sure::Stack::AllocateBytes(1024 * 64))
  , coro_(std::move(
    new coro::Coroutine(stack_.MutView(), std::move(routine))
  ))
  , id_(++fiber_id)
{
  Ll("Fiber: created");
}

Fiber::~Fiber() {
  delete coro_;
}

void Fiber::Run() {
  Ll("Run: starts");
  current_fiber = this;
  coro_->Resume();
  
  if (coro_->IsCompleted()) {
    Ll("Run: before delete this;");
    delete this;
    return;
  }

  auto awaiter = std::exchange(awaiter_, nullptr);
  Ll("Run: awaiter->AwaitSuspend(this)");
  awaiter->AwaitSuspend(this);
}

Fiber* Fiber::Self() {
  return current_fiber;
}

void Fiber::Schedule() {
  exe::tp::Submit(scheduler_, [&]() { 
    Ll("Schedule: Fiber's routine before Run()");
    Run(); 
  });
}

void Fiber::Suspend(IAwaiter* awaiter) {
  Ll("Suspend: awaiter: %p", awaiter);
  awaiter_ = awaiter;
  coro_->Suspend();
}

void Fiber::Switch() {
  // Not implemented
}

void Fiber::Ll(const char* format, ...) {
  if (!kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Fiber::%s, id: %lu, this: %p, name: %s\n", pid.str().c_str(), format, id_, (void*)this, name.c_str());
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::fibers
