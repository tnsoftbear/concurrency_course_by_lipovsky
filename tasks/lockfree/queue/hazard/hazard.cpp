#include <memory>

#include "thread_state.hpp"
#include "manager.hpp"

namespace hazard {

//const bool kShouldPrint = true;

Manager* Manager::Get() {
  static Manager instance;
  return &instance;
}

std::shared_ptr<Mutator> Manager::MakeMutator() {
  std::string pid = Manager::GetPid();
  if (mutators_.count(pid) == 0) {
    mutators_[pid] = std::make_shared<Mutator>(this);
  }
  return mutators_[pid];
}

void Manager::Collect() {
  Ll("Collect: starts");
  GetThreadState().DeleteForAllThreads();
  mutators_.clear();
}

ThreadState& Manager::GetThreadState() {
  return Mutator::GetThreadState();
  //return thread_state;
}

void Manager::Ll(const char* format, ...) {
  if (!kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  //sprintf(buf, "%s Strand::%s, qc: %lu\n", pid.str().c_str(), format, tasks_.Count());
  sprintf(buf, "%s Manager::%s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

}  // namespace hazard
