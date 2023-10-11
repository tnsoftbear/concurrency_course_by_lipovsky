#pragma once

#include <cstdio>
#include <utility>

namespace exe::tp {

template <typename P, typename F>
void Submit(P& pool, F&& fun) {
  //printf("exe::tp::Submit: Enter\n");
  pool.Submit(std::forward<F>(fun));
}

}  // namespace exe::tp
