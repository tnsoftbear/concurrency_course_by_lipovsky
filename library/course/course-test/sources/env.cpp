#include <course/twist/env.hpp>

#include <wheels/test/framework.hpp>

#include <iostream>

#include <fmt/core.h>

namespace course::twist {

struct TestEnv : ::twist::rt::IEnv {
  size_t Seed() const override {
    return wheels::test::TestHash();
  }

  bool KeepRunning() const override {
    static const auto kSafeMargin = 500ms;
    return wheels::test::TestTimeLeft() > kSafeMargin;
  }

  void PrintLine(std::string message) override {
    std::cout << message << std::endl;
  }

  void Exception() override {
    wheels::test::FailTestByException();
  }

  void Assert(wheels::SourceLocation where, std::string reason) override {
    wheels::test::FailTestByAssert({reason, where});
  }
};

::twist::rt::IEnv* TestEnv() {
  static struct TestEnv instance;
  return &instance;
}

}  // namespace course::twist
