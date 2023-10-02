#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>
#include <twist/ed/wait/spin.hpp>

#include <cstdlib>
#include <iostream>
#include "twist/rt/layer/facade/wait/futex.hpp"
#include <twist/ed/stdlike/thread.hpp>

using twist::ed::stdlike::atomic;

class WaitGroup {
 public:
  // += count
  void Add(size_t count) {
    Ll("WG-Add: start");
    done_.store(0);
    counter_.fetch_add(count);
    Ll("WG-Add: end");
  }

  // =- 1
  void Done() {
    Ll("WG-Done: before counter_.fetch_sub(1)");
    if (counter_.fetch_sub(1) == 1) {
      Ll("WG-Done: before done_.store(1)");
      done_.store(1);
      Ll("WG-Done: after done_.store(1), before wake");
      twist::ed::futex::WakeKey k = twist::ed::futex::PrepareWake(done_);
      twist::ed::futex::WakeAll(k);
    }
  }

  // == 0
  // One-shot
  void Wait() {
    Ll("WG-Wait: Before wait");
    while (done_.load() == 0) {
      twist::ed::futex::Wait(done_, 0);
      Ll("WG-Wait: After wait");
    }
  }

  uint32_t GetCounter() {
    return counter_.load();
  }

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

 private:
  atomic<uint32_t> counter_{0};
  atomic<uint32_t> done_{1};
  bool should_print_{true};
};
