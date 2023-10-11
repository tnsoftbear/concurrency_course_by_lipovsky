#include <exe/fibers/sched/yield.hpp>
#include <exe/fibers/sched/go.hpp>
#include "exe/fibers/core/scheduler.hpp"
#include "exe/support/queue.hpp"
#include "exe/tp/submit.hpp"
#include <exe/coro/core.hpp>
#include <exe/fibers/core/fiber.hpp>

namespace exe::fibers {

void Ll2(const char* format, ...) {
  if (!exe::tp::kShouldPrint) {
    return;
  }

  char buf[250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s %s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

void Yield() {
  // Здесь происходит состояние гонки, если запланированная задача файбера из Schedule() выполняется раньше, 
  // чем произойдет переключение контекста в Suspend(). См. In_yield_submit_suspend_data_race.log
  // Хз, как иначе реализовать задержку выполнения следующей задачи. 
  // Липовский упоминает некую ф-ция SubmitContinuation(). (Хз как реализовать, возможно, с ещё одной очередью..)
  Ll2("Yield: before Schedule");
  auto fiber = Fiber::Self();

  exe::tp::Submit(fiber->GetScheduler(), [fiber]() { 
    twist::ed::SpinWait spin_wait;
    while (!fiber->IsSuspended()) {
      spin_wait();
    }
    fiber->Schedule();
    //fiber->Run(); 
  });

  Ll2("Yield: before Suspend()");
  coro::Coroutine::Suspend();
}

}  // namespace exe::fibers
