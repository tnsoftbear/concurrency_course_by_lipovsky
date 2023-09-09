#pragma once

template <typename Mutex>
class LockGuard {
 public:
  explicit LockGuard(Mutex& m) : m_(&m) {
    (*m_).lock();
  }

  // Non-copyable
  LockGuard(const LockGuard&) = delete;
  LockGuard& operator=(const LockGuard&) = delete;

  // Non-movable
  LockGuard(LockGuard&&) = delete;
  LockGuard& operator=(LockGuard&&) = delete;

  ~LockGuard() {
    (*m_).unlock();
  }
 private:
  Mutex* m_;
};
