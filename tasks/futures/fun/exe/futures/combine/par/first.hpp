#pragma once

#include <exe/futures/types/future.hpp>
#include <exe/executors/inline.hpp>
#include "exe/futures/make/contract.hpp"
#include <map>
#include <exe/support/ref_counter.hpp>

namespace exe::futures {

template <typename T>
Future<T> First(Future<T> f1, Future<T> f2) {
  RefCounter* cb_cnt = new RefCounter(2, 2);
  auto [f, p] = Contract<T>();

  auto cb = [p = std::move(p), cb_cnt](Result<T> r) mutable {
    if (r) { // success
      std::move(p).Set(r);
      cb_cnt->ReleaseRef();
      return;
    }

    if (cb_cnt->value.fetch_sub(1) == 1) {
      std::move(p).Set(r); // last error
      cb_cnt->ReleaseRef();
      return;
    }

    cb_cnt->ReleaseRef();
  };

  std::move(f1)
    .Via(executors::Inline())
    .Subscribe([cb](Result<T> r) mutable {
      cb(r);
    });

  std::move(f2)
    .Via(executors::Inline())
    .Subscribe([cb](Result<T> r) mutable {
      cb(r);
    });

  return std::move(f);
}

}  // namespace exe::futures
