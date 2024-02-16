#pragma once

#include "Check.h"
#include "SafeQueue.h"
#include "ThreadUtils.h"
#include <chrono>
#include <fstream>
#include <string>
#include <thread>

/* TODO: Make this a smart singletone */
class QuickLogger
{
private:
    enum class LogElementType : int8_t
    {
        NONE = 0,
        CHAR = 1,
        INTEGER = 2,
        LONG_INTEGER = 3,
        LONG_LONG_INTEGER = 4,
        UNSIGNED_INTEGER = 5,
        UNSIGNED_LONG_INTEGER = 6,
        UNSIGNED_LONG_LONG_INTEGER = 7,
        FLOAT = 8,
        DOUBLE = 9,
    };
    struct LogElement
    {
        LogElementType type = LogElementType::NONE;
        union {
            char c;
            int i;
            long l;
            long long ll;
            unsigned u;
            unsigned long ul;
            unsigned long long ull;
            float f;
            double d;
        } data;
    };

public:
    explicit QuickLogger(std::string const &path) : queue(1024 * 1024 * 8)
    {
        outputStream.open(path.c_str());
        CHECK_FATAL(outputStream.is_open(), "Could not open file: ", path);
        loggerThread = CreateAndStartThread(-1, "LoggerThread", [&] { FlushQueue(); });
        CHECK_FATAL(loggerThread != nullptr, "Could not create the logger thread");
    };
    ~QuickLogger()
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        shouldStop = true;
        loggerThread->join();
    }

    QuickLogger() = delete;
    QuickLogger(const QuickLogger &) = delete;
    QuickLogger(const QuickLogger &&) = delete;
    QuickLogger &operator=(const QuickLogger &) = delete;
    QuickLogger &operator=(const QuickLogger &&) = delete;

public:
    void Log()
    {
    }

    template <typename Arg, typename... Args> void Log(Arg const &value, Args &&...args)
    {
        PushValue(value);

        Log(args...);
    }

    void PushValue(LogElement const &value)
    {
        *queue.GetNextWriteTo() = value;
        queue.UpdateWriteIndex();
    }

    void PushValue(char const c)
    {
        PushValue(LogElement{LogElementType::CHAR, {.c = c}});
    }

    void PushValue(char const *c)
    {
        while (*c)
        {
            PushValue(*c);
            ++c;
        }
    }

    void PushValue(std::string const &c)
    {
        PushValue(c.c_str());
    }

    void PushValue(int const c)
    {
        PushValue(LogElement{LogElementType::INTEGER, {.i = c}});
    }

    void PushValue(long const c)
    {
        PushValue(LogElement{LogElementType::LONG_INTEGER, {.l = c}});
    }

    void PushValue(long long const c)
    {
        PushValue(LogElement{LogElementType::LONG_LONG_INTEGER, {.ll = c}});
    }

    void PushValue(unsigned const c)
    {
        PushValue(LogElement{LogElementType::UNSIGNED_INTEGER, {.u = c}});
    }

    void PushValue(unsigned long const c)
    {
        PushValue(LogElement{LogElementType::UNSIGNED_LONG_INTEGER, {.ul = c}});
    }

    void PushValue(unsigned long long const c)
    {
        PushValue(LogElement{LogElementType::UNSIGNED_LONG_LONG_INTEGER, {.ull = c}});
    }

    void PushValue(float const c)
    {
        PushValue(LogElement{LogElementType::FLOAT, {.f = c}});
    }

    void PushValue(f64 const c)
    {
        PushValue(LogElement{LogElementType::DOUBLE, {.d = c}});
    }

private:
    void FlushQueue()
    {
        while (!shouldStop)
        {
            for (auto next = queue.GetNextRead(); queue.GetSize() && next; next = queue.GetNextRead())
            {
                switch (next->type)
                {
                case LogElementType::CHAR:
                    LogValue(next->data.c);
                    break;
                case LogElementType::INTEGER:
                    LogValue(next->data.i);
                    break;
                case LogElementType::LONG_INTEGER:
                    LogValue(next->data.l);
                    break;
                case LogElementType::LONG_LONG_INTEGER:
                    LogValue(next->data.ll);
                    break;
                case LogElementType::UNSIGNED_INTEGER:
                    LogValue(next->data.u);
                    break;
                case LogElementType::UNSIGNED_LONG_INTEGER:
                    LogValue(next->data.l);
                    break;
                case LogElementType::UNSIGNED_LONG_LONG_INTEGER:
                    LogValue(next->data.ull);
                    break;
                case LogElementType::FLOAT:
                    LogValue(next->data.f);
                    break;
                case LogElementType::DOUBLE:
                    LogValue(next->data.d);
                    break;
                case LogElementType::NONE:
                default:
                    break;
                }
                queue.UpdateReadIndex();
            }
            outputStream.flush();

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    template <typename T> void LogValue(T c)
    {
        outputStream << c;
    }

private:
    std::ofstream outputStream;

    SafeQueue<LogElement> queue;

    std::unique_ptr<std::thread> loggerThread;
    std::atomic<bool> shouldStop = false;
};
