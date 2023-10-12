#include <exe/coro/core.hpp>

#include <wheels/core/assert.hpp>
#include <wheels/core/compiler.hpp>

#include "exe/coro/runnable.hpp"
#include "sure/stack/mmap.hpp"
#include "sure/trampoline.hpp"

#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <exe/tp/thread_pool.hpp> // для константы exe::tp::kShouldPrint

namespace exe::coro {

Coroutine::Coroutine(
  wheels::MutableMemView stack,
  Routine routine
)
  : routine_(std::move(routine))
{
  context_.Setup(stack, this);
}

void Coroutine::Resume() {
  // We are here by Fiber::Schedule() -> Fiber::Run()
  caller_context_.SwitchTo(context_); // Go to at 1st run in Coroutine::Run(); and at 2nd and later in Coroutine::Suspend() after SwitchTo().
  // We are here by call of SwitchTo(caller_context_) in Coroutine::Suspend()
  // and after by call of ExitTo(caller_context_) in Coroutine::Run()
  if (eptr_) {
    std::rethrow_exception(eptr_);
  }
}

/**
 * Вызовом fibers::Yield() мы сохраним текущий RIP в context_,
 * таким образом обеспечим продолжение исполнения после Yield()
 * при следующем вызове Resume().
 */
void Coroutine::Suspend() {
  // We are here by fibers::Yield()
  context_.SwitchTo(caller_context_); // Go to Couroutine::Resume() after SwitchTo()
  // We are here by 2nd and later calls of SwitchTo(context_) in Coroutine::Resume()
}

void Coroutine::Run() noexcept {
  // We are here by 1st call of SwitchTo(context_) in Coroutine::Resume()
  try {
    routine_();
  } catch (...) {
    eptr_ = std::current_exception();
  }

  completed_ = true;
  context_.ExitTo(caller_context_); // Go to Couroutine::Resume() after SwitchTo()

  WHEELS_UNREACHABLE();
}

void Coroutine::Ll(const char* format, ...) {
  if (!exe::tp::kShouldPrint) {
    return;
  }

  char buf[250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Coroutine::%s, coro_: %p\n", pid.str().c_str(), format, (void*)this);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::coro
