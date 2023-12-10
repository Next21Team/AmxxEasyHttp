#pragma once

#include <amxxmodule.h>

namespace utils
{
    // Uses the MF_SetAmxStringUTF8Char method if available, otherwise MF_SetAmxString
    int SetAmxStringUTF8CharSafe(AMX* amx, cell amx_addr, const char* source, size_t sourcelen, int maxlen);
}