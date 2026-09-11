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
    std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Added" << std::endl;
    break;
  case efsw::Actions::Delete:
    std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Delete" << std::endl;
    break;
  case efsw::Actions::Modified: {
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
    std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Moved from (" << oldFilename << ")" << std::endl;
    break;
  default:
    std::cout << "Should never happen!" << std::endl;
  }
}