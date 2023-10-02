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


// Это решение не использует атомики, а синхронизирует доступ к счётчикам через мьютекс.
// Но оно тоже не проходит тест со ThreadSanitizer
// clippy target stress_tests FaultyThreadsTSan

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

class CyclicBarrier {
 public:
  explicit CyclicBarrier(size_t participants)
    : participants_(participants)
  {
    mtx_ = std::make_unique<mutex>();
    arrived_mtx_ = std::make_unique<mutex>();
    departed_mtx_ = std::make_unique<mutex>();
  }

  void ArriveAndWait() {
    Ll("~~~ ArriveAndWait() ~~~");
    if (arrived_ >= participants_ || departed_ != 0) {
      Ll("Limit-Lock because arrived_[%lu] >= participants_[%lu] || departed_[%lu] != 0", arrived_, participants_, departed_);
      std::unique_lock<mutex> limit_lock(*mtx_);  // тот же мьютекс, что и ниже
      limit_reached_cv_.wait(limit_lock, [&] { 
        Ll("In limit-lock wait arrived_: %lu >= participants_: %lu", arrived_, participants_);
        return arrived_ < participants_ && departed_ == 0; 
      });
    }

    // защищаем операцию ++arrived_ мьютексом чтобы не делать атомик
    std::unique_lock<mutex> lock(*mtx_);

Ll("1a]");
    arrived_mtx_->lock(); // mutex JIC. Операция уже под мьютексом, что выше
Ll("2a]");
    arrived_ = arrived_ + 1;
    size_t arrival_nr = arrived_; 
Ll("3a]");
    arrived_mtx_->unlock();

    Ll("Arrival_nr: %lu (arrived: %lu)", arrival_nr, arrived_);
    arrival_waiting_cv_.wait(lock, [&] { 
      Ll("In wait for full arrival group. arrival_nr: %lu, arrived_: %lu >= participants_: %lu", arrival_nr, arrived_, participants_);
      return arrived_ == participants_; 
    });
    if (arrived_ == participants_ && !is_notified_) {
      is_notified_ = true;
      Ll("Notify all when arrival_nr: %lu (arrived: %lu)", arrival_nr, arrived_);
      arrival_waiting_cv_.notify_all();
    }

  // защищаем операцию ++departed_ мьютексом чтобы не делать атомик
Ll("1d]");
    departed_mtx_->lock();
Ll("2d]");
    departed_ = departed_ + 1;
Ll("3d]");
    departed_mtx_->unlock();

    if (departed_ == arrived_) {
      arrived_ = 0;
      departed_ = 0;
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
  volatile size_t arrived_{0};
  volatile size_t departed_{0};
  std::unique_ptr<mutex> mtx_;
  std::unique_ptr<mutex> arrived_mtx_;
  std::unique_ptr<mutex> departed_mtx_;
  bool is_notified_{false};
  condition_variable arrival_waiting_cv_;
  condition_variable limit_reached_cv_;
};
