#pragma once

#include <exe/futures/syntax/pipe.hpp>
#include <exe/executors/inline.hpp>

namespace exe::futures {

namespace pipe {

struct [[nodiscard]] Flatten {
  template <typename T>
  Future<T> Pipe(Future<Future<T>> inf) {
    auto [outf, outp] = Contract<T>();
    std::move(inf).Subscribe([outp = std::move(outp)](Result<Future<T>> inf_res) mutable {
      if (inf_res) {
        auto midf = std::move(inf_res.value());
        std::move(midf).Subscribe([outp = std::move(outp)](Result<T> midf_res) mutable {
          std::move(outp).Set(std::move(midf_res.value()));
        });
      } else {
        std::move(outp).SetError(inf_res.error());
      }
    });
    return std::move(outf);
  }
};

}  // namespace pipe

// Future<Future<T>> -> Future<T>

inline auto Flatten() {
  return pipe::Flatten{};
}

}  // namespace exe::futures
