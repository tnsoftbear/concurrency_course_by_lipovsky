#pragma once

#include "atomic_stamped_ptr.hpp"

#include <twist/ed/stdlike/atomic.hpp>

#include <optional>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

// Treiber unbounded lock-free stack
// https://en.wikipedia.org/wiki/Treiber_stack

using twist::ed::stdlike::atomic;

//const bool kShouldPrint = false;
const bool kShouldPrint = true;

template <typename T>
class LockFreeStack {
  struct Node {
    T value;
    Node* next{nullptr};
  };

 public:
  void Push(T item) {
    Ll("Push: starts");
    Node* new_node = new Node{std::move(item), nullptr};
    new_node->next = head_.load();
    while (!head_.compare_exchange_weak(new_node->next, new_node)) {};
  }

  std::optional<T> TryPop() {
    Ll("Pop: starts");

    Node* old_head = head_.load();
    while (true) {
      if (old_head == nullptr) {
        return std::nullopt;
      }

      if (head_.compare_exchange_weak(old_head, old_head->next)) {
        T item = std::move(old_head->value);
        delete old_head;
        return item;
      }
    }    

    // Node* old_head = head_.load();
    // while (old_head && !head_.compare_exchange_weak(old_head, old_head->next)) {};
    // if (old_head) {
    //   T item = std::move(old_head->value);
    //   delete old_head;
    //   return item;
    // }

    return std::nullopt;
  }

  ~LockFreeStack() {
    while (head_.load() != nullptr) {
      auto old_head = head_.load();
      head_.store(old_head->next);
      delete old_head;
    }
  }

 private:
  atomic<Node*> head_{nullptr};


  void Ll(const char* format, ...) {
    if (!kShouldPrint) {
        return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s LockFreeStack::%s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};
