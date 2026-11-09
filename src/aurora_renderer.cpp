#include "aurora_renderer.h"
#include "aurora_conditions.h"
#include "aurora_ribbon.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <mod/logger.h>
#include <plugin-sdk/game_sa/entity/Camera.h>
#include <plugin-sdk/game_sa/entity/PlayerPed.h>
#include <plugin-sdk/game_sa/plugin.h>
#include <plugin-sdk/game_sa/utils/TextureUtils.h>
#include <plugin-sdk/renderware/RwCamera.h>
#include <plugin-sdk/renderware/RwFrame.h>

namespace AuroraRenderer {
namespace {
std::vector<AuroraRibbon::Vertex> field;
std::vector<RwIm3DVertex> drawVertices;
std::vector<uint16_t> indices;
CCamera *camera = nullptr;
bool *cutscene = nullptr;
int32_t *currArea = nullptr;
uint8_t *clockHour = nullptr;
bool initialized = false;
bool textureFailed = false;
RenderStats stats{};
std::chrono::steady_clock::time_point startTime;

const auto FreeTexture = [](RwTexture *tex) {
    if (tex)
        RwTextureDestroy(tex);
};
std::unique_ptr<RwTexture, decltype(FreeTexture)> curtainTexture(nullptr, FreeTexture);

struct SavedRenderState {
    static constexpr std::array<RwRenderState, 14> states{rwRENDERSTATEZWRITEENABLE,
                                                          rwRENDERSTATEZTESTENABLE,
                                                          rwRENDERSTATEFOGENABLE,
                                                          rwRENDERSTATEVERTEXALPHAENABLE,
                                                          rwRENDERSTATESRCBLEND,
                                                          rwRENDERSTATEDESTBLEND,
                                                          rwRENDERSTATECULLMODE,
                                                          rwRENDERSTATETEXTURERASTER,
                                                          rwRENDERSTATETEXTUREFILTER,
                                                          rwRENDERSTATETEXTUREADDRESSU,
                                                          rwRENDERSTATETEXTUREADDRESSV,
                                                          rwRENDERSTATEALPHATESTFUNCTION,
                                                          rwRENDERSTATEALPHATESTFUNCTIONREF,
                                                          rwRENDERSTATESHADEMODE};
    std::array<uintptr_t, states.size()> values{};
    SavedRenderState() {
        for (size_t i = 0; i < states.size(); ++i)
            RwRenderStateGet(states[i], &values[i]);
    }
    ~SavedRenderState() {
        for (size_t i = 0; i < states.size(); ++i)
            RwRenderStateSet(states[i], reinterpret_cast<void *>(values[i]));
    }
};

void SetState(RwRenderState key, uintptr_t value) { RwRenderStateSet(key, reinterpret_cast<void *>(value)); }

bool EnsureTexture() {
    if (curtainTexture)
        return true;
    if (textureFailed)
        return false;
    if (!RwImageCreate || !RwImageAllocatePixels || !RwImageDestroy || !RwImageFindRasterFormat || !RwRasterCreate ||
        !RwRasterSetFromImage || !RwRasterDestroy || !RwTextureCreate || !RwTextureDestroy || !RwTextureSetName) {
        textureFailed = true;
        logger->Error("Aurora: required texture upload symbol is missing");
        return false;
    }
    const auto freeImage = [](RwImage *image) {
        if (image)
            RwImageDestroy(image);
    };
    std::unique_ptr<RwImage, decltype(freeImage)> image(
        RwImageCreate(AuroraRibbon::TextureWidth, AuroraRibbon::TextureHeight, 32), freeImage);
    if (!image || !RwImageAllocatePixels(image.get()) || !image->cpPixels ||
        image->stride < AuroraRibbon::TextureWidth * 4) {
        textureFailed = true;
        logger->Error("Aurora: unable to allocate curtain image");
        return false;
    }
    std::vector<AuroraRibbon::Pixel> pixels;
    AuroraRibbon::BuildCurtainPixels(pixels);
    for (int y = 0; y < AuroraRibbon::TextureHeight; ++y)
        std::memcpy(image->cpPixels + y * image->stride, pixels.data() + y * AuroraRibbon::TextureWidth,
                    AuroraRibbon::TextureWidth * 4);
    curtainTexture.reset(TextureUtils::CreateFromImage(image.get(), "aurora_filaments"));
    textureFailed = !curtainTexture;
    if (textureFailed)
        logger->Error("Aurora: curtain texture upload failed");
    return curtainTexture != nullptr;
}
} // namespace

void Init() {
    if (initialized)
        return;
    AuroraConditions::Init();
    camera = GetMainLibrarySymbol<CCamera *>("TheCamera");
    cutscene = GetMainLibrarySymbol<bool *>("_ZN12CCutsceneMgr10ms_runningE");
    currArea = GetMainLibrarySymbol<int32_t *>("_ZN5CGame8currAreaE");
    clockHour = GetMainLibrarySymbol<uint8_t *>("_ZN6CClock18ms_nGameClockHoursE");
    constexpr size_t maxVertices = 3 * (128 * 4 + 1) * 9;
    field.reserve(maxVertices);
    drawVertices.reserve(maxVertices);
    indices.reserve(3 * 128 * 4 * 8 * 6);
    startTime = std::chrono::steady_clock::now();
    initialized = true;
}

const RenderStats &GetStats() { return stats; }

void Shutdown() {
    curtainTexture.reset(); // Release GPU memory before CGame shuts RenderWare down.
    textureFailed = false;
    stats.hasRendered = false;
}

void Render(const AuroraConfig &cfg) {
    Init();
    stats.hasRendered = false;
    stats.shouldRenderResult = false;
    stats.vertexCount = stats.indexCount = 0;
    stats.currArea = currArea ? *currArea : -1;
    stats.gameHour = clockHour ? *clockHour : -1;
    if (!FindPlayerPed || !FindPlayerPed(-1)) {
        stats.blockReason = "Player not loaded";
        return;
    }
    if (cutscene && *cutscene) {
        stats.blockReason = "Cutscene";
        return;
    }
    // CPlaceable::GetPosition can describe the camera's target; use the actual eye frame.
    auto *rwCamera = camera ? camera->m_pRwCamera : nullptr;
    auto *frame = rwCamera ? RwCameraGetFrame(rwCamera) : nullptr;
    auto *matrix = frame && RwFrameGetLTM ? RwFrameGetLTM(frame) : nullptr;
    if (!matrix || !std::isfinite(matrix->pos.x) || !std::isfinite(matrix->pos.y) || !std::isfinite(matrix->pos.z) ||
        !std::isfinite(rwCamera->farPlane) || rwCamera->farPlane <= 5) {
        stats.blockReason = "Scene camera unavailable";
        return;
    }
    const AuroraRibbon::Point eye{matrix->pos.x, matrix->pos.y, matrix->pos.z};
    float fade = 1.0f;
    stats.shouldRenderResult = AuroraConditions::ShouldRender(cfg, eye.x, eye.y, eye.z, stats.currArea, fade);
    stats.alphaMult = fade;
    stats.blockReason = AuroraConditions::GetLastBlockReason();
    if (!stats.shouldRenderResult || fade <= 0.001f)
        return;
    if (!RwRenderStateGet || !RwRenderStateSet || !RwIm3DTransform || !RwIm3DRenderIndexedPrimitive || !RwIm3DEnd) {
        stats.blockReason = "Required RenderWare draw symbol missing";
        return;
    }
    if (!EnsureTexture()) {
        stats.blockReason = "Curtain texture upload failed (see AML log)";
        return;
    }
    AuroraConfig renderCfg = cfg;
    renderCfg.intensity *= fade;
    const float seconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();
    AuroraRibbon::BuildMesh(renderCfg, seconds, field, indices);
    const float scale = AuroraRibbon::SkyScale(field, eye, rwCamera->farPlane);
    drawVertices.resize(field.size());
    for (size_t i = 0; i < field.size(); ++i) {
        const auto &v = field[i];
        const auto p = AuroraRibbon::ProjectToSky(v.position, eye, scale);
        auto &rw = drawVertices[i];
        rw.x = p.x;
        rw.y = p.y;
        rw.z = p.z;
        rw.normal = {0, 0, 1};
        rw.r = v.color[0];
        rw.g = v.color[1];
        rw.b = v.color[2];
        rw.a = v.color[3];
        rw.u = v.u;
        rw.v = v.v;
    }
    stats.vertexCount = drawVertices.size();
    stats.indexCount = indices.size();

    SavedRenderState saved;
    // This draw is in the sky pass, before terrain/buildings; never write their depth buffer.
    SetState(rwRENDERSTATEZWRITEENABLE, false);
    SetState(rwRENDERSTATEZTESTENABLE, false);
    SetState(rwRENDERSTATEFOGENABLE, false);
    SetState(rwRENDERSTATEVERTEXALPHAENABLE, true);
    SetState(rwRENDERSTATESRCBLEND, rwBLENDSRCALPHA);
    SetState(rwRENDERSTATEDESTBLEND, rwBLENDONE);
    SetState(rwRENDERSTATECULLMODE, rwCULLMODECULLNONE);
    SetState(rwRENDERSTATESHADEMODE, rwSHADEMODEGOURAUD);
    SetState(rwRENDERSTATETEXTUREFILTER, rwFILTERLINEAR);
    SetState(rwRENDERSTATETEXTUREADDRESSU, rwTEXTUREADDRESSWRAP);
    SetState(rwRENDERSTATETEXTUREADDRESSV, rwTEXTUREADDRESSCLAMP);
    SetState(rwRENDERSTATEALPHATESTFUNCTION, rwALPHATESTFUNCTIONALWAYS);
    SetState(rwRENDERSTATEALPHATESTFUNCTIONREF, 0);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, curtainTexture->raster);
    constexpr auto flags = rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA | rwIM3D_VERTEXUV;
    if (!RwIm3DTransform(drawVertices.data(), static_cast<RwUInt32>(drawVertices.size()), nullptr, flags)) {
        stats.blockReason = "RwIm3DTransform failed";
        return;
    }
    const bool submitted =
        RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, indices.data(), static_cast<RwInt32>(indices.size()));
    const bool ended = RwIm3DEnd();
    stats.hasRendered = submitted && ended;
    stats.blockReason = stats.hasRendered ? "Draw submitted" : "RenderWare draw failed";
    if (stats.hasRendered)
        ++stats.frameCount;
}
} // namespace AuroraRenderer
