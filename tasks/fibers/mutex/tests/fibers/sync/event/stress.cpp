#include <course/twist/test.hpp>

#include <exe/executors/thread_pool.hpp>

#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/sched/yield.hpp>

#include <exe/fibers/sync/event.hpp>

#include <twist/test/repeat.hpp>

using namespace exe;

  void Ll(const char* format, ...) {
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

//////////////////////////////////////////////////////////////////////

void StressTest() {
  executors::ThreadPool scheduler{5};
  scheduler.Start();

  for (twist::test::Repeat repeat; repeat.Test(); ) {
    size_t waiters = 1 + repeat.Iter() % 4;

    fibers::Event event;
    bool work = false;
    std::atomic<size_t> acks{0};

    for (size_t i = 0; i < waiters; ++i) {
      fibers::Go(scheduler, [&] {
        auto fiber = exe::fibers::Fiber::Self();
        fiber->name = "Waiter #" + std::to_string(fiber->GetId());
        Ll("Wait-Routine: starts, id: %lu, fiber: %p, name: %s", fiber->GetId(), fiber, fiber->name.c_str());
        event.Wait();
        ASSERT_TRUE(work);
        ++acks;
        Ll("Wait-Routine: ends, id: %lu, fiber: %p, name: %s", fiber->GetId(), fiber, fiber->name.c_str());
      });
    }

    fibers::Go(scheduler, [&] {
      auto fiber = exe::fibers::Fiber::Self();
      fiber->name = "Firer #" + std::to_string(fiber->GetId());
      Ll("Fire-Routine: id: %lu, name: %s", fiber->GetId(), fiber->name.c_str());
      work = true;
      event.Fire();
    });

    scheduler.WaitIdle();

    Ll("acks.load(): %lu, waiters: %lu", acks.load(), waiters);
    ASSERT_EQ(acks.load(), waiters);
  }

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST_SUITE(Event) {
  TWIST_TEST(Event, 5s) {
    StressTest();
  }
}

RUN_ALL_TESTS();
