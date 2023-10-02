#pragma once

#include <thread>
#include <tp/queue.hpp>
#include <tp/task.hpp>

#include <twist/ed/stdlike/thread.hpp>

#include "wait_group.hpp"

namespace tp {

using WorkersT = std::vector<twist::ed::stdlike::thread>;
using QueueT = UnboundedBlockingQueue<Task>;

// Fixed-size pool of worker threads

class ThreadPool {
 public:
  explicit ThreadPool(size_t thread_total);
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
  static std::string DetectPid();

 private:
  size_t thread_total_;
  QueueT tasks_;
  WorkersT workers_;
  twist::ed::stdlike::condition_variable worker_routine_cv_;
  twist::ed::stdlike::mutex worker_routine_mtx_;
  atomic<bool> is_running_{false}; // Если не атомик, то падает на clippy target tp_stress_tests FaultyThreadsTSan
  twist::ed::stdlike::mutex mtx_;
  WaitGroup incomplete_task_wg_;
  // atomic<size_t> worker_routine_wait_counter_{0};
  // atomic<size_t> completed_tasks_;
  // atomic<size_t> submitted_tasks_;
};

}  // namespace tp
