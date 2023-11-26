#pragma once

#include <atomic>
#include <memory>
#include <pthread.h>
#include <string>
#include <thread>
#include <utility>

#include "Logger.h"
#include "Types.h"

inline auto SetThreadCore(int coreId)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);

    return (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0);
}

template <typename T, typename... Args>
inline std::unique_ptr<std::thread> CreateAndStartThread(s32 coreId, std::string const &name, T &&func, Args &&...args)
{
    std::atomic<bool> running(false);
    std::atomic<bool> failed(false);

    s32 totalThreads = (s32)std::thread::hardware_concurrency();

    auto thread_body = [&] {
        if (coreId >= 0 && coreId <= totalThreads && !SetThreadCore(coreId))
        {
            SHOWWARNING("Failed to set core affinity for ", name, " (", pthread_self(), ") to ", coreId);
            failed = true;
            return;
        }

        SHOWINFO("Set core affinity for ", name, "(", pthread_self(), ") to ", coreId);
        running = true;

        std::forward<T>(func)(std::forward<Args>(args)...);
    };

    auto t = std::make_unique<std::thread>(thread_body);

    /* Spin while waiting */
    while (!running && !failed)
        ;

    if (failed.load())
    {
        t->join();
        t = nullptr;
    }
    return t;
}
