#include "../../lock_free_stack.hpp"

#include <course/twist/test.hpp>

#include <twist/test/budget.hpp>
#include <twist/test/lockfree.hpp>
#include <twist/test/race.hpp>
#include <twist/test/random.hpp>

#include <iostream>
#include <limits>

//////////////////////////////////////////////////////////////////////

void Ll(const char* format, ...) {
  bool k_should_print = true;
  if (!k_should_print) {
    return;
  }

  char buf [250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  //sprintf(buf, "%s Strand::%s, qc: %lu\n", pid.str().c_str(), format, tasks_.Count());
  sprintf(buf, "%s StressTest::%s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

template <typename T>
struct Counted {
  static std::atomic<size_t> count;
  static size_t limit;

  Counted() {
    IncrementCount();
  }

  Counted(const Counted&) {
    IncrementCount();
  }

  Counted(Counted&&) {
    IncrementCount();
  }

  Counted& operator=(const Counted&) {
    // No-op
  }

  Counted& operator=(Counted&&) {
    // No-op
  }

  ~Counted() {
    DecrementCount();
  }

  static void SetLimit(size_t value) {
    limit = value;
  }

  static size_t ObjectCount() {
    return count.load();
  }

 private:
  static void IncrementCount() {
    ASSERT_TRUE_M(
        count.fetch_add(1) + 1 < limit,
        "Too many alive test objects");
  }

  static void DecrementCount() {
    count.fetch_sub(1);
  }
};

template <typename T>
std::atomic<size_t> Counted<T>::count = 0;

template <typename T>
size_t Counted<T>::limit = std::numeric_limits<size_t>::max();

//////////////////////////////////////////////////////////////////////

struct Widget: public Counted<Widget> {
  size_t data;

  explicit Widget(size_t d)
      : data(d) {
  }
};

//////////////////////////////////////////////////////////////////////

void StressTest(size_t threads, size_t batch_size_limit) {
  twist::test::SetLockFreeAdversary();

  {
    twist::test::ReportProgressFor<LockFreeStack<Widget>> stack;

    std::atomic<size_t> ops{0};
    std::atomic<size_t> pushed{0};
    std::atomic<size_t> popped{0};

    Widget::SetLimit(1024);

    twist::test::Race race;

    for (size_t i = 0; i < threads; ++i) {
      race.Add([&] {
        twist::test::EnablePark lf_here;

        for (twist::test::TimeBudget budget; budget; ) {
          size_t batch_size = twist::test::Random(1, batch_size_limit + 1);

          // Push

          for (size_t j = 0; j < batch_size; ++j) {
            size_t value = twist::test::Random(1000007);
            Widget w{value};

            pushed.fetch_add(w.data);
            //pushed.fetch_add(1);
            stack->Push(std::move(w));
            ++ops;
            // Ll("Pushed, j: %lu, value: %lu, ops: %lu, bs: %lu", j, value, ops.load(), batch_size);
          }

          // Pop

          for (size_t j = 0; j < batch_size; ++j) {
            auto w = stack->TryPop();
            // if (w.has_value()) {
            //   Ll("Popped, j: %lu, value: %lu, ops: %lu, bs: %lu", j, w->data, ops.load(), batch_size);
            // } else {
            //   Ll("Popped, j: %lu, no value, ops: %lu", j, ops.load());
            // }
            ASSERT_TRUE(w);
            popped.fetch_add(w->data);
            //popped.fetch_add(1);
          }
        }
      });
    }

    race.Run();

    hazard::Manager::Get()->Collect();

    std::cout << "Operations: " << ops.load() << std::endl;
    std::cout << "Pushed: " << pushed.load() << std::endl;
    std::cout << "Popped: " << popped.load() << std::endl;

    ASSERT_EQ(pushed.load(), popped.load());

    // Stack is empty
    ASSERT_FALSE(stack->TryPop());
  }

  {
    size_t count = Widget::ObjectCount();
    std::cout << "Widgets: " << count << std::endl;
    // Memory leak
    ASSERT_EQ(count, 0);
  }
}

//////////////////////////////////////////////////////////////////////

TEST_SUITE(LockFreeStack) {
  TWIST_TEST(Stress0, 1s) {
    StressTest(/*threads=*/2, /*batch_size_limit=*/2);
  }

  TWIST_TEST(Stress1, 5s) {
    StressTest(/*threads=*/2, /*batch_size_limit=*/2);
  }

  TWIST_TEST(Stress2, 5s) {
    StressTest(/*threads=*/5, /*batch_size_limit=*/1);
  }

  TWIST_TEST(Stress3, 5s) {
    StressTest(/*threads=*/5, /*batch_size_limit=*/3);
  }

  TWIST_TEST(Stress4, 5s) {
    StressTest(/*threads=*/5, /*batch_size_limit=*/5);
  }
}

RUN_ALL_TESTS()
