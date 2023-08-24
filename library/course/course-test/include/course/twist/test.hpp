#pragma once

#include "env.hpp"

#include <wheels/test/framework.hpp>

#include <twist/rt/run.hpp>
#include <twist/test/repeat.hpp>

#include <fmt/core.h>

////////////////////////////////////////////////////////////////////////////////

#define _TWIST_TEST_IMPL(name, options) \
  void TwistTestRoutine##name();        \
  TEST(name, options) {                 \
    ::twist::rt::Run(::course::twist::TestEnv(), [] { \
      TwistTestRoutine##name(); \
    });                    \
  }                                     \
  void TwistTestRoutine##name()

////////////////////////////////////////////////////////////////////////////////

#define TWIST_TEST(name, budget) \
  _TWIST_TEST_IMPL(name, ::wheels::test::TestOptions().TimeLimit(budget))

////////////////////////////////////////////////////////////////////////////////

#define TWIST_TEST_REPEAT(name, budget)                               \
  void TwistIteratedTestRoutine##name();                          \
  TEST(name, wheels::test::TestOptions().TimeLimit(budget)) {     \
    auto* env = ::course::twist::TestEnv();                         \
                                                                  \
    ::twist::rt::Run(env, [env] {                                          \
      ::twist::test::Repeat repeat{};                                     \
      while (repeat()) {                                              \
        TwistIteratedTestRoutine##name();                                                                      \
      }                                                               \
      env->PrintLine(fmt::format("Iterations: {}", repeat.IterCount()));                                                                  \
    });                                                                  \
  }                                                               \
  void TwistIteratedTestRoutine##name()

////////////////////////////////////////////////////////////////////////////////

namespace course::twist {

namespace detail {

template <typename... Args>
class TestRunsRegistrar {
  using Self = TestRunsRegistrar;

  using TestTemplate = std::function<void(Args... args)>;
  using TL = std::chrono::milliseconds;

 public:
  TestRunsRegistrar(std::string suite, void (*routine)(Args... args))
      : suite_(suite),
        template_(routine) {
  }

  Self* operator->() {
    return this;
  }

  Self& Run(Args... args) {
    auto main = [temp = template_, args = std::make_tuple(args...)]() {
      std::apply(temp, args);
    };

    auto run = [main] {
      ::twist::rt::Run(course::twist::TestEnv(), [&] {
        main();
      });
    };

    Add(run);

    return *this;
  }

  Self& TimeLimit(TL value) {
    time_limit_ = value;
    return *this;
  }

 private:
  void Add(std::function<void()> run) {
    std::string name = fmt::format("Run-{}", ++index_);
    auto options = wheels::test::TestOptions().TimeLimit(time_limit_);

    auto test = wheels::test::MakeTest(run, name, suite_, options);
    wheels::test::RegisterTest(std::move(test));
  }

 private:
  std::string suite_;
  TestTemplate template_;
  TL time_limit_{10s};
  size_t index_{0};
};

}  // namespace detail

}  // namespace course::twist

#define TWIST_TEST_TEMPLATE(suite, routine) \
  static ::course::twist::detail::TestRunsRegistrar UNIQUE_NAME( \
      twist_runs_registrar__) =             \
      ::course::twist::detail::TestRunsRegistrar(#suite, routine)
