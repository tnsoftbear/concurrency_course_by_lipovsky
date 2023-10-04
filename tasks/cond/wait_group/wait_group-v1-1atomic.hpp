#pragma once

#include <atomic>
#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>
#include <twist/ed/wait/spin.hpp>

#include <cstdlib>

#include <iostream>
#include <sstream>
#include <twist/ed/stdlike/thread.hpp> // for debug logs

using twist::ed::stdlike::atomic;

// Реализация с 1 атомиком

class WaitGroup {
 public:
  explicit WaitGroup(uint32_t init = 0) 
    : init_{init}
    {
      // Ll("Construct: enter");
    }

  // += count
  void Add(size_t count) {
    //Ll("Add: enter count: %d", count);
    counter_.fetch_add(count, std::memory_order_relaxed);
    //Ll("Add: after counter_.fetch_add(%d)", count);
  }

  // =- 1
  void Done() {
    //Ll("Done: enter");
    
    // check - необходимо посчитать заранее. 
    // Иначе мы сталкиваемся с ситуацией, когда первый поток сделал a.fetch_sub(), а второй поток зашел в Wait, 
    // вышел из цикла по брейку, вышел из Wait ф-ции и в коллере убил объект WaitGroup, потому что Wait всё. 
    // Далее первый поток обращается к свойству init в объекте которого нет.
    // Т.е. при таком фиксе стресс тесты с адрес-санитайзером проходят в контексте фреймворка Липовского twist.
    // Но меня терзают сомнения, что оно будет рабоать в реале, ведь внутри ифа мы всё ещё обращаемся к свойству убитого объекта: a.notify_all().
    uint32_t check = init_ + 1;
    if (counter_.fetch_sub(1, std::memory_order_acq_rel) == check) {
      //Ll("Done: before wake");
      twist::ed::futex::WakeKey k = twist::ed::futex::PrepareWake(counter_);
      twist::ed::futex::WakeAll(k);
      //Ll("Done: after wake");
    }
  }

  // == 0
  // One-shot
  void Wait() {
    //Ll("Wait: enter");
    while (true) {
      uint32_t c = counter_.load(std::memory_order_acquire);
      if (c == init_) { 
        //Ll("Wait: break c: %d == init_: %d", c, init_);
        break;
      }
      //Ll("Wait: before wait on c: %d",  c);
      twist::ed::futex::Wait(counter_, c, std::memory_order_relaxed);
      //Ll("Wait: after wait on c: %d", c);
    }
  }

  uint32_t GetCounter() {
    return counter_.load();
  }

 private:
  atomic<uint32_t> counter_{0};
  uint32_t init_{0};
  //bool should_print_{true};
  bool should_print_{false};

 private:
  void Ll(const char* format, ...) {
    if (!should_print_) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s %s. counter: %d\n", pid.str().c_str(), format, (int)counter_.load());
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};
