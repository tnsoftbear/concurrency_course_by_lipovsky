#pragma once

#include "atomic_stamped_ptr.hpp"

#include <atomic>
#include <mutex>
#include <twist/ed/stdlike/atomic.hpp>

#include <optional>

#include <iostream>

// Treiber unbounded lock-free stack
// https://en.wikipedia.org/wiki/Treiber_stack

using twist::ed::stdlike::atomic;

std::memory_order acq = std::memory_order_acquire;
std::memory_order rel = std::memory_order_release;
std::memory_order rlx = std::memory_order_relaxed;
std::memory_order seq = std::memory_order_seq_cst;

template <typename T>
class LockFreeStack {
  struct Node {
    T value;
    StampedPtr<Node> next{nullptr, 0};
    atomic<int> inner_counter{0};

    explicit Node(T&& v) : value(std::move(v)) {};
  };

 public:
  void Push(T item) {
    Node* new_node = new Node(std::move(item));
    StampedPtr<Node> new_head = StampedPtr<Node>{new_node, 0};
    new_head->next = head_.Load();
    while (!head_.CompareExchangeWeak(new_head->next, new_head, rel, rlx)) {};
  }

  std::optional<T> TryPop() {
    StampedPtr<Node> old_head;
    for (;;) {
      do {
        old_head = head_.Load();
        if (!old_head) {
          return std::nullopt;
        }
      } while (!head_.CompareExchangeWeak(old_head, old_head.IncrementStamp(), acq, rlx));
      old_head = old_head.IncrementStamp();

      auto tmp_head = old_head;
      if (head_.CompareExchangeWeak(tmp_head, old_head->next, acq, rlx)) {
        T value = std::move(old_head->value);
        int res = 1 - old_head.stamp;
        if (old_head->inner_counter.fetch_add(old_head.stamp - 1) == res) {
          delete old_head.raw_ptr;
        }
        return value;
      } else {
        if (old_head->inner_counter.fetch_sub(1) == 1) {
          delete old_head.raw_ptr;
        }
      }
    }
  }

  ~LockFreeStack() {
    StampedPtr<Node> head = head_.Load();
    while (head) {
      StampedPtr<Node> old_head = head;
      head = old_head->next;
      delete old_head.raw_ptr;
    }
  }

 private:
  AtomicStampedPtr<Node> head_{StampedPtr<Node>{nullptr, 0}};

};

/**
  * В TryPop() можно свести inner-counter через outer-counter и так,
  * но так на 1 fetch-операцию больше:

        old_head->inner_counter.fetch_add(old_head.stamp);
        if (old_head->inner_counter.fetch_sub(1) == 1) {
          delete old_head.raw_ptr;
        }
 
 *
 * Полезняшки:
 *

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

  //const bool kShouldPrint = false;
  const bool kShouldPrint = true;

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

  void PrintChain() {
    StampedPtr<Node> head = head_.Load();
    while (head) {
      std::cout << head.raw_ptr << "(o: " << head.stamp << ", i: " << head->inner_counter.load() << "), ";
      head = head->next;
    }
    std::cout << std::endl;
  }

*/