#include "aurora_lighting.h"
#include "aurora_renderer.h"
#include "aurora_scene.h"
#include <mod/logger.h>
#include <plugin-sdk/game_sa/Events.h>
#include <plugin-sdk/renderware/RpLight.h>

namespace AuroraLighting {
namespace {
const AuroraConfig *settings = nullptr;
RwRGBAReal *worldAmbient = nullptr, *objectAmbient = nullptr;
RpLight **ambientLight = nullptr;
RpLight *appliedLight = nullptr;
void (*originalSetup)(RpWorld *) = nullptr;
AuroraScene::AmbientOverride overlay;
bool available = false;

AuroraScene::Color Color(const RwRGBAReal &c) { return {c.red, c.green, c.blue}; }
void Write(RwRGBAReal &c, AuroraScene::Color value) {
    c.red = value.r;
    c.green = value.g;
    c.blue = value.b;
}

AuroraScene::AmbientState Read() {
    return {Color(*worldAmbient), Color(*objectAmbient),
            ambientLight && *ambientLight ? Color(*RpLightGetColor(*ambientLight)) : AuroraScene::Color{}};
}

void SetupHook(RpWorld *world) {
    Restore();
    originalSetup(world);
    if (!settings || !settings->enableEnvironmentLight || !ambientLight || !*ambientLight)
        return;
    const auto contribution = AuroraRenderer::EnvironmentContribution(*settings);
    if (contribution.r == 0 && contribution.g == 0 && contribution.b == 0)
        return;
    auto state = Read();
    overlay.Apply(state, contribution);
    appliedLight = *ambientLight;
    Write(*worldAmbient, state.world);
    Write(*objectAmbient, state.objects);
    auto lightColor = *RpLightGetColor(appliedLight);
    Write(lightColor, state.light);
    RpLightSetColor(appliedLight, &lightColor);
}
} // namespace

void Restore() {
    if (!overlay.Active())
        return;
    auto state = Read();
    overlay.Restore(state);
    Write(*worldAmbient, state.world);
    Write(*objectAmbient, state.objects);
    if (ambientLight && *ambientLight && *ambientLight == appliedLight) {
        auto lightColor = *RpLightGetColor(appliedLight);
        Write(lightColor, state.light);
        RpLightSetColor(appliedLight, &lightColor);
    }
    appliedLight = nullptr;
}

bool Available() { return available; }

void Init(const AuroraConfig *cfg) {
    settings = cfg;
    if (available)
        return;
    worldAmbient = GetMainLibrarySymbol<RwRGBAReal *>("AmbientLightColourForFrame");
    objectAmbient = GetMainLibrarySymbol<RwRGBAReal *>("AmbientLightColourForFrame_PedsCarsAndObjects");
    ambientLight = GetMainLibrarySymbol<RpLight **>("pAmbient");
    auto *setup = GetMainLibrarySymbol<void *>("_Z28SetLightsWithTimeOfDayColourP7RpWorld");
    if (!worldAmbient || !objectAmbient || !ambientLight || !setup || !RpLightSetColor) {
        logger->Error("Aurora: environment lighting symbols unavailable; sky effects remain enabled");
        return;
    }
    if (!aml->Hook(setup, reinterpret_cast<void *>(&SetupHook), reinterpret_cast<void **>(&originalSetup))) {
        logger->Error("Aurora: environment lighting hook failed");
        return;
    }
    // Every path restores before UI or world/light destruction, including early frame exits.
    Events::drawingEvent.before += Restore;
    Events::frameEndEvent.before += [](bool) { Restore(); };
    Events::shutdownGameEvent.before += Restore;
    Events::shutdownRwEvent.before += Restore;
    available = true;
}
} // namespace AuroraLighting
