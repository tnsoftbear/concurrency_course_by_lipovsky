#pragma once

// #include <memory>
// #include <cassert>

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

#include <exception>
namespace stdlike {

using twist::ed::stdlike::mutex;
using twist::ed::stdlike::condition_variable;

template <typename T>
class SharedState {
 public:

  T Get() {
    std::unique_lock<mutex> lock(*mtx_);
    readiness_cv_.wait(lock, [this] { 
      return !std::holds_alternative<std::monostate>(v_); 
    });

    if (std::holds_alternative<std::exception_ptr>(v_)) {
        auto eptr = std::get<std::exception_ptr>(v_);
        std::rethrow_exception(eptr);
    } else {
        return std::get<T>(v_);
    }
  }

  void SetValue(T value) {
    v_ = value;
    readiness_cv_.notify_one();
  }

  void SetException(std::exception_ptr expt) {
    v_ = expt;
    readiness_cv_.notify_one();
  }

 private:
  std::variant<std::monostate, T, std::exception_ptr> v_;
  std::unique_ptr<mutex> mtx_ = std::make_unique<mutex>();
  condition_variable readiness_cv_;
};

}  // namespace stdlike
