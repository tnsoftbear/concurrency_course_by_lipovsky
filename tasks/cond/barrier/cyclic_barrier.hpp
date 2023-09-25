#pragma once

#include <cstddef>
#include <mutex>
#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

#include <cstdlib>

using twist::ed::stdlike::mutex;
using twist::ed::stdlike::condition_variable;

#include <twist/ed/stdlike/thread.hpp>
#include <sstream>
#include <iostream>

#include <twist/ed/stdlike/atomic.hpp>
using twist::ed::stdlike::atomic;

// Это решение использует атомики, иначе не проходят тесты ThreadSanitizer профиля:
// clippy target stress_tests FaultyThreadsTSan

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

class CyclicBarrier {
 public:
  explicit CyclicBarrier(size_t participants)
    : participants_(participants)
  {
    mtx_ = std::make_unique<mutex>();
    arrived_.store(0);
  }

  void ArriveAndWait() {
    Ll("~~~ ArriveAndWait() ~~~");
    if (arrived_.load() >= participants_ || departed_.load() != 0) {
      Ll("Limit-Lock because arrived_[%lu] >= participants_[%lu] || departed_[%lu] != 0", arrived_.load(), participants_, departed_.load());
      std::unique_lock<mutex> limit_reached_lock(*mtx_);  // тот же мьютекс, что и ниже
      limit_reached_cv_.wait(limit_reached_lock, [&] { 
        Ll("In limit-lock wait arrived_: %lu >= participants_: %lu", arrived_.load(), participants_);
        return arrived_.load() < participants_ && departed_.load() == 0; 
      });
    }

    auto arrival_waiting_lock = std::make_unique<std::unique_lock<mutex>>(*mtx_);
    // Здесь счётчик arrived_ защищён мьютексом и этого как буд-то бы должно быть достаточно и помогает избежать атомика..
    // Но такое решение не проходит при профиле ThreadSanitizer, поэтому используется атомик.
    size_t arrival_nr = arrived_.fetch_add(1);
    Ll("Arrival_nr: %lu (arrived: %lu)", arrival_nr, arrived_.load());
    cv_.wait(*arrival_waiting_lock, [&] { 
      Ll("In wait for full arrival group. arrival_nr: %lu, arrived_: %lu >= participants_: %lu", arrival_nr, arrived_.load(), participants_);
      return arrived_.load() >= participants_; 
    });
    if (arrived_.load() >= participants_ && !is_notified_) {
      is_notified_ = true;
      Ll("Notify all when arrival_nr: %lu (arrived: %lu)", arrival_nr, arrived_.load());
      cv_.notify_all();
    }
    departed_.fetch_add(1);
    Ll("Departed %lu for arrival_nr: %lu", departed_.load(), arrival_nr);
    if (departed_.load() >= arrived_.load()) {
      arrived_.store(0);
      departed_.store(0);
      is_notified_ = false;
      limit_reached_cv_.notify_all();
      Ll("Drop departed_/arrived_, unlock limit-lock");
    }
  }

  void Ll(const char* format, ...) {
    if (!kShouldPrint) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s %s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }

 private:
  size_t participants_{0};
  // Используются атомики, потому что иначе не проходят тесты с профилем FaultyThreadsTSan
  atomic<size_t> arrived_{0};
  atomic<size_t> departed_{0};
  std::unique_ptr<mutex> mtx_;
  condition_variable cv_;
  condition_variable limit_reached_cv_;
  bool is_notified_{false};
};
