#pragma once

#include <exe/futures/types/future.hpp>
#include <exe/executors/inline.hpp>
#include "exe/futures/make/contract.hpp"
#include <map>
#include <twist/ed/stdlike/atomic.hpp>

using twist::ed::stdlike::atomic;

namespace exe::futures {

/**
 # Нерешённая проблема
 
 Я не знаю, как иначе сохранить время жизни атомарного счётчка, поэтому применил такое некрасивое решение 
 со статическими глобальными переменными. Аналогично сделано в реализации Strand.
 Здесь каждому вызову First() назначается идентификатор, который захватывает лямбда 
 и по которому она обращается к мапе со статическим временем жизни, чтобы трекать кол-во вызовов подписок на фьючи.

 ## Детали проблемы
 
 Если создать атомарный счётчик в области видимости First(), то замкнуть его в лямбде можно только по ссылке, 
 потому что копирование не поддерживается реализацией atomic. 
 Но так объект счётчика пропадёт вместе со окончанием временем жизни ф-ции First().

 Семантика перемещенниия тоже не работает (`counter = std::move(counter)`). Вылезает ошибка:
 > error: call to implicitly-deleted copy constructor of 'typename remove_reference<atomic<unsigned long> &>::type' (aka 'std :: atomic<unsigned long>')
 Я знаю, что семантика перемещения запрещена в std :: atomic, как и семантика копирования. Полагаю, что аналогично сделано и в атомике twist.
 Меня смущает текст ошибки, который ссылается на имплицитно удалённый _конструктор копирования_, когда я прошу _семантику перемещения_?
  
 Если заворачивать логику ф-ции в объект комбинатора, то сталкиваюсь с аналогичной проблемой.

 std::atomic в мапе используюется вместо stdlike::atomic, потому что последний не позволяет создать и инициализировать значение в мапе,
 например таким образом: `first_of::cb_cnt[id].store(2);`. А как иначе, я не знаю (через emplace, insert не выходит).
 */

namespace first_of {
static atomic<size_t> max_id{0}; // Счётчкик идентификаторов для комбинатора First
static std::map<size_t, std::atomic<size_t>> cb_cnt; // Ожидаемое кол-во вызовов колбеков для фьюч
}

template <typename T>
Future<T> First(Future<T> f1, Future<T> f2) {
  auto id = first_of::max_id.fetch_add(1);
  first_of::cb_cnt[id].store(2);
  auto [f, p] = Contract<T>();

  auto cb = [p = std::move(p), id](Result<T> r) mutable {
    if (r) {
      std::move(p).Set(r);
      return;
    }
    if (first_of::cb_cnt[id].fetch_sub(1) == 1) {
      std::move(p).Set(r);
      return;
    }
  };

  std::move(f1)
    .Via(executors::Inline())
    .Subscribe([cb](Result<T> r) mutable {
      cb(r);
    });

  std::move(f2)
    .Via(executors::Inline())
    .Subscribe([cb](Result<T> r) mutable {
      cb(r);
    });

  return std::move(f);
}

}  // namespace exe::futures
