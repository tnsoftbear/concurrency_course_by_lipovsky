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

// Это решение проходит: clippy target stress_tests Debug
// И зависает на: clippy target stress_tests FaultyThreadsASan

// const bool kShouldPrint{true};
const bool kShouldPrint{false};

class QueueSpinLock {
 public:
  class Guard {
    friend class QueueSpinLock;

   public:
    explicit Guard(QueueSpinLock& host)
        : host(host) {
      host.Acquire(this);
    }

    ~Guard() {
      host.Release(this);
    }

   public:
    QueueSpinLock& host;
    Guard* next{nullptr};
    atomic<bool> is_owner{false};
  };

 public:
   QueueSpinLock() {}

 private:
  void Acquire(Guard* waiter) {
    bool success = false;
    do { // CAS-loop
      mtx.lock();
      Guard* prev_tail = waiter->host.tail.load();
      Guard* tmp_tail = prev_tail;
      success = waiter->host.tail.compare_exchange_weak(tmp_tail, waiter);
      if (success) {
        if (prev_tail != nullptr) {
          waiter->is_owner.store(false);
          prev_tail->next = waiter;
        } else {
          waiter->is_owner.store(true);
        }
      }
      mtx.unlock();
    } while (!success);

    twist::ed::SpinWait spin_wait;
    while (!waiter->is_owner.load()) {
      spin_wait();
    }
  }

  void Release(Guard* owner) {
    if (owner->host.tail.load() == nullptr) {
      // nothing to release
      return;
    }

    bool success = false;
    do {
      mtx.lock();
      if (owner->next == nullptr) {
        Guard* tmp_owner = owner;
        success = owner->host.tail.compare_exchange_strong(tmp_owner, nullptr);
      } else {
        owner->next->is_owner.store(true);
        success = true;
      }
      mtx.unlock();
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

 public:
  atomic<Guard*> tail{nullptr};
  mutex mtx;

};
