#include <course/twist/test.hpp>

#include <twist/test/repeat.hpp>
#include <twist/test/random.hpp>

#include <exe/executors/thread_pool.hpp>
#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/sync/wait_group.hpp>

#include <fmt/core.h>

#include <array>
#include <atomic>
#include <chrono>

using namespace exe;
using namespace std::chrono_literals;

//////////////////////////////////////////////////////////////////////

void Ll(const char* format, ...) {
  //const bool k_should_print = true;
  const bool k_should_print = false;
  if (!k_should_print) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Test::%s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

void StressTest() {
  executors::ThreadPool scheduler{/*threads=*/4};
  scheduler.Start();

  twist::test::Repeat repeat;

  while (repeat()) {
    size_t workers = twist::test::Random(1, 4);
    size_t waiters = twist::test::Random(1, 4);
    Ll("Test: workers: %lu, waiters: %lu", workers, waiters);

    std::array<bool, 5> work;
    work.fill(false);

    std::atomic<size_t> acks{0};

    fibers::WaitGroup wg;
    wg.Add(workers);

    // Waiters

    for (size_t i = 0; i < waiters; ++i) {
      fibers::Go(scheduler, [&] {
        wg.Wait();
        for (size_t j = 0; j < workers; ++j) {
          ASSERT_TRUE(work[j]);
        }
        acks.fetch_add(1);
      });
    }

    // Workers

    for (size_t j = 0; j < workers; ++j) {
      fibers::Go(scheduler, [&, j] {
        work[j] = true;
        wg.Done();
      });
    }

    scheduler.WaitIdle();

    Ll("Result: acks: %lu, waiters: %lu", acks.load(), waiters);
    ASSERT_EQ(acks.load(), waiters);
  }

  fmt::println("Iterations: {}", repeat.IterCount());

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST_SUITE(WaitGroup) {
  TWIST_TEST(Stress, 5s) {
    StressTest();
  }
}

RUN_ALL_TESTS()
