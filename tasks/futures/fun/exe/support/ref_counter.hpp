#pragma once

#include <cstdio>
#include <twist/ed/stdlike/atomic.hpp>

using twist::ed::stdlike::atomic;

class RefCounter {
  public:
    atomic<size_t> value{0};

    explicit RefCounter(size_t init_ref_count = 0, size_t init_value = 0)
    {
      value.store(init_value);
      ref_count_.store(init_ref_count);
    };

    void AddRef() {
      ref_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void ReleaseRef() {
      if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        DestroySelf();
      }
    }

    void DestroySelf() {
      delete this;
    }
  private:
    atomic<size_t> ref_count_{0};
};
