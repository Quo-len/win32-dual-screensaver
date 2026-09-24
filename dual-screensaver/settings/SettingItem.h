#pragma once
#include <string>
#include <vector>

enum class SettingType {
    Int,
    Float,
    Bool
};

struct SettingItem {
    const wchar_t* key;        // Registry key name, e.g. L"ASpeed"
    const wchar_t* label;      // UI Label, e.g. L"A Speed:"
    SettingType type;
    void* valPtr;              // Pointer to variable
    double defVal;             // Default value
    double minVal;             // Minimum clamped value
    double maxVal;             // Maximum clamped value
    int precision;             // Decimal places (e.g. 3 for "%.3f", 1 for "%.1f", 0 for int)
    const wchar_t* description = nullptr; // Optional longer description to explain what it does
};
