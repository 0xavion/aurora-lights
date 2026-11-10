#include "aurora_scene.h"
#include <cmath>

namespace AuroraScene {
namespace {
constexpr float Pi = 3.14159265358979323846f;
float Unit(float x) { return std::isfinite(x) ? std::clamp(x, 0.0f, 1.0f) : 0.0f; }
struct Random {
    uint32_t state;
    float Next() {
        state += 0x9e3779b9u;
        uint32_t h = state;
        h = (h ^ (h >> 16)) * 0x7feb352du;
        h = (h ^ (h >> 15)) * 0x846ca68bu;
        return static_cast<float>((h ^ (h >> 16)) >> 8) / 16777216.0f;
    }
    float Range(float a, float b) { return a + (b - a) * Next(); }
};
float AddChannel(float base, float amount) {
    // Preserve an existing HDR ambient value supplied by the game or another mod.
    return base + std::min(Unit(amount), std::max(0.0f, 1 - base));
}
Color Add(Color a, Color b) { return {AddChannel(a.r, b.r), AddChannel(a.g, b.g), AddChannel(a.b, b.b)}; }
} // namespace

int NightKey(int month, int day, int hour) {
    constexpr int days[]{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    month = std::clamp(month, 1, 12);
    day = std::clamp(day, 1, days[month - 1]);
    int ordinal = day - 1;
    for (int m = 1; m < month; ++m)
        ordinal += days[m - 1];
    return (ordinal + 365 - (hour < 12 ? 1 : 0)) % 365;
}

Layout MakeLayout(int seed, int night, int count) {
    Random rng{static_cast<uint32_t>(seed) ^ (static_cast<uint32_t>(night) * 0x85ebca6bu)};
    Layout layout;
    layout.count = std::clamp(count, 1, MaxPatches);
    const float rotation = rng.Range(0, 2 * Pi);
    for (int i = 0; i < layout.count; ++i) {
        auto &p = layout.patches[i];
        const float angle = rotation + (i + rng.Range(-0.16f, 0.16f)) * 2 * Pi / layout.count;
        const float distance = rng.Range(2800, 6200);
        p.center = {std::cos(angle) * distance, std::sin(angle) * distance, rng.Range(3400, 4600)};
        p.heading = angle + Pi * 0.5f + rng.Range(-0.6f, 0.6f);
        p.length = rng.Range(4000, 8000);
        p.bend = rng.Range(450, 1400);
        p.height = rng.Range(0.65f, 1.20f);
        p.speed = rng.Range(0.55f, 1.25f);
        p.phase = rng.Range(0, 2 * Pi);
        p.brightness = (i == 0 ? 1.0f : rng.Range(0.40f, 0.75f));
        p.hue = rng.Range(-0.12f, 0.12f);
    }
    return layout;
}

float Pulse(const Patch &patch, float seconds, float speed) {
    const float time = std::isfinite(seconds) ? seconds : 0;
    speed = std::isfinite(speed) ? std::clamp(speed, 0.0f, 5.0f) : 1;
    return 0.78f + 0.22f * std::sin(time * speed * patch.speed * 0.11f + patch.phase);
}

void NightTransition::Update(const AuroraConfig &cfg, int night, float seconds) {
    const int count = std::clamp(cfg.patchCount, 1, MaxPatches);
    if (ready && cfg.placementSeed == lastSeed && night == lastNight && count == lastCount)
        return;
    // Finish the current crossfade; the latest requested settings are reconsidered next frame.
    if (ready && Blend(seconds) < 1)
        return;
    previous = ready ? current : Layout{};
    current = MakeLayout(cfg.placementSeed, night, count);
    changedAt = std::isfinite(seconds) ? seconds : 0;
    lastSeed = cfg.placementSeed;
    lastNight = night;
    lastCount = count;
    ready = true;
}

float NightTransition::Blend(float seconds) const {
    const float t = Unit((seconds - changedAt) / TransitionSeconds);
    return t * t * (3 - 2 * t);
}

Color EnvironmentLight(const AuroraConfig &cfg, const Layout &layout, AuroraRibbon::Point observer, float seconds) {
    if (!cfg.enabled || !cfg.enableEnvironmentLight || !std::isfinite(observer.x) || !std::isfinite(observer.y))
        return {};
    const float scale = std::isfinite(cfg.curtainRadius) ? std::clamp(cfg.curtainRadius / 550, 0.18f, 2.73f) : 1;
    const float variation = Unit(cfg.variationStrength);
    float contribution = 0;
    for (int i = 0; i < std::clamp(layout.count, 0, MaxPatches); ++i) {
        const auto &p = layout.patches[i];
        const float x = observer.x - p.center.x * scale, y = observer.y - p.center.y * scale;
        const float distance = std::sqrt(x * x + y * y);
        const float coverage = Unit(1 - distance / (p.length * scale));
        contribution +=
            coverage * coverage * p.brightness * (1 - variation + variation * Pulse(p, seconds, cfg.waveSpeed));
    }
    const float strength =
        std::min(0.35f, contribution * Unit(cfg.environmentLightStrength) *
                            (std::isfinite(cfg.intensity) ? std::clamp(cfg.intensity, 0.0f, 2.5f) : 0));
    return {strength * cfg.baseColorR / 255, strength * cfg.baseColorG / 255, strength * cfg.baseColorB / 255};
}

Color Mix(Color a, Color b, float t) {
    t = Unit(t);
    return {a.r * (1 - t) + b.r * t, a.g * (1 - t) + b.g * t, a.b * (1 - t) + b.b * t};
}

void AmbientOverride::Apply(AmbientState &state, Color contribution) {
    Restore(state);
    if (contribution.r == 0 && contribution.g == 0 && contribution.b == 0)
        return;
    original = state;
    state = {Add(state.world, contribution), Add(state.objects, contribution), Add(state.light, contribution)};
    active = true;
}

void AmbientOverride::Restore(AmbientState &state) {
    if (active) {
        state = original;
        active = false;
    }
}
} // namespace AuroraScene
