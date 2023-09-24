#pragma once

#include <cstddef>
#include <mutex>
#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>
#include <twist/ed/stdlike/atomic.hpp>

#include <twist/ed/wait/sys.hpp>

#include <cstdlib>

using twist::ed::stdlike::mutex;
using twist::ed::stdlike::condition_variable;

#include <iostream>

using twist::ed::stdlike::atomic;

// clippy target stress_tests FaultyFibers

class CyclicBarrier {
 public:
  explicit CyclicBarrier(size_t participants)
    : participants_(participants)
  {
    //std::cout << "Expected participants: " << participants_ << std::endl;
    mtx_ = std::make_unique<mutex>();
  }

  void ArriveAndWait() {
    while (arrived_.load() >= participants_ || departed_ != 0) {
      
    }

    /* size_t arrival_nr = */ arrived_.fetch_add(1);
    // printf("arrival_nr #: %lu (arrived: %lu)\n", arrival_nr, arrived_);
    std::unique_lock<mutex> lock(*mtx_);
    // printf("std::unique_lock<mutex> lock(*mtx_) for #: %lu arrived: %lu\n", arrival_nr, arrived_);
    cv_.wait(lock, [&] { 
      // printf("In wait arrival_nr: %lu, arrived_: %lu >= participants_: %lu\n", arrival_nr, arrived_, participants_);
      return arrived_.load() >= participants_; 
    });
    if (arrived_.load() >= participants_) {
      // printf("Notify all when arrival_nr #: %lu (arrived: %lu)\n", arrival_nr, arrived_);
      cv_.notify_all();
    }
    //printf("After wait arrival_nr: %lu, Waked for arrived: %lu\n", arrival_nr, arrived_);
    ++departed_;
    //printf("++departed_ arrival_nr: %lu, Departed: %lu (expected arrived: %lu)\n", arrival_nr, departed_, arrived_);
    if (departed_ >= arrived_.load()) {
      arrived_.store(0);
      //printf("arrived_ = 0; arrival_nr: %lu, Arrived dropped\n", arrival_nr);
      departed_ = 0;
      //printf("departed_ = 0; arrival_nr: %lu, Departed dropped\n", arrival_nr);
    }
  }


 private:
  size_t participants_{0};
  atomic<size_t> arrived_{0};
  size_t departed_{0};
  condition_variable cv_;
  //std::unique_lock<mutex> lock_;
  std::unique_ptr<mutex> mtx_;
};


  // void ArriveAndWait() {
  //   while (arrived_ >= participants_ || departed_ != 0) {
  //   }
  //   size_t arrival_nr = ++arrived_;
  //   if (arrived_ >= participants_) {
  //     printf("Notify all when arrival_nr #: %lu (arrived: %lu)\n", arrival_nr, arrived_);
  //     cv_.notify_all();
  //   }
  //   printf("arrival_nr #: %lu (arrived: %lu)\n", arrival_nr, arrived_);
  //   std::unique_lock<mutex> lock(*mtx_);
  //   printf("Unique locked for #: %lu arrived: %lu\n", arrival_nr, arrived_);
  //   cv_.wait(lock, [&] { 
  //     printf("arrival_nr: %lu, arrived_: %lu >= participants_: %lu\n", arrival_nr, arrived_, participants_);
  //     return arrived_ >= participants_; 
  //   });
  //   printf("arrival_nr: %lu, Waked for arrived: %lu\n", arrival_nr, arrived_);
  //   ++departed_;
  //   printf("arrival_nr: %lu, Departed: %lu (expected arrived: %lu)\n", arrival_nr, departed_, arrived_);
  //   if (departed_ >= arrived_) {
  //     arrived_ = 0;
  //     printf("arrival_nr: %lu, Arrived dropped\n", arrival_nr);
  //     departed_ = 0;
  //     printf("arrival_nr: %lu, Departed dropped\n", arrival_nr);
  //   }
  // }