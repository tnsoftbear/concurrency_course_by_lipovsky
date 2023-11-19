#pragma once

#include <array>
#include <span>

#include <twist/ed/stdlike/atomic.hpp>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

using twist::ed::stdlike::atomic;

// Single-Producer / Multi-Consumer Bounded Ring Buffer
template <typename T, size_t Capacity>
class WorkStealingQueue {
  struct Slot {
    T* item;
  };

 public:
 
  bool TryPush(T* item) {
    Ll("TryPush: starts, size: %lu", buffer_.size());
    size_t current_tail = tail_.load(std::memory_order_relaxed);
    //Ll("TryPush: current_tail: %lu", current_tail);
    //Ll("TryPush: next_tail: %lu", next_tail);
    while (true) {
      if (
        is_empty_.load()
        || current_tail != head_.load(std::memory_order_acquire)
      ) {
        size_t next_tail = (current_tail + 1) % Capacity;
        //tail_.store(next_tail, std::memory_order_release);
        bool success = tail_.compare_exchange_strong(current_tail, next_tail, std::memory_order_release);
        if (success) {
          buffer_[current_tail].item = item;
          is_empty_.store(false);
          return true;
        }
      } else {
        is_full_.store(true);
        return false;
      }
    }
  }

  T* TryPop() {
    Ll("TryPop: starts, is_empty_: %lu, is_full_: %lu", (int)is_empty_, (int)is_full_);
    size_t current_head = head_.load(std::memory_order_relaxed);

    //Ll("TryPop: current_head: %lu, tail: %lu", current_head, tail_.load());
    if (
      !is_full_.load()
      && current_head == tail_.load(std::memory_order_acquire)
    ) {
      is_empty_.store(true);
      return nullptr;  // Queue is empty
    }

    T* item = buffer_[current_head].item;
    head_.store((current_head + 1) % Capacity, std::memory_order_release);
    Ll("TryPop: change head: %lu", head_.load());
    is_full_.store(false);

    return item;
  }

  size_t Grab(std::span<T*> out_buffer) {
    if (is_empty_.load()) {
      return 0;
    }

    size_t current_head = head_.load(std::memory_order_relaxed);
    size_t current_tail = tail_.load(std::memory_order_acquire);
    size_t next_tail = (current_tail + 1) % Capacity;

    size_t count = 0;

    Ll("Grab: Start while: current_head: %lu, current_tail: %lu, next_tail: %lu", current_head, current_tail, next_tail);
    while (count < out_buffer.size()) {
      out_buffer[count++] = buffer_[current_head].item;
      current_head = (current_head + 1) % Capacity;
      Ll("Grab: current_head: %lu, current_tail: %lu", current_head, current_tail);
      if (current_head == current_tail) {
        Ll("Grab: current_head(%lu) == current_tail(%lu), break, count: %lu", current_head, current_tail, count);
        break;
      }
    }

    head_.store(current_head, std::memory_order_release);

    return count;
  }

 private:
  alignas(64) atomic<size_t> head_{0};
  alignas(64) atomic<size_t> tail_{0};
  std::array<Slot, Capacity> buffer_;
  atomic<bool> is_empty_{true};
  atomic<bool> is_full_{false};

  void Ll(const char* format, ...) {
    const bool k_should_print = true;
    if (!k_should_print) {
      return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s WorkStealingQueue::%s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};
