#include <course/twist/test.hpp>

#include <twist/test/plate.hpp>
#include <twist/test/repeat.hpp>

#include <exe/executors/thread_pool.hpp>
#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/sync/mutex.hpp>

#include <atomic>
#include <chrono>

using namespace exe;
using namespace std::chrono_literals;

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

//////////////////////////////////////////////////////////////////////

void StressTest1(size_t fibers) {
  executors::ThreadPool scheduler{4};
  scheduler.Start();

  fibers::Mutex mutex;
  twist::test::Plate plate;

  for (size_t i = 0; i < fibers; ++i) {
    fibers::Go(scheduler, [&] {
      for (twist::test::TimeBudget budget; budget.Withdraw(); ) {
        mutex.Lock();
        plate.Access();
        mutex.Unlock();
      }
    });
  }

  scheduler.WaitIdle();

  std::cout << "# critical sections: " << plate.AccessCount() << std::endl;

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

void StressTest2() {
  executors::ThreadPool scheduler{4};
  scheduler.Start();

  for (twist::test::Repeat repeat; repeat.Test(); ) {
    size_t fibers = 2 + repeat.Iter() % 5;

    fibers::Mutex mutex;
    std::atomic<size_t> cs{0};

    for (size_t j = 0; j < fibers; ++j) {
      fibers::Go(scheduler, [&] {
        //printf("cs: %lu, fibers: %lu\n", cs.load(), fibers);
        // auto id = exe::fibers::Fiber::Self()->GetId();
        // if (id % 1000 == 0) {
        //   Ll("Routine: before mutex.Lock(); id: %lu", id);
        // }
        mutex.Lock();
        //Ll("Routine: before ++cs; cs: %lu, id: %lu", cs.load(), id);
        ++cs;
        //Ll("Routine: before mutex.Unock(); cs: %lu, id: %lu", cs.load(), id);
        mutex.Unlock();
      });
    }

    scheduler.WaitIdle();

    //Ll("cs: %lu, fibers: %lu", cs.load(), fibers);
    ASSERT_EQ(cs, fibers);
  }

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST_SUITE(Mutex) {
  TWIST_TEST(Stress_1_4, 5s) {
    StressTest1(/*fibers=*/4);
  }

  TWIST_TEST(Stress_1_16, 5s) {
    StressTest1(/*fibers=*/16);
  }

  TWIST_TEST(Stress_2, 5s) {
    StressTest2();
  }
}

RUN_ALL_TESTS()
