#include <tp/thread_pool.hpp>

#include <twist/ed/local/ptr.hpp>

#include <wheels/core/panic.hpp>
#include "tp/queue.hpp"

namespace tp {

//const bool kShouldPrintTp = true;
const bool kShouldPrintTp = false;

static ThreadPool* ctp;
static std::string ctp_tid;

ThreadPool::ThreadPool(size_t thread_total)
 : thread_total_(thread_total)
 , is_running_(false)
{
  ctp = this;
  ctp_tid = ThreadPool::DetectPid();
}

// Пул должен быть запущен с помощью явного вызова метода Start
void ThreadPool::Start() {
  for(size_t i=0; i < thread_total_; ++i) {
    workers_.emplace_back([this]() {
      WorkerRoutine();
    });
  }
}

ThreadPool::~ThreadPool() {
  assert(tasks_.IsEmpty());
  assert(tasks_.IsClosed());
  ctp = nullptr;
  ctp_tid = "";
}

// Запланировать задачу на исполнение в пуле
// Вызов Submit не дожидается завершения задачи, он лишь добавляет ее в очередь задач пула, после чего возвращает управление.
// Метод Submit можно вызывать из разных потоков, без внешней синхронизации.
void ThreadPool::Submit(Task task) {
  incomplete_task_wg_.Add(1);
  tasks_.Put(std::move(task));
  is_running_.store(true);
  {
    // Без лока падает на: clippy target tp_stress_tests FaultyFibers --suite WaitIdle --test Series
    std::lock_guard guard(worker_routine_mtx_);
    worker_routine_cv_.notify_one();
  }
}

void ThreadPool::WorkerRoutine() {
  while (true) {
    std::optional<Task> opt_task = tasks_.Take();
    if (opt_task.has_value()) {
      Task task = std::move(opt_task.value());
      task();
      incomplete_task_wg_.Done();
    } else if (!is_running_.load()) {
      break;
    }

    {
      std::unique_lock<mutex> lock(worker_routine_mtx_);
      worker_routine_cv_.wait(lock, [&]() { 
        return !tasks_.IsEmpty() || !is_running_.load(); 
      });
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
  incomplete_task_wg_.Wait();
}

// Пул должен быть остановлен до своего разрушения явно с помощью метода Stop.
// Вызов Stop возвращает управление, когда все потоки пула остановлены.
// Вызвать метод Stop можно только один раз.
// Вызов Stop для пула означает, что новые задачи в него планироваться больше не будут.
void ThreadPool::Stop() {
  is_running_.store(false);
  tasks_.Close();
  {
    // Важный лок, без него мы пошлём сигнал нотификации между проверкой на уснуть и wait,
    // таким образом нотификация будет пропущена и поток уснёт навсегда.
    std::lock_guard<mutex> lock(worker_routine_mtx_);
    worker_routine_cv_.notify_all();
  }

  for (auto& worker : workers_) {
    worker.join();
  }
  workers_.clear();
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
  sprintf(buf, "%s %s, run: %d, qc: %lu, wg: %d\n", pid.str().c_str(), format, (int)is_running_.load(), tasks_.Count(), incomplete_task_wg_.GetCounter());
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace tp

