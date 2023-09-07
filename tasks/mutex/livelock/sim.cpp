#include <wheels/test/framework.hpp>

// https://gitlab.com/Lipovsky/tinyfibers
#include <tf/sched/spawn.hpp>
#include <tf/sched/yield.hpp>
#include <tf/sync/wait_group.hpp>

#include <tf/rt/scheduler.hpp>
#include "tf/run.hpp"

using tf::WaitGroup;
using tf::Yield;

void LiveLock() {
  static const size_t kIterations = 100;

  size_t cs_count = 0;

  // TrickyLock state
  size_t thread_count = 0;

  auto contender = [&] {
    // std::cout << "Id: " << tf::rt::Scheduler::Current()->RunningFiber()->Id() << std::endl;
    for (size_t i = 0; i < kIterations; ++i) {
      // TrickyLock::Lock
      while (thread_count++ > 0) {
        // std::cout << "Id: " << tf::rt::Scheduler::Current()->RunningFiber()->Id() << "; Yield1 " << std::endl;
        Yield();
        --thread_count;
      }
      // Spinlock acquired

      {
        // Critical section
        ++cs_count;
        ASSERT_TRUE_M(cs_count < 3, "Too many critical sections");
        // End of critical section
      }
      // std::cout << "Id: " << tf::rt::Scheduler::Current()->RunningFiber()->Id() << "; Yield2 " << std::endl;
      Yield();
      // TrickyLock::Unlock
      --thread_count;
      // Spinlock released
    }
  };

  // Spawn two fibers
  WaitGroup wg;
  wg.Spawn(contender).Spawn(contender).Wait();
};
