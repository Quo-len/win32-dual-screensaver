#include "AsteroidsSettings.h"
#include <vector>

constexpr float DEFAULT_ASTEROIDS_THRUST = 0.16f;
constexpr float DEFAULT_ASTEROIDS_AI_SKILL = 1.0f;
constexpr int   DEFAULT_ASTEROIDS_ROCKS = 4;

float g_AsteroidsShipThrust = DEFAULT_ASTEROIDS_THRUST;
float g_AsteroidsAiSkill    = DEFAULT_ASTEROIDS_AI_SKILL;
int   g_AsteroidsStartRocks = DEFAULT_ASTEROIDS_ROCKS;

std::vector<SettingItem> GetAsteroidsSettings() {
    return {
        { L"AsteroidsRocks",   L"Initial Asteroids:", SettingType::Int,   &g_AsteroidsStartRocks, DEFAULT_ASTEROIDS_ROCKS,    2,    8,    0 },
        { L"AsteroidsThrust",  L"Ship Thrust:",       SettingType::Float, &g_AsteroidsShipThrust, DEFAULT_ASTEROIDS_THRUST,  0.05, 0.40, 2 },
        { L"AsteroidsAiSkill", L"AI Pilot Skill:",    SettingType::Float, &g_AsteroidsAiSkill,    DEFAULT_ASTEROIDS_AI_SKILL, 0.5,  2.0,  1 }
    };
}
