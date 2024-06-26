#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <optional>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

#include <queue>

namespace exe::support {

// Michael-Scott unbounded lock-free queue

/**
 * Очистка памяти от удалённых элементов происходит следующим образом.
 * Ноды, которые извлекаются из очереди, указатели на них кладутся в очередь deleted_.
 * Старшая половина этой очереди очищается каждый раз, когда очередь превышает 10 записей.
 */


//const bool kShouldPrint = true;
const bool kShouldPrint = false;

using twist::ed::stdlike::atomic;

template <typename T>
class MSQueue {
  struct Node {
    std::optional<T> value;
    atomic<Node*> next{nullptr};
  };
 
 private:
  atomic<Node*> head_{nullptr};
  atomic<Node*> tail_{nullptr};
  atomic<size_t> size_{0};
  std::queue<Node*> deleted_{};

 public:
  MSQueue() {
    Node* sentinel = new Node{std::nullopt};
    head_.store(sentinel);
    tail_.store(sentinel);
  }

  MSQueue(const MSQueue& other) = delete;
  MSQueue& operator=(const MSQueue&) = delete;

  // // Конструктор перемещения
  // MSQueue(MSQueue&& other) {
  //   Ll("Move ctor starts");
  //   head_.store(other.head_.load());
  //   Ll("After head_.store(other.head_.load());");
  //   tail_.store(other.tail_.load());
  //   Ll("After tail_.store(other.tail_.load());");
  //   size_.store(other.size_.load());
  //   Ll("After size_.store(other.size_.load());");
  //   //deleted_ = std::move(other.deleted_);
  //   other.head_.store(nullptr);
  //   other.tail_.store(nullptr);
  //   other.size_.store(0);
  //   Ll("Move ctor ends");
  // }

  // // Оператор присвоения перемещения
  // MSQueue& operator=(MSQueue&& other) {
  //   //printf("Move =\n");
  //   if (this != &other) {
  //     head_.store(other.head_.load());
  //     tail_.store(other.tail_.load());
  //     size_.store(other.size_.load());
  //     //deleted_ = std::move(other.deleted_);
  //     other.head_.store(nullptr);
  //     other.tail_.store(nullptr);
  //     other.size_.store(0);
  //   }
  //   return *this;
  // }

  // Актуально только под локом, иначе дата рейс с Put(). Put() тоже под локом.
  void Swap(MSQueue& other) {
    other.head_.exchange(head_.exchange(other.head_.load()));
    other.tail_.exchange(tail_.exchange(other.tail_.load()));
    other.size_.exchange(size_.exchange(other.size_.load()));
  }

  ~MSQueue() {
    //Ll("~MSQueue: start");
    DeleteList(head_.load());
    //Ll("~MSQueue: ends");
  }

  void Put(T item) {
    //Ll("Put");
    Node* new_node = new Node{std::move(item)};
    Node* curr_tail;
    while (true) {
      // Ll("Push in while");
      Node* curr_tail = tail_.load();

      if (curr_tail->next != nullptr) {
        // Helping for Concurrent Push
        if (tail_.compare_exchange_weak(curr_tail, curr_tail->next)) {
          continue;
        }
      }

      Node* null_ptr = nullptr;
      if (curr_tail->next.compare_exchange_weak(null_ptr, new_node)) {
        break;
      }
    }

    tail_.compare_exchange_strong(curr_tail, new_node);
    size_.fetch_add(1);
  }

  std::optional<T> Take() {
    while (true) {
      Node* curr_head = head_.load();
      if (curr_head->next == nullptr) {
        return std::nullopt;
      }
      if (head_.compare_exchange_weak(curr_head, curr_head->next)) {
        Node* next_head = curr_head->next;
        T item = std::move(*(next_head->value));
        
        deleted_.push(curr_head);
        CollectGarbage();
        
        size_.fetch_sub(1);
        return item;
      }
    }
  }

  bool IsEmpty() {
    return size_.load() == 0;
  }

  size_t Count() {
    return size_.load();
  }

  void Ll(const char* format, ...) {
    if (!kShouldPrint) {
        return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s MSQueue::%s, queue: %p\n", pid.str().c_str(), format, (void*)this);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
 private:
  void DeleteList(Node* head) {
    while (head != nullptr) {
      Node* to_delete = head;
      head = head->next;
      delete to_delete;
    }
    while (!deleted_.empty()) {
      auto node = deleted_.front();
      deleted_.pop();
      delete node;
    }
  }

  // Clears half of nodes that were marked as deleted, if their count more than 10
  // Thus it deletes 5 oldest marked deleted.
  void CollectGarbage() {
    size_t size = deleted_.size();
    if (size < 10) {
      return;
    }
    while (deleted_.size() > size / 2) {
      auto node = deleted_.front();
      deleted_.pop();
      delete node;
    }
    Ll("Clear half of deleted: %lu", size / 2);
  }
};

}