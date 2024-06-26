#include <exe/tp/thread_pool.hpp>
#include <exe/tp/submit.hpp>

#include <course/twist/test.hpp>

#include <twist/test/race.hpp>
#include <twist/test/budget.hpp>
#include <twist/test/repeat.hpp>
#include <twist/test/yield.hpp>

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/stdlike/thread.hpp>

#include <atomic>

using namespace exe;

////////////////////////////////////////////////////////////////////////////////

namespace submit {

void KeepAlive() {
  if (twist::test::KeepRunning()) {
    tp::Submit(*tp::ThreadPool::Current(), [] {
      KeepAlive();
    });
  }
}

void StressTest(size_t threads, size_t clients, size_t limit) {
  tp::ThreadPool pool{threads};

  pool.Start();

  tp::Submit(pool, [] {
    KeepAlive();
  });

  std::atomic<size_t> completed{0};

  twist::ed::stdlike::atomic<size_t> queue{0};

  twist::test::Race race;

  for (size_t i = 0; i < clients; ++i) {
    race.Add([&]() {
      while (twist::test::KeepRunning()) {
        // TrySubmit
        if (++queue <= limit) {
          tp::Submit(pool, [&] {
            --queue;
            ++completed;
          });
        } else {
          --queue;
          twist::test::Yield();
        }
      }
    });
  }

  race.Run();

  pool.WaitIdle();
  pool.Stop();

  std::cout << "Tasks completed: " << completed.load() << std::endl;

  ASSERT_EQ(queue.load(), 0);
  ASSERT_GT(completed.load(), 8888);
}

}  // namespace submit

TWIST_TEST_TEMPLATE(Submit, submit::StressTest)
  ->TimeLimit(4s)
  ->Run(3, 5, 111)
  ->Run(4, 3, 13)
  ->Run(2, 4, 5)
  ->Run(9, 10, 33);

////////////////////////////////////////////////////////////////////////////////

namespace wait_idle {

void TestOneTask() {
  tp::ThreadPool pool{4};

  pool.Start();

  for (twist::test::Repeat repeat; repeat.Test(); ) {
    size_t completed = 0;

    tp::Submit(pool, [&] {
      ++completed;
    });

    pool.WaitIdle();

    ASSERT_EQ(completed, 1);
  }

  pool.Stop();
}

void TestSeries() {
  tp::ThreadPool pool{1};

  pool.Start();

  for (twist::test::Repeat repeat; repeat.Test(); ) {
    const size_t tasks = 1 + repeat.Iter() % 3;

    size_t completed = 0;
    for (size_t i = 0; i < tasks; ++i) {
      tp::Submit(pool, [&] {
        ++completed;
      });
    }

    pool.WaitIdle();

    ASSERT_EQ(completed, tasks);
  }

  pool.Stop();
}

void TestCurrent() {
  tp::ThreadPool pool{2};

  pool.Start();

  for (twist::test::Repeat repeat; repeat.Test(); ) {
    bool done = false;

    tp::Submit(pool, [&] {
      tp::Submit(*tp::ThreadPool::Current(), [&] {
        done = true;
      });
    });

    pool.WaitIdle();

    ASSERT_TRUE(done);
  }

  pool.Stop();
}

void TestConcurrent() {
  tp::ThreadPool pool{2};

  pool.Start();

  std::atomic<size_t> completed = 0;

  twist::ed::stdlike::thread t1([&] {
    tp::Submit(pool, [&] {
      ++completed;
    });
  });

  twist::ed::stdlike::thread t2([&] {
    pool.WaitIdle();
  });

  t1.join();
  t2.join();

  ASSERT_TRUE(completed.load() <= 1);

  pool.Stop();
}

}  // namespace wait_idle

TEST_SUITE(WaitIdle) {
  TWIST_TEST(OneTask, 5s) {
    wait_idle::TestOneTask();
  }

  TWIST_TEST(Series, 5s) {
    wait_idle::TestSeries();
  }

  TWIST_TEST(Current, 5s) {
    wait_idle::TestCurrent();
  }

  TWIST_TEST_REPEAT(Concurrent, 5s) {
    wait_idle::TestConcurrent();
  }
}

RUN_ALL_TESTS()
