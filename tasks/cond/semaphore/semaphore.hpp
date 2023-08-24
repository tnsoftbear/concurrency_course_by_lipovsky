#pragma once

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

#include <cstdlib>

class Semaphore {
 public:
  explicit Semaphore(size_t /*tokens*/) {
    // Not implemented
  }

  void Acquire() {
    // Not implemented
  }

  void Release() {
    // Not implemented
  }

 private:
  // Tokens
};
