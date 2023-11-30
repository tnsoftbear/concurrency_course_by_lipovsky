#pragma once

#include <exe/futures/types/future.hpp>
#include <exe/executors/executor.hpp>

#include <exe/result/traits/value_of.hpp>

#include <type_traits>

namespace exe::futures {

namespace traits {

template <typename F>
using SubmitT = result::traits::ValueOf<std::invoke_result_t<F>>;

}  // namespace traits

template <typename F>
Future<traits::SubmitT<F>> Submit(executors::IExecutor& exe, F fun) {
  std::shared_ptr<SharedState<traits::SubmitT<F>>> ss = std::make_shared<SharedState<traits::SubmitT<F>>>();
  ss->Clear();
  // [ss]{
  //   ss->Set fun();
  // }
  // return Future<T>(ss);  
  exe.Submit(fun);
  return Future<traits::SubmitT<F>>(ss);
}

}  // namespace exe::futures
