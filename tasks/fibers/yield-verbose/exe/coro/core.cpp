#include <exe/coro/core.hpp>

#include <mutex>
#include <twist/ed/local/var.hpp>

#include <wheels/core/assert.hpp>
#include <wheels/core/compiler.hpp>

//#include "sure/context.hpp"
#include "exe/coro/runnable.hpp"
#include "sure/stack/mmap.hpp"
#include "sure/trampoline.hpp"

#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <exe/tp/thread_pool.hpp> // для константы exe::tp::kShouldPrint

namespace exe::coro {

static twist::ed::ThreadLocal<std::stack<Coroutine*>> coros;
static atomic<size_t> id_counter{1000};
static mutex global_mutex;

Coroutine::Coroutine(
  wheels::MutableMemView stack,
  IRunnable* runnable
)
  : runnable_(runnable)
{
  context_.Setup(stack, this);
  id_.store(id_counter.fetch_add(1) + 1);
}

Coroutine::~Coroutine() {
  Ll("~Coroutine()");
}

void Coroutine::Resume() {
  Ll("Resume(): start");
  {
    std::unique_lock guard(global_mutex); // проверить необходимость
    coros->push(this);
    SetStatus(Status::Running);
  }
  Ll("Resume(): before caller_context_.SwitchTo(context_);");
  caller_context_.SwitchTo(context_);
  Ll("Resume(): after caller_context_.SwitchTo(context_);");
  if (eptr_) {
    Ll("Resume: std::rethrow_exception(eptr_);");
    std::rethrow_exception(eptr_);
  }
}

void Coroutine::Suspend() {
  Coroutine* coro;
  {
    std::unique_lock guard(global_mutex); // проверить необходимость
    coro = coros->top();
    coros->pop();
    coro->SetStatus(Status::Suspended);
  }
  coro->SwitchToScheduler();
}

void Coroutine::SwitchToScheduler() {
  Ll("Suspend/SwitchToScheduler: before context_.SwitchTo(caller_context_)");
  context_.SwitchTo(caller_context_);
  Ll("Suspend/SwitchToScheduler: after context_.SwitchTo(caller_context_)");
}

void Coroutine::Run() noexcept {
  try {
    runnable_->RunCoro();
  } catch (...) {
    Ll("Run: Catch exception");
    eptr_ = std::current_exception();
  }

  {
    std::unique_lock lock(global_mutex);
    SetStatus(Status::Completed);
    coros->pop();
  }
  Ll("Run: before context_.ExitTo(caller_context_);");
  context_.ExitTo(caller_context_);

  WHEELS_UNREACHABLE();
}

void Coroutine::Ll(const char* format, ...) {
  if (!exe::tp::kShouldPrint) {
    return;
  }

  char buf[250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Coroutine::%s, id: %lu, status: %lu, coro_: %p\n", pid.str().c_str(), format, id_.load(), (size_t)status_, (void*)this);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::coro
