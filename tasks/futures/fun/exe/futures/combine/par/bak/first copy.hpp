#pragma once

#include <exe/futures/types/future.hpp>
#include "exe/futures/make/contract.hpp"
#include <twist/ed/stdlike/atomic.hpp>

using twist::ed::stdlike::atomic;

namespace exe::futures {

template <typename T>
class FirstCombiner {
  public:
    explicit FirstCombiner(Promise<T>&& promise)
      : promise_(std::move(promise)) {}
    void operator()(Result<T> r) {
      if (r) {
        printf("Success\n");
        std::move(promise_).Set(r);
        //counter.store(2);
        return;
      }
      if (counter_.fetch_sub(1) == 1) {
        printf("Failure\n");
        std::move(promise_).Set(r);
        //counter.store(2);
        return;
      }
      printf("Skip, counter: %lu\n", counter_.load());
    }
  private:
    Promise<T> promise_;
    atomic<size_t> counter_{2};
};

template <typename T>
Future<T> First(Future<T> f1, Future<T> f2) {
  auto [f, p] = Contract<T>();
  FirstCombiner<T> combiner{std::move(p)};
  f1.Subscribe([&combiner](Result<T> r) mutable {
    printf("In f1.Subscribe\n");
    combiner(r);
  });
  f2.Subscribe([&combiner](Result<T> r) mutable {
    printf("In f2.Subscribe\n");
    combiner(r);
  });
  return std::move(f);
}

}  // namespace exe::futures
