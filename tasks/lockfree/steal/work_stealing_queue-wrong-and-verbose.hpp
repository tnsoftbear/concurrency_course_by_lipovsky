#pragma once

#include <array>
#include <atomic>
#include <span>

#include <twist/ed/stdlike/atomic.hpp>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

using twist::ed::stdlike::atomic;

// Single-Producer / Multi-Consumer Bounded Ring Buffer
// Паттерн доступа – вариация _Single Producer_ / _Multiple Consumers_:
// - Методы `TryPush` и `TryPop` вызывает только один выделенный поток.
// - Метод `Grab` могут вызывать конкурентно (с другими `Grab` и `TryPop` /
// `TryPush`) нескольких потоков.

template <typename T, size_t Capacity>
class WorkStealingQueue {
  struct Slot {
    T* item;
  };

  static constexpr size_t kRealCapacity = Capacity + 1;

 public:
  /* Producer only: updates tail index after setting the element in place */
  bool TryPush(T* item) {
    printf("TryPush\n");
    Ll("TryPush: starts");
    if (IsFull()) {
      Ll("TryPush: is full");
      return false;
    }
    size_t current_tail = tail_.load();
    size_t next_tail = Increment(current_tail);
    auto v = item->value;
    buffer_[current_tail].item = item;
    tail_.store(next_tail);
    Ll("TryPush: ends, idx: %lu, value: %lu", current_tail, v);
    return true;
  }

  // bool TryPush(T* item) {
  //   Ll("TryPush: starts");
  //   while (true) {
  //     if (IsFull()) {
  //       Ll("TryPush: full, return false;");
  //       return false;
  //     }

  //     size_t current_tail = tail_.load();
  //     size_t next_tail = Increment(current_tail);
  //     T* old_item = buffer_[current_tail].item;
  //     buffer_[current_tail].item = item;
  //     if (tail_.compare_exchange_strong(current_tail, next_tail)) {
  //       Ll("TryPush: ends");
  //       return true;
  //     } else {
  //       buffer_[current_tail].item = old_item;
  //       Ll("TryPush: repeat iteration");
  //     }
  //     //tail_.store(next_tail, std::memory_order::release);
  //   }
  // }

  /* Consumer only: updates head index after retrieving the element */
  T* TryPop() {
    Ll("TryPop: starts, is_empty_: %lu, is_full_: %lu", (int)IsEmpty(), (int)IsFull());

    size_t current_head = head_.load();
    while (true) {
      if (IsEmpty()) {
        Ll("TryPop: return null");
        return nullptr;
      }

      T* item = buffer_[current_head].item;
      if (head_.compare_exchange_strong(current_head, Increment(current_head))) {
        Ll("TryPop: return item, idx: %lu, value: %lu", current_head, item->value);
        return item;
      } else {
        Ll("TryPop: repeat iteration");
      }
    }
  }

  size_t Grab(std::span<T*> out_buffer) {
    Ll("Grab: starts, out_buffer.size: %lu", out_buffer.size());
    size_t current_head = head_.load();

    while (true) {
      size_t size = CountElements();
      if (size == 0) {
        Ll("Grab: return 0");
        return 0;
      }

      size_t to_grab = out_buffer.size() < size ? out_buffer.size() : size;
      for (size_t i = 0; i < to_grab; ++i) {
        size_t index = ToIndex(current_head + i);
        Ll("Grab: moving index: %lu ...", index);
        //auto v = buffer_[index].item->value;
        out_buffer[i] = buffer_[index].item;
        //Ll("Grab: moved index: %lu, value: %lu", index, v);
      }
      
      size_t new_head = ToIndex(current_head + to_grab);
      Ll("Grab: before compare_exchange_strong, current_head: %lu, new_head: %lu", current_head, new_head);
      if (head_.compare_exchange_strong(current_head, new_head)) {
        Ll("Grab: return to_grab: %lu", to_grab);
        return to_grab;
      } else {
        Ll("Grab: repeat iteration");
      }

      Ll("Grab: repeat iteration");
    }
  }

 private:
  alignas(64) atomic<size_t> head_{0};
  alignas(64) atomic<size_t> tail_{0};
  std::array<Slot, kRealCapacity> buffer_;

 private:
  bool IsEmpty() const {
    return (head_.load() == tail_.load());
  }

  bool IsFull() const {
    const auto next_tail = Increment(tail_.load() + Capacity - 1);
    return (next_tail == head_.load());
  }

  size_t Increment(size_t value) const {
    return ToIndex(value + 1);
  }

  size_t CountElements() const {
    size_t current_tail = tail_.load();
    size_t current_head = head_.load();
    return current_tail >= current_head
      ? current_tail - current_head
      : kRealCapacity - (current_head - current_tail);
  }

  static size_t ToIndex(size_t pos) {
    return pos % kRealCapacity;
  }

  void Ll(const char* format, ...) {
    const bool k_should_print = true;
    //const bool k_should_print = false;
    if (!k_should_print) {
      return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s WorkStealingQueue::%s, buf-size: %lu, elem-count: %lu, head: %lu, tail: %lu, this: %p\n", pid.str().c_str(), format, buffer_.size(), CountElements(), head_.load(), tail_.load(), (void*)this);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};
