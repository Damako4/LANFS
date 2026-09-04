#pragma once

#include <efsw/efsw.hpp>

class UpdateListener : public efsw::FileWatchListener {
public:
  void handleFileAction(efsw::WatchID watchid, const std::string &dir,
                        const std::string &filename, efsw::Action action,
                        const std::string &oldFilename) override;
};