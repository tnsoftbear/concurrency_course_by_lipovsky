#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/core/fiber.hpp>

namespace exe::fibers {

void Ll(const char* format, ...) {
  if (!exe::tp::kShouldPrint) {
    return;
  }

  char buf[250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s %s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

void Go(Scheduler& scheduler, Routine routine) {
  Ll("fibers::Go-2: Start, scheduler: %p", &scheduler);
  fibers::Fiber* fiber = new fibers::Fiber(scheduler, std::move(routine));
  Ll("fibers::Go-2: before Submit");
  fiber->Schedule();
  Ll("fibers::Go-2: End");
}

void Go(Routine routine) {
  Ll("fibers::Go-1: Start");
  Go(*Scheduler::Current(), std::move(routine));
  Ll("fibers::Go-1: End");
}

}  // namespace exe::fibers
