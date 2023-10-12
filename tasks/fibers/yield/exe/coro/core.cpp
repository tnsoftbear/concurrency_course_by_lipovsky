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
  caller_context_.SwitchTo(context_);
  if (eptr_) {
    std::rethrow_exception(eptr_);
  }
}

void Coroutine::Suspend() {
  context_.SwitchTo(caller_context_);
}

void Coroutine::Run() noexcept {
  try {
    routine_();
  } catch (...) {
    eptr_ = std::current_exception();
  }

  completed_ = true;
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
  sprintf(buf, "%s Coroutine::%s, coro_: %p\n", pid.str().c_str(), format, (void*)this);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::coro
