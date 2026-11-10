#pragma once

#include "aurora_config.h"
#include "aurora_scene.h"
#include <cstddef>
#include <cstdint>

namespace AuroraRenderer {

struct RenderStats {
    bool hasRendered;
    uint32_t frameCount;
    size_t vertexCount;
    size_t indexCount;
    int currArea;
    int gameHour;
    bool shouldRenderResult;
    float alphaMult;
    const char *blockReason;
    int nightKey;
    int patchCount;
};

void Init();
void Render(const AuroraConfig &cfg);
void Shutdown();
const RenderStats &GetStats();
AuroraScene::Color EnvironmentContribution(const AuroraConfig &cfg);

} // namespace AuroraRenderer
