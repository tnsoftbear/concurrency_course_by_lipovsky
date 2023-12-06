#pragma once

namespace refer {

struct IManaged {
  virtual ~IManaged() = default;

  virtual void AddRef() = 0;
  virtual void ReleaseRef() = 0;

  // For optimizations
  virtual bool IsManualLifetime() const = 0;
};

}  // namespace refer
