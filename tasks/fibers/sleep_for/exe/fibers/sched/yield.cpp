#include <exe/fibers/sched/yield.hpp>
#include <exe/fibers/core/fiber.hpp>

namespace exe::fibers {

void Yield() {
  // Возвращаемся в контекст сохранённый в Resume(), т.е. в Fiber::Run() после core_->Resume()
  Fiber::Self()->Suspend();
}

}  // namespace exe::fibers
