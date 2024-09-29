#include "amxx_utils.h"

namespace utils
{
    int SetAmxStringUTF8CharSafe(AMX* amx, cell amx_addr, const char* source, size_t sourcelen, int maxlen)
    {
        if (MF_SetAmxStringUTF8Char != nullptr)
            return MF_SetAmxStringUTF8Char(amx, amx_addr, source, sourcelen, maxlen);

        return MF_SetAmxString(amx, amx_addr, source, maxlen);
    }

    std::string GetPluginName(AMX* amx)
    {
        int plugin_id = MF_FindScriptByAmx(amx);
        std::string plugin_name = MF_GetScriptName(plugin_id);

        return plugin_name;
    }
}