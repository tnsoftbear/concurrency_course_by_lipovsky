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
    const bool k_should_print = true;
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

    // static const size_t kFibers = 10;
    // static const size_t kSectionsPerFiber = 1024;
    static const size_t kFibers = 10;
    static const size_t kSectionsPerFiber = 1024;

    for (size_t i = 0; i < kFibers; ++i) {
      fibers::Go(scheduler, [&] {
        for (size_t j = 0; j < kSectionsPerFiber; ++j) {
          Ll("Before std::lock_guard guard(mutex);");
          std::lock_guard guard(mutex);
          Ll("Before ++cs;");
          ++cs;
          Ll("For iteration ends, j: %lu", j);
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
      mutex.Lock();
      std::this_thread::sleep_for(1s);
      mutex.Unlock();
    });

    fibers::Go(scheduler, [&] {
      mutex.Lock();
      mutex.Unlock();
    });

    scheduler.WaitIdle();

    ASSERT_TRUE(cpu_timer.Spent() < 100ms);

    scheduler.Stop();
  }
}

RUN_ALL_TESTS()
