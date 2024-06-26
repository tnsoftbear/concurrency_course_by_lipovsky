#include <cstddef>
#include <exe/executors/thread_pool.hpp>
#include <exe/executors/strand.hpp>
#include <exe/executors/manual.hpp>
#include <exe/executors/submit.hpp>

#include <wheels/test/framework.hpp>
#include <wheels/core/stop_watch.hpp>

#include <thread>
#include <deque>
#include <atomic>

using namespace exe;
using namespace std::chrono_literals;

void AssertRunningOn(executors::ThreadPool& pool) {
  ASSERT_TRUE(executors::ThreadPool::Current() == &pool);
}

//const bool kShouldPrint = true;
 const bool kShouldPrint = false;

void Ll(const char* format, ...) {
  if (!kShouldPrint) {
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

TEST_SUITE(Strand) {
  SIMPLE_TEST(JustWorks) {
    executors::ThreadPool pool{4};
    pool.Start();

    executors::Strand strand{pool};

    bool done = false;

    executors::Submit(strand, [&] {
      done = true;
      Ll("Routine ends");
    });

    Ll("Before pool.WaitIdle();");
    pool.WaitIdle();

    ASSERT_TRUE(done);

    Ll("Before pool.Stop();");
    pool.Stop();
    Ll("Test ends");
  }

  SIMPLE_TEST(Decorator) {
    executors::ThreadPool pool{4};
    pool.Start();

    executors::Strand strand{pool};

    bool done{false};

    for (size_t i = 0; i < 128; ++i) {
      executors::Submit(strand, [&] {
        AssertRunningOn(pool);
        done = true;
      });
    }

    pool.WaitIdle();

    ASSERT_TRUE(done);

    pool.Stop();
  }

  SIMPLE_TEST(Counter) {
    executors::ThreadPool pool{13};
    pool.Start();

    size_t counter = 0;

    executors::Strand strand{pool};

    static const size_t kIncrements = 1234;

    for (size_t i = 0; i < kIncrements; ++i) {
      executors::Submit(strand, [&] {
        AssertRunningOn(pool);
        ++counter;
      });
    };

    pool.WaitIdle();

    printf("counter: %lu, kIncrements: %lu\n", counter, kIncrements);
    ASSERT_EQ(counter, kIncrements);

    pool.Stop();
  }

  SIMPLE_TEST(Fifo) {
    executors::ThreadPool pool{13};
    pool.Start();

    executors::Strand strand{pool};

    size_t done = 0;

    static const size_t kTasks = 12345;

    for (size_t i = 0; i < kTasks; ++i) {
      executors::Submit(strand, [&, i] {
        AssertRunningOn(pool);
        //Ll("Done: %lu, i: %lu", done, i);
        ASSERT_EQ(done++, i);
      });
    };

    pool.WaitIdle();

    //Ll("Done: %lu, kTasks: %lu", done, kTasks);
    ASSERT_EQ(done, kTasks);

    pool.Stop();
  }

  class Robot {
   public:
    explicit Robot(executors::IExecutor& executor)
        : strand_(executor) {
    }

    void Push() {
      executors::Submit(strand_, [this] {
        ++steps_;
      });
    }

    size_t Steps() const {
      return steps_;
    }

   private:
    executors::Strand strand_;
    size_t steps_{0};
  };

  SIMPLE_TEST(ConcurrentStrands) {
    executors::ThreadPool pool{16};
    pool.Start();

    static const size_t kStrands = 50;

    std::deque<Robot> robots;
    for (size_t i = 0; i < kStrands; ++i) {
      robots.emplace_back(pool);
    }

    static const size_t kPushes = 25;
    static const size_t kIterations = 25;

    for (size_t i = 0; i < kIterations; ++i) {
      for (size_t j = 0; j < kStrands; ++j) {
        for (size_t k = 0; k < kPushes; ++k) {
          //Ll("Iterations i: %lu, Strands j: %lu, Pushes k: %lu", i, j, k);
          robots[j].Push();
        }
      }
    }

    pool.WaitIdle();

    for (size_t i = 0; i < kStrands; ++i) {
      //Ll("i: %lu, Steps: %lu, kPushes * kIterations: %lu", i, robots[i].Steps(), kPushes * kIterations);
      ASSERT_EQ(robots[i].Steps(), kPushes * kIterations);
    }

    pool.Stop();
  }

  SIMPLE_TEST(ConcurrentSubmits) {
    executors::ThreadPool workers{2};
    executors::Strand strand{workers};

    executors::ThreadPool clients{4};

    workers.Start();
    clients.Start();

    static const size_t kTasks = 1024;

    size_t task_count{0};

    for (size_t i = 0; i < kTasks; ++i) {
      executors::Submit(clients, [&] {
        executors::Submit(strand, [&] {
          AssertRunningOn(workers);
          ++task_count;
        });
      });
    }

    clients.WaitIdle();
    workers.WaitIdle();

    ASSERT_EQ(task_count, kTasks);

    workers.Stop();
    clients.Stop();
  }

  SIMPLE_TEST(StrandOverManual) {
    executors::ManualExecutor manual;
    executors::Strand strand{manual};

    static const size_t kTasks = 117;

    size_t tasks = 0;

    for (size_t i = 0; i < kTasks; ++i) {
      executors::Submit(strand, [&] {
        ++tasks;
      });
    }

    manual.Drain();

    ASSERT_EQ(tasks, kTasks);
  }

  SIMPLE_TEST(Batching) {
    executors::ManualExecutor manual;
    executors::Strand strand{manual};

    static const size_t kTasks = 100;

    size_t completed = 0;
    for (size_t i = 0; i < kTasks; ++i) {
      executors::Submit(strand, [&completed] {
        ++completed;
      });
    };

    size_t tasks = manual.Drain();
    ASSERT_LT(tasks, 5);
  }

  SIMPLE_TEST(StrandOverStrand) {
    executors::ThreadPool pool{4};
    pool.Start();

    executors::Strand strand_1{pool};
    executors::Strand strand_2{(executors::IExecutor&)strand_1};

    static const size_t kTasks = 17;

    size_t tasks = 0;

    for (size_t i = 0; i < kTasks; ++i) {
      executors::Submit(strand_2, [&tasks] {
        ++tasks;
      });
    }

    pool.WaitIdle();

    ASSERT_EQ(tasks, kTasks);

    pool.Stop();
  }

  SIMPLE_TEST(DoNotOccupyThread) {
    executors::ThreadPool pool{1};
    pool.Start();

    executors::Strand strand{pool};

    executors::Submit(pool, [] {
      Ll("before sleep_for(1s)");
      std::this_thread::sleep_for(1s);
    });

    std::atomic<bool> stop_requested{false};

    // auto snooze = []() {
    //   Ll("before sleep_for(10ms)");
    //   std::this_thread::sleep_for(10ms);
    // };

    for (size_t i = 0; i < 100; ++i) {
      Ll("i: %lu", i);
      //executors::Submit(strand, snooze);
      Ll("Submitted snooze for i: %lu", i);
      executors::Submit(strand, [i]() {
        std::this_thread::sleep_for(10ms);
        Ll("Completed snooze for i: %lu", i);
      });
    }

    Ll("Submitted stop_requested.store(true)");
    executors::Submit(pool, [&stop_requested] {
      Ll("stop_requested.store(true)");
      stop_requested.store(true);
    });

    size_t j = 0;
    while (!stop_requested.load()) {
      j++;
      Ll("Submitted snooze for j: %lu", j);
      //executors::Submit(strand, snooze);
      executors::Submit(strand, [j]() {
        std::this_thread::sleep_for(10ms);
        Ll("Completed snooze for j: %lu", j);
      });
      std::this_thread::sleep_for(10ms);
    }

    pool.WaitIdle();
    pool.Stop();
  }

  SIMPLE_TEST(NonBlockingSubmit) {
    executors::ThreadPool pool{1};
    executors::Strand strand{pool};

    pool.Start();

    executors::Submit(strand, [&] {
      // Bubble
      std::this_thread::sleep_for(3s);
    });

    std::this_thread::sleep_for(256ms);

    {
      wheels::StopWatch stop_watch;
      executors::Submit(strand, [&] {
        // Do nothing
      });

      ASSERT_LE(stop_watch.Elapsed(), 100ms);
    }

    pool.WaitIdle();
    pool.Stop();
  }
}

RUN_ALL_TESTS()
