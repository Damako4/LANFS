#pragma once

#include <efsw/efsw.hpp>
#include <FileEventQueue.hpp>
#include <mutex>
#include <unordered_map>
#include <chrono>

class UpdateListener : public efsw::FileWatchListener {
public:
  UpdateListener(FileEventQueue &queue) : queue(queue) {};
  void handleFileAction(efsw::WatchID watchid, const std::string &dir,
                        const std::string &filename, efsw::Action action,
                        const std::string &oldFilename) override;
private:
    FileEventQueue &queue;
    std::mutex debounceMutex;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastEventTime;
    static constexpr auto debounceWindow = std::chrono::milliseconds(300);
};