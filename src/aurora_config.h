#pragma once

#include <algorithm>
#include <cstdint>

struct AuroraConfig {
    // [General]
    bool enabled;
    bool enableMenu;
    bool showMenuButton;

    // [Conditions]
    bool alwaysActive;
    int startHour;
    int endHour;
    bool allWeathers;
    int weatherFilter;
    int zoneFilter; // 0=All San Andreas, 1=Countryside/Mountains, 2=Desert, 3=Cities

    // [Visuals]
    float intensity;
    float waveSpeed;
    float curtainHeight;
    float curtainRadius;
    int ribbonSegments;
    uint8_t baseColorR;
    uint8_t baseColorG;
    uint8_t baseColorB;
    uint8_t topColorR;
    uint8_t topColorG;
    uint8_t topColorB;
};

class Config;

void AuroraConfig_SetDefaults(AuroraConfig &cfg);
void AuroraConfig_Load(AuroraConfig &cfg, Config *amlCfg);
void AuroraConfig_Save(const AuroraConfig &cfg, Config *amlCfg);
