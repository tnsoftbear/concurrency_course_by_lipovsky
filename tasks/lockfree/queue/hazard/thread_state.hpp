#pragma once

//#include <hazard/fwd.hpp>

#include <map>
#include <vector>
#include <iostream>
#include <twist/ed/local/var.hpp>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

namespace hazard {

//using hazard::Manager;

struct ThreadState {
 public:
  std::function<void(void*)> deleteFunction{};
  std::map<size_t, void*> guarded_global{};
  std::map<std::string, std::map<size_t, void*>> guarded_local{};
  std::map<std::string, std::vector<void*>> retired{};

  void PrintMe(std::string prefix = "") {
    return;

    std::cout << "Retire: " << prefix << ", Guarded global: ";
    for (auto g : guarded_global) {
      std::cout << g.first << "," << g.second << " | ";
    }
    
    std::cout << "\nRetire: " << prefix << ", Guarded local: ";
    for (auto gl_map : guarded_local) {
      std::cout << gl_map.first << ": ";
      for (auto g : gl_map.second) {
        std::cout << "[" << g.first << "] " << g.second << " | ";
      }
    }

    std::cout << "\nRetire: " << prefix << ", Retired: ";
    for (auto rt_map : retired) {
      std::cout << rt_map.first << ": ";
      for (auto r : rt_map.second) {
        std::cout << r << " | ";
      }
      std::cout << " (size: " << rt_map.second.size() << ")" << std::endl;
    }
    std::cout << std::endl;
  }

  void SetDeleteFunction(std::function<void(void*)> func) {
    deleteFunction = func;
  }

  void DeleteForAllThreads() {
    Ll("Start deleting for all threads, retired.size: %lu", retired.size());
    for (auto r : retired) {
      Ll("r: %s, size: %lu", r.first.c_str(), r.second.size());
      DeleteForThread(r.first);
    }
  }

  void DeleteForThisThread() {
    DeleteForThread(GetPid());
  }

  void DeleteForThread(std::string pid) {
    Ll("ThreadState: Delete retired pointers for thread: %s", pid.c_str());
    std::for_each(retired[pid].begin(), retired[pid].end(), deleteFunction);
    retired[pid].clear();
  }

  bool IsGuardedByThread(void* ptr, std::string skip_pid) {
    for (auto gl_map : guarded_local) {
      if (gl_map.first == skip_pid) {
        continue;
      }
      for (auto gl : gl_map.second) {
        if (gl.second == ptr) {
          Ll("IsGuardedByThread: Pointer %p is guarded by another thread: %s in slot: %lu", ptr, gl_map.first.c_str(), gl.first);
          return true;
        }
      }
    }
    Ll("IsGuardedByThread: Pointer %p is not guarded by another thread", ptr);
    return false;
  }

  void RemoveFromGuardedLocal(void* ptr, std::string pid) {
    for (auto gl : guarded_local[pid]) {
      if (gl.second == ptr) {
        guarded_local[pid][gl.first] = nullptr;
      }
    }
  }

  void RemoveFromGuardedGlobal(void* ptr) {
    for (auto g : guarded_global) {
      if (g.second == ptr) {
        guarded_global[g.first] = nullptr;
      }
    }
  }

  void PushRetired(void* ptr) {
    auto pid = GetPid();
    for (auto r : retired[pid]) {
      if (ptr == r) {
        Ll("PushRetired: Pointer %p already in retired list of thread: %s, skip adding", ptr, pid.c_str());
        return;
      }
    }
    Ll("PushRetired: Pointer added to retired list %p for thread %s", ptr, pid.c_str());
    retired[pid].push_back(ptr);
  }

  std::string GetPid() {
    std::ostringstream pid;
    pid << twist::ed::stdlike::this_thread::get_id();
    return pid.str();
  }

  void Ll(const char* format, ...) {
    bool k_should_print = false;
    //bool k_should_print = true;
    if (!k_should_print) {
      return;
    }

    char buf [250];
    std::ostringstream pid;
    pid << "[" << twist::ed::stdlike::this_thread::get_id() << "]";
    //sprintf(buf, "%s Strand::%s, qc: %lu\n", pid.str().c_str(), format, tasks_.Count());
    sprintf(buf, "%s Mutator::%s\n", pid.str().c_str(), format);
    va_list args;
    va_start(args, format);
    vprintf(buf, args);
    va_end(args);
  }
};

}  // namespace hazard
