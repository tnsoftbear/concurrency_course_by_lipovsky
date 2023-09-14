#pragma once

#include <cstddef>
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

const bool kShouldPrint{true};
//const bool kShouldPrint{false};

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
    bool is_owner{true};
  };

 public:
   QueueSpinLock() {}

 private:
  void Acquire(Guard* waiter) {
    char* s = PrintChain(waiter);
    Ll("Acquire| Start for waiter(%p) (%s)", (void*)waiter, s);
    delete s;

    Guard* prev_tail = waiter->host.tail.exchange(waiter);
    if (prev_tail != nullptr) {
      waiter->is_owner = false;
      prev_tail->next = waiter;
      Ll("Acquire| waiter(%p)->is_owner=false: and prev_tail(%p)->next=waiter", waiter, prev_tail);

      twist::ed::SpinWait spin_wait;
      while (!waiter->is_owner) {
        Ll("Acquire| Spinning waiter(%p)", (void*)waiter);
        spin_wait();
      }
      Ll("Acquire| After spin for waiter(%p)", (void*)waiter);
    } else {
      waiter->is_owner = true;
      Ll("Acquire| Waiter(%p) is owner free of spin", (void*)waiter);
    }
  }

  void Release(Guard* owner) {
    char* s = PrintChain(owner);
    Ll("Release: Start for owner(%p) (%s)", (void*)owner, s);
    delete s;

    Guard* owner_tmp = owner;
    QueueSpinLock& host = owner->host;
    if (host.tail.compare_exchange_strong(owner_tmp, nullptr)) {
      Ll("Release: Released owner(%p) when it is tail", (void*)owner);
      return;
    }

    while (owner->next == nullptr) {
      Ll("while (owner(%p)->next == nullptr)", owner);
    }
    owner->next->is_owner = true;
    Ll("Release: Released owner(%p) and owner->next(%p)->is_owner=true", (void*)owner, (void*)owner->next);
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

    std::string buf("");
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "] ";
    buf.append(pid.str());
    buf.append(format);
    buf.append("\n");
    va_list args;
    va_start(args, format);
    vprintf(&*buf.begin(), args);
    va_end(args);
  }

 public:
  twist::ed::stdlike::atomic<Guard*> tail{nullptr};
};
