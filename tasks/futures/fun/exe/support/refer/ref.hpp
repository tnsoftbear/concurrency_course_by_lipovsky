#pragma once

#include "managed.hpp"

// std::exchange
#include <utility>
#include <cassert>

namespace refer {

struct AdoptRef {};

//////////////////////////////////////////////////////////////////////

// Minimal intrusive ptr

template <typename T>
class Ref {
  static_assert(std::is_base_of_v<IManaged, T>);

 public:
  // Ctor

  Ref(std::nullptr_t)
      : obj_(nullptr) {
  }

  template <typename U>
  Ref(U* obj)
      : obj_(obj) {
    static_assert(std::is_base_of_v<T, U>);

    if (IsValid()) {
      AddRef();
    }
  }

  // Do not increment ref count
  template <typename U>
  Ref(U* obj, AdoptRef)
      : obj_(obj) {
    static_assert(std::is_base_of_v<T, U>);
  }

  Ref()
      : obj_(nullptr) {
  }

  // Dtor

  ~Ref() {
    if (IsValid()) {
      ReleaseRef();
    }
  }

  // Copy

  Ref(const Ref<T>& that)
      : obj_(that.obj_) {
    if (IsValid()) {
      AddRef();
    }
  }

  template <typename U>
  Ref(const Ref<U>& that)
      : obj_(that.Get()) {
    static_assert(std::is_base_of_v<T, U>);
    if (IsValid()) {
      AddRef();
    }
  }

  // Move

  Ref(Ref<T>&& that)
      : obj_(that.Release()) {
  }

  template <typename U>
  Ref(Ref<U>&& that)
      : obj_(that.Release()) {
    static_assert(std::is_base_of_v<T, U>);
  }

  // Assigment
  // Copy-and-swap idiom
  Ref& operator=(Ref<T> that) {
    SwapWith(that);
    return *this;
  }

  void Reset() {
    if (IsValid()) {
      ReleaseRef();
      obj_ = nullptr;
    }
  }

  // Get non-owning pointer to object
  T* Get() const {
    return obj_;
  }

  // Unsafe
  T* Release() {
    return std::exchange(obj_, nullptr);
  }

  bool IsValid() const {
    return obj_ != nullptr;
  }

  // Mimic to raw pointer

  explicit operator bool() const {
    return IsValid();
  }

  T* operator->() const {
    return obj_;
  }

  T& operator*() const {
    return *obj_;
  }

  bool IsManualLifetime() const {
    assert(IsValid());
    return obj_->IsManualLifetime();
  }

 private:
  void ReleaseRef() {
    obj_->ReleaseRef();
  }

  void AddRef() {
    obj_->AddRef();
  }

  template <typename U>
  void SwapWith(Ref<U>& that) {
    T* this_obj = obj_;
    obj_ = that.obj_;
    that.obj_ = this_obj;
  }

 private:
  T* obj_;
};

//////////////////////////////////////////////////////////////////////

template <typename T>
Ref<T> Adopt(T* obj) {
  return Ref<T>(obj, AdoptRef{});
}

}  // namespace refer
