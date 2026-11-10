#pragma once

#include "aurora_ribbon.h"

namespace AuroraScene {
constexpr int MaxPatches = 6;
constexpr float TransitionSeconds = 3.0f;

struct Patch {
    AuroraRibbon::Point center{};
    float heading{}, length{}, bend{}, height{}, speed{}, phase{}, brightness{}, hue{};
};
struct Layout {
    std::array<Patch, MaxPatches> patches{};
    int count{};
};

int NightKey(int month, int day, int hour);
Layout MakeLayout(int seed, int night, int count);
float Pulse(const Patch &patch, float seconds, float speed);

class NightTransition {
  public:
    void Update(const AuroraConfig &cfg, int night, float seconds);
    float Blend(float seconds) const;
    const Layout &Previous() const { return previous; }
    const Layout &Current() const { return current; }

  private:
    Layout previous{}, current{};
    bool ready = false;
    int lastSeed{}, lastNight{}, lastCount{};
    float changedAt{};
};

struct Color {
    float r{}, g{}, b{};
};
Color EnvironmentLight(const AuroraConfig &cfg, const Layout &layout, AuroraRibbon::Point observer, float seconds);
Color Mix(Color a, Color b, float t);
struct AmbientState {
    Color world, objects, light;
};
class AmbientOverride {
  public:
    void Apply(AmbientState &state, Color contribution);
    void Restore(AmbientState &state);
    bool Active() const { return active; }

  private:
    AmbientState original{};
    bool active = false;
};
} // namespace AuroraScene
