#include "aurora_config.h"
#include <cmath>
#include <mod/config.h>

namespace {

inline float SafeClampFloat(float val, float minVal, float maxVal, float defaultVal) {
    if (!std::isfinite(val))
        return defaultVal;
    return std::clamp(val, minVal, maxVal);
}

} // namespace

void AuroraConfig_SetDefaults(AuroraConfig &cfg) {
    // [General]
    cfg.enabled = true;
    cfg.enableMenu = true;
    cfg.showMenuButton = true;

    // [Conditions]
    cfg.alwaysActive = false;
    cfg.startHour = 21;
    cfg.endHour = 5;
    cfg.allWeathers = true;
    cfg.weatherFilter = -1;
    cfg.zoneFilter = 0;

    // [Visuals]
    cfg.intensity = 1.0f;
    cfg.waveSpeed = 1.0f;
    cfg.curtainHeight = 140.0f;
    cfg.curtainRadius = 550.0f;
    cfg.ribbonSegments = 64;
    cfg.baseColorR = 0;
    cfg.baseColorG = 240;
    cfg.baseColorB = 160;
    cfg.topColorR = 140;
    cfg.topColorG = 50;
    cfg.topColorB = 200;
}

void AuroraConfig_Load(AuroraConfig &cfg, Config *amlCfg) {
    AuroraConfig_SetDefaults(cfg);
    if (!amlCfg)
        return;

    // Make sure ICFG stream is parsed and initialized
    amlCfg->Init();

    // [General]
    cfg.enabled = amlCfg->GetBool("Enable", cfg.enabled, "General");
    cfg.enableMenu = amlCfg->GetBool("EnableMenu", cfg.enableMenu, "General");
    cfg.showMenuButton = amlCfg->GetBool("ShowMenuButton", cfg.showMenuButton, "General");

    // [Conditions]
    cfg.alwaysActive = amlCfg->GetBool("AlwaysActive", cfg.alwaysActive, "Conditions");
    cfg.startHour = std::clamp(amlCfg->GetInt("StartHour", cfg.startHour, "Conditions"), 0, 23);
    cfg.endHour = std::clamp(amlCfg->GetInt("EndHour", cfg.endHour, "Conditions"), 0, 23);
    cfg.allWeathers = amlCfg->GetBool("AllWeathers", cfg.allWeathers, "Conditions");
    cfg.weatherFilter = amlCfg->GetInt("WeatherFilter", cfg.weatherFilter, "Conditions");
    cfg.zoneFilter = std::clamp(amlCfg->GetInt("ZoneFilter", cfg.zoneFilter, "Conditions"), 0, 3);

    // [Visuals]
    cfg.intensity = SafeClampFloat(amlCfg->GetFloat("Intensity", cfg.intensity, "Visuals"), 0.0f, 2.5f, 1.0f);
    cfg.waveSpeed = SafeClampFloat(amlCfg->GetFloat("WaveSpeed", cfg.waveSpeed, "Visuals"), 0.0f, 5.0f, 1.0f);
    cfg.curtainHeight =
        SafeClampFloat(amlCfg->GetFloat("CurtainHeight", cfg.curtainHeight, "Visuals"), 20.0f, 500.0f, 140.0f);
    cfg.curtainRadius =
        SafeClampFloat(amlCfg->GetFloat("CurtainRadius", cfg.curtainRadius, "Visuals"), 100.0f, 1500.0f, 550.0f);
    cfg.ribbonSegments = std::clamp(amlCfg->GetInt("RibbonSegments", cfg.ribbonSegments, "Visuals"), 16, 128);

    cfg.baseColorR = static_cast<uint8_t>(std::clamp(amlCfg->GetInt("BaseColorR", cfg.baseColorR, "Visuals"), 0, 255));
    cfg.baseColorG = static_cast<uint8_t>(std::clamp(amlCfg->GetInt("BaseColorG", cfg.baseColorG, "Visuals"), 0, 255));
    cfg.baseColorB = static_cast<uint8_t>(std::clamp(amlCfg->GetInt("BaseColorB", cfg.baseColorB, "Visuals"), 0, 255));

    cfg.topColorR = static_cast<uint8_t>(std::clamp(amlCfg->GetInt("TopColorR", cfg.topColorR, "Visuals"), 0, 255));
    cfg.topColorG = static_cast<uint8_t>(std::clamp(amlCfg->GetInt("TopColorG", cfg.topColorG, "Visuals"), 0, 255));
    cfg.topColorB = static_cast<uint8_t>(std::clamp(amlCfg->GetInt("TopColorB", cfg.topColorB, "Visuals"), 0, 255));
}

void AuroraConfig_Save(const AuroraConfig &cfg, Config *amlCfg) {
    if (!amlCfg)
        return;

    amlCfg->Init();

    auto saveBool = [&](const char *key, bool val, const char *sec) {
        if (auto *e = amlCfg->Bind(key, val, sec)) {
            e->SetBool(val);
            delete e;
        }
    };
    auto saveInt = [&](const char *key, int val, const char *sec) {
        if (auto *e = amlCfg->Bind(key, val, sec)) {
            e->SetInt(val);
            delete e;
        }
    };
    auto saveFloat = [&](const char *key, float val, const char *sec) {
        if (auto *e = amlCfg->Bind(key, val, sec)) {
            e->SetFloat(val);
            delete e;
        }
    };

    saveBool("Enable", cfg.enabled, "General");
    saveBool("EnableMenu", cfg.enableMenu, "General");
    saveBool("ShowMenuButton", cfg.showMenuButton, "General");

    saveBool("AlwaysActive", cfg.alwaysActive, "Conditions");
    saveInt("StartHour", cfg.startHour, "Conditions");
    saveInt("EndHour", cfg.endHour, "Conditions");
    saveBool("AllWeathers", cfg.allWeathers, "Conditions");
    saveInt("WeatherFilter", cfg.weatherFilter, "Conditions");
    saveInt("ZoneFilter", cfg.zoneFilter, "Conditions");

    saveFloat("Intensity", cfg.intensity, "Visuals");
    saveFloat("WaveSpeed", cfg.waveSpeed, "Visuals");
    saveFloat("CurtainHeight", cfg.curtainHeight, "Visuals");
    saveFloat("CurtainRadius", cfg.curtainRadius, "Visuals");
    saveInt("RibbonSegments", cfg.ribbonSegments, "Visuals");

    saveInt("BaseColorR", cfg.baseColorR, "Visuals");
    saveInt("BaseColorG", cfg.baseColorG, "Visuals");
    saveInt("BaseColorB", cfg.baseColorB, "Visuals");

    saveInt("TopColorR", cfg.topColorR, "Visuals");
    saveInt("TopColorG", cfg.topColorG, "Visuals");
    saveInt("TopColorB", cfg.topColorB, "Visuals");

    amlCfg->Save();
}
