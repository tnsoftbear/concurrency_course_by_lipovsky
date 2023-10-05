#pragma once

#include <stack>
#include <sure/context.hpp>
#include <sure/stack.hpp>

#include <function2/function2.hpp>

#include <exception>
#include "sure/stack/mmap.hpp"

// Simple stackful coroutine

void Ll(const char* format, ...);

struct ContextNode {
  sure::ExecutionContext* caller_context = nullptr;
  sure::ExecutionContext* own_context = nullptr;
};

class Coroutine : private sure::ITrampoline {
 public:
  using Routine = fu2::unique_function<void()>;

  explicit Coroutine(Routine routine);

  void Resume();

  // Suspend running coroutine
  static void Suspend();

  bool IsCompleted() const;

 public:
  static std::stack<ContextNode> context_stack;

 private:
  Routine routine_;
  sure::ExecutionContext context_;
  sure::ExecutionContext caller_context_;
  sure::Stack stack_;
  bool completed_{false};
  std::exception_ptr eptr_;

 private:
  // ITrampoline
  void Run() noexcept override;
};
