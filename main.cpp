#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>
#include <plugin-sdk/game_sa/Events.h>
#include <plugin-sdk/renderware/imgui/imgui_impl_renderware.h>

#include "src/aurora_config.h"
#include "src/aurora_menu.h"
#include "src/aurora_renderer.h"
#include "src/aurora_lighting.h"

MYMOD(xavion.auroralights, AuroraLights, 1.0, Xavion)

namespace {

Config g_AmlConfig("AuroraLights");
AuroraConfig g_AuroraConfig;

} // namespace

ON_MOD_LOAD() {
    logger->SetTag("AuroraLights");

    uintptr_t pGameLib = aml->GetLib("libGTASA.so");
    void *hGameHandle = aml->GetLibHandle("libGTASA.so");

    if (!pGameLib || !hGameHandle) {
        logger->Error("Cannot locate libGTASA.so; AuroraLights disabled.");
        return;
    }

    // Determine version and ABI
#ifdef AML32
    const uint32_t verSig = *reinterpret_cast<uint32_t *>(pGameLib + 0x202020);
    if (verSig == 0xE8BDB001) {
        logger->Info("Detected GTA San Andreas v2.00 (ARM32 / armeabi-v7a)");
    } else if (verSig == 0x61766E49) {
        logger->Info("Detected GTA San Andreas v2.10 (ARM32 / armeabi-v7a)");
    } else {
        logger->Info("Running on unknown/modified ARM32 build (0x%08X); initializing via symbol lookups.", verSig);
    }
#else
    logger->Info("Detected GTA San Andreas v2.10 (ARM64 / arm64-v8a)");
#endif

    // Load configuration from AuroraLights.ini
    AuroraConfig_Load(g_AuroraConfig, &g_AmlConfig);
    AuroraConfig_Save(g_AuroraConfig, &g_AmlConfig);

    AuroraRenderer::Init();
    AuroraLighting::Init(&g_AuroraConfig);
    AuroraMenu::Init(&g_AmlConfig);

    // 3D Scene Rendering: Render the Aurora Borealis in the 3D sky
    Events::cloudsRenderEvent.after += []() { AuroraRenderer::Render(g_AuroraConfig); };
    Events::shutdownRwEvent += AuroraRenderer::Shutdown;

    // The backend hooks touch, HUD and pause-menu rendering; touches outside the menu windows reach the game.
    if (!ImGui_ImplRenderWare_Init([] { AuroraMenu::Draw(g_AuroraConfig); }))
        logger->Error("ImGui init failed; the Aurora menu is unavailable.");

    logger->Info("Aurora Lights mod loaded successfully!");
}
