#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>
#include <twist/ed/wait/spin.hpp>

#include <cstdlib>
#include <sstream>
#include <twist/ed/stdlike/thread.hpp> // for debug logs

using twist::ed::stdlike::atomic;

// Реализация с 2 атомиками

class WaitGroup {
 public:
  explicit WaitGroup() {}

  // += count
  void Add(size_t count) {
    if (counter_.fetch_add(count) == 0) {
      done_.store(0);
    }
  }

  // =- 1
  void Done() {
    if (counter_.fetch_sub(1) - 1 == 0) {
      done_.store(1);
      twist::ed::futex::WakeKey k = twist::ed::futex::PrepareWake(done_);
      twist::ed::futex::WakeAll(k);
    }
  }

  // == 0
  // One-shot
  void Wait() {
    // a) while необходим для решения проблемы spurious wake up.
    // b) while условие обязательно на свойстве done_, а не counter_.load() != 0, иначе возможна следующая гонка (см. тест внизу);
    // Поток-1 начинает Done() и уменьшает счётчик counter_ до 0, 
    // Поток-2 коллера входит в Wait(), видит счётчик 0, выходит из Wait(), удаляет объект WaitGroup.
    // Поток-1 продолжает выполнение ф-ции Done(), где пытается писать в свойство done_.store(1) для уже удалённого объекта this.
    while (done_.load() == 0) {
      twist::ed::futex::Wait(done_, 0);
    }
  }

  uint32_t GetCounter() {
    return counter_.load();
  }

 private:
  atomic<uint32_t> counter_{0};
  atomic<uint32_t> done_{1};
  bool should_print_{true};

 private:
  void Ll(const char* format, ...) {
    if (!should_print_) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s %s done: %d, counter: %d\n", pid.str().c_str(), format, (int)done_.load(), (int)counter_.load());
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};

/**

void StorageTest() {
  auto* wg = new WaitGroup{};

  wg->Add(1);
  twist::ed::stdlike::thread t([wg] {
    wg->Done();
  });

  wg->Wait();
  delete wg;

  t.join();
}

*/