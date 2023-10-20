#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

#include <optional>

#include <iostream>
#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/stdlike/thread.hpp>
#include <sstream>

using twist::ed::stdlike::atomic;
using twist::ed::stdlike::condition_variable;
using twist::ed::stdlike::mutex;


namespace exe::support {

//const bool kShouldPrintQueue = true;
const bool kShouldPrintQueue = false;

// Unbounded blocking multi-producers/multi-consumers (MPMC) queue

template <typename T>
class UnboundedBlockingQueue {
 public:
  UnboundedBlockingQueue() {
    Ll("Construct");
    mtx_ = std::make_unique<mutex>();
    is_opened_ = true;
  }

  ~UnboundedBlockingQueue() {
    Close();
  }

  // Если очередь еще не закрыта (вызовом Close), то положить в нее value и
  // вернуть true, в противном случае вернуть false.
  bool Put(T e) {
    Ll("Put: start");
    {
      std::lock_guard<mutex> lock(*mtx_);
      if (!is_opened_) {
        Ll("Put: return false");
        return false;
      }

      queue_.push(std::move(e));
      cv_.notify_one();
    }
    Ll("Put: return true");
    return true;
  }

  // Дождаться и извлечь элемент из головы очереди; если же очередь закрыта и
  // пуста, то вернуть std::nullopt.
  std::optional<T> Take() {
    Ll("Take: start");
    std::unique_lock<mutex> lock(*mtx_);
    Ll("Take: before wait");
    cv_.wait(lock, [this] {
      return !is_opened_ || !queue_.empty();
    });
    Ll("Take: after wait");

    // Здесь лок не нужен, он берётся после wait
    // std::lock_guard<mutex> lock(*mtx_);

    if (!is_opened_ && queue_.empty()) {
      Ll("Take: return nullopt");
      return std::nullopt;
    }

    T elem = std::move(queue_.front());
    queue_.pop();
    Ll("Take: return e ");
    return elem;
  }

  // Закрыть очередь для новых Put-ов. Уже добавленные в очередь элементы
  // останутся доступными для извлечения.
  void Close() {
    {
      std::lock_guard<mutex> lock(*mtx_);
      if (!is_opened_) {
        return;
      }

      is_opened_ = false;
    }
    cv_.notify_all();
    Ll("Close after notify all");
  }

  void Ll(const char* format, ...) {
    if (!kShouldPrintQueue) {
      return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s %s c:%lu\n", pid.str().c_str(), format, Count());
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }

  size_t Count() const {
    std::unique_lock<mutex> lock(*mtx_);
    return queue_.size();
  }

  bool IsEmpty() const {
    std::unique_lock<mutex> lock(*mtx_);
    return queue_.size() == 0;
  }

  bool IsClosed() {
    // std::unique_lock<mutex> lock(*mtx_);
    return !is_opened_;
  }

 private:
  bool is_opened_{true};
  twist::ed::stdlike::condition_variable cv_;
  std::unique_ptr<twist::ed::stdlike::mutex> mtx_;
  std::queue<T> queue_;
};

}  // namespace exe::tp
