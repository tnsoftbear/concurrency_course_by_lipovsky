#pragma once

#include <twist/ed/stdlike/mutex.hpp>

//////////////////////////////////////////////////////////////////////

/*
 * Safe API for mutual exclusion
 *
 * Usage:
 *
 * Mutexed<std::vector<Apple>> apples;
 *
 * {
 *   auto owner_ref = apples->Acquire();
 *   owner_ref->push_back(Apple{});
 * }  // <- release ownership
 *
 */

template<typename T, typename Lock, class Mutex>
struct Accessor {
  Accessor(T& object, Mutex& mutex) : object_(object), lock_(mutex) {}

  T* operator->() {
    return &object_;
  }

  const T* operator->() const {
    return &object_;
  }

  const T& operator*() const {
    return object_;
  }

  T& operator*() {
    return object_;
  }

  T& object_;
  Lock lock_;
};

template <typename T, class Mutex = twist::ed::stdlike::mutex>
class Mutexed {

 public:
  // https://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/
  template <typename... Args>
  explicit Mutexed(Args&&... args)
      : object_(std::forward<Args>(args)...) {
  }

  auto Acquire() {
    return Accessor<T, std::lock_guard<Mutex>, Mutex>{ object_, mutex_ };
  }

 private:
  T object_;
  Mutex mutex_;  // Guards access to object_
};

//////////////////////////////////////////////////////////////////////

template <typename T>
auto Acquire(Mutexed<T>& object) {
  return object.Acquire();
}
