#include "philosopher.hpp"

#include <twist/test/inject_fault.hpp>

namespace dining {

Philosopher::Philosopher(Table& table, size_t seat)
    : table_(table),
      seat_(seat),
      left_fork_(table_.LeftFork(seat)),
      right_fork_(table_.RightFork(seat)) {
}

void Philosopher::Eat() {
  AcquireForks();
  EatWithForks();
  ReleaseForks();
}

// Acquire left_fork_ and right_fork_
void Philosopher::AcquireForks() {
  // Your code goes here
}

void Philosopher::EatWithForks() {
  table_.AccessPlate(seat_);
  // Try to provoke data race
  table_.AccessPlate(table_.ToRight(seat_));
  ++meals_;
}

// Release left_fork_ and right_fork_
void Philosopher::ReleaseForks() {
  // Your code goes here
}

void Philosopher::Think() {
  // Random pause or context switch
  twist::test::InjectFault();
}

}  // namespace dining
