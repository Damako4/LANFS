#include <UpdateListener.hpp>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void UpdateListener::handleFileAction(efsw::WatchID watchid, const std::string &dir, const std::string &filename, efsw::Action action, const std::string &oldFilename) {
  // tmp files being created by server, ignore
  // Potential TODO: Have tmp files be created in /tmp/
  if (fs::path(oldFilename).extension() == ".tmp" || fs::path(filename).extension() == ".tmp") {
    return;
  }
  FileEvent event;
  switch (action) {
  case efsw::Actions::Add:
    break;
  case efsw::Actions::Delete:
    break;
  case efsw::Actions::Modified: {
    // Debounce multiple modify events within a small time frame
    auto now = std::chrono::steady_clock::now();
    {
      std::lock_guard<std::mutex> lock(debounceMutex);
      auto it = lastEventTime.find(filename);
      if (it != lastEventTime.end() && (now - it->second) < debounceWindow) {
        it->second = now;
        return;
      }
      lastEventTime[filename] = now;
    }
    event.fileName = filename;
    queue.push(event);
    break;
  }
  case efsw::Actions::Moved:
    break;
  default:
    throw std::runtime_error("Error in UpdateListener: This should never happen!");
  }
}