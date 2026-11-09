#include "aurora_conditions.h"
#include <mod/amlmod.h>
#include <plugin-sdk/game_sa/plugin.h>

namespace AuroraConditions {

namespace {

uint8_t *pClockHours = nullptr;
uint8_t *pClockMinutes = nullptr;
int16_t *pOldWeather = nullptr;
int16_t *pNewWeather = nullptr;
const char *s_LastBlockReason = "Uninitialized";

bool CheckZone(int zoneFilter, float x, float y) {
    if (zoneFilter <= 0)
        return true; // 0 = All San Andreas

    const bool inDesert = (y > 600.0f && x < 800.0f);
    const bool inLS = (x >= 300.0f && y <= -500.0f);
    const bool inSF = (x <= -1200.0f && y >= -1000.0f && y <= 1600.0f);
    const bool inLV = (x >= 800.0f && y >= 500.0f);
    const bool inCity = inLS || inSF || inLV;

    switch (zoneFilter) {
    case 1: // Countryside & Mountains (Flint, Whetstone, Red County)
        return !inCity && !inDesert;
    case 2: // Desert & Badlands (Bone County, Tierra Robada)
        return inDesert;
    case 3: // Cities only
        return inCity;
    default:
        return true;
    }
}

} // namespace

void Init() {
    void *hGame = aml->GetLibHandle("libGTASA.so");
    if (!hGame)
        return;

    pClockHours = reinterpret_cast<uint8_t *>(aml->GetSym(hGame, "_ZN6CClock18ms_nGameClockHoursE"));
    pClockMinutes = reinterpret_cast<uint8_t *>(aml->GetSym(hGame, "_ZN6CClock20ms_nGameClockMinutesE"));
    pOldWeather = reinterpret_cast<int16_t *>(aml->GetSym(hGame, "_ZN8CWeather14OldWeatherTypeE"));
    pNewWeather = reinterpret_cast<int16_t *>(aml->GetSym(hGame, "_ZN8CWeather14NewWeatherTypeE"));
}

const char *GetLastBlockReason() { return s_LastBlockReason; }

bool ShouldRender(const AuroraConfig &cfg, float camX, float camY, float camZ, int areaCode, float &outAlphaMult) {
    outAlphaMult = 1.0f;

    // Must be globally enabled
    if (!cfg.enabled) {
        s_LastBlockReason = "Mod is Disabled (Master Enable = OFF)";
        return false;
    }

    // Never render in interiors (house, casino, gym, etc.)
    if (areaCode != 0) {
        s_LastBlockReason = "Player is Inside an Interior (Area != 0)";
        return false;
    }

    // If AlwaysActive is on (for testing / user preference), bypass all conditions
    if (cfg.alwaysActive) {
        s_LastBlockReason = "Always Active 24/7";
        outAlphaMult = 1.0f;
        return true;
    }

    // 1. Time Check via direct memory read of ms_nGameClockHours
    bool inTimeRange = true;
    if (pClockHours) {
        const uint8_t h = *pClockHours;
        if (cfg.startHour <= cfg.endHour) {
            inTimeRange = (h >= cfg.startHour && h < cfg.endHour);
        } else {
            inTimeRange = (h >= cfg.startHour || h < cfg.endHour);
        }
    }

    if (!inTimeRange) {
        s_LastBlockReason = "Outside Time Window (Check Start/End Hour)";
        return false;
    }

    // Smooth twilight fade in/out during twilight transitions
    if (pClockHours && pClockMinutes) {
        const uint8_t h = *pClockHours;
        const uint8_t m = *pClockMinutes;
        if (h == cfg.startHour) {
            outAlphaMult = std::max(0.35f, static_cast<float>(m) / 60.0f);
        } else {
            int prevEndHour = (cfg.endHour - 1 + 24) % 24;
            if (h == prevEndHour) {
                outAlphaMult = std::max(0.35f, 1.0f - (static_cast<float>(m) / 60.0f));
            }
        }
    }

    // 2. Weather Check
    if (!cfg.allWeathers && cfg.weatherFilter >= 0) {
        int currentWeather = -1;
        if (pNewWeather) {
            currentWeather = *pNewWeather;
        } else if (pOldWeather) {
            currentWeather = *pOldWeather;
        }

        if (currentWeather >= 0 && currentWeather != cfg.weatherFilter) {
            s_LastBlockReason = "Weather Filter Mismatch";
            return false;
        }
    }

    // 3. Zone / Geographic Region Check
    if (!CheckZone(cfg.zoneFilter, camX, camY)) {
        s_LastBlockReason = "Zone Filter Mismatch (Set to 'All San Andreas')";
        return false;
    }

    s_LastBlockReason = "Active";
    return true;
}

} // namespace AuroraConditions
