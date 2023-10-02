#pragma once

#include <condition_variable>
#include <mutex>
#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

#include <optional>

#include <iostream>
#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/stdlike/thread.hpp>
#include <sstream>

using twist::ed::stdlike::atomic;
using twist::ed::stdlike::mutex;
using twist::ed::stdlike::condition_variable;


namespace tp {

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

// Unbounded blocking multi-producers/multi-consumers (MPMC) queue

template <typename T>
class UnboundedBlockingQueue {
 struct Node {
  T elem;
  Node* prev{nullptr};
 };
 public:
  UnboundedBlockingQueue() {
    Ll("Construct");
    mtx_ = std::make_unique<mutex>();
    head_ = nullptr;
    tail_ = nullptr;
    is_opened_ = true;
  }

  ~UnboundedBlockingQueue() {
    Close();
  }

  // Если очередь еще не закрыта (вызовом Close), то положить в нее value и вернуть true,
  // в противном случае вернуть false.
  bool Put(T e) {
    Ll("Enter put");
    std::lock_guard<mutex> lock(*mtx_);
    Ll("Enter put after lock");
    if (!is_opened_) {
      Ll("Put return false");
      return false;
    }

    Node* node = new Node{.elem=std::move(e)};
    Ll("Put e");
    if (head_ == nullptr) {
      head_ = node;
      cv_.notify_one();
    }
    if (tail_ != nullptr) {
      tail_->prev = node;
    }
    tail_ = node;
    
    Ll("Put return true");
    return true;
  }

  // Дождаться и извлечь элемент из головы очереди; если же очередь закрыта и пуста, то вернуть std::nullopt.
  std::optional<T> Take() {
    Ll("Enter Take");
    std::unique_lock<mutex> lock2(*mtx_);
    Ll("Enter Take wait");
    cv_.wait(lock2, [this] { return !is_opened_ || head_ != nullptr; });
    Ll("Take after wait");

    //std::lock_guard<mutex> lock(*mtx_);
    //Ll("Enter Take after lock");

    if (!is_opened_ && head_ == nullptr) {
      Ll("Take nullopt");
      return std::nullopt;
    }

    if (head_->prev == nullptr) {
      T elem = std::move(head_->elem);
      head_ = nullptr;
      tail_ = nullptr;
      Ll("Take-1 e ");
      return elem;
    }

    T elem = std::move(head_->elem); 
    head_ = head_->prev;
    Ll("Take-2 e ");
    return elem;
  }

  // Закрыть очередь для новых Put-ов. Уже добавленные в очередь элементы останутся доступными для извлечения.
  void Close() {
    {
      std::lock_guard<mutex> lock(*mtx_);
      if (!is_opened_) {
        return;
      }
      Ll("Close");
      is_opened_ = false;
    }
    Ll("Close after is_opened_ = false");
    cv_.notify_all();
    Ll("Close after notify all");
  }

  void Ll(const char* format, ...) {
    if (!kShouldPrint) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s %s c:%lu\n", pid.str().c_str(), format, Count());
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }


  size_t Count() {
    size_t c = 0;
    Node* node = head_;
    while (node != nullptr) {
      c++;
      node = node->prev;
    }
    return c;
  }

 private:
  bool is_opened_{true};
  Node* head_{nullptr};
  Node* tail_{nullptr};
  twist::ed::stdlike::condition_variable cv_;
  std::unique_ptr<twist::ed::stdlike::mutex> mtx_;
  //std::unique_ptr<std::unique_lock<twist::ed::stdlike::mutex>> lock_;
  // Buffer
};

}  // namespace tp
