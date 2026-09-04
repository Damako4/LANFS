// ShutdownSignal.hpp
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

class ShutdownSignal {
public:
  static ShutdownSignal &instance() {
    static ShutdownSignal s;
    return s;
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return stopRequested.load(); });
  }

  void requestStop() {
    {
      std::lock_guard<std::mutex> lock(mtx);
      stopRequested = true;
    }
    cv.notify_all();
  }

private:
  std::atomic<bool> stopRequested{false};
  std::mutex mtx;
  std::condition_variable cv;
};

inline void handleSignal(int /*signum*/) {
  ShutdownSignal::instance().requestStop();
}