#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>

#include <cstdint>

namespace stdlike {

class CondVar {
 public:
  // Mutex - BasicLockable
  // https://en.cppreference.com/w/cpp/named_req/BasicLockable
  template <class Mutex>
  void Wait(Mutex&) {
    // Not implemented
  }

  void NotifyOne() {
    // Not implemented
  }

  void NotifyAll() {
    // Not implemented
  }

 private:
  // ???
};

}  // namespace stdlike
