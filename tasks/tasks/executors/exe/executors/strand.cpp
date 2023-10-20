#include <exe/executors/strand.hpp>

#include <twist/ed/wait/spin.hpp>

namespace exe::executors {

//const bool kShouldPrint = true;
 const bool kShouldPrint = false;

Strand::Strand(IExecutor& underlying)
  : underlying_(underlying) {
}

void Strand::Submit(Task task) {
  tasks_.Put(std::move(task));
  if (is_running_.load()) {
    return;
  }

  lock2_.Lock();

  auto decorated = [this]() mutable  {
    exe::support::UnboundedBlockingQueue<Task> processing_tasks;
    lock_.Lock();
    while (!tasks_.IsEmpty()) {
      processing_tasks.Put(tasks_.Take().value());
    }
    lock_.Unlock();

    while (!processing_tasks.IsEmpty()) {
      processing_tasks.Take().value()();
    }

    is_running_.store(false);
    if (!tasks_.IsEmpty()) {
      Submit([]{});
    }
  };

  is_running_.store(true);
  underlying_.Submit(std::move(decorated));

  lock2_.Unlock();
}

// void Strand::Submit(Task task) {
//   tasks_.Put(std::move(task));
//   Ll("After tasks_.Put()");
//   if (is_running_.load()) {
//     return;
//   }
//   is_running_.store(true);
//   auto decorated = [this]()  {
//     Ll("Lambda started");
//     exe::support::UnboundedBlockingQueue<Task> processing_tasks;
    
//     lock_.Lock();
//     while (!tasks_.IsEmpty()) {
//       processing_tasks.Put(tasks_.Take().value());
//     }
//     lock_.Unlock();

//     size_t i = 0;
//     while (!processing_tasks.IsEmpty()) {
//       std::optional<Task> opt_task = processing_tasks.Take();
//       i++;
//       Task task = std::move(opt_task.value());
//       task();
//       Ll("Task completed, i: %lu", i);
//     }
//     is_running_.store(false);
//     if (!tasks_.IsEmpty()) {
//       Submit([]{});
//     }
//   };
//   Ll("Before underlying_.Submit()");
//   underlying_.Submit(std::move(decorated));
// }


void Strand::Ll(const char* format, ...) {
  if (!kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Strand::%s, qc: %lu\n", pid.str().c_str(), format, tasks_.Count());
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::executors
