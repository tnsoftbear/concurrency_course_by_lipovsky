#include "coroutine.hpp"

#include <twist/ed/local/ptr.hpp>

#include <wheels/core/assert.hpp>
#include <wheels/core/compiler.hpp>
#include "sure/context.hpp"
#include "sure/stack/mmap.hpp"

#include <wheels/core/exception.hpp>

#include <twist/ed/stdlike/thread.hpp> // for debug logs

std::stack<ContextNode> Coroutine::context_stack;

Coroutine::Coroutine(Routine routine)
  : routine_(std::move(routine))
  , stack_(sure::Stack::AllocateBytes(1024 * 64))
{
  context_.Setup(stack_.MutView(), this);
}

void Coroutine::Resume() {
  ContextNode node = {
    .caller_context = &caller_context_,
    .own_context = &context_,
  };
  
  context_stack.push(node);
  caller_context_.SwitchTo(context_);

  if (eptr_) {
    std::rethrow_exception(eptr_);
  }
}

void Coroutine::Suspend() {
  auto node = context_stack.top();
  context_stack.pop();
  node.own_context->SwitchTo(*node.caller_context);
}

bool Coroutine::IsCompleted() const {
  return completed_;
}

void Coroutine::Run() noexcept {
  try {
    routine_();
  } catch (...) {
    eptr_ = std::current_exception();
  }

  completed_ = true;
  context_stack.pop();
  Ll("Run: before context_.ExitTo(caller_context_);");
  context_.ExitTo(caller_context_);

  WHEELS_UNREACHABLE();
}

void Ll(const char* format, ...) {
  bool should_print_ = true;
  if (!should_print_) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s %s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}