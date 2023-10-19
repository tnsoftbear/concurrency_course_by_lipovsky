#pragma once

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

namespace stdlike {
namespace details {

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

    if (v_.index() == 2) {
        auto eptr = std::move(std::get<2>(v_));
        std::rethrow_exception(eptr);
    } else {
        return std::move(std::get<1>(v_));
    }
  }

  void SetValue(T value) {
    {
      v_.template emplace<1>(std::move(value));
      std::lock_guard<mutex> guard(*mtx_);
    }
    readiness_cv_.notify_one();
  }

  void SetException(std::exception_ptr expt) {
    {
      v_.template emplace<2>(std::move(expt));
      std::lock_guard<mutex> guard(*mtx_);
    }
    readiness_cv_.notify_one();
  }

 private:
  std::variant<std::monostate, T, std::exception_ptr> v_;
  std::unique_ptr<mutex> mtx_ = std::make_unique<mutex>();
  condition_variable readiness_cv_;
};

}  // namespace details
}  // namespace stdlike


/**
 * Забавно, что следующие варианты тоже рабочие:

  void SetValue(T value) {
    v_.template emplace<1>(std::move(value));
    {
      std::lock_guard<mutex> guard(*mtx_);
      readiness_cv_.notify_one();
    }
  }

  void SetValue(T value) {
    v_.template emplace<1>(std::move(value));
    {
      std::lock_guard<mutex> guard(*mtx_);
    }
    readiness_cv_.notify_one();
  }
*/