#include <exe/executors/strand.hpp>
#include <twist/ed/wait/spin.hpp>
#include <map>

/**
 Алгоритм этого решения скопирован из await fw.
 https://gitlab.com/Lipovsky/await/-/blob/master/await/tasks/exe/strand.cpp?ref_type=heads
 Здесь так же используется идиома PImpl, в которой мы выносим логику странда в под-класс Impl.
 К свойству класса Impl, мы обращаемся через обёртку умного указателя с подсчётом ссылок.
 Это позволяет контроллировать время жизни объекта, пока наш враппер вокруг задач `Run()` выполняется до конца в пуле потоков.
 Этот умный указатель я тоже скопировал из https://gitlab.com/Lipovsky/refer/

 Это помогло решить проблему, с которой я столкнулся в других решениях,
 которая проявляется в тесте: clippy target strand_lifetime_tests FaultyFibersASan
 После завершения задачи, объект странда удаляется из памяти, потому что в этом тесте был сделан automation.reset() 
 См. /workspace/concurrency-course/tasks/tasks/executors/tests/executors/strand/lifetime.cpp
 А т.к. this удалён, мы больше не можем обращаться к свойствам объекта и принимать по ним решения.
 В данном случае речь идёт о счётчике scheduled, по которому мы решаем продолжать ли обслуживать очередь ожидания задач или нет.

 В другом (работающем, но с ненужными хаками) решении я вынес этот счётчик в статическую память в мапу. См. детали в strand-static-atomic.cpp.
 
 В решениях лекций 2021, 2022 годов Strand наследовался от std::enable_shared_from_this<Strand>, 
 чтобы создать шаред-пойнтер на себя и передать его в лямбду уходящую в пул потоков (`self = shared_from_this()`), 
 чтобы продлить время жизни стренда до окончания исполнения `Run()`.
 Но сейчас такое решение падает с ошибкой:
 Test 'Robots_1' FAILED ¯\_(ツ)_/¯: Test subprocess terminated by signal 6, stderr: 
 Panicked at /tmp/clippy-build/FaultyFibers/_deps/twist-src/twist/rt/layer/fiber/runtime/fiber.cpp
 :virtual void twist :: rt::fiber::Fiber::Run()[Line 32]: Uncaught exception in fiber #11: bad_weak_ptr

 Так же в лекциях присутствует решение Р.Липовского со спинлоком, см. bak/strand-spinlock.cpp, оно падает с ошибкой:
 Panicked at /tmp/clippy-build/FaultyFibers/_deps/twist-src/twist/rt/layer/fiber/runtime/fiber.cpp
 :virtual void twist :: rt::fiber::Fiber::Run()[Line 32]: Uncaught exception in fiber #3: bad_optional_access 
 */

namespace exe::executors {

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

Strand::Impl::Impl(IExecutor& executor)
  : underlying_(executor) {}

void Strand::Impl::Submit(Task task) {
  tasks_.Put(std::move(task));
  if (scheduled_.fetch_add(1) == 0) {
    SubmitSelf();
  }
}

void Strand::Impl::SubmitSelf() {
  AddRef();
  underlying_.Submit([this]()  {
    Run();
  });
}

void Strand::Impl::Run() noexcept {
  TaskQueue processing_tasks;
  do {
    processing_tasks.Put(tasks_.Take().value());
  } while (!tasks_.IsEmpty());

  const size_t completed = RunTasks(processing_tasks);
  const size_t prev_queue_size = scheduled_.fetch_sub(completed);
  if (prev_queue_size > completed) {
    SubmitSelf();
  }
  ReleaseRef();
}

size_t Strand::Impl::RunTasks(TaskQueue& processing_tasks) {
  size_t count = 0;
  while (!processing_tasks.IsEmpty()) {
    processing_tasks.Take().value()();
    ++count;
  }
  return count;
}

void Strand::Impl::Ll(const char* format, ...) {
  if (!kShouldPrint) {
    return;
  }
  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  //sprintf(buf, "%s Strand::%s, qc: %lu\n", pid.str().c_str(), format, tasks_.Count());
  sprintf(buf, "%s Strand::%s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::executors
