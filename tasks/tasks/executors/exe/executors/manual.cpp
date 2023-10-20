#include <exe/executors/manual.hpp>

namespace exe::executors {

void ManualExecutor::Submit(Task task) {
  tasks_.Put(std::move(task));
}

// Run tasks

size_t ManualExecutor::RunAtMost(size_t limit) {
  for (size_t i = 0; i < limit; i++) {
    if (tasks_.IsEmpty()) {
      return i;
    }

    std::optional<Task> opt_task = tasks_.Take();
    Task task = std::move(opt_task.value());
    task();
  }
  return limit;
}

size_t ManualExecutor::Drain() {
  size_t i = 0;
  while (true) {
    if (tasks_.IsEmpty()) {
      return i;
    }

    std::optional<Task> opt_task = tasks_.Take();
    Task task = std::move(opt_task.value());
    task();
    ++i;
  }
}

}  // namespace exe::executors
