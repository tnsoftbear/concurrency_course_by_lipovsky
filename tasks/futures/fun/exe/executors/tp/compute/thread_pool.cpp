#include <exe/executors/tp/compute/thread_pool.hpp>

#include <twist/ed/local/ptr.hpp>

#include <wheels/core/panic.hpp>

namespace exe::executors::tp::compute {

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

static twist::ed::ThreadLocalPtr<ThreadPool> current_pool;

ThreadPool::ThreadPool(size_t thread_total)
    : thread_total_(thread_total),
      is_running_(false),
      worker_routine_wait_counter_(0),
      completed_tasks_(0),
      submitted_tasks_(0),
      processing_tasks_(0) {
  Ll("Construct: threads %lu", thread_total);
  workers_.reserve(thread_total_);
}

ThreadPool::~ThreadPool() {
  Ll("~ThreadPool() starts");
  assert(tasks_.IsEmpty());
  assert(tasks_.IsClosed());
  Ll("~ThreadPool() Destructed");
}

void ThreadPool::Start() {
  Ll("Start: enter");
  if (thread_total_ == 0) {
    return;
  }

  for (size_t i = 0; i < thread_total_; ++i) {
    workers_.emplace_back([this]() {
      WorkerRoutine();
    });
  }
}

void ThreadPool::Submit(Task task) {
  Ll("Submit: enter");
  incomplete_task_wg_.Add(1);
  tasks_.Put(std::move(task));
  is_running_.store(true);
  submitted_tasks_.fetch_add(1);
  {
    std::unique_lock<mutex> lock(worker_routine_mtx_);
    worker_routine_cv_.notify_one();
  }
  Ll("Submit: end");
}

void ThreadPool::WorkerRoutine() {
  Ll("WorkerRoutine: enter");
  current_pool = this;
  while (true) {
    Ll("WorkerRoutine: before tasks_.Take()");
    std::optional<Task> opt_task = tasks_.Take();
    Ll("WorkerRoutine: before opt_task.has_value(): %d",
       (int)opt_task.has_value());
    if (opt_task.has_value()) {
      processing_tasks_.fetch_add(1);
      Ll("WorkerRoutine: before opt_task.value()();");
      Task task = std::move(opt_task.value());
      task();
      processing_tasks_.fetch_sub(1);
      Ll("WorkerRoutine: after task()");
      incomplete_task_wg_.Done();
      completed_tasks_.fetch_add(1);
      Ll("WorkerRoutine: wg_.Done()");
    } else if (!is_running_.load()) {
      Ll("WorkerRoutine: break (is_running_ == false)");
      break;
    }

    {
      std::unique_lock<mutex> lock(worker_routine_mtx_);
      worker_routine_wait_counter_.fetch_add(1);
      Ll("WorkerRoutine: Before wait");
      worker_routine_cv_.wait(lock, [&]() {
        return !tasks_.IsEmpty() || !is_running_.load();
      });
      Ll("WorkerRoutine: After wait");
      worker_routine_wait_counter_.fetch_sub(1);
    }
  }
  Ll("WorkerRoutine: ends");
}

ThreadPool* ThreadPool::Current() {
  return current_pool;
}

void ThreadPool::WaitIdle() {
  Ll("WaitIdle: enter");
  incomplete_task_wg_.Wait();
  Ll("WaitIdle: after wait");
}

void ThreadPool::Stop() {
  Ll("Stop: enter");

  is_running_.store(false);
  tasks_.Close();
  {
    // Важный лок, без него мы пошлём сигнал нотификации между проверкой на
    // уснуть и wait, таким образом нотификация будет пропущена и поток уснёт
    // навсегда.
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
  current_pool = nullptr;
  Ll("Stop: ended");
}

void ThreadPool::Ll(const char* format, ...) {
  if (!kShouldPrint) {
    return;
  }

  char buf[250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf,
          "%s ThreadPool::%s, run: %d, qc: %lu, wg: %d, waits: %lu, submit: "
          "%lu, complete: %lu, process: %lu\n",
          pid.str().c_str(), format, (int)is_running_.load(), tasks_.Count(),
          incomplete_task_wg_.GetCounter(), worker_routine_wait_counter_.load(),
          submitted_tasks_.load(), completed_tasks_.load(),
          processing_tasks_.load());
  // sprintf(buf, "%s %s, run: %d, qc: %lu, wg: %d\n", pid.str().c_str(),
  // format, (int)is_running_.load(), tasks_.Count(),
  // incomplete_task_wg_.GetCounter());
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::executors::tp::compute
