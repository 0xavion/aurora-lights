#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>
#include <plugin-sdk/game_sa/Events.h>
#include <plugin-sdk/game_sa/engine/RsGlobal.h>
#include <plugin-sdk/game_sa/entity/PlayerPed.h>

#include "src/aurora_config.h"
#include "src/aurora_menu.h"
#include "src/aurora_renderer.h"

MYMOD(xavion.auroralights, AuroraLights, 1.0, Xavion)

namespace {

Config g_AmlConfig("AuroraLights");
AuroraConfig g_AuroraConfig;

RsGlobalType *g_pScreen = nullptr;
int (*g_pWindowWidth)() = nullptr;
int (*g_pWindowHeight)() = nullptr;
void (*g_pOriginalTouch)(int, int, int, int) = nullptr;
void (*g_pOriginalMobileMenuRender)(void *) = nullptr;
void (*g_pOriginalShutdown)() = nullptr;

void Render2DUI() {
    auto *player = FindPlayerPed ? FindPlayerPed(-1) : nullptr;
    if (!player)
        return;

    int scrW = (g_pScreen && g_pScreen->maximumWidth > 0) ? g_pScreen->maximumWidth : 0;
    int scrH = (g_pScreen && g_pScreen->maximumHeight > 0) ? g_pScreen->maximumHeight : 0;
    if (scrW <= 0 && g_pWindowWidth)
        scrW = g_pWindowWidth();
    if (scrH <= 0 && g_pWindowHeight)
        scrH = g_pWindowHeight();

    if (scrW > 0 && scrH > 0) {
        AuroraMenu::Render(g_AuroraConfig, scrW, scrH);
    }
}

void MobileMenuRenderHook(void *menu) {
    if (g_pOriginalMobileMenuRender) {
        g_pOriginalMobileMenuRender(menu);
    }

    int scrW = (g_pScreen && g_pScreen->maximumWidth > 0) ? g_pScreen->maximumWidth : 0;
    int scrH = (g_pScreen && g_pScreen->maximumHeight > 0) ? g_pScreen->maximumHeight : 0;
    if (scrW <= 0 && g_pWindowWidth)
        scrW = g_pWindowWidth();
    if (scrH <= 0 && g_pWindowHeight)
        scrH = g_pWindowHeight();

    if (scrW > 0 && scrH > 0) {
        AuroraMenu::Render(g_AuroraConfig, scrW, scrH);
    }
}

void TouchHook(int type, int finger, int x, int y) {
    const int winW = g_pWindowWidth ? g_pWindowWidth() : 0;
    const int winH = g_pWindowHeight ? g_pWindowHeight() : 0;

    if (!AuroraMenu::ProcessTouch(type, finger, x, y, winW, winH)) {
        if (g_pOriginalTouch) {
            g_pOriginalTouch(type, finger, x, y);
        }
    }
}

void ShutdownHook() {
    AuroraRenderer::Shutdown();
    AuroraMenu::Shutdown();
    if (g_pOriginalShutdown) {
        g_pOriginalShutdown();
    }
}

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

    g_pScreen = GetMainLibrarySymbol<RsGlobalType *>("RsGlobal");
    g_pWindowWidth = GetMainLibrarySymbol<int (*)()>("_Z17OS_ScreenGetWidthv");
    g_pWindowHeight = GetMainLibrarySymbol<int (*)()>("_Z18OS_ScreenGetHeightv");
    const auto touch = GetMainLibrarySymbol<void *>("_Z14AND_TouchEventiiii");
    const auto shutdown = GetMainLibrarySymbol<void *>("_ZN5CGame18ShutdownRenderWareEv");

    if (!touch || !shutdown) {
        logger->Error("Required game touch/shutdown symbols missing; AuroraLights disabled.");
        return;
    }

    AuroraRenderer::Init();
    AuroraMenu::Init(&g_AmlConfig);

    // 3D Scene Rendering: Render the Aurora Borealis in the 3D sky
    Events::cloudsRenderEvent.after += []() { AuroraRenderer::Render(g_AuroraConfig); };

    // 2D HUD / ImGui Rendering: Render the touch menu and launcher button
    Events::drawingEvent.after += Render2DUI;

    const auto mobileMenuRender = GetMainLibrarySymbol<void *>("_ZN10MobileMenu6RenderEv");
    if (mobileMenuRender) {
        aml->Hook(mobileMenuRender, reinterpret_cast<void *>(&MobileMenuRenderHook),
                  reinterpret_cast<void **>(&g_pOriginalMobileMenuRender));
    }

    if (!aml->Hook(shutdown, reinterpret_cast<void *>(&ShutdownHook),
                   reinterpret_cast<void **>(&g_pOriginalShutdown)) ||
        !aml->Hook(touch, reinterpret_cast<void *>(&TouchHook), reinterpret_cast<void **>(&g_pOriginalTouch))) {
        logger->Error("Failed to install touch/shutdown hooks for AuroraLights.");
        return;
    }

    logger->Info("Aurora Lights mod loaded successfully!");
}
