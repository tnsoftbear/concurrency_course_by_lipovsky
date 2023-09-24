#pragma once

#include <cstddef>
#include <mutex>
#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>
#include <twist/ed/stdlike/atomic.hpp>

#include <cstdlib>

using twist::ed::stdlike::mutex;
using twist::ed::stdlike::condition_variable;

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/stdlike/thread.hpp>
#include <sstream>
#include <iostream>

using twist::ed::stdlike::atomic;

// clippy target stress_tests FaultyFibers

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

class CyclicBarrier {
 public:
  explicit CyclicBarrier(size_t participants)
    : participants_(participants)
  {
    mtx_ = std::make_unique<mutex>();
    //limit_mtx_ = std::make_unique<mutex>();
    arrived_.store(0);
  }

  void ArriveAndWait() {
    Ll("~~~ ArriveAndWait() ~~~");
    if (arrived_.load() >= participants_ || departed_ != 0) {
      Ll("Limit-Lock because arrived_.load()[%lu] >= participants_[%lu] || departed_[%lu] != 0", arrived_.load(), participants_, departed_);
      std::unique_lock<mutex> lock(*mtx_);
      cv_limit_mtx_.wait(lock, [&] { 
        //Ll("In wait arrival_nr: %lu, arrived_: %lu >= participants_: %lu", arrival_nr, arrived_.load(), participants_);
        return arrived_.load() < participants_ && departed_ == 0; 
      });
    }

    // while (arrived_.load() >= participants_ || departed_ != 0) {
    // }

    size_t arrival_nr = arrived_.fetch_add(1) + 1;
    Ll("Arrival_nr: %lu (arrived: %lu)", arrival_nr, arrived_.load());
    std::unique_lock<mutex> lock(*mtx_);
    cv_.wait(lock, [&] { 
      //Ll("In wait arrival_nr: %lu, arrived_: %lu >= participants_: %lu", arrival_nr, arrived_.load(), participants_);
      return arrived_.load() >= participants_; 
    });
    if (arrived_.load() >= participants_ && !is_notified_) {
      is_notified_ = true;
      Ll("Notify all when arrival_nr: %lu (arrived: %lu)", arrival_nr, arrived_.load());
      cv_.notify_all();
    }
    ++departed_;
    if (departed_ >= arrived_.load()) {
      arrived_.store(0);
      departed_ = 0;
      is_notified_ = false;
      cv_limit_mtx_.notify_all();
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
  atomic<size_t> arrived_{0};
  size_t departed_{0};
  condition_variable cv_;
  //std::unique_lock<mutex> lock_;
  std::unique_ptr<mutex> mtx_;
  //std::unique_ptr<mutex> limit_mtx_;
  bool is_notified_{false};
  condition_variable cv_limit_mtx_;
};
