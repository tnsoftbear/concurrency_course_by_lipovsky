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
  Ll("Construct: start");
  context_.Setup(stack_.MutView(), this);
  Ll("Construct: end");
}

void Coroutine::Resume() {
  Ll("Resume: start");
  ContextNode node = {
    .caller_context = &caller_context_,
    .own_context = &context_,
  };
  
  Ll("Resume: push caller: %p, own: %p", node.caller_context, node.own_context);
  context_stack.push(node);
  Ll("Resume: before switchTo queue-count: %lu", context_stack.size());
  caller_context_.SwitchTo(context_);

  if (eptr_) {
    Ll("Run: before rethrow_exception");
    std::rethrow_exception(eptr_);
  }
  Ll("Resume: end");
}

void Coroutine::Suspend() {
  Ll("Suspend: start");
  auto node = context_stack.top();
  Ll("Suspend: before pop");
  context_stack.pop();
  Ll("Suspend: pop caller: %p, own: %p", node.caller_context, node.own_context);
  Ll("Suspend: before SwitchTo, count: %lu", context_stack.size());
  node.own_context->SwitchTo(*node.caller_context);
  Ll("Suspend: end");
}

bool Coroutine::IsCompleted() const {
  return completed_;
}

void Coroutine::Run() noexcept {
  Ll("Run: start");
  // std::exception_ptr eptr;
  try {
    routine_();
  } catch (...) {
    Ll("Run: catch exception");
    eptr_ = std::current_exception();
    // WHEELS_PANIC(
    //     "Uncaught exception in coroutine: " << wheels::CurrentExceptionMessage());
  }
  Ll("Run: after routine_()");

  completed_ = true;

  // if (eptr_) { 
  //   // Ll("Run: before rethrow_exception");
  //   // std::rethrow_exception(eptr);
  //   Ll("Run: return;");
  //   return;
  // }

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