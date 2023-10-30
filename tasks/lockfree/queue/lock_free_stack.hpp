#pragma once

#include <twist/ed/stdlike/atomic.hpp>

//#include <hazard/manager.hpp>
#include <hazard/hazard.cpp>
#include <hazard/mutator.cpp>

#include <optional>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>
#include "hazard/mutator.hpp"

// Treiber unbounded MPMC lock-free stack

const bool kShouldPrint = true;
//const bool kShouldPrint = false;

template <typename T>
class LockFreeStack {
  struct Node {
    T item;
    Node* next{nullptr};
  };

 public:
  explicit LockFreeStack(hazard::Manager* gc)
      : gc_(gc) {
  }

  LockFreeStack()
      : LockFreeStack(hazard::Manager::Get()) {
  }

  void Push(T item) {
    Ll("Push: starts");
    Node* new_node = new Node{std::move(item)};
    new_node->next = top_.load();

    while (!top_.compare_exchange_weak(new_node->next, new_node)) {
      ;
    }
  }

  std::optional<T> TryPop() {
    Ll("TryPop: starts");
    auto mutator = gc_->MakeMutator();

    while (true) {
      Node* curr_top = mutator->Protect(0, top_);

      if (curr_top == nullptr) {
        return std::nullopt;
      }

      if (top_.compare_exchange_weak(curr_top, curr_top->next)) {
        T item = std::move(curr_top->item);
        mutator->Retire(curr_top);
        return item;
      }
    }
  }

  ~LockFreeStack() {
    Node* curr = top_.load();
    while (curr != nullptr) {
      Node* next = curr->next;
      delete curr;
      curr = next;
    }
  }

 private:
  hazard::Manager* gc_;
  twist::ed::stdlike::atomic<Node*> top_{nullptr};

 private:
  void Ll(const char* format, ...) {
    if (!kShouldPrint) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    //sprintf(buf, "%s Strand::%s, qc: %lu\n", pid.str().c_str(), format, tasks_.Count());
    sprintf(buf, "%s LockFreeStack::%s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};
