#include <ostream>
#include <wheels/test/framework.hpp>

#include "../barrier.hpp"
#include "tf/sched/sleep_for.hpp"

// https://gitlab.com/Lipovsky/tinyfibers
#include <tf/sched/spawn.hpp>
#include <tf/sched/yield.hpp>
#include <tf/sync/mutex.hpp>
#include <tf/sync/wait_group.hpp>

// #include <twist/ed/std/atomic.hpp>

using tf::Mutex;
using tf::Spawn;
using tf::WaitGroup;
using tf::Yield;
using std::cout;
using std::endl;

void TwoFibersDeadLock() {
  // Mutexes
  Mutex a;
  Mutex b;
  bool a_locked{false};
  bool b_locked{false};

  // Fibers

  auto first = [&] {
    a.Lock();
    a_locked = true;
    cout << "a.Locked" << endl;
    Yield();
    if (!b_locked) {
      a.Unlock();
      a_locked = false;
      cout << "a.Unlocked" << endl;
    } else {
      b.Lock();
      cout << "b.Locked in first()";
    }
  };

  auto second = [&] {
    b.Lock();
    b_locked = true;
    cout << "b.Locked" << endl;
    Yield();
    if (!a_locked) {
      b.Unlock();
      b_locked = false;
      cout << "b.Unlocked" << endl;
    } else {
      a.Lock();
      cout << "a.Locked in second()";
    }
  };

  // No deadlock with one fiber

  // No deadlock expected here
  // Run routine twice to check that
  // routine leaves mutexes in unlocked state
  Spawn(first).Join();
  Spawn(first).Join();

  // Same for `second`
  Spawn(second).Join();
  Spawn(second).Join();

  cout << "No deadlock expected here" << endl;

  ReadyToDeadLock();

  // Deadlock with two fibers
  WaitGroup wg;
  cout << "Start testing" << endl;
  wg.Spawn(first).Spawn(second).Wait();

  cout << "End testing" << endl;
  // We do not expect to reach this line
  FAIL_TEST("No deadlock =(");
}
