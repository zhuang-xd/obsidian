#include "base/sequence_manager.h"
#include "base/logging.h"
#include "base/eintr_wrapper.h"
#include "base/time.h"
#include "base/uart_helper.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdio.h>
#include <sys/eventfd.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace {
static constexpr int32_t kTaskInternalTime = 60 * base::Time::kMicrosecondsPerMillisecond;
static constexpr int8_t kThreadPoolSize = 2;
}  // namespace

SequenceManager::SequenceManager(Deletate* deletate)
    : deletate_(deletate) {
}

SequenceManager::~SequenceManager()
{
    Stop();
}

void SequenceManager::Start()
{
    LOGI("SequenceManager::Start");
    thread_pool_.reserve(kThreadPoolSize);
    is_running_= true;
    for (uint8_t index = 0; index < kThreadPoolSize; index++) {
        auto  thread_data = std::make_unique<ThreadMetadata>();
        thread_data->wake_event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        thread_data->thread_impl = new std::thread(std::bind(&SequenceManager::ThreadMain, this, thread_data.get()));
        thread_pool_.emplace_back(std::move(thread_data));
    }
}

void SequenceManager::Stop()
{
    is_running_.store(false, std::memory_order_release);
    ScheduleWork();

    for (auto& item: thread_pool_) {
        auto& thread = item->thread_impl;
        if (thread->joinable()) {
            thread->join();
            delete thread;
            thread = nullptr;
        }
    }
}

void SequenceManager::PostTask(const Task& task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        task_queue_.emplace(task);
    }
    ScheduleWork();
}


void SequenceManager::ThreadMain(void* arg)
{
    LOGI("SequenceManager::ThreadMain");
    ThreadMetadata* thread_data = (ThreadMetadata*)arg;
    int wake_event_fd = thread_data->wake_event_fd;
    while (is_running_.load(std::memory_order_relaxed)) {
        thread_data->is_idle.store(true, std::memory_order_release);
        int32_t ret = UartHelper::WaitDeviceForRead(wake_event_fd, base::TickDelta(kTaskInternalTime));
        bool contine_run = true;
        if (ret > 0) {
            uint64_t value = 0;
            HANDLE_EINTR(read(wake_event_fd, &value, sizeof(value)));
            thread_data->is_idle.store(false, std::memory_order_release);
            contine_run = DoWork(thread_data);
        }

        if (!contine_run) {
            break;
        }
    }

    close(wake_event_fd);
}

void SequenceManager::RefreshStatus()
{
    if (!is_running_.load(std::memory_order_relaxed)) {
        deletate_->OnAllTaskFinished();
        return;
    }

    bool is_idle = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        is_idle = task_queue_.empty();
    }

    if (!is_idle) {
        return;
    }

    for (const auto& item : thread_pool_) {
        if (!item->is_idle.load(std::memory_order_relaxed)) {
            is_idle = false;
            break;
        }
    }

    if (is_idle && deletate_) {
        deletate_->OnAllTaskFinished();
    }
}

void SequenceManager::ScheduleWork(void)
{
    LOGT("DelayedTaskManager::ScheduleWork");
    const uint64_t value = 1;
    for (auto& item : thread_pool_) {
        int wake_event_fd = item->wake_event_fd;
        HANDLE_EINTR(write(wake_event_fd, &value, sizeof(value)));
    }
}

bool SequenceManager::DoWork(ThreadMetadata* thread_data)
{
    LOGT("DoWork");
    bool contine_run = true;
    Task task;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!task_queue_.empty()) {
            task = task_queue_.front();
            task_queue_.pop();
        }
    }

    if (!is_running_.load(std::memory_order_relaxed)) {
        RefreshStatus();
        return false;
    }

    bool need_notify = true;
    if (task.custom_data) {
        need_notify = task.func(task.custom_data);
    }

    if (contine_run && task_queue_.empty()) {
        thread_data->is_idle.store(true, std::memory_order_release);
    }

    if (need_notify) {
        RefreshStatus();
    }

    return contine_run;
}
