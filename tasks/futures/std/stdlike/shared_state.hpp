#pragma once

// #include <memory>
// #include <cassert>

#include <mutex>
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
      return v_.index() != 0; 
    });

    //if (std::holds_alternative<std::exception_ptr>(v_)) {
    if (v_.index() == 2) {
        auto eptr = std::move(std::get<2>(v_));
        std::rethrow_exception(eptr);
    } else {
        return std::move(std::get<1>(v_));
    }
  }

  void SetValue(T value) {
    {
      std::lock_guard<mutex> guard(*mtx_);
      v_.template emplace<1>(std::move(value));
    }
    readiness_cv_.notify_one();
  }

  void SetException(std::exception_ptr expt) {
    {
      std::lock_guard<mutex> guard(*mtx_);
      v_.template emplace<2>(std::move(expt));
    }
    readiness_cv_.notify_one();
  }

 private:
  std::variant<std::monostate, T, std::exception_ptr> v_;
  std::unique_ptr<mutex> mtx_ = std::make_unique<mutex>();
  condition_variable readiness_cv_;
};

}  // namespace stdlike
