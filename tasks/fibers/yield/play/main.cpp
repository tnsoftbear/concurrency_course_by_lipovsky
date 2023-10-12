#include <exe/tp/thread_pool.hpp>

#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/sched/yield.hpp>

#include <fmt/core.h>

// clear && clippy target playground Debug

using namespace exe;

void Ll(const char* format, ...) {
  if (false) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s play::%s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

int main() {
  tp::ThreadPool scheduler{/*threads=*/1};
  scheduler.Start();

  for (size_t i = 0; i < 1; ++i) {
    fibers::Go(scheduler, [i] {
      for (size_t j = 0; j < 1; ++j) {
        Ll("play::Routine#: %lu, Before yield#: %lu", i, j);
        fibers::Yield();
        Ll("play::Routine#: %lu, After yield#: %lu", i, j);
      }
    });
  }

  scheduler.WaitIdle();
  scheduler.Stop();

  return 0;
}
