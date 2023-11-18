#include <course/twist/test.hpp>

#include <exe/executors/thread_pool.hpp>

#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/sync/wait_group.hpp>

#include <twist/test/repeat.hpp>

using namespace exe;

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

void StorageTest() {
  executors::ThreadPool scheduler{5};
  scheduler.Start();

  for (twist::test::Repeat repeat; repeat.Test(); ) {
    fibers::Go(scheduler, [] {
      auto* wg = new fibers::WaitGroup{};

      wg->Add(1);
      fibers::Go([wg] {
        wg->Done();
      });

      wg->Wait();
      Ll("Wait-Routine: before delete wg;, id: %lu", fibers::Fiber::Self()->GetId());
      delete wg;
      Ll("Wait-Routine: ends");
    });

    scheduler.WaitIdle();
  }

  scheduler.Stop();
}

TEST_SUITE(WaitGroup) {
  TWIST_TEST(Storage, 5s) {
    StorageTest();
  }
}

RUN_ALL_TESTS();
