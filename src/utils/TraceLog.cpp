#include "TraceLog.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace ezhttp::trace
{
    namespace
    {
        std::mutex g_trace_mutex;
        std::string g_trace_path;
        bool g_initialized = false;
        bool g_enabled = false;
        bool g_header_written = false;

        unsigned long GetThreadIdForLog()
        {
#ifdef _WIN32
            return GetCurrentThreadId();
#else
            return static_cast<unsigned long>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
        }

        void FillLocalTime(std::time_t now, std::tm &local_tm)
        {
#ifdef _WIN32
            localtime_s(&local_tm, &now);
#else
            localtime_r(&now, &local_tm);
#endif
        }

        void EnsureTraceHeaderLocked()
        {
            if (!g_initialized || !g_enabled || g_trace_path.empty() || g_header_written)
                return;

            std::error_code ec;
            std::filesystem::create_directories(std::filesystem::path(g_trace_path).parent_path(), ec);

            if (FILE *file = std::fopen(g_trace_path.c_str(), "w"))
            {
                std::fputs("==== ezhttp trace start ====\n", file);
                std::fflush(file);
                std::fclose(file);
                g_header_written = true;
            }
        }
    }

    void Initialize(const char *path)
    {
        std::lock_guard lock_guard(g_trace_mutex);

        g_trace_path = path == nullptr ? std::string() : std::string(path);
        g_initialized = !g_trace_path.empty();
        g_header_written = false;

        if (!g_initialized)
            return;

        EnsureTraceHeaderLocked();
    }

    void SetEnabled(bool enabled)
    {
        std::lock_guard lock_guard(g_trace_mutex);
        g_enabled = enabled;
        EnsureTraceHeaderLocked();
    }

    bool IsEnabled()
    {
        std::lock_guard lock_guard(g_trace_mutex);
        return g_enabled;
    }

    void Shutdown()
    {
        std::lock_guard lock_guard(g_trace_mutex);
        g_initialized = false;
        g_enabled = false;
        g_header_written = false;
        g_trace_path.clear();
    }

    void Writef(const char *component, const char *format, ...)
    {
        char message_buffer[1024];
        va_list args;
        va_start(args, format);
        std::vsnprintf(message_buffer, sizeof(message_buffer), format, args);
        va_end(args);

        std::lock_guard lock_guard(g_trace_mutex);
        if (!g_initialized || !g_enabled || g_trace_path.empty())
            return;

        EnsureTraceHeaderLocked();

        FILE *file = std::fopen(g_trace_path.c_str(), "a");
        if (file == nullptr)
            return;

        const auto now = std::chrono::system_clock::now();
        const auto time_t_now = std::chrono::system_clock::to_time_t(now);
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % std::chrono::seconds(1);

        std::tm local_tm{};
        FillLocalTime(time_t_now, local_tm);

        char timestamp_buffer[64];
        std::strftime(timestamp_buffer, sizeof(timestamp_buffer), "%Y-%m-%d %H:%M:%S", &local_tm);

        std::fprintf(
            file,
            "[%s.%03lld][tid=%lu][%s] %s\n",
            timestamp_buffer,
            static_cast<long long>(millis.count()),
            GetThreadIdForLog(),
            component == nullptr ? "trace" : component,
            message_buffer
        );
        std::fflush(file);
        std::fclose(file);
    }
}
