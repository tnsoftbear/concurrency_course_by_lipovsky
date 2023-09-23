#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/spin.hpp>
#include <twist/ed/stdlike/thread.hpp>

#include <sstream>
#include <iostream>
#include <string>

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

const bool kShouldPrint{true};
// const bool kShouldPrint{false};

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
    bool locked{false};
  };

 public:
   QueueSpinLock() {}

 private:
  void Acquire(Guard* waiter) {
    char* s = PrintChain(waiter);
    Ll("Start Acquire for waiter: %p (%s)", (void*)waiter, s);
    delete s;
    QueueSpinLock& host = waiter->host;

    bool success = false;
    do { // CAS-loop
      Guard* prev_tail = host.tail.load();
      Guard* tmp_tail = prev_tail;
      success = host.tail.compare_exchange_weak(tmp_tail, waiter);
      if (success) {
        if (prev_tail != nullptr) {
          waiter->locked = true;
          Ll("Set waiter->locked=true: %p", (void*)waiter);
          prev_tail->next = waiter;
          Ll("Set prev_tail->next: %p for prev_tail: %p", (void*)waiter, (void*)prev_tail);
        } else {
          waiter->locked = false;
          Ll("First node set: %p", (void*)waiter);
        }
        Ll("Successfully changed tail from %p to %p", (void*)prev_tail, (void*)waiter);
      } else {
        Ll("Cannot change tail from %p to %p", (void*)prev_tail, (void*)waiter);
      }
    } while (!success);

    char* ss = PrintChain(waiter);
    Ll("Before spin for waiter: %p (%s)", (void*)waiter, s);
    delete(ss);

    twist::ed::SpinWait spin_wait;
    while (waiter->locked) {
      spin_wait();
      Ll("Spinning waiter: %p", (void*)waiter);
    }
    Ll("After spin for waiter: %p", (void*)waiter);
  }

  void Release(Guard* owner) {
    char* s = PrintChain(owner);
    Ll("Release: Start for owner: %p (%s)", (void*)owner, s);
    delete s;

    QueueSpinLock& host = owner->host;
    if (host.tail.load() == nullptr) {
      // nothing to release
      Ll("Release: Nothing to release host.tail is null");
      return;
    }

    bool success = false;
    do {
      if (owner->next == nullptr) {
        Guard* tmp_owner = owner;
        success = host.tail.compare_exchange_strong(tmp_owner, nullptr);
        if (success) {
          Ll("Release: Ok changed tail to null");
        } else {
          Ll("Release: Not changed tail to null, will repeat");
        }
      } else {
        owner->next->locked = false;
        success = true;
        Ll("Release: owner->next->locked=false");
      }
    } while (!success);

    Ll("Release: Released owner: %p", (void*)owner);
  }

  char* PrintChain(Guard* guard) {
    char* out = new char[256];
    if (!kShouldPrint) {
      return out;
    }

    Guard* node = guard;
    do {
      sprintf(out + strlen(out), " -> %p", (void*)node);
      node = node->next;
    } while(node != nullptr);

    QueueSpinLock& host = guard->host;
    auto tail_node = host.tail.load();
    sprintf(out + strlen(out), " (tail: %p)", (void*)tail_node);
    return out;
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
  twist::ed::stdlike::atomic<Guard*> tail{nullptr};
};
