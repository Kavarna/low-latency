#pragma once

#include <assert.h>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

template <typename... Args> inline std::string Format(Args &&...args)
{
    if constexpr (sizeof...(Args) == 0)
        return "";

    std::ostringstream stream;

    (stream << ... << args);

    return stream.str();
}

template <typename... Args> inline void DebugPrint(Args &&...args)
{
    if constexpr (sizeof...(Args) == 0)
        return;

#if DEBUG || _DEBUG
    (std::cout << ... << args);
    std::cout.flush();
#endif
}

template <typename... Args> inline void DebugPrintLine(Args &&...args)
{
    if constexpr (sizeof...(Args) == 0)
        return;

#if DEBUG || _DEBUG
    DebugPrint(std::forward<Args>(args)..., "\n");
#endif
}

template <typename... Args>
inline void Log(const char *prefix, const char *file, unsigned int line, const char *functionName, Args &&...args)
{
    if constexpr (sizeof...(Args) == 0)
        return;

#if DEBUG || _DEBUG
    DebugPrint("[", prefix, "] ", file, ":", line, " (", functionName, ") => ");
    DebugPrint(std::forward<Args>(args)..., "\n");
#endif
}

constexpr const char *GetShortFileName(const char *str, uint32_t len)
{
    const char *result = str;
    for (uint32_t i = len - 1; i >= 0 && i < len; --i)
    {
        if (str[i] == '\\')
        {
            result = str + i + 1;
            break;
        }
    }
    return result;
}

constexpr const char *GetShortFunctionName(const char *str, uint32_t len)
{
    const char *result = str;
    for (uint32_t i = len - 1; i >= 0 && i < len; --i)
    {
        if (str[i] == ':')
        {
            result = str + i + 1;
            break;
        }
    }
    return result;
}

#define SHOWINFO(...)                                                                                                  \
    {                                                                                                                  \
        constexpr auto fileName = GetShortFileName(__FILE__, sizeof(__FILE__) - 1);                                    \
        constexpr auto functionName = GetShortFunctionName(__FUNCTION__, sizeof(__FUNCTION__) - 1);                    \
        Log("  LOG  ", fileName, __LINE__, functionName, __VA_ARGS__);                                                 \
    }

#define SHOWWARNING(...)                                                                                               \
    {                                                                                                                  \
        constexpr auto fileName = GetShortFileName(__FILE__, sizeof(__FILE__) - 1);                                    \
        constexpr auto functionName = GetShortFunctionName(__FUNCTION__, sizeof(__FUNCTION__) - 1);                    \
        Log("WARNING", fileName, __LINE__, functionName, __VA_ARGS__);                                                 \
    }

#define SHOWERROR(...)                                                                                                 \
    {                                                                                                                  \
        constexpr auto fileName = GetShortFileName(__FILE__, sizeof(__FILE__) - 1);                                    \
        constexpr auto functionName = GetShortFunctionName(__FUNCTION__, sizeof(__FUNCTION__) - 1);                    \
        Log(" ERROR ", fileName, __LINE__, functionName, __VA_ARGS__);                                                 \
    }

#define SHOWFATAL(...)                                                                                                 \
    {                                                                                                                  \
        constexpr auto fileName = GetShortFileName(__FILE__, sizeof(__FILE__) - 1);                                    \
        constexpr auto functionName = GetShortFunctionName(__FUNCTION__, sizeof(__FUNCTION__) - 1);                    \
        Log(" FATAL ", fileName, __LINE__, functionName, __VA_ARGS__);                                                 \
    }

#define CHECKNR(cond, ...)                                                                                             \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
        SHOWERROR(__VA_ARGS__);                                                                                        \
    }

#define CHECK_FATAL(cond, ...)                                                                                         \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
        SHOWFATAL(__VA_ARGS__);                                                                                        \
        assert(false);                                                                                                 \
    }

#define CHECK(cond, retVal, ...)                                                                                       \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
        SHOWERROR(__VA_ARGS__);                                                                                        \
        return (retVal);                                                                                               \
    }

#if DEBUG || _DEBUG
#define DCHECKNR(cond, ...)                                                                                            \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
        SHOWERROR(__VA_ARGS__);                                                                                        \
    }

#define DCHECK_FATAL(cond, ...)                                                                                        \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
        SHOWFATAL(__VA_ARGS__);                                                                                        \
        assert(false);                                                                                                 \
    }

#define DCHECK(cond, retVal, ...)                                                                                      \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
        SHOWERROR(__VA_ARGS__);                                                                                        \
        return (retVal);                                                                                               \
    }

#else

#define DCHECKNR(cond, ...)

#define DCHECK_FATAL(cond, ...)

#define DCHECK(cond, retVal, ...)

#endif
