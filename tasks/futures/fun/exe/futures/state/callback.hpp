#pragma once

#include <exe/result/types/result.hpp>
#include <function2/function2.hpp>

namespace exe::futures::details {

template <typename T>
using Callback = fu2::unique_function<void(Result<T>)>;
}