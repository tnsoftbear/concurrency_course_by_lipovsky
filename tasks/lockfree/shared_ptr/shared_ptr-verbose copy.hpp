#pragma once

#include "atomic_stamped_ptr.hpp"

#include <cstddef>
#include <twist/ed/stdlike/atomic.hpp>
#include <utility>

using twist::ed::stdlike::atomic;

#include <twist/ed/stdlike/mutex.hpp>

//////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <sstream>
#include <twist/ed/stdlike/thread.hpp> // for debug logs

const bool kShouldPrint = false;
//const bool kShouldPrint = true;

namespace detail {

struct SplitCount {
  int32_t transient{0};
  int32_t strong{0};  // > 0

  bool operator==(const SplitCount& other) const {
    return (transient == other.transient) && (strong == other.strong);
  }
};

static_assert(sizeof(SplitCount) == sizeof(size_t), "Not supported");

}  // namespace detail

void Ll(const char* format, ...) {
  if (!kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s %s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

//////////////////////////////////////////////////////////////////////

template <typename T> class SharedPtr {

public:
 struct ControlBlock {
  T* data_ptr{nullptr};
  atomic<detail::SplitCount> counter = detail::SplitCount{};
  ControlBlock(T* data_ptr, detail::SplitCount cnt)
    : data_ptr(data_ptr)
     {
      counter.store(cnt);
    }
  ControlBlock(const ControlBlock& that)
    : ControlBlock(that.data_ptr, that.counter.load()) {}
  ControlBlock() : ControlBlock(nullptr, detail::SplitCount{0, 0}) {}
  ~ControlBlock() {
    Ll("~ControlBlock() : %p", this);
    delete data_ptr;
    data_ptr = nullptr;
  }
  ControlBlock& operator=(const ControlBlock& that) {
    if (this != &that) {
      data_ptr = that.data_ptr;
      counter.store(that.counter.load());
    }
    return *this;
  }
  ControlBlock(ControlBlock&& that) noexcept
    : ControlBlock(that) {
      that.data_ptr = nullptr;
      that.counter.store(detail::SplitCount{0, 0});
    }
  ControlBlock& operator=(ControlBlock&& that) noexcept {
    if (this != &that) {
      data_ptr = std::exchange(that.data_ptr, nullptr);
      counter.store(that.counter.load());
      that.counter.store(detail::SplitCount{0, 0});
    }
    return *this;
  }
 };

public:
  SharedPtr(T* data, detail::SplitCount counter)
      : ctrl_ptr(new ControlBlock(data, counter)) {
    IncrementStrong();
    PrintMe("SharedPtr(T* data, detail::SplitCount counter)");
  }

  explicit SharedPtr(ControlBlock* ctrl_ptr)
      : ctrl_ptr(ctrl_ptr) {
    IncrementStrong();
    PrintMe("SharedPtr(ControlBlock* ctrl)");
  }

  SharedPtr() {
    PrintMe("SharedPtr()");
  }

  // Copy ctor
  SharedPtr(const SharedPtr<T>& that)
      : ctrl_ptr(that.ctrl_ptr)
  {
    IncrementStrong();
    PrintMe("SharedPtr(const SharedPtr<T>& that)");
  }

  // Copy assignment
  SharedPtr<T>& operator=(const SharedPtr<T>& that) {
    if (this != &that) {
      Reset();
      ctrl_ptr = that.ctrl_ptr;
      IncrementStrong();
    }
    PrintMe("SharedPtr<T>& operator=(const SharedPtr<T>& that)");
    return *this;
  }

  // Move ctor
  SharedPtr(SharedPtr<T>&& that) noexcept
      : ctrl_ptr(std::move(that.ctrl_ptr)) {
    that.ctrl_ptr = nullptr;
    PrintMe("SharedPtr(SharedPtr<T>&& that)");
  }

  // Move assignment
  SharedPtr<T>& operator=(SharedPtr<T>&& that) noexcept {
    if (this != &that) {
      Reset();
      ctrl_ptr = std::exchange(that.ctrl_ptr, nullptr);
    }
    PrintMe("SharedPtr<T>& operator=(SharedPtr<T>&& that)");
    return *this;
  }

  ~SharedPtr() {
    Reset();
    PrintMe("~SharedPtr() ends");
  }

  T* operator->() const { 
    return ctrl_ptr != nullptr ? ctrl_ptr->data_ptr : nullptr;
  }

  T& operator*() const { return *ctrl_ptr->data_ptr; }

  explicit operator bool() const { return ctrl_ptr != nullptr; }

  // cleanup any existing data
  void Reset() {
    if (ctrl_ptr != nullptr) {
      PrintMe("Reset: starts");
      DecrementStrong();
      ctrl_ptr = nullptr;
    }
  }

  void IncrementCounter(int32_t strong, int32_t transient) {
    if (ctrl_ptr != nullptr) {
      detail::SplitCount counter_old, counter_new;
      counter_old = ctrl_ptr->counter.load();
      do {
        counter_new = counter_old;  // Хз, почему так не работает: counter_new = ctrl_ptr->counter.load();
        counter_new.strong += strong;
        counter_new.transient += transient;
      } while (!ctrl_ptr->counter.compare_exchange_weak(counter_old, counter_new));
      Ll("IncrementCounter: ends for change of strong by %d now is %d and change of transient by %d now is %d in this: %p, ctrl: %p", strong, counter_new.strong, transient, counter_new.transient, this, ctrl_ptr);

      if (
        counter_new.strong == 0
        && counter_new.transient == 0
      ) {
        Ll("Reset: to 0, delete ctrl: %p", ctrl_ptr);
        auto delete_ptr = ctrl_ptr;
        ctrl_ptr = nullptr;
        delete delete_ptr;
        //ctrl_ptr = nullptr;
      }
    }
  }

  void IncrementStrong(int32_t inc = 1) {
    IncrementCounter(inc, 0);
  }

  void DecrementStrong(int32_t dec = 1) {
    IncrementCounter(-1 * dec, 0);
  }

  void IncrementTransient(int32_t inc = 1) {
    IncrementCounter(0, inc);
  }

  void DecrementTransient(int32_t dec = 1) {
    IncrementCounter(0, -1 * dec);
  }

  void PrintMe(std::string prefix = "") {
    return;
    if (ctrl_ptr) {
      Ll("SharedPtr::%s (this: %p, ctrl: %p) strong: %d, transient: %d",
        prefix.c_str(),
        this,
        ctrl_ptr,
        ctrl_ptr->counter.load().strong,
        ctrl_ptr->counter.load().transient
      );
    } else {
      Ll("SharedPtr::%s (this: %p, ctrl: %p)", prefix.c_str(), this, ctrl_ptr);
    }
  }
//private:
public:
  ControlBlock* ctrl_ptr{nullptr};

private:
};

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  T* data = new T(std::forward<Args>(args)...);
  detail::SplitCount counter = detail::SplitCount{0, 0};
  return SharedPtr<T>{data, counter};
}

//////////////////////////////////////////////////////////////////////

template <typename T>
class AtomicSharedPtr {
 public:
  // nullptr
  AtomicSharedPtr() : atomic_stamped_ptr_(StampedPtr<T>{nullptr, 0}) {
  }

  ~AtomicSharedPtr() {
    Ll("~AtomicSharedPtr()");
    Store(SharedPtr<T>());
  }

  SharedPtr<T> Load() {
    auto stamped_ptr = AcquireTransientRef();
    Ll("AtomicSharedPtr::Load: stamp: %d", stamped_ptr.stamp);
    auto shared_ptr = ToSharedPtr(stamped_ptr);
    shared_ptr.PrintMe("AtomicSharedPtr::Load: Before shared_ptr.DecrementTransient(1);");
    shared_ptr.DecrementTransient(1);

    // Handle stamp overflow
    size_t limit = 64000;
    if (stamped_ptr.stamp > limit) {
      shared_ptr.IncrementTransient(limit / 2);
      stamped_ptr.stamp -= (limit / 2);
      atomic_stamped_ptr_.Store(stamped_ptr);
    }

    return shared_ptr;
  }

  void Store(SharedPtr<T> target) {
    Ll("AtomicSharedPtr::Store()");
    target.PrintMe("AtomicSharedPtr::Store: Before IncrementStrong");
    target.IncrementStrong();
    StampedPtr<T> stamped_ptr = ToStampedPtr(target);
    StampedPtr<T> old_stamped_ptr = atomic_stamped_ptr_.Exchange(stamped_ptr);
    SharedPtr<T> old_shared_ptr = ToSharedPtr(old_stamped_ptr);
    old_shared_ptr.PrintMe("AtomicSharedPtr::Store: Before old_shared_ptr.IncrementCounter(-1, old_stamped_ptr.stamp)");
    old_shared_ptr.IncrementCounter(-1, old_stamped_ptr.stamp);
  }

  explicit operator SharedPtr<T>() {
    return Load();
  }

  bool CompareExchangeWeak2(SharedPtr<T>& expected, SharedPtr<T> desired) {
    StampedPtr<T> newptr = ToStampedPtr(desired);
    StampedPtr<T> oldptr = AcquireTransientRef();
    Ll("AtomicSharedPtr::CompareExchangeWeak: oldptr.stamp: %d", oldptr.stamp);
    if ((void*)oldptr.raw_ptr != (void*)expected.ctrl_ptr) {
      expected = ToSharedPtr(oldptr);
      expected.PrintMe("AtomicSharedPtr::CompareExchangeWeak: Before expected.DecrementTransient();");
      expected.DecrementTransient();
      return false;
    }
    StampedPtr<T> expectedptr = oldptr;
    if (atomic_stamped_ptr_.CompareExchangeWeak(expectedptr, newptr)) {
      desired.PrintMe("AtomicSharedPtr::CompareExchangeWeak: Before desired.IncrementStrong();");
      desired.IncrementStrong();
      expected = ToSharedPtr(expectedptr);
      expected.PrintMe("AtomicSharedPtr::CompareExchangeWeak: Before expected.IncrementCounter(-1, oldptr.stamp - 1);");
      expected.IncrementCounter(-1, oldptr.stamp - 1);
      // expected.PrintMe("CompareExchangeWeak ok, expected");
      return true;
    } else {
      if (oldptr.raw_ptr != nullptr) {
        expected = ToSharedPtr(oldptr);
        expected.PrintMe("AtomicSharedPtr::CompareExchangeWeak: Before expected.DecrementTransient();");
        expected.DecrementTransient();
      } else {
        Ll("AtomicSharedPtr::CompareExchangeWeak: Before expected = SharedPtr<T>();");
        expected = SharedPtr<T>();
      }
      return false;
    }
  }


  bool CompareExchangeWeak(SharedPtr<T>& expected, SharedPtr<T> desired) {
    Ll("AtomicSharedPtr::CompareExchangeWeak()");
    //SharedPtr<T> old_shared_ptr = Load();
    //auto expected_copy = expected;
    //expected.IncrementStrong();
    StampedPtr<T> old_stamped_ptr = AcquireTransientRef();
    if ((void*)old_stamped_ptr.raw_ptr != (void*)expected.ctrl_ptr) {
      Ll("AtomicSharedPtr:: if ((void*)old_stamped_ptr.raw_ptr != (void*)expected.ctrl_ptr)");
      expected = ToSharedPtr(old_stamped_ptr);
      expected.DecrementTransient();
      return false;
    }

    StampedPtr<T> exp_stamp_ptr = ToStampedPtr(expected, old_stamped_ptr.stamp);
    StampedPtr<T> des_stamp_ptr = ToStampedPtr(desired);
    bool success = atomic_stamped_ptr_.CompareExchangeWeak(exp_stamp_ptr, des_stamp_ptr);
    Ll("Success: %d", (int)success);
    if (!success) {
      //expected.Reset();
      SharedPtr<T> old_shared_ptr = ToSharedPtr(old_stamped_ptr);
      old_shared_ptr.DecrementTransient();
      //exp_stamp_ptr = atom_stamp_ptr_.Load();
      //expected = ToSharedPtr(exp_stamp_ptr);
      //SharedPtr<T> expected = Load();
      //expected.DecrementStrong();
      expected = Load();
    } else {
      desired.IncrementStrong();
      expected.IncrementCounter(-1, old_stamped_ptr.stamp - 1);
      expected.PrintMe("CompareExchangeWeak ok, expected");
    }
    return success;
  }

 private:
  AtomicStampedPtr<T> atomic_stamped_ptr_;
  twist::ed::stdlike::mutex mtx_;

 private:
  StampedPtr<T> ToStampedPtr(SharedPtr<T>& shared_ptr, uint64_t stamp = 0) {
    StampedPtr<T> stamp_ptr{.raw_ptr=reinterpret_cast<T*>(shared_ptr.ctrl_ptr), .stamp=stamp};
    return stamp_ptr;
  }

  SharedPtr<T> ToSharedPtr(StampedPtr<T>& stamped_ptr) {
    typename SharedPtr<T>::ControlBlock* ctrl_ptr = reinterpret_cast<typename SharedPtr<T>::ControlBlock*>(stamped_ptr.raw_ptr);
    SharedPtr<T> shared_ptr = SharedPtr<T>(ctrl_ptr);
    return shared_ptr;
  }

  StampedPtr<T> AcquireTransientRef() {
    StampedPtr<T> next_stamped_ptr;
    StampedPtr<T> curr_stamped_ptr = atomic_stamped_ptr_.Load();
    do {
      next_stamped_ptr = curr_stamped_ptr;
      if (!next_stamped_ptr) {
        break;
      }
      next_stamped_ptr = next_stamped_ptr.IncrementStamp();
    } while (!atomic_stamped_ptr_.CompareExchangeWeak(curr_stamped_ptr, next_stamped_ptr));
    return next_stamped_ptr;
  }
};

// clippy target shared_ptr_stress_tests FaultyThreadsASan
// clippy target shared_ptr_stress_tests FaultyThreadsTSan