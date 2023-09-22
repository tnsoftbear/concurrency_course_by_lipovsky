#include <mutex>

struct Accessor {
  public:
    Accessor(std::mutex& m)
     : lock_(m)    // (1)
    {
        // lock_ = m; // std::lock_guard::lock_guard(m);        // (2)
    };
  private:
    std::lock_guard<std::mutex> lock_;
};

int main() {
  std::mutex m; // = std::mutex();
  auto a = new Accessor(m);
}