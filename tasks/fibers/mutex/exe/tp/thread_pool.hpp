#pragma once

#include <exe/tp/task.hpp>

#include <exe/support/queue.hpp>
#include <exe/support/wait_group.hpp>
#include <twist/ed/stdlike/thread.hpp>

namespace exe::tp {

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

// Fixed-size pool of worker threads

class ThreadPool {
 public:
  explicit ThreadPool(size_t threads);
  ~ThreadPool();

  // Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // Non-movable
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  // Launches worker threads
  void Start();

  // Schedules task for execution in one of the worker threads
  void Submit(Task);

  // Locates current thread pool from worker thread
  static ThreadPool* Current();

  // Waits until outstanding work count reaches zero
  void WaitIdle();

  // Stops the worker threads as soon as possible
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
  exe::support::WaitGroup incomplete_task_wg_;
  atomic<size_t> worker_routine_wait_counter_{0};
  atomic<size_t> completed_tasks_{0};
  atomic<size_t> submitted_tasks_{0};
  atomic<size_t> processing_tasks_{0};
};

}  // namespace exe::tp
