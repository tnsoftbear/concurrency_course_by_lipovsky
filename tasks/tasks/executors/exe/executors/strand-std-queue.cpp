#include <exe/executors/strand.hpp>

#include <mutex>
#include <twist/ed/wait/spin.hpp>

namespace exe::executors {

// const bool kShouldPrint = true;
 const bool kShouldPrint = false;

Strand::Strand(IExecutor& underlying)
  : underlying_(underlying) {
}

void Strand::Submit(Task task) {
  lock_.Lock();
  tasks_.push(std::move(task));
  lock_.Unlock();
  
  if (is_running_.load()) {
    return;
  }

  auto decorated = [this]() mutable  {
    std::queue<Task> processing_tasks{};
    
    lock_.Lock();
    while (!tasks_.empty()) {
      Task t = std::move(tasks_.front());
      processing_tasks.push(std::move(t));
      tasks_.pop();
    }
    lock_.Unlock();

    while (!processing_tasks.empty()) {
      std::move(processing_tasks.front())();
      processing_tasks.pop();
    }

    is_running_.store(false);

    lock_.Lock();
    if (!tasks_.empty()) {
      lock_.Unlock();
      Submit([]{});
    } else {
      lock_.Unlock();
    }
  };

  is_running_.store(true);
  underlying_.Submit(std::move(decorated));
}

void Strand::Ll(const char* format, ...) {
  if (!kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Strand::%s, qc: %lu\n", pid.str().c_str(), format, tasks_.size());
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace exe::executors
