#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/wait/spin.hpp>
#include <twist/ed/stdlike/thread.hpp>

#include <sstream>
#include <iostream>
#include <string>

using twist::ed::stdlike::atomic;
using twist::ed::stdlike::mutex;

/*
 * Scalable Queue SpinLock
 *
 * Usage:
 *
 * QueueSpinLock spinlock;
 *
 * {
 *   QueueSpinLock::Guard guard(spinlock);  // <-- Acquire
 *   // <-- Critical section
 * }  // <-- Release
 *
 */

// const bool kShouldPrint{true};
const bool kShouldPrint{false};

class QueueSpinLock {
 public:
  class Guard {
    friend class QueueSpinLock;

   public:
    explicit Guard(QueueSpinLock& host)
        : host_(host) {
      host_.Acquire(this);
    }

    ~Guard() {
      host_.Release(this);
    }

   private:
    QueueSpinLock& host_;
    atomic<Guard*> next_{nullptr};
    atomic<bool> is_owner_{false};
  };

 public:
   QueueSpinLock() {}

 private:
  void Acquire(Guard* waiter) {
    Guard* prev_tail = waiter->host_.tail_.exchange(waiter);
    bool is_owner = prev_tail == nullptr;
    waiter->is_owner_.store(is_owner);
    if (!is_owner) {
      prev_tail->next_.store(waiter);
    }

    twist::ed::SpinWait spin_wait;
    while (!waiter->is_owner_.load()) {
      spin_wait();
    }
  }

  void Release(Guard* owner) {
    bool success = false;
    do {
      if (owner->next_.load() == nullptr) {
        Guard* tmp_owner = owner;
        success = owner->host_.tail_.compare_exchange_strong(tmp_owner, nullptr);
      } else {
        owner->next_.load()->is_owner_.store(true);
        success = true;
      }
    } while (!success);
  }

  void Ll(const char* format, ...) {
    if (!kShouldPrint) {
      return;
    }

    char buf [50];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s %s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }

 private:
  atomic<Guard*> tail_{nullptr};
};
