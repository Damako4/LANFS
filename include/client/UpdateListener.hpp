#pragma once

#include <efsw/efsw.hpp>
#include <FileEventQueue.hpp>

class UpdateListener : public efsw::FileWatchListener {
public:
  UpdateListener(FileEventQueue &queue) : queue(queue) {};
  void handleFileAction(efsw::WatchID watchid, const std::string &dir,
                        const std::string &filename, efsw::Action action,
                        const std::string &oldFilename) override;
private:
    FileEventQueue &queue;
};