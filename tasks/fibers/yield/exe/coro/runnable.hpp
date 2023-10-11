# pragma once

namespace exe::coro {

struct IRunnable {
    virtual ~IRunnable() = default;
    virtual void RunCoro() = 0;
};

}