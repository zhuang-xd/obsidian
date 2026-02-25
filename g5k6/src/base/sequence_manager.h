#ifndef BASE_SEQUENCE_MANAGER_H_
#define BASE_SEQUENCE_MANAGER_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class SequenceManager {
public:
    class Deletate {
        public:
            virtual bool OnAllTaskFinished(void) = 0;
            virtual ~Deletate() {};
    };

    using TaskFunction = std::function<bool(void*)>;
    struct Task {
        TaskFunction func{nullptr};
        void* custom_data{nullptr};
    };

public:
    SequenceManager(Deletate* deletate);
    ~SequenceManager();

public:
    void Start();
    void Stop();
    void PostTask(const Task& task);
    bool IsIdle();

private:
    struct ThreadMetadata;
    void ThreadMain(void* arg);
    void ScheduleWork(void);
    bool DoWork(ThreadMetadata* thread_data);
    void RefreshStatus();

private:
    // thread pool
    struct ThreadMetadata {
        int wake_event_fd{-1};
        std::atomic_bool is_idle{false};
        std::thread* thread_impl{nullptr};
    };

    std::vector<std::unique_ptr<ThreadMetadata>> thread_pool_;
    std::queue<Task> task_queue_;

    std::atomic_bool is_running_{false};
    std::mutex mutex_;

    Deletate* deletate_;
};

#endif  // BASE_SEQUENCE_MANAGER_H_
