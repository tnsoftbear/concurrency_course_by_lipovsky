#include <exe/executors/strand.hpp>

#include <mutex>
#include <twist/ed/wait/spin.hpp>

namespace exe::executors {

//const bool kShouldPrint = true;
 const bool kShouldPrint = false;

Strand::Strand(IExecutor& underlying)
  : underlying_(underlying) {
}

Strand::~Strand() {
  std::unique_lock<mutex> lock(*living_mtx_);
  living_cv_.wait(lock, [this] { return is_ended_.load(); });
  //printf("Strand::~Strand()\n");
}

void Strand::Submit(Task task) {
  tasks_.Put(std::move(task));
  if (is_running_.load()) {
    return;
  }

  auto decorated = [this]() mutable  {
    auto* self = this;
    exe::support::UnboundedBlockingQueue<Task> processing_tasks;
    self->lock_.Lock();
    while (!self->tasks_.IsEmpty()) {
      processing_tasks.Put(tasks_.Take().value());
    }
    self->lock_.Unlock();

    while (!processing_tasks.IsEmpty()) {
      processing_tasks.Take().value()();
    }

    self->is_running_.store(false);
    if (!self->tasks_.IsEmpty()) {
      self->Submit([]{});
    }

    {
      //std::lock_guard lk(*living_mtx_);
      is_ended_.store(true);
      living_cv_.notify_one();
    }
  };

  is_running_.store(true);
  underlying_.Submit(std::move(decorated));
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
