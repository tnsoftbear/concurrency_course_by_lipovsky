#include <exe/coro/simple.hpp>
#include <exe/coro/core.hpp>
#include <twist/ed/local/ptr.hpp>
#include <wheels/core/defer.hpp>

namespace exe::coro {

// Simple stackful coroutine

static twist::ed::ThreadLocalPtr<SimpleCoroutine> current;

SimpleCoroutine::SimpleCoroutine(Routine routine)
: routine_(std::move(routine))
, stack_(AllocateStack())
, impl_(stack_.MutView(), this) {}

SimpleCoroutine::~SimpleCoroutine() {
  //Ll("~SimpleCoroutine()");
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
  auto mmv = stack_.Release();
  sure::Stack::Acquire(mmv);
}

void SimpleCoroutine::RunCoro() {
  routine_();
}

}  // namespace exe::coro