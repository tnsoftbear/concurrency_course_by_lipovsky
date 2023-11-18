#include <wheels/test/framework.hpp>

#include <exe/executors/thread_pool.hpp>

#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/sched/yield.hpp>
#include <exe/fibers/sync/mutex.hpp>

#include <wheels/test/util/cpu_timer.hpp>

#include <atomic>
#include <chrono>
#include <thread>

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

TEST_SUITE(Mutex) {
  SIMPLE_TEST(JustWorks) {
    executors::ThreadPool scheduler{4};
    scheduler.Start();

    fibers::Mutex mutex;
    size_t cs = 0;

    fibers::Go(scheduler, [&] {
      for (size_t j = 0; j < 11; ++j) {
        std::lock_guard guard(mutex);
        ++cs;
      }
    });

    scheduler.WaitIdle();

    Ll("cs: %lu", cs);
    ASSERT_EQ(cs, 11);

    scheduler.Stop();
  }

  SIMPLE_TEST(Counter) {
    executors::ThreadPool scheduler{4};
    scheduler.Start();

    fibers::Mutex mutex;
    size_t cs = 0;
    //atomic<size_t> acs{0};

    static const size_t kFibers = 10;
    static const size_t kSectionsPerFiber = 1024;

    for (size_t i = 0; i < kFibers; ++i) {
      fibers::Go(scheduler, [&] {
        for (size_t j = 0; j < kSectionsPerFiber; ++j) {
          auto id = exe::fibers::Fiber::Self()->GetId();
          Ll("Routine: std::lock_guard guard(mutex);, id: %d", id);
          std::lock_guard guard(mutex);
          Ll("Routine: Before ++cs, id: %d", id);
          ++cs;
          // ++acs;
          // if (cs != acs) {
          //   Ll("Routine: cs != acs !!!!!!!!!!!!!!!!, id: %d", id);
          //   ASSERT_EQ(cs, acs.load());
          //   cs = acs.load();
          // }
          Ll("Routine: For iteration ends, j: %lu, id: %d", j, id);
        }
      });
    }

    scheduler.WaitIdle();

    std::cout << "# cs = " << cs
              << " (expected = " << kFibers * kSectionsPerFiber << ")"
              << std::endl;

    ASSERT_EQ(cs, kFibers * kSectionsPerFiber);

    scheduler.Stop();
  }

  SIMPLE_TEST(DoNotWasteCpu) {
    executors::ThreadPool scheduler{4};
    scheduler.Start();

    fibers::Mutex mutex;

    wheels::ProcessCPUTimer cpu_timer;

    fibers::Go(scheduler, [&] {
      auto id = exe::fibers::Fiber::Self()->GetId();
      mutex.Lock();
      Ll("Sleeping-Routine: before std::this_thread::sleep_for(1s);, id: %lu", id);
      std::this_thread::sleep_for(1s);
      Ll("Sleeping-Routine: before mutex.Unlock();, id: %lu", id);
      mutex.Unlock();
    });

    fibers::Go(scheduler, [&] {
      auto id = exe::fibers::Fiber::Self()->GetId();
      Ll("Non-Sleeping-Routine: before mutex.Lock();, id: %lu", id);
      mutex.Lock();
      Ll("Non-Sleeping-Routine: before mutex.Unlock();, id: %lu", id);
      mutex.Unlock();
    });

    scheduler.WaitIdle();

    ASSERT_TRUE(cpu_timer.Spent() < 100ms);

    scheduler.Stop();
  }
}

RUN_ALL_TESTS()
