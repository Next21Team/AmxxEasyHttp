#pragma once

namespace ezhttp::trace
{
    void Initialize(const char *path);
    void SetEnabled(bool enabled);
    bool IsEnabled();
    void Shutdown();
    void Writef(const char *component, const char *format, ...);
}
