#pragma once

#include <bits/chrono.h>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>

using Nanos = int64_t;

constexpr Nanos NANOS_TO_MICROS = 1000;
constexpr Nanos MICROS_TO_MILLIS = 1000;
constexpr Nanos MILLIS_TO_SECS = 1000;
constexpr Nanos NANOS_TO_MILLIS = NANOS_TO_MICROS * MICROS_TO_MILLIS;
constexpr Nanos NANOS_TO_SECS = NANOS_TO_MICROS * MICROS_TO_MILLIS * MILLIS_TO_SECS;

inline auto GetCurrentNanos()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

inline void GetCurrentTimeStr(std::string &timeStr)
{
    const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    timeStr.assign(ctime(&time));
    if (!timeStr.empty())
        timeStr.at(timeStr.length() - 1) = '\0';
}
