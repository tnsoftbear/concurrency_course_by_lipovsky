#pragma once

#include <atomic>
#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>
#include <twist/ed/wait/spin.hpp>

#include <cstdlib>
#include <sstream>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs

using twist::ed::stdlike::atomic;

// Реализация с 2 атомиками

namespace exe::threads {

class WaitGroup {
 public:
  explicit WaitGroup() {
  }

  // += count
  void Add(size_t count) {
    counter_.fetch_add(count, std::memory_order_relaxed);
    done_.store(0, std::memory_order_relaxed);
  }

  // =- 1
  void Done() {
    if (0 == counter_.fetch_sub(1, std::memory_order_acq_rel) - 1) {
      twist::ed::futex::WakeKey k = twist::ed::futex::PrepareWake(done_);
      done_.store(1);
      twist::ed::futex::WakeAll(k);
    }
  }

  // == 0
  // One-shot
  void Wait() {
    while (0 == done_.load(std::memory_order_acquire)) {
      twist::ed::futex::Wait(done_, 0, std::memory_order_relaxed);
    }
  }

  uint32_t GetCounter() {
    return counter_.load();
  }

 private:
  atomic<uint32_t> counter_{0};
  atomic<uint32_t> done_{1};

 private:
  void Ll(const char* format, ...) {
    const bool should_print = false;
    if (!should_print) {
      return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s %s done: %d, counter: %d\n", pid.str().c_str(), format,
            (int)done_.load(), (int)counter_.load());
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};

}  // namespace exe::threads