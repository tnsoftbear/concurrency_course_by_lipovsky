#pragma once

#include <map>

#include "thread_state.hpp"
#include "mutator.hpp"

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

namespace hazard {

// сборщик мусора.
class Manager {
 private:
  std::map<std::string, std::shared_ptr<Mutator>> mutators_;
 public:
  //static ThreadState thread_state;

  static Manager* Get();

  /**
   * построить _мутатор_, с помощью которого поток в операции над лок-фри контейнером сможет
   * - защищать объекты в памяти от удаления и 
   * - планировать удаление объектов
   */
  std::shared_ptr<Mutator> MakeMutator();

  // собрать мусор во всех retire list-ах всех зарегистрированных потоков
  void Collect();

  static std::string GetPid() {
    std::ostringstream pid;
    pid << twist::ed::stdlike::this_thread::get_id();
    return pid.str();
  }

  static ThreadState& GetThreadState();

 private:
  void Ll(const char* format, ...);
};

}  // namespace hazard
