#pragma once

#include "mutator.hpp"
#include "manager.hpp"
#include "thread_state.hpp"

#include <twist/ed/stdlike/atomic.hpp>

#include <cstddef>
#include <twist/ed/stdlike/thread.hpp>  // for debug logs
#include <sstream>

namespace hazard {

Mutator::Mutator(Manager* manager/* , size_t max_thread */)
  : manager_(manager)
  //, max_threads_(max_threads)
{
  (void)manager_;
}

ThreadState& Mutator::GetThreadState() {
  //return Manager::GetThreadState(); 
  //return Mutator::thread_state;
  return thread_state;
}

// прочитать указатель на объект из атомика `ptr` 
// и защитить этот объект от удаления для последующих обращений к нему,
// используя локальный слот `index`.
template <typename T>
T* Mutator::Protect(size_t index, AtomicPtr<T>& ptr) {
  Ll("Protect: starts for %p", ptr.load());
  auto& ts = GetThreadState();
  auto pid = GetPid();
  T* t;
  do {
    t = ptr.load();
    if (t == nullptr) {
      Ll("Protect: nullptr inside. return");
      return nullptr;
    }
    ts.guarded_global[index] = t;
    ts.guarded_local[pid][index] = t;
  } while (t != ptr.load());
  Ll("Protect: guarded %p", t);
  return static_cast<T*>(t);
}

// анонсировать обращение к объекту для других потоков через локальный слот `index`
template <typename T>
void Mutator::Announce(size_t index, T* ptr) {
  (void)index;
  (void*)ptr;
  //ts.announced_ptrs[index] = ptr;
}

// добавить объект в очередь на удаление (retire list)
template <typename T>
void Mutator::Retire(T* ptr) {
  auto pid = GetPid();
  auto& ts = GetThreadState();
  auto& rtr = ts.retired[pid];
    ts.SetDeleteFunction([this](void* ptr) {
    if (T* typed_ptr = static_cast<T*>(ptr)) {
      Ll("Deleting: %p", ptr);
      delete typed_ptr;
    }
  });
  Ll("Retire: starts, r.size: %lu, push retired ptr: %p", rtr.size(), ptr);

  ts.PrintMe("BEGIN");

  ts.RemoveFromGuardedLocal(ptr, pid);
  ts.RemoveFromGuardedGlobal(ptr);
  // bool is_guarded_by_other_thread = ts.IsGuardedByThread(ptr, pid);
  // if (!is_guarded_by_other_thread) {
  //   ts.PushRetired(ptr);
  // }

  ts.PushRetired(ptr);

  size_t i = 0;
  if (rtr.size() > 10) {
    Ll("Retire: retired size is full, start cleaning. Size: %lu", rtr.size());
    auto& grd = ts.guarded_global;
    auto it = rtr.begin();
    auto size = rtr.size();
    while (i < size / 2) {
    //while (i < size) {
      Ll("Retire: check retired %p", *it);
      bool is_guarded = false;
      for (auto& g : grd) {
        Ll("g.first: %lu, g.second: %p, *it: %p", g.first, g.second, *it);
        if (g.second == *it) {
          is_guarded = true;
          break;
        }
      }
      if (!is_guarded) {
        Ll("Retire: Pointer is not guarded, thus it will be deleted, ptr: %p", *it);
        ts.deleteFunction(*it);
        rtr.erase(it);
      } else {
        it++;
        Ll("Retire: Pointer is guarded, skip deleting, ptr: %p", *it);
      }
      i++;
    }
  }
  ts.PrintMe("END");
  Ll("Retire: ends");
}

// сбросить все защищенные мутатором указатели
void Mutator::Clear() {
  Ll("Clear");
  auto& ts = GetThreadState();
  auto pid = GetPid();
  for (auto& gl : ts.guarded_local[pid]) {
    if (gl.second == nullptr) {
      continue;
    }
    ts.PushRetired(gl.second);
    ts.guarded_local[pid][gl.first] = nullptr;
  }
}

Mutator::~Mutator() {
  Clear();
  GetThreadState().PrintMe("~Mutator");
}

}  // namespace hazard
