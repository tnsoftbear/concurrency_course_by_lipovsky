#pragma once

#include "atomic_stamped_ptr.hpp"

#include <cstddef>
#include <twist/ed/stdlike/atomic.hpp>
#include <utility>

using twist::ed::stdlike::atomic;

//////////////////////////////////////////////////////////////////////

// --- for debug logs ---

#include <cstdlib>
#include <sstream>
#include <twist/ed/stdlike/thread.hpp>
const bool kShouldPrint = false;
//const bool kShouldPrint = true;

// --- --- --- --- --- ---

namespace detail {

struct SplitCount {
  int32_t transient{0}; // транзитивный счётчик
  int32_t strong{0};  // > 0 ссс - счётчик сильных ссылок

  // Оператор сравнения нужен для операций в twist::ed::stdlike::atomic
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

template <typename T>
class AtomicSharedPtr;

template <typename T> class SharedPtr {
public:
 struct ControlBlock {
  T* data_ptr{nullptr};
  atomic<detail::SplitCount> counter = detail::SplitCount{};
  ControlBlock(T* data_ptr, detail::SplitCount cnt)
    : data_ptr(data_ptr)
    , counter(cnt) {}
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
      : ctrl_ptr_(new ControlBlock(data, counter)) {
    IncrementStrong();
  }

  explicit SharedPtr(ControlBlock* ctrl_ptr)
      : ctrl_ptr_(ctrl_ptr) {
    IncrementStrong();
  }

  SharedPtr() {
  }

  // Copy ctor
  SharedPtr(const SharedPtr<T>& that)
      : ctrl_ptr_(that.ctrl_ptr_)
  {
    IncrementStrong();
  }

  // Copy assignment
  SharedPtr<T>& operator=(const SharedPtr<T>& that) {
    if (this != &that) {
      Reset();
      ctrl_ptr_ = that.ctrl_ptr_;
      IncrementStrong();
    }
    return *this;
  }

  // Move ctor
  SharedPtr(SharedPtr<T>&& that) noexcept
      : ctrl_ptr_(std::move(that.ctrl_ptr_)) {
    that.ctrl_ptr_ = nullptr;
  }

  // Move assignment
  SharedPtr<T>& operator=(SharedPtr<T>&& that) noexcept {
    if (this != &that) {
      Reset();
      ctrl_ptr_ = std::exchange(that.ctrl_ptr_, nullptr);
    }
    return *this;
  }

  ~SharedPtr() {
    Reset();
  }

  T* operator->() const { 
    return ctrl_ptr_->data_ptr;
  }

  T& operator*() const { return *ctrl_ptr_->data_ptr; }

  explicit operator bool() const { return ctrl_ptr_ != nullptr; }

  // cleanup any existing data
  void Reset() {
    if (ctrl_ptr_ != nullptr) {
      DecrementStrong();
      ctrl_ptr_ = nullptr;
    }
  }

private: // public, потому что обращаюсь к указателю в AtomicSharedPtr для сохранения его в StampedPtr
  friend class AtomicSharedPtr<T>;
  ControlBlock* ctrl_ptr_{nullptr};

private:
  void IncrementCounter(int32_t strong, int32_t transient) {
    if (ctrl_ptr_ != nullptr) {
      detail::SplitCount counter_old, counter_new;
      counter_old = ctrl_ptr_->counter.load();
      do {
        counter_new = counter_old;  // Хз, почему так не работает: counter_new = ctrl_ptr->counter.load();
        counter_new.strong += strong;
        counter_new.transient += transient;
      } while (!ctrl_ptr_->counter.compare_exchange_weak(counter_old, counter_new));

      if (
        counter_new.strong == 0
        && counter_new.transient == 0
      ) {
        delete ctrl_ptr_;
      }
    }
  }

  void IncrementStrong(int32_t inc = 1) {
    IncrementCounter(inc, 0);
  }

  void DecrementStrong(int32_t dec = 1) {
    IncrementCounter(-dec, 0);
  }

  void IncrementTransient(int32_t inc = 1) {
    IncrementCounter(0, inc);
  }

  void DecrementTransient(int32_t dec = 1) {
    IncrementCounter(0, -dec);
  }

  void PrintMe(std::string prefix = "") {
    return;
    if (ctrl_ptr_) {
      Ll("SharedPtr::%s (this: %p, ctrl: %p) strong: %d, transient: %d",
        prefix.c_str(),
        this,
        ctrl_ptr_,
        ctrl_ptr_->counter.load().strong,
        ctrl_ptr_->counter.load().transient
      );
    } else {
      Ll("SharedPtr::%s (this: %p, ctrl: %p)", prefix.c_str(), this, ctrl_ptr_);
    }
  }
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
    Store(SharedPtr<T>());
  }

  SharedPtr<T> Load() {
    // Увеличиваем транзитивный плюсовой счётчик (stamped_ptr.stamp)
    auto stamped_ptr = AcquireTransientRef();
    auto shared_ptr = ToSharedPtr(stamped_ptr);
    // Уменьшаем транзитивный минусовой счётчик (counter.transient из ControlBlock)
    shared_ptr.DecrementTransient(1);

    // Handle stamp overflow - сокращаем транзитивный счётчик
    size_t limit = 64000;
    if (stamped_ptr.stamp > limit) {
      shared_ptr.IncrementTransient(limit);
      stamped_ptr.stamp -= limit;
      atomic_stamped_ptr_.Store(stamped_ptr);
    }

    return shared_ptr;
  }

  void Store(SharedPtr<T> target) {
    // Увеличиваем ссс, потому что передаём владение в AtomicStampedPtr, где не используется SharedPtr, а StampedPtr, который не имеет счётчика.
    target.IncrementStrong();
    StampedPtr<T> stamped_ptr = ToStampedPtr(target);
    StampedPtr<T> old_stamped_ptr = atomic_stamped_ptr_.Exchange(stamped_ptr);
    SharedPtr<T> old_shared_ptr = ToSharedPtr(old_stamped_ptr);
    // Уменьшаем ссс, потому что забираем объект из владения AtomicStampedPtr.
    // А так же сводим транзитивный счётчик: counter.transient + stamped_ptrstamp
    old_shared_ptr.IncrementCounter(-1, old_stamped_ptr.stamp);
  }

  explicit operator SharedPtr<T>() {
    return Load();
  }

  bool CompareExchangeWeak(SharedPtr<T>& expected, SharedPtr<T> desired) {
    StampedPtr<T> old_stamped_ptr = AcquireTransientRef();

    if ((void*)old_stamped_ptr.raw_ptr != (void*)expected.ctrl_ptr_) {
      expected = ToSharedPtr(old_stamped_ptr);
      expected.DecrementTransient();
      return false;
    }

    StampedPtr<T> exp_stamp_ptr = ToStampedPtr(expected, old_stamped_ptr.stamp);
    StampedPtr<T> des_stamp_ptr = ToStampedPtr(desired);
    bool success = atomic_stamped_ptr_.CompareExchangeWeak(exp_stamp_ptr, des_stamp_ptr);
    if (success) {
      // Увеличиваем ссс, потому что передаём владение объекта desired в AtomicStampedPtr
      desired.IncrementStrong();
      // Уменьшаем ссс, потому что забрали объект expected из хранилища AtomicStampedPtr
      expected.IncrementCounter(-1, old_stamped_ptr.stamp - 1);
    } else {
      SharedPtr<T> old_shared_ptr = ToSharedPtr(old_stamped_ptr);
      old_shared_ptr.DecrementTransient();
      expected = Load();
    }
    return success;
  }

 private:
  AtomicStampedPtr<T> atomic_stamped_ptr_;

 private:
  StampedPtr<T> ToStampedPtr(SharedPtr<T>& shared_ptr, uint64_t stamp = 0) {
    StampedPtr<T> stamp_ptr{.raw_ptr=reinterpret_cast<T*>(shared_ptr.ctrl_ptr_), .stamp=stamp};
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