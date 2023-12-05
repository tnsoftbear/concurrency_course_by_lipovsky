#pragma once

#include <exe/futures/types/future.hpp>
#include <exe/executors/inline.hpp>

#include <map>
#include <tuple>

#include <exe/result/make/err.hpp>
#include <exe/result/make/ok.hpp>
#include <twist/ed/stdlike/atomic.hpp>

using twist::ed::stdlike::atomic;

using exe::result::Err;
using exe::result::Ok;

namespace exe::futures {

// Вероятно, грязное решение со статическими переменными. См. причины в first.hpp
namespace both {
static atomic<size_t> max_id{0}; // Счётчкик идентификаторов для комбинатора First
static std::map<size_t, std::atomic<size_t>> cb_cnt; // Ожидаемое кол-во вызовов колбеков для фьюч
}

template <typename X, typename Y>
Future<std::tuple<X, Y>> Both(Future<X> fx, Future<Y> fy) {
  auto id = both::max_id++;
  both::cb_cnt[id].store(2);
  auto [f, p] = Contract<std::tuple<X, Y>>();

  auto cb = [p = std::move(p), id](Result<X> rx, Result<Y> ry, bool is_x) mutable {
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
    if (both::cb_cnt[id]-- == 1) {
      std::move(p).Set(Ok(res_tup));
    }
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
