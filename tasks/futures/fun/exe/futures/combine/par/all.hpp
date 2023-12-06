#pragma once

#include <exe/futures/types/future.hpp>
#include <exe/executors/inline.hpp>

#include <map>
#include <tuple>

#include <exe/result/make/err.hpp>
#include <exe/result/make/ok.hpp>

using exe::result::Err;
using exe::result::Ok;

namespace exe::futures {

template <typename X, typename Y>
class RefCounterForBoth {
  public:
    atomic<size_t> value{0};
    std::tuple<X, Y> res_tup;

    explicit RefCounterForBoth(size_t init_ref_count = 0, size_t init_value = 0)
    {
      value.store(init_value);
      ref_count_.store(init_ref_count);
    };

    void AddRef() {
      ref_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void ReleaseRef() {
      if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        DestroySelf();
      }
    }

    void DestroySelf() {
      delete this;
    }
  private:
    atomic<size_t> ref_count_{0};
};

template <typename X, typename Y>
Future<std::tuple<X, Y>> Both(Future<X> fx, Future<Y> fy) {
  RefCounterForBoth<X, Y>* cb_cnt = new RefCounterForBoth<X, Y>(2, 2);
  auto [f, p] = Contract<std::tuple<X, Y>>();

  auto cb = [p = std::move(p), cb_cnt](Result<X> rx, Result<Y> ry, bool is_x) mutable {
    if (is_x) {
      if (rx) {
        std::get<0>(cb_cnt->res_tup) = rx.value();
      } else {
        std::move(p).SetError(rx.error());
        cb_cnt->ReleaseRef();
        return;
      }
    } else {
      if (ry) {
        std::get<1>(cb_cnt->res_tup) = ry.value();
      } else {
        std::move(p).SetError(ry.error());
        cb_cnt->ReleaseRef();
        return;
      }
    }

    if (cb_cnt->value.fetch_sub(1) == 1) {
      std::move(p).Set(Ok(cb_cnt->res_tup));
    }
    cb_cnt->ReleaseRef();
  };
  
  std::move(fx)
    .Via(executors::Inline())
    .Subscribe([cb](Result<X> rx) mutable {
      Result<Y> ry;
      cb(rx, ry, true);
    });

  std::move(fy)
    .Via(executors::Inline())
    .Subscribe([cb](Result<Y> ry) mutable {
      Result<X> rx;
      cb(rx, ry, false);
    });

  return std::move(f);
}

// + variadic All

}  // namespace exe::futures
