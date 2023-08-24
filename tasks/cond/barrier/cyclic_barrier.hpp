#pragma once

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

#include <cstdlib>

class CyclicBarrier {
 public:
  explicit CyclicBarrier(size_t /*participants*/) {
    // Not implemented
  }

  void ArriveAndWait() {
    // Not implemented
  }

 private:
};
