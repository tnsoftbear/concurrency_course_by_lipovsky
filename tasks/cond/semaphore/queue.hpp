#pragma once

#include "tagged_semaphore.hpp"

#include <deque>

// Bounded Blocking Multi-Producer/Multi-Consumer (MPMC) Queue

template <typename T>
class BoundedBlockingQueue {
 private:
  struct SemTag {};
  
  using Token = typename TaggedSemaphore<SemTag>::Token;
 
 public:
  explicit BoundedBlockingQueue(size_t capacity)
    : free_sem_(capacity)
    , avail_sem_(0)
    , mutex_sem_(1)
  {
  }

  void Put(T value) {
    Token token(std::move(free_sem_.Acquire()));

    Token mutex_token(std::move(mutex_sem_.Acquire()));
    buffer_.push_back(std::move(value));
    mutex_sem_.Release(std::move(mutex_token));

    avail_sem_.Release(std::move(token));
  }

  T Take() {
    Token token(std::move(avail_sem_.Acquire()));

    Token mutex_token(std::move(mutex_sem_.Acquire()));
    T value = std::move(buffer_.front());
    buffer_.pop_front();
    mutex_sem_.Release(std::move(mutex_token));

    free_sem_.Release(std::move(token));
    
    return value;
  }

 private:
  std::deque<T> buffer_;
  TaggedSemaphore<SemTag> free_sem_;
  TaggedSemaphore<SemTag> avail_sem_;
  TaggedSemaphore<SemTag> mutex_sem_;

 private:
  void Ll(const char* format, ...) {
    if (!kShouldPrint) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "Queue: %s %s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};
