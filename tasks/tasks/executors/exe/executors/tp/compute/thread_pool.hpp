#pragma once

#include <exe/executors/executor.hpp>

#include <exe/support/queue.hpp>
#include <exe/support/wait_group.hpp>
#include <twist/ed/stdlike/thread.hpp>

namespace exe::executors::tp::compute {

// Thread pool for independent CPU-bound tasks
// Fixed pool of worker threads + shared unbounded blocking queue

class ThreadPool : public IExecutor {
 public:
  explicit ThreadPool(size_t threads);
  ~ThreadPool();

  // Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // Non-movable
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  void Start();

  // IExecutor
  void Submit(Task);

  static ThreadPool* Current();

  void WaitIdle();

  void Stop();

 private:
  void WorkerRoutine();
  void Ll(const char* format, ...);

 private:
  size_t thread_total_;
  exe::support::UnboundedBlockingQueue<Task> tasks_;
  std::vector<twist::ed::stdlike::thread> workers_;
  twist::ed::stdlike::condition_variable worker_routine_cv_;
  twist::ed::stdlike::mutex worker_routine_mtx_;
  atomic<bool> is_running_{false};  // Если не атомик, то падает на clippy
                                    // target tp_stress_tests FaultyThreadsTSan
  twist::ed::stdlike::mutex mtx_;
  WaitGroup incomplete_task_wg_;
  atomic<size_t> worker_routine_wait_counter_{0};
  atomic<size_t> completed_tasks_{0};
  atomic<size_t> submitted_tasks_{0};
  atomic<size_t> processing_tasks_{0};
};

}  // namespace exe::executors::tp::compute
