#pragma once

#include <mutex>
#include <iostream>
#include <queue>
#include <optional>

/**
 * @brief A file event
 */
struct FileEvent {
    std::string fileName; ///< Name of file
};

class FileEventQueue {
public:
    /**
     * @brief Push @p event the queue
     */
    void push(FileEvent event) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(std::move(event));
    }

    /**
     * @brief Pop an event from the queue
     * 
     * @return Optional<FileEvent> which is std::nullopt if queue is empty
     */
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