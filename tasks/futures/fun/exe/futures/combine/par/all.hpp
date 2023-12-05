#pragma once

#include <exe/futures/types/future.hpp>

#include <tuple>

#include "exe/executors/inline.hpp"
#include <exe/result/make/ok.hpp>

using exe::result::Err;
using exe::result::Ok;

namespace exe::futures {

template <typename X, typename Y>
Future<std::tuple<X, Y>> Both(Future<X> fx, Future<Y> fy) {
  auto [f, p] = Contract<std::tuple<X, Y>>();
  auto cb = [p = std::move(p)](Result<X> rx, Result<Y> ry, bool is_x) mutable {
    static atomic<size_t> counter{2};
    static std::tuple<X, Y> res_tup;
    if (is_x) {
      if (rx) {
        std::get<0>(res_tup) = rx.value();
      } else {
        std::move(p).SetError(rx.error());
        return;
      }
    } else {
      if (ry) {
        std::get<1>(res_tup) = ry.value();
      } else {
        std::move(p).SetError(ry.error());
        return;
      }
    }
    if (counter.fetch_sub(1) == 1) {
      std::move(p).Set(Ok(res_tup));
    }
  };
  
  fx.Subscribe([cb](Result<X> rx) mutable {
    printf("In f1.Subscribe\n");
    Result<Y> ry;
    cb(rx, ry, true);
  });

  fy.Subscribe([cb](Result<Y> ry) mutable {
    printf("In f2.Subscribe\n");
    Result<X> rx;
    cb(rx, ry, false);
  });

  return std::move(f);
}

// + variadic All

}  // namespace exe::futures
