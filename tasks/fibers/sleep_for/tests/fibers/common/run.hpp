#pragma once

#include <exe/fibers/sched/go.hpp>

#include <asio/io_context.hpp>

#include <vector>
#include <thread>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>
#include "exe/fibers/core/scheduler.hpp"

const bool kShouldPrint = true;

void Ll(const char* format, ...) {
  if (!kShouldPrint) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Fiber::%s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

template <typename F>
void RunScheduler(size_t threads, F init) {
  asio::io_context scheduler;

  bool done = false;

  // Spawn initial fiber
  exe::fibers::Go(scheduler, [init, &done]() {
    init();
    done = true;
  });

  std::vector<std::thread> workers;

  for (size_t i = 0; i < threads - 1; ++i) {
    workers.emplace_back([&scheduler] {
      exe::fibers::SetCurrent(scheduler); // TODO: avoid modification
      scheduler.run();
    });
  }

  //exe::fibers::SetCurrent(scheduler);
  scheduler.run();

  // Join runners
  for (auto& t : workers) {
    t.join();
  }

  ASSERT_TRUE(done);
}
