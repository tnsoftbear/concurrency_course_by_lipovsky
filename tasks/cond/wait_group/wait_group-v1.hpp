#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>
#include <twist/ed/wait/spin.hpp>

#include <cstdlib>
#include <iostream>
#include "twist/rt/layer/facade/wait/futex.hpp"

using twist::ed::stdlike::atomic;

// Плохая реализация с 2 атомиками, приводит к race condition, но не на тестах этого задания, а на тестах tasks/thread-pool

class WaitGroup {
 public:
  // += count
  void Add(size_t count) {
    counter_.fetch_add(count);
    done_.store(0);
  }

  // =- 1
  void Done() {
    uint32_t old = counter_.fetch_sub(1);
    if (old == 1) {
      done_.store(1);
      twist::ed::futex::WakeKey k = twist::ed::futex::PrepareWake(done_);
      twist::ed::futex::WakeAll(k);
    }
    //std::cout << "Counter: " << old - 1 << std::endl;
  }

  // == 0
  // One-shot
  void Wait() {
    //std::cout << "Before wait" << std::endl;
    while (done_.load() == 0) {
      twist::ed::futex::Wait(done_, 0);
    }
    //std::cout << "After wait" << std::endl;
  }

 private:
  atomic<uint32_t> counter_{0};
  atomic<uint32_t> done_{1};
};
