#pragma once

#include <mutex>
#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>
#include <twist/ed/wait/spin.hpp>

#include <cstdlib>
#include <sstream>
#include <twist/ed/stdlike/thread.hpp> // for debug logs

#include "exe/support/msqueue.hpp"
#include "exe/fibers/core/fiber.cpp"
#include "exe/fibers/core/awaiter.hpp"
#include "exe/threads/spinlock.hpp"

using twist::ed::stdlike::atomic;

namespace exe::fibers {

// https://gobyexample.com/waitgroups

using WaitQueue = exe::support::MSQueue<Fiber*>;

class WaitGroup {
 public:
  void Add(size_t count) {
    Ll("Add: count: %lu ------------------------- ", count);
    done_calls_countdown_.fetch_add(count);
  }

  void Done() {
    auto id = Fiber::Self()->GetId();
    Ll("Done: starts, id: %lu", id);
    if (done_calls_countdown_.fetch_sub(1) == 1) {
      Ll("Done: rescheduling queued");
      
      mtx_.Lock();
      bool is_empty = wait_q_.IsEmpty();
      mtx_.Unlock();
      
      // Если очередь пуста, то авайтер можно заканчивать
      if (is_empty) {
        is_last_wait_completed_.store(true);
        return;
      }

      while (!is_empty) {
        Ll("Done: while iteration starts");
        std::optional<Fiber*> fiber_opt = wait_q_.Take();
        is_empty = wait_q_.IsEmpty();
        if (is_empty) {
          Ll("Done: queue is empty, before decrease counter to 0");
          // Установка флага для выполнения последнего AwaitSuspend() происходит до перепланировки этого файбера,
          // потому что иначе логика файбера может успеть выполниться, рутина завершиться с удалением группы ожидания
          // и мы попытаемся записать свойство в удалённом объекте (но это не случай, когда WaitGroupAwaiter ждёт на спине)
          is_last_wait_completed_.store(true);
        }
        Fiber* fiber = std::move(fiber_opt.value());
        Ll("Done: before fiber->Schedule(), dequeued-id: %lu", fiber->GetId());
        // возобновляем приостановленный Wait() (после fiber->Suspend(&awaiter);)
        fiber->Schedule();
      }
    }
    Ll("Done: ends, id: %lu", id);
  }

  void Wait() {
    auto fiber = Fiber::Self();
    auto id = fiber->GetId();
    suspended_waits_counter_++;
    WaitGroupAwaiter awaiter{*this};
    Ll("Wait: before fiber->Suspend(), id: %lu", id);
    fiber->Suspend(&awaiter);
    suspended_waits_counter_--;
    Ll("Wait: ends, id: %lu", id);
  }

 private:
  // Очередь файберов, ожидающих возобновление, когда произойдёт необходимое кол-во вызовов Done()
  WaitQueue wait_q_;
  // Ожидаемое кол-во вызовов Done() для завершения требований группы ожидания
  atomic<uint32_t> done_calls_countdown_{0};
  // Счётчик приостановленных файберов спомощью Wait()
  atomic<uint32_t> suspended_waits_counter_{0};
  // is ready to complete the last wait (by re-scheduling its fiber that was suspended)
  atomic<bool> is_last_wait_completed_{false};
  // Мутекс защищает гонку между остановкой и возобновлением файбера при помещении его в очередь ожидания.
  exe::threads::SpinLock mtx_;

  class WaitGroupAwaiter : public IAwaiter {
    private:
      WaitGroup& wg_;
    public:
      explicit WaitGroupAwaiter(WaitGroup& host) : wg_(host) {}
      void AwaitSuspend(Fiber* fiber) {
        if (!TryEnqueue(wg_, fiber)) {
          Reschedule(wg_, fiber);
        }
      }

      // Положить файбер в очередь ожидания, если ещё не выполнинлось достаточное кол-во Done() вызовов.
      bool TryEnqueue(WaitGroup& wg, Fiber* fiber) {
        std::lock_guard lock(wg.mtx_);
        if (wg.done_calls_countdown_.load() != 0) {
          wg.wait_q_.Put(std::move(fiber));
          wg.Ll("AwaitSuspend: Put in queue, id: %lu", fiber->GetId());
          return true;
        }
        return false;
      }

      // Перепланировать файбер, который не надо добавлять в очередь ожидания.
      void Reschedule(WaitGroup& wg, Fiber* fiber) {
        // Мы здесь, когда произошло ожидаемое кол-во вызовов Done(), которое необходимо для завершения группы ожидания. 
        // См. done_func_countdown_ == 0 в TryEnqueue(),
        // Т.к. группа ожидания выполнила свою задачу, то все следующие приостановленные файберы 
        // (а их может быть больше, чем счётчик установленный в Add(count)), 
        // больше не кладутся в очередь ожидания wait_q_, и могут быть возобновлены перепланировкой.
        //
        // Но нам необходимо обработать граничный случай последнего приостановленного файбера (suspended_wait_count_ == 1),
        // мы хотим его возобновить только когда ф-ция Done() полностью опустошит очередь ожидания файберов.
        // Иначе он и его рутина может успеть завершиться, и объект группы ожидания будет удалён, пока Done() всё ещё исполняется.
        if (wg.suspended_waits_counter_.load() == 1) {
          twist::ed::SpinWait spin_wait;
          while (!wg.is_last_wait_completed_.load()) {
            wg.Ll("AwaitSuspend: spin, id: %lu", fiber->GetId());
            spin_wait();
          }
        }

        // Запланировать завершение файбера.
        wg.Ll("AwaitSuspend: fiber->Schedule();");
        fiber->Schedule();
      }
  };

 private:
  void Ll(const char* format, ...) {
  //const bool k_should_print = true;
  const bool k_should_print = false;
    if (!k_should_print) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    sprintf(buf, "%s %s, counter: %d\n", pid.str().c_str(), format, (int)done_calls_countdown_.load());
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};

}  // namespace exe::fibers
