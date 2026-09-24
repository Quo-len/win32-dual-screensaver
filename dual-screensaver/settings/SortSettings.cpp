#include "SortSettings.h"
#include <vector>

constexpr int DEFAULT_SORT_ITEM_COUNT = 60;

int g_SortItemCount = DEFAULT_SORT_ITEM_COUNT;

std::vector<SettingItem> GetSortSettings() {
    return {
        { L"SortItemCount", L"Item Count:", SettingType::Int, &g_SortItemCount, (double)DEFAULT_SORT_ITEM_COUNT, 10, 200, 0 }
    };
}
