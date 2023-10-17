#pragma once

#include <asio/io_context.hpp>

namespace exe::fibers {

using Scheduler = asio::io_context;

Scheduler& GetCurrent();

void SetCurrent(Scheduler& scheduler);

template <typename F>
void Submit(Scheduler& scheduler, F&& fun);

}
