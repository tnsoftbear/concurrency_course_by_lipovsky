#pragma once

#include <exe/futures/types/future.hpp>
#include "exe/futures/make/contract.hpp"
#include <twist/ed/stdlike/atomic.hpp>

using twist::ed::stdlike::atomic;

namespace exe::futures {

template <typename T>
Future<T> First(Future<T> f1, Future<T> f2) {
  auto [f, p] = Contract<T>();
  auto cb = [p = std::move(p)](Result<T> r) mutable {
    static atomic<size_t> counter{2};
    if (r) {
      printf("Success\n");
      std::move(p).Set(r);
      //counter.store(2);
      return;
    }
    if (counter.fetch_sub(1) == 1) {
      printf("Failure\n");
      std::move(p).Set(r);
      //counter.store(2);
      return;
    }
    printf("Skip, counter: %lu\n", counter.load());
  };
  f1.Subscribe([cb](Result<T> r) mutable {
    printf("In f1.Subscribe\n");
    cb(r);
  });
  f2.Subscribe([cb](Result<T> r) mutable {
    printf("In f2.Subscribe\n");
    cb(r);
  });
  return std::move(f);
}

}  // namespace exe::futures
