#include <exe/fibers/core/fiber.hpp>
#include <cstddef>

#include <memory>
#include <mutex>
#include <twist/ed/local/ptr.hpp>

#include <map>

#include <twist/ed/stdlike/thread.hpp>
#include <exe/fibers/core/scheduler.cpp>
#include <asio/defer.hpp>

namespace exe::fibers {

const bool kShouldPrint = false;

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
}

Fiber::~Fiber() {
  Ll("~Fiber()");
  delete coro_;
}

void Fiber::Run() {
  current_fiber = this;
  status_ = Status::Running;
  Ll("Run(): before Resume()");
  coro_->Resume();
  Ll("Run(): after Resume()");
  
  if (coro_->IsCompleted()) {
    Ll("Run: before delete this;");
    delete this;
    return;
  }
  
  if (status_ == Status::Sleeping) {
    Ll("Run: return; because sleeping");
    return;
  }
  
  // Schedule the next round of suspended execution after fibers::Yield() of routine
  Schedule();
}

void Fiber::MarkSleep() {
  Ll("Sleep");
  status_ = Status::Sleeping;
}

void Fiber::Wake() {
  Ll("Wake");
  status_ = Status::Runnable;
  Run(); 
}

Fiber* Fiber::Self() {
  return current_fiber;
}

void Fiber::Schedule() {
  Ll("Schedule: start");
  exe::fibers::Submit(scheduler_, [&]() { Run(); });
}

void Fiber::Suspend() {
  Ll("Suspend(): start");
  coro_->Suspend();
  Ll("Suspend(): end");
}

void Fiber::Ll(const char* format, ...) {
  if (!kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Fiber::%s, id: %lu, st: %d, this: %p, coro_: %p\n", pid.str().c_str(), format, id_, status_, (void*)this, (void*)coro_);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::fibers
