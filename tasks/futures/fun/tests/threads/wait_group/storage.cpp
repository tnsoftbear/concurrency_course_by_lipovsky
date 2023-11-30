#include <exe/threads/wait_group.hpp>

#include <course/twist/test.hpp>

#include <twist/ed/stdlike/thread.hpp>

using exe::threads::WaitGroup;

  void Ll(const char* format, ...) {
    const bool should_print = true;
    if (!should_print) {
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

//////////////////////////////////////////////////////////////////////

void StorageTest() {
  // Help AddressSanitizer
  auto* wg = new WaitGroup{};

  wg->Add(1);
  twist::ed::stdlike::thread t([wg] {
    wg->Done();
  });

  wg->Wait();
  delete wg;

  t.join();
}

//////////////////////////////////////////////////////////////////////

TEST_SUITE(WaitGroup) {
  TWIST_TEST_REPEAT(Storage, 5s) {
    StorageTest();
  }
}

RUN_ALL_TESTS()
