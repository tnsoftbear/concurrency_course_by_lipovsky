#pragma once

#include <condition_variable>
#include <mutex>
#include <twist/ed/stdlike/atomic.hpp>

#include <cstdlib>
#include <iostream>
#include <twist/ed/stdlike/condition_variable.hpp>
#include <twist/ed/stdlike/mutex.hpp>

using twist::ed::stdlike::atomic;
using twist::ed::stdlike::condition_variable;
using twist::ed::stdlike::mutex;

class WaitGroup {
 public:
  // += count
  void Add(size_t count) {
    counter_.fetch_add(count);
  }

  // =- 1
  void Done() {
    if (counter_.fetch_sub(1) == 1) {
      std::lock_guard guard(mtx_);
      idle_cv_.notify_all();
    }
  }

  // == 0
  // One-shot
  void Wait() {
    std::unique_lock lock(mtx_);
    idle_cv_.wait(lock, [this]() { return counter_.load() == 0; });
  }

  uint32_t GetCounter() {
    return counter_.load();
  }

 private:
  atomic<uint32_t> counter_{0};
  condition_variable idle_cv_;
  mutex mtx_;
};
