#include "scheduler.hpp"
#include <twist/ed/local/var.hpp>
#include <asio/post.hpp>

namespace exe::fibers {

static twist::ed::ThreadLocalPtr<Scheduler> current_pool;

Scheduler& GetCurrent() {
  return *current_pool;
}

void SetCurrent(Scheduler& scheduler) {
    current_pool = &scheduler;
}

template <typename F>
void Submit(Scheduler& scheduler, F&& fun) {
  asio::post(scheduler, std::forward<F>(fun));
}

}
