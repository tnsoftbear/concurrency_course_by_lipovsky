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

/**
 Сначала, я сохранял корректные значения индекса массива в tail_ и head_, которое высчитывается спомощью ToIndex(),
 но стресс тесты падали. Потом я перестал этого делать, и стал сохранять в head_, tail_ последовательно увеличенные значения,
 в то же время корректный индекс массива буфера стал высчитывать в момент обращения к элементу массива для записи или чтения,
 это решило проблему. 
 Вероятно, это решает проблему ABA при стравнении значения в CAS. Иначе, счётчик мог прокрутить круг и привести к неверным выводам из-за ложного совпадения значений.
 Аналогичное решение можно видеть здесь:
https://gitlab.com/Lipovsky/await/-/blob/master/await/tasks/exe/pools/fast/queues/work_stealing_queue.hpp?ref_type=heads

 Про мемори-ордеринги:
https://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular?msg=4970744#xx4970744xx
*/ 

std::memory_order relaxed = std::memory_order_relaxed;
std::memory_order acquire = std::memory_order_acquire;
std::memory_order release = std::memory_order_release;
// std::memory_order  = std::memory_order_;
// std::memory_order  = std::memory_order_;

template <typename T, size_t Capacity>
class WorkStealingQueue {
  struct Slot {
    atomic<T*> item{nullptr};
  };

  static constexpr size_t kRealCapacity = Capacity + 1;

 public:
  /* Producer only: updates tail index after setting the element in place */
  bool TryPush(T* item) {
    if (IsFull()) {
      return false;
    }
    size_t current_tail = tail_.load(relaxed);
    buffer_[ToIndex(current_tail)].item.store(item, relaxed);
    tail_.store(current_tail + 1, release);
    return true;
  }

  /* Consumer only: updates head index after retrieving the element */
  T* TryPop() {
    size_t current_head = head_.load(relaxed);
    while (true) {
      if (IsEmpty()) {
        return nullptr;
      }

      T* item = buffer_[ToIndex(current_head)].item.load(relaxed);
      if (head_.compare_exchange_strong(current_head, current_head + 1, release)) {
        return item;
      }
    }
  }

  size_t Grab(std::span<T*> out_buffer) {
    size_t current_head = head_.load(relaxed);

    while (true) {
      size_t element_count = CountElements();
      if (element_count == 0) {
        return 0;
      }

      size_t to_grab = out_buffer.size() < element_count ? out_buffer.size() : element_count;
      for (size_t i = 0; i < to_grab; ++i) {
        size_t index = ToIndex(current_head + i);
        out_buffer[i] = buffer_[index].item.load(relaxed);
      }
      
      if (head_.compare_exchange_strong(current_head, current_head + to_grab, release)) {
        return to_grab;
      }
    }
  }

 private:
  alignas(64) atomic<size_t> head_{0};
  alignas(64) atomic<size_t> tail_{0};
  std::array<Slot, kRealCapacity> buffer_;

 private:
  bool IsEmpty() const {
    return head_.load(relaxed) == tail_.load(relaxed);
  }

  bool IsFull() const {
    return tail_.load(relaxed) - head_.load(relaxed) == Capacity;
  }

  size_t Increment(size_t value) const {
    return ToIndex(value + 1);
  }

  size_t CountElements() const {
    return tail_.load(acquire) - head_.load(acquire);
  }

  static size_t ToIndex(size_t pos) {
    return pos % kRealCapacity;
  }

  void Ll(const char* format, ...) {
    //const bool k_should_print = true;
    const bool k_should_print = false;
    if (!k_should_print) {
      return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s WorkStealingQueue::%s, Capacity: %lu, kRealCapacity: %lu, buf-size: %lu, elem-count: %lu, head: %lu, tail: %lu, this: %p\n", pid.str().c_str(), format, Capacity, kRealCapacity, buffer_.size(), CountElements(), head_.load(), tail_.load(), (void*)this);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};
