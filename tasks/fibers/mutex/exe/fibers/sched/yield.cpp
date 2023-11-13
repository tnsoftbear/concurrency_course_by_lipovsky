#include <exe/fibers/sched/yield.hpp>
#include <exe/fibers/core/fiber.hpp>
#include <exe/fibers/core/awaiter.hpp>

namespace exe::fibers {

namespace {

class YieldAwaiter : public IAwaiter {
  public:
  void AwaitSuspend(Fiber* fiber) override {
    fiber->Schedule();
  }
};

}


void Yield() {
  YieldAwaiter awaiter;
  //printf("Yield: awaiter: %p\n", (void*)&awaiter);
  Fiber::Self()->Suspend(&awaiter);
}

}  // namespace exe::fibers
