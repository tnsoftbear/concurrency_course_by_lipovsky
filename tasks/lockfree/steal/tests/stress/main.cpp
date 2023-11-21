#include "../../work_stealing_queue.hpp"

#include <twist/rt/layer/fault/adversary/lockfree.hpp>

#include <course/twist/test.hpp>

#include <twist/test/race.hpp>
#include <twist/test/lockfree.hpp>
#include <twist/test/budget.hpp>

#include <array>
#include <iostream>

void Ll(const char* format, ...) {
  //const bool k_should_print = true;
  const bool k_should_print = false;
  if (!k_should_print) {
    return;
  }

  char buf[250];
  std::ostringstream pid;
  pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
  sprintf(buf, "%s Test::%s\n", pid.str().c_str(), format);
  va_list args;
  va_start(args, format);
  vprintf(buf, args);
  va_end(args);
}

/////////////////////////////////////////////////////////////////////

void SetupAdversary() {
#if !__has_feature(thread_sanitizer)
  twist::test::SetLockFreeAdversary();
#endif
}

/////////////////////////////////////////////////////////////////////

struct TestObject {
  size_t value;
};

class TestObjectMaker {
 public:
  TestObjectMaker() {
    PrepareNext();
  }

  ~TestObjectMaker() {
    delete next_;
  }

  TestObject* Get() {
    return next_;
  }

  void PrepareNext() {
    next_ = new TestObject{next_value_++};
  }

 private:
  size_t next_value_ = 0;
  TestObject* next_;
};

/////////////////////////////////////////////////////////////////////

template <size_t Queues, size_t Capacity>
void StressTest() {
  SetupAdversary();

  std::array<twist::test::ReportProgressFor<WorkStealingQueue<TestObject, Capacity>>, Queues> queues_;

  // Checksums
  std::atomic<size_t> produced_cs{0};
  std::atomic<size_t> consumed_cs{0};
  std::atomic<size_t> stolen_cs{0};

  twist::test::Race race;

  for (size_t i = 0; i < Queues; ++i) {
    race.Add([&, i]() {
      twist::rt::fault::GetAdversary()->EnablePark();

      size_t random = i;  // Seed

      TestObjectMaker obj_maker;

      for (size_t iter = 0; twist::test::KeepRunning(); ++iter) {
        // TryPush

        for (size_t j = 0; j < iter % 3; ++j) {
          TestObject* obj_to_push = obj_maker.Get();
          size_t obj_value = obj_to_push->value;

          Ll("Push-Test: queues[%lu]", i);
          if (queues_[i]->TryPush(obj_to_push)) {
            random += obj_value;
            produced_cs.fetch_add(obj_value, std::memory_order_relaxed);

            obj_maker.PrepareNext();
          }
        }

        // Grab

        TestObject* steal_buffer[5];

        if ((iter + i) % 5 == 0) {
          size_t steal_target = random % Queues;  // Pseudo-random target

          Ll("Grab-Test: queues[%lu]", steal_target);
          size_t stolen = queues_[steal_target]->Grab({steal_buffer, 5});

          for (size_t s = 0; s < stolen; ++s) {
            stolen_cs.fetch_add(steal_buffer[s]->value, std::memory_order_relaxed);
            //printf("Grab-Test: delete steal_buffer[s];\n");
            //Ll("Grab-Test: delete %lu", steal_buffer[s]->value);
            delete steal_buffer[s];
            //printf("Grab-Test: deleted\n");
          }
          continue;
        }

        // TryPop

        if (random % 2 == 0) {
          continue;
        }

        //Ll("Pop-Test: queues[%lu]", i);
        if (TestObject* obj = queues_[i]->TryPop()) {
          consumed_cs.fetch_add(obj->value, std::memory_order_relaxed);
          //printf("Pop-Test: delete obj;\n");
          //Ll("Pop-Test: delete %lu", obj->value);
          delete obj;
          //printf("Pop-Test: deleted\n");
        }
      }

      // Cleanup

      while (TestObject* obj = queues_[i]->TryPop()) {
        consumed_cs += obj->value;
        //Ll("Cleanup-Test: delete %lu", obj->value);
        delete obj;
      }
    });
  }

  race.Run();

  std::cout << "Produced: " << produced_cs.load() << std::endl;
  std::cout << "Consumed: " << consumed_cs.load() << std::endl;
  std::cout << "Stolen: "   << stolen_cs.load() << std::endl;

  ASSERT_EQ(produced_cs.load(), consumed_cs.load() + stolen_cs.load());
}

//////////////////////////////////////////////////////////////////////

TEST_SUITE(WorkStealingQueue) {
  TWIST_TEST(Stress_1, 5s) {
    StressTest</*Queues=*/2,  /*Capacity=*/5>();
  }

  TWIST_TEST(Stress_2, 5s) {
    StressTest</*Queues=*/4, /*Capacity=*/16>();
  }

  TWIST_TEST(Stress_3, 5s) {
    StressTest</*Queues=*/4, /*Capacity=*/33>();
  }

  TWIST_TEST(Stress_4, 5s) {
    StressTest</*Queues=*/4, /*Capacity=*/128>();
  }
}

RUN_ALL_TESTS()
