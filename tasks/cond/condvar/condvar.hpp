#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>

#include <cstdint>

#include "mutex.hpp"

#include <iostream>

namespace stdlike {

using twist::ed::futex::WakeKey;
using twist::ed::futex::PrepareWake;
using twist::ed::stdlike::atomic;

class CondVar {
 public:
  static const int kAtomicsSize = 100;
  CondVar() {
    for (int i=0; i<kAtomicsSize; i++) {
      atomics_[i] = new atomic<uint32_t>(0);
      atomics_[i]->store(0);
    }
  }

  ~CondVar() {
    for (int i=0; i<kAtomicsSize; i++) {
      delete atomics_[i];
    }
  }

  template <class Mutex>
  void Wait(Mutex& m) {
    uint32_t tail = t_.fetch_add(1);
    m.unlock();
    twist::ed::futex::Wait(*atomics_[tail%kAtomicsSize], 0);
    m.lock();
    atomics_[tail%kAtomicsSize]->store(0);
  }

  void NotifyOne() {
    if (h_.load() >= t_.load()) {
      return;
    }

    uint32_t head = h_.fetch_add(1);
    atomics_[head%kAtomicsSize]->store(1);
    //std::cout << "notifyone Wake on head:\t" << head << std::endl;
    auto k = twist::ed::futex::PrepareWake(*atomics_[head%kAtomicsSize]);
    WakeOne(k);
  }

  void NotifyAll() {
    if (h_.load() >= t_.load()) {
      return;
    }

    uint32_t head;
    while (h_.load() < t_.load()) {
      head = h_.fetch_add(1);
      atomics_[head%kAtomicsSize]->store(1);
      auto k = twist::ed::futex::PrepareWake(*atomics_[head%kAtomicsSize]);
      WakeAll(k);
      //std::cout << "NotifyAll Wake on head:\t" << head << std::endl;
    }
  }

 private:
  atomic<uint32_t>* atomics_[kAtomicsSize];
  atomic<uint32_t> h_{0};
  atomic<uint32_t> t_{0};
};

}  // namespace stdlike
