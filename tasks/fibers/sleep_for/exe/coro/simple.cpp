#include <exe/coro/simple.hpp>
#include <twist/ed/local/ptr.hpp>
#include <wheels/core/defer.hpp>

namespace exe::coro {

// Simple stackful coroutine

static twist::ed::ThreadLocalPtr<SimpleCoroutine> current;

SimpleCoroutine::SimpleCoroutine(Routine routine)
  : stack_(AllocateStack())
  , impl_(stack_.MutView(), std::move(routine)) {}

SimpleCoroutine::~SimpleCoroutine() {
  ReleaseResources();
}

void SimpleCoroutine::Resume() {
  SimpleCoroutine* prev = current;
  current = this;
  wheels::Defer rollback([prev]() {
    current = prev;
  });

  impl_.Resume();
}

void SimpleCoroutine::Suspend() {
  current->impl_.Suspend();
}

// private

sure::Stack SimpleCoroutine::AllocateStack() {
  return sure::Stack::AllocateBytes(1024 * 64);
}

void SimpleCoroutine::ReleaseResources() {
  // Я хз, как правильно чистить ресурсы. Видел так сделано в await фреймворке.
  auto mmv = stack_.Release();
  sure::Stack::Acquire(mmv);
}

}  // namespace exe::coro
