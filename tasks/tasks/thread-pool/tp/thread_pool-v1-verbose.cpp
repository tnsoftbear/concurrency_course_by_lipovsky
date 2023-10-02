#include <mutex>
#include <tp/thread_pool.hpp>

#include <twist/ed/local/ptr.hpp>

#include <wheels/core/panic.hpp>
#include "tp/queue.hpp"

//void Ll(const char* format, ...);

namespace tp {

const bool kShouldPrintTp = true;
//const bool kShouldPrintTp = false;

static ThreadPool* ctp;
// static thread::id ctp_tid;
static std::string ctp_tid;

//typedef unsigned long int pthread_t;

ThreadPool::ThreadPool(size_t thread_total)
 : thread_total_(thread_total)
 , is_running_(false)
 , worker_routine_wait_counter_(0)
 , completed_tasks_(0)
 , submitted_tasks_(0)
{
  Ll("Construct: threads %lu", thread_total);
  ctp = this;
  ctp_tid = ThreadPool::DetectPid();
  workers_.reserve(thread_total_);
}

// Пул должен быть запущен с помощью явного вызова метода Start
void ThreadPool::Start() {
  Ll("Start: enter");
  if (thread_total_ == 0) {
    return;
  }

  for(size_t i=0; i < thread_total_; ++i) {
    workers_.emplace_back([this]() {
      WorkerRoutine();
    });
  }
}

ThreadPool::~ThreadPool() {
  assert(tasks_.IsEmpty());
  assert(tasks_.IsClosed());
  Ll("Destructed");
}

// Запланировать задачу на исполнение в пуле
// Вызов Submit не дожидается завершения задачи, он лишь добавляет ее в очередь задач пула, после чего возвращает управление.
// Метод Submit можно вызывать из разных потоков, без внешней синхронизации.
void ThreadPool::Submit(Task task) {
  Ll("Submit: enter");
  {
    // clippy target tp_stress_tests FaultyFibers --suite WaitIdle --test Series
    std::unique_lock<mutex> lock(worker_routine_mtx_);
  incomplete_task_wg_.Add(1);
  tasks_.Put(std::move(task));
  is_running_.store(true);
  submitted_tasks_.fetch_add(1);
    worker_routine_cv_.notify_one();
  }
  Ll("Submit: end");
}

void ThreadPool::WorkerRoutine() {
  Ll("WorkerRoutine: enter");
  while (true) {
    Ll("WorkerRoutine: before tasks_.Take()");
    std::optional<Task> opt_task = tasks_.Take();
    Ll("WorkerRoutine: before opt_task.has_value(): %d", (int)opt_task.has_value());
    if (opt_task.has_value()) {
      Ll("WorkerRoutine: before opt_task.value()();");
      Task task = std::move(opt_task.value());
      task();
      Ll("WorkerRoutine: after task()");
      incomplete_task_wg_.Done();
      completed_tasks_.fetch_add(1);
      Ll("WorkerRoutine: wg_.Done()");
    } else if (!is_running_.load()) {
      break;
    }

    {
      std::unique_lock<mutex> lock(worker_routine_mtx_);
      worker_routine_wait_counter_.fetch_add(1);
      worker_routine_cv_.wait(lock, [&]() { return !tasks_.IsEmpty() || !is_running_.load(); });
      worker_routine_wait_counter_.fetch_sub(1);
    }
  }
}

// Метод Current возвращает
// указатель на текущий пул, если его вызвали из потока-воркера, и
// nullptr в противном случае.
ThreadPool* ThreadPool::Current() {
  auto this_thread_id = DetectPid();
  return this_thread_id == ctp_tid ? nullptr : ctp;
}

// Метод WaitIdle блокирует вызвавший его поток до тех пор, пока в пуле не закончатся задачи.
// Метод WaitIdle можно вызвать несколько раз, а можно не вызывать ни разу.
// Метод не изменяет состояние пула, он только наблюдает за ним.
void ThreadPool::WaitIdle() {
  Ll("WaitIdle: enter");
  incomplete_task_wg_.Wait();
  Ll("WaitIdle: after wait");
}

// Пул должен быть остановлен до своего разрушения явно с помощью метода Stop.
// Вызов Stop возвращает управление, когда все потоки пула остановлены.
// Вызвать метод Stop можно только один раз.
// Вызов Stop для пула означает, что новые задачи в него планироваться больше не будут.
void ThreadPool::Stop() {
  Ll("Stop: enter");

  is_running_.store(false);
  tasks_.Close();
  {
    // Важный лок, без него мы пошлём сигнал нотификации между проверкой на уснуть и wait,
    // таким образом нотификация будет пропущена и поток уснёт навсегда.
    std::lock_guard<mutex> lock(worker_routine_mtx_);
    worker_routine_cv_.notify_all();
  }

  size_t i = 0;
  for (auto& worker : workers_) {
    i++;
    Ll("Stop: before worker.join, i: %lu", i);
    worker.join();
    Ll("Stop: after worker.join, i: %lu", i);
  }
  workers_.clear();
  ctp = nullptr;
  ctp_tid = "";
  Ll("Stop: ended");
}

std::string ThreadPool::DetectPid() {
  // Хз, какая-то странная ошибка при переопределении FaultyFibers
  // return twist::ed::stdlike::this_thread::get_id()
  std::ostringstream pid;
  pid << twist::ed::stdlike::this_thread::get_id();
  return pid.str().c_str();
}

void ThreadPool::Ll(const char* format, ...) {
  if (!kShouldPrintTp) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s %s, run: %d, qc: %lu, wg: %d, waits: %lu, submit: %lu, complete: %lu, tp id: %s\n", pid.str().c_str(), format, (int)is_running_.load(), tasks_.Count(), incomplete_task_wg_.GetCounter(), worker_routine_wait_counter_.load(), submitted_tasks_.load(), completed_tasks_.load(), tp::ctp_tid.c_str());
  // sprintf(buf, "%s %s, run: %d, qc: %lu, wg: %d\n", pid.str().c_str(), format, (int)is_running_.load(), tasks_.Count(), incomplete_task_wg_.GetCounter());
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace tp

