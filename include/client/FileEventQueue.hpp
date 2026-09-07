#pragma once

#include <mutex>
#include <iostream>
#include <queue>
#include <optional>

struct FileEvent {
    std::string fileName;
};

class FileEventQueue {
public:
    void push(FileEvent event) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(std::move(event));
    }

    std::optional<FileEvent> pop() {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty()) return std::nullopt;
        FileEvent event = std::move(queue.front());
        queue.pop();
        return event;
    }
private:
    std::mutex mutex;
    std::queue<FileEvent> queue;
};