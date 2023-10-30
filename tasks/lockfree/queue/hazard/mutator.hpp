#pragma once

#include "manager.hpp"
//#include "fwd.hpp"
#include "thread_state.hpp"

#include <twist/ed/stdlike/atomic.hpp>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

namespace hazard {

//const bool kShouldPrint = true;
const bool kShouldPrint = false;

static ThreadState thread_state;

// Forward declaration
class Manager;

class Mutator {
  template <typename T>
  using AtomicPtr = twist::ed::stdlike::atomic<T*>;

 private:
  Manager* manager_;

 public:
  // static ThreadState thread_state;

 public:
  explicit Mutator(Manager* manager/* , size_t max_thread */);

  static ThreadState& GetThreadState();

  // прочитать указатель на объект из атомика `ptr` 
  // и защитить этот объект от удаления для последующих обращений к нему,
  // используя локальный слот `index`.
  template <typename T>
  T* Protect(size_t index, AtomicPtr<T>& ptr);

  // анонсировать обращение к объекту для других потоков через локальный слот `index`
  template <typename T>
  void Announce(size_t index, T* ptr);

  // добавить объект в очередь на удаление (retire list)
  template <typename T>
  void Retire(T* ptr);

  // сбросить все защищенные мутатором указатели
  void Clear();

  ~Mutator();

  static std::string GetPid() {
    std::ostringstream pid;
    pid << twist::ed::stdlike::this_thread::get_id();
    return pid.str();
  }  

  void Ll(const char* format, ...) {
    if (!kShouldPrint) {
      return;
    }

    char buf [250];
    auto pid = GetPid();
    sprintf(buf, "[%s] Mutator::%s\n", pid.c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};

}  // namespace hazard
