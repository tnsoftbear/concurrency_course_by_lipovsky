#include <cstddef>
#include <exe/fibers/core/fiber.hpp>
#include <exe/coro/core.hpp>

#include <memory>
#include <mutex>
#include <twist/ed/local/ptr.hpp>

#include <map>
#include <exe/fibers/core/scheduler.hpp>
#include "exe/tp/submit.hpp"

// 1] main() -> fibers::Go() -> new Fiber() + fiber->Schedule() 
// 2] -> ThreadPool::Submit({ fiber->Run() }) -> exits from fibers::Go() -> scheduler.WaitIdle() -> ThreadPool calls task() from queue in loop
// 3] fiber->Run() -> coroutine->Resume() saves caller_context_
// 4] -> switch execution to context_ with trampoline entry-point: coroutine->Run() -> routine_()
// 5] routine_() does its job and calls fibers::Yield() -> fiber->Suspend() 
// 6] -> coroutine->Suspend() saves execution in context_, restores caller_context_ from the point [3]
// 7] exits coroutine->Resume() -> exits to fiber->Run() 
// 8] -> fiber->Schedule(); -> 2] -> 3], coroutine->Resume() saves execution context in caller_context_ and switches it to context_ previously saved at the point [6].
// 9] exits from coroutine->Suspend() -> exits from fibers::Yield() -> continue routine_()
// 10] routine_() ends and exits to Coroutine::Run() -> context_.ExitTo() switch context saved at [8] -> exits to Coroutine::Resume()
// 11] -> exits to Fiber::Run() -> (IsCompleted == true) -> delete this; -> exits to ThreadPool loop.

// ### Поток исполнения

// 1] В `main()` вызывается `fibers::Go()`, которая создаёт новый `Fiber`. `Fiber` создает объект `Coroutine`, куда передаёт пользовательскую лямбду, она сохраняется в поле `Couroutine::routine_`. Вызывается `fiber->Schedule()`. 
// 2] Вызов ф-ции `Run()` файбера добавляется в очередь планировщика, который реализован через пул потоков. Выход из `fibers::Go()`. Вызов `scheduler.WaitIdle()` ожидает завершения задач добавленных в пул потоков.
// 3] Задача пула потоков вызвает запланированный `fiber->Run()`, что вызвает `coroutine->Resume()`, которая сохраняет текущий контекст исполнения (регистры) в памят выделенную под стек вызовов в свойство `caller_context_`.
// 4] Это так же приводит к вызову точки входа в пользовательскую лямбду `routine_()`, которая запускается в `coroutine->Run()`. Эта точка входа была зарегистирована в конструкторе `Coroutine`, т.к класс реализует `ITrampoline` интерфейс.
// 5] `routine_()` выполняется до вызова `fibers::Yield()`, что вызывает `fiber->Suspend()` и `coroutine->Suspend()`.
// 6] `coroutine->Suspend()` приостанавливает исполнение лямбды в корутине. Она сохраняет текущий контекст исполнения в `context_`, и восстанавливает контексти исполнения из `caller_context_`, который был сохраннён на этапе [3].
// 7] Старый контекст возвращает нас в `coroutine->Resume()`, из которой мы выходим в `fiber->Run()`.
// 8] Здесь мы перепланируем исполнение вызвав `fiber->Schedule()`, это приводит к повторению шага [2] и [3]. Как известно, на шаге [3] `coroutine->Resume()` опять сохраняет контекст исполнения в `caller_context_` и восстанавливает его в из `context_`, который был сохранён на шаге [6].
// 9] Это возвращает нас в `coroutine->Suspend()`, из которой мы выходим в `fibers::Yield()`, продолжаем исполнение пользовательской лямбды `routine_()`.
// 10] `routine_()` продолжается до конца и выходит в `Coroutine::Run()`, она вызывает `context_.ExitTo()`, что переключает контекст исполнения на `caller_context_` сохранённый на шаге [8], что приводит к завершению `Coroutine::Resume()`.
// 11] Мы выходим в `Fiber::Run()`, видим что корутина завершилась и удаляем её и это файбер. Выходим в луп пула потоков.

// clippy target fibers_sched_unit_tests DebugASan --suite Fibers --test Yield3
// clippy target fibers_sched_stress_tests FaultyThreadsTSan
// clippy target fibers_sched_stress_tests Debug

namespace exe::fibers {

static twist::ed::ThreadLocalPtr<Fiber> current_fiber = nullptr;

Fiber::Fiber(Scheduler& scheduler, Routine routine)
  : scheduler_(scheduler)
  , stack_(sure::Stack::AllocateBytes(1024 * 64))
  , coro_(std::move(
    new coro::Coroutine(stack_.MutView(), std::move(routine))
  ))
{
}

void Fiber::Run() {
  current_fiber = this;
  coro_->Resume();
  
  if (coro_->IsCompleted()) {
    delete coro_;
    delete this;
    return;
  }
  
  // Schedule the next round of suspended execution after fibers::Yield() of routine
  Schedule();
}

Fiber* Fiber::Self() {
  return current_fiber;
}

void Fiber::Schedule() {
  exe::tp::Submit(scheduler_, [&]() { Run(); });
}

void Fiber::Suspend() {
  coro_->Suspend();
}

void Fiber::Ll(const char* format, ...) {
  if (!exe::tp::kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Fiber::%s, this: %p, coro_: %p\n", pid.str().c_str(), format, (void*)this, (void*)coro_);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::fibers
