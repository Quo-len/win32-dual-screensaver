#include "ScreensaverRegistry.h"
#include <algorithm>
#include <cstring>
#include <cwctype>

const WCHAR* g_modeNames[64] = { nullptr };
int g_numScreensavers = 0;

namespace ScreensaverRegistry {

static std::vector<ScreensaverDef>& GetInternalRegistry() {
    static std::vector<ScreensaverDef> registry;
    return registry;
}

void Register(const ScreensaverDef& def) {
    auto& list = GetInternalRegistry();
    list.push_back(def);
    if (def.id >= 0 && def.id < 64) {
        g_modeNames[def.id] = def.name;
    }
    if (def.id + 1 > g_numScreensavers) {
        g_numScreensavers = def.id + 1;
    }
}

const std::vector<ScreensaverDef>& GetAll() {
    auto& list = GetInternalRegistry();
    static size_t lastSortedCount = 0;
    if (list.size() != lastSortedCount) {
        std::sort(list.begin(), list.end(), [](const ScreensaverDef& a, const ScreensaverDef& b) {
            return a.id < b.id;
        });
        lastSortedCount = list.size();
    }
    return list;
}

int GetCount() {
    return (int)GetAll().size();
}

const ScreensaverDef* GetById(int id) {
    const auto& list = GetAll();
    for (const auto& item : list) {
        if (item.id == id) return &item;
    }
    return nullptr;
}

const ScreensaverDef* FindByAlias(const wchar_t* alias) {
    if (!alias || !alias[0]) return nullptr;
    char buffer[128];
    int len = WideCharToMultiByte(CP_UTF8, 0, alias, -1, buffer, sizeof(buffer), NULL, NULL);
    if (len <= 0) return nullptr;
    return FindByAlias(buffer);
}

const ScreensaverDef* FindByAlias(const char* alias) {
    if (!alias || !alias[0]) return nullptr;
    const auto& list = GetAll();
    for (const auto& item : list) {
        if (_stricmp(alias, item.codeName) == 0) return &item;
        for (const auto& a : item.aliases) {
            if (_stricmp(alias, a.c_str()) == 0) return &item;
        }
    }
    return nullptr;
}

void Execute(int id, const RenderContext& ctx) {
    const auto* def = GetById(id);
    if (def && def->render) {
        def->render(ctx);
    }
}

} // namespace ScreensaverRegistry

