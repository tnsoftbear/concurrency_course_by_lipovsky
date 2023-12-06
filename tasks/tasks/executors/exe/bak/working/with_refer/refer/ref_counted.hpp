#pragma once

#include "ref.hpp"

#include <twist/ed/stdlike/atomic.hpp>

using twist::ed::stdlike::atomic;

namespace refer {

//////////////////////////////////////////////////////////////////////

template <typename T>
class RefCounted : public virtual IManaged {
 public:
  RefCounted(bool initial = true)
      : ref_count_(initial ? 1 : 0) {
  }

  void AddRef() override {
    ref_count_.fetch_add(1, std::memory_order_relaxed);
  }

  void ReleaseRef() override {
    if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      DestroySelf();
    }
  }

  bool IsManualLifetime() const override {
    return false;
  }

  size_t GetRefCount() const {
    return ref_count_.load();
  }

  Ref<T> RefFromThis() {
    return Ref<T>{AsT()};
  }

 private:
  T* AsT() {
    return static_cast<T*>(this);
  }

  void DestroySelf() {
    delete AsT();
  }

 private:
  atomic<size_t> ref_count_;
};

//////////////////////////////////////////////////////////////////////

template <typename T, typename ... Args>
Ref<T> New(Args&& ... args) {
  static_assert(std::is_base_of_v<RefCounted<T>, T>);
  T* obj = new T(std::forward<Args>(args)...);
  return Ref<T>(obj, AdoptRef{});
}

}
