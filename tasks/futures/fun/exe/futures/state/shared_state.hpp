#pragma once

#include <memory>
#include <optional>
#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

#include <exe/result/types/result.hpp>
#include <utility>
#include <variant>
#include "exe/executors/inline.hpp"
#include "exe/result/types/error.hpp"
#include "exe/result/types/status.hpp"
#include "tl/expected.hpp"
#include <type_traits>

#include <iostream>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

#include <exe/executors/executor.hpp>
#include "callback.hpp"

#include <twist/ed/stdlike/atomic.hpp>
using twist::ed::stdlike::atomic;

namespace exe::futures::details {

using twist::ed::stdlike::condition_variable;
using twist::ed::stdlike::mutex;

#include <typeinfo>

template <typename T>
class SharedState {
  enum class State {
    Initial = 0,
    CallbackSet = 1,
    ResultSet = 2,
    ResultConsumed = 3, // Terminal state
  };
 public:
  Result<T> SyncGet() {
    Ll("Before readiness_cv_.wait");
    std::unique_lock<mutex> lock(*mtx_);
    readiness_cv_.wait(lock, [this] {
      return result_opt_.has_value();
    });
    Ll("After readiness_cv_.wait");

    return std::move(result_opt_.value());
  }

  void SetResult(Result<T>&& result) {
    {
      std::lock_guard<mutex> guard(*mtx_);
      result_opt_.emplace(std::move(result));
    }
    readiness_cv_.notify_one();

    if (state_.exchange(State::ResultSet) == State::CallbackSet) {
      SubmitCallbackIntoExecutor();
    }
  }

  void SetCallback(Callback<T>&& callback) {
    callback_opt_ = std::move(callback);

    if (state_.exchange(State::CallbackSet) == State::ResultSet) {
      SubmitCallbackIntoExecutor();
    }
  }

  void SetExecutor(executors::IExecutor* executor) {
    executor_ = executor;
  }

  executors::IExecutor& GetExecutor() {
    return *executor_;
  }

  void Clear() {
    result_opt_.reset();
    callback_opt_.reset();
  }

 private:
  std::optional<Result<T>> result_opt_;
  std::unique_ptr<mutex> mtx_ = std::make_unique<mutex>();
  condition_variable readiness_cv_;
  std::optional<Callback<T>> callback_opt_;
  executors::IExecutor* executor_ = &executors::Inline();
  atomic<State> state_{State::Initial};

  void SubmitCallbackIntoExecutor() {
    Ll("SubmitCallbackIntoExecutor(): starts Executor is %s", typeid(*executor_).name());
    executor_->Submit([
      result = std::move(*result_opt_)
      , callback = std::move(*callback_opt_)
      , this
    ]() mutable {
      Ll("Start invoking callback inside executor");;
      callback(std::move(result));
    });
  }

  void Ll(const char* format, ...) {
    bool const k_should_print = false;
    if (!k_should_print) {
      return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s SharedState::%s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};

}  // namespace exe::futures::details
