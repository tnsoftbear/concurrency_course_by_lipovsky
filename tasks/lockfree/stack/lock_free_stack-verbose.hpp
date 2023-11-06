#pragma once

#include "atomic_stamped_ptr.hpp"

#include <mutex>
#include <twist/ed/stdlike/atomic.hpp>

#include <optional>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

#include <iostream>

#include <vector>
#include <algorithm>
#include <twist/ed/stdlike/mutex.hpp>

// Treiber unbounded lock-free stack
// https://en.wikipedia.org/wiki/Treiber_stack

using twist::ed::stdlike::atomic;

//const bool kShouldPrint = false;
const bool kShouldPrint = true;
static size_t it = 0;
static atomic<size_t> deleted_counter{0};
static atomic<size_t> push_counter{0};
static atomic<size_t> pop_counter{0};

template <typename T>
class LockFreeStack {
  struct Node {
    T value;
    StampedPtr<Node> next{nullptr, 0};
    atomic<int> inner_counter{0};

    explicit Node(T&& v) : value(std::move(v)) {
      printf("Node: value: %zu\n", value.data);
    };
  };

 public:
  void Push(T item) {
    
    Ll("Push: starts for item: %d", item.data);
    
    //Node* new_node = new Node{std::move(item)};
    Node* new_node = new Node(std::move(item));
    Ll("Push: starts, new_node: %p, i: %d, next: %p, value: %zu", new_node, new_node->inner_counter.load(), (*new_node).next, (*new_node).value.data);
    StampedPtr<Node> new_head = StampedPtr<Node>{new_node, 0};
    new_head->next = head_.Load();
    while (!head_.CompareExchangeWeak(new_head->next, new_head)) {};

    push_counter.fetch_add(1);
    Ll("Push: ends, added: %p, head: %p, value: %zu, push-cnt: %lu", new_head.raw_ptr, head_.Load().raw_ptr, new_node->value.data, push_counter.load());

    PrintChain();
    // ++it;
    // if (it > 4) {
    //   abort();
    // }
  }

  std::optional<T> TryPop() {
    Ll("Pop: starts");
    StampedPtr<Node> old_head;

    for (;;) {
      auto h = head_.Load();
      if (h) {
        Ll("Pop: iteration, head: %p, ho: %d, hi: %d", h.raw_ptr, h.stamp, h->inner_counter.load());
      }

      do {
        old_head = head_.Load(); // .IncrementStamp();
        if (!old_head) {
          Ll("Pop: return null");
          return std::nullopt;
        }
      } while (!head_.CompareExchangeWeak(old_head, old_head.IncrementStamp()));

      h = head_.Load();
      Ll("a] Pop: taken p: %p, o: %lu, i: %d, head: %p, ho: %d, hi: %d", old_head.raw_ptr, old_head.stamp, old_head->inner_counter.load(), h.raw_ptr, h.stamp, h->inner_counter.load());
      old_head = old_head.IncrementStamp();
      Ll("b] Pop: taken p: %p, o: %lu, i: %d, head: %p, ho: %d, hi: %d",  old_head.raw_ptr, old_head.stamp, old_head->inner_counter.load(), h.raw_ptr, h.stamp, h->inner_counter.load());
      
      auto tmp_head = old_head;
      if (head_.CompareExchangeWeak(tmp_head, old_head->next)) {
        int res = 1 - old_head.stamp;
        //old_head->inner_counter.fetch_add(old_head.stamp - 1);
        //deletion_.push_back(old_head.raw_ptr);
        T value = std::move(old_head->value);
        Ll("Pop: move for deletion: %p, o: %d, i: %d", old_head.raw_ptr, old_head.stamp, old_head->inner_counter.load());
        if (old_head->inner_counter.fetch_add(old_head.stamp - 1) == res) {
          deleted_counter.fetch_add(1);
          Ll("Pop: plus deletion success: %p, o: %d, i: %d, del-cnt: %lu", old_head.raw_ptr, old_head.stamp, old_head->inner_counter.load(), deleted_counter.load());
          delete old_head.raw_ptr;
        } else {
          Ll("Pop: plus deletion failed: %p, o: %d, i: %d", old_head.raw_ptr, old_head.stamp, old_head->inner_counter.load());
        }
        pop_counter.fetch_add(1);
        Ll("Pop: return value: %d, pop-cnt: %lu", value.data, pop_counter.load());
        return value;
      } else {
        //old_head->inner_counter.fetch_sub(1);
        Ll("Pop: minus only case !!!! p: %p, o: %d, i: %d, h: %p, ho: %d", old_head.raw_ptr, old_head.stamp, old_head->inner_counter.load(), head_.Load().raw_ptr, head_.Load().stamp);
        if (old_head->inner_counter.fetch_sub(1) == 1) {
          deleted_counter.fetch_add(1);
          Ll("Pop: minus deletion success: %p, o: %d, i: %d, del-cnt: %lu", old_head.raw_ptr, old_head.stamp, old_head->inner_counter.load(), deleted_counter.load());
          delete old_head.raw_ptr;
        } else {
          Ll("Pop: minus deletion failed: %p, o: %d, i: %d", old_head.raw_ptr, old_head.stamp, old_head->inner_counter.load());
        }
      }
      //CollectGarbage();
    }
  }

  void CollectGarbage() {
    if (deletion_.empty()) {
      return;
    }
    mtx_.lock();
    auto new_end = std::remove_if(deletion_.begin(), deletion_.end(), [this](Node* ptr) { 
      if (ptr->inner_counter.load() == 0) {
        deleted_counter++;
        Ll("CollectGarbage: Deleting p: %p, nr: %lu", ptr, deleted_counter);
        delete ptr;
        return true;
      }
      return false;
    });
    deletion_.erase(new_end, deletion_.end());
    Ll("CollectGarbage: Complete, size: %lu", deletion_.size());
    mtx_.unlock();
  }

  void PrintChain() {
    //mtx_.lock();
    StampedPtr<Node> head = head_.Load();
    while (head) {
      std::cout << head.raw_ptr << "(o: " << head.stamp << ", i: " << head->inner_counter.load() << "), ";
      head = head->next;
    }
    std::cout << std::endl;
    //mtx_.unlock();
  }

  ~LockFreeStack() {
    Ll("~LockFreeStack()");
    StampedPtr<Node> head = head_.Load();
    while (head) {
      StampedPtr<Node> old_head = head;
      head = old_head->next;
      delete old_head.raw_ptr;
      //delete old_head;
    }
  }

 private:
  AtomicStampedPtr<Node> head_{StampedPtr<Node>{nullptr, 0}};
  std::vector<Node*> deletion_;
  twist::ed::stdlike::mutex mtx_;

  void Ll(const char* format, ...) {
    if (!kShouldPrint) {
        return;
    }

    char buf[250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s LockFreeStack::%s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};
