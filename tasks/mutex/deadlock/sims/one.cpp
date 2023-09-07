#include <wheels/test/framework.hpp>

// https://gitlab.com/Lipovsky/tinyfibers
#include <tf/sched/spawn.hpp>
#include <tf/sched/yield.hpp>
#include <tf/sync/mutex.hpp>

using tf::Mutex;
using tf::Spawn;
using tf::Yield;

void OneFiberDeadLock() {
  Mutex mutex;

  std::function<void()> fiber = [&] {
    mutex.Lock();
    fiber();
  };

  Spawn(fiber).Join();

  // We do not expect to reach this line
  FAIL_TEST("No deadlock =(");
}
