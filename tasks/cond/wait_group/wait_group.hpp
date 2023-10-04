#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>
#include <twist/ed/wait/spin.hpp>

#include <cstdlib>
#include <sstream>
#include <twist/ed/stdlike/thread.hpp> // for debug logs

using twist::ed::stdlike::atomic;

// Реализация с 2 атомиками

class WaitGroup {
 public:
  explicit WaitGroup(uint32_t init = 0) 
    : init_{init}
    {}

  // += count
  void Add(size_t count) {
    if (counter_.fetch_add(count) == init_) {
      done_.store(0);
    }
  }

  // =- 1
  void Done() {
    if (counter_.fetch_sub(1) - 1 == init_) {
      done_.store(1);
      twist::ed::futex::WakeKey k = twist::ed::futex::PrepareWake(done_);
      twist::ed::futex::WakeAll(k);
    }
  }

  // == 0
  // One-shot
  void Wait() {
    while (counter_.load() != init_) {
      twist::ed::futex::Wait(done_, 0);
    }
  }

  uint32_t GetCounter() {
    return counter_.load();
  }

 private:
  atomic<uint32_t> counter_{0};
  atomic<uint32_t> done_{1};
  uint32_t init_{0};
  bool should_print_{true};

 private:
  void Ll(const char* format, ...) {
    if (!should_print_) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s %s done: %d, counter: %d\n", pid.str().c_str(), format, (int)done_.load(), (int)counter_.load());
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};
