#pragma once

#include <twist/ed/stdlike/atomic.hpp>

#include <hazard/manager.hpp>

#include <optional>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

#include <queue>

// Michael-Scott unbounded MPMC lock-free queue

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

using twist::ed::stdlike::atomic;

template <typename T>
class LockFreeQueue {
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
  explicit LockFreeQueue(hazard::Manager* gc)
      : gc_(gc) {
    Node* sentinel = new Node{std::nullopt};
    head_.store(sentinel);
    tail_.store(sentinel);
  }

  LockFreeQueue()
      : LockFreeQueue(hazard::Manager::Get()) {
  }

  ~LockFreeQueue() {
    DeleteList(head_.load());
  }

  void Push(T item) {
    Node* new_node = new Node{std::move(item)};
    Node* curr_tail;
    while (true) {
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

  std::optional<T> TryPop() {
    while (true) {
      //Ll("TryPop in while");
      Node* curr_head = head_.load();
      if (curr_head->next == nullptr) {
        //Ll("Return nullopt");
        return std::nullopt;
      }
      if (head_.compare_exchange_weak(curr_head, curr_head->next)) {
        Node* next_head = curr_head->next;
        T item = std::move(*(next_head->value));
        
        //delete curr_head;
        //deleted_.push(curr_head);
        //CollectGarbage();
        
        size_.fetch_sub(1);
        //Ll("Return item");
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

 private:
  hazard::Manager* gc_;

  void DeleteList(Node* head) {
    while (head != nullptr) {
      Node* to_delete = head;
      head = head->next;
      delete to_delete;
      //Ll("Deleted node");
    }
    while (!deleted_.empty()) {
      auto node = deleted_.front();
      deleted_.pop();
      delete node;
      //Ll("Deleted hazard");
    }
  }

  // Clears half of nodes that were marked as deleted, if their count more than 10
  // Thus it deletes 5 oldest marked deleted.
  void CollectGarbage() {
    size_t size = deleted_.size();
    if (size < 1000) {
      return;
    }
    while (deleted_.size() > size / 2) {
      auto node = deleted_.front();
      deleted_.pop();
      delete node;
      //Ll("Deleting hazards");
    }
    Ll("Clear half of deleted: %lu", size / 2);
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
};
