#include <exe/executors/strand.hpp>
#include <twist/ed/wait/spin.hpp>
#include <map>
// #include <twist/ed/stdlike/atomic.hpp>

// using twist::ed::stdlike::atomic;

/**
 * Алгоритм этого решения скопирован из await fw.
 * https://gitlab.com/Lipovsky/await/-/blob/master/await/tasks/exe/strand.cpp?ref_type=heads
 * Но он не помогает решить проблему, с которой я столкнулся в других решениях,
 * которая проявляется в тесте: clippy target strand_lifetime_tests FaultyFibersASan
 * После завершения задачи, объект странда удаляется из памяти, потому что в этом тесте был сделан automation.reset() 
 * См. /workspace/concurrency-course/tasks/tasks/executors/tests/executors/strand/lifetime.cpp
 * А т.к. this удалён, мы больше не можем обращаться к свойствам объекта и принимать по ним решения.
 * В данном случае речь идёт о счётчике scheduled, по которому мы решаем продолжать ли обслуживать очередь ожидания задач или нет.
 * Поэтому я вынес этот счётчик в статическую память в мапу. 
 * Кроме того, приходится использовать std :: atomic вместо twist/atomic, 
 * потому что я не знаю, как инициализировать атомики из твиста, когда они в мапе, чтобы это работало для FaultyFibers.
 * В итоге решение не выглядит ожидаемо верным. Надо убрать атомики и статические переменные.
 *
 * Подсказки:
 * RL: Если вы не используете exchange, вы пишете плохой стренд.
 * IK: Хотя какой exchange здесь может быть, если атомики запрещены?
 */

namespace exe::executors {

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

static std::map<size_t, std::atomic<size_t>> scheduled; // Не выходит сделать через twsit-атомик, не получается инициализировать значение при тесте FaultyFibers
static atomic<size_t> max_id{0};  // не атомиком тоже работает, наверно, нет подходящео теста

Strand::Strand(IExecutor& underlying)
  : underlying_(underlying) 
  , id_(max_id.fetch_add(1)) {
    scheduled[id_].store(0);
  }

Strand::~Strand() {
  scheduled.erase(id_);
}

void Strand::Submit(Task task) {
  tasks_.Put(std::move(task));
  if (scheduled[id_].fetch_add(1) == 0) {
    SubmitSelf();
  }
}

void Strand::SubmitSelf() {
  underlying_.Submit([this]()  {
    Run();
  });
}

void Strand::Run() {
  TaskQueue processing_tasks;
  do {
    processing_tasks.Put(tasks_.Take().value());
  } while (!tasks_.IsEmpty());

  size_t id = id_;
  const size_t completed = RunTasks(processing_tasks);
  const size_t prev_queue_size = scheduled[id].fetch_sub(completed);
  if (prev_queue_size > completed) {
    SubmitSelf();
  }
}

size_t Strand::RunTasks(TaskQueue& processing_tasks) {
  size_t count = 0;
  while (!processing_tasks.IsEmpty()) {
    processing_tasks.Take().value()();
    ++count;
  }
  return count;
}

void Strand::Ll(const char* format, ...) {
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
