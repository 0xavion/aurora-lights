#include "aurora_ribbon.h"
#include "aurora_scene.h"
#include <cmath>

namespace AuroraRibbon {
namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr int Rows = 9;

float Smooth(float lo, float hi, float x) {
    const float t = std::clamp((x - lo) / (hi - lo), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float Bounded(float x, float lo, float hi, float fallback) {
    return std::isfinite(x) ? std::clamp(x, lo, hi) : fallback;
}

// Periodic value noise for a seamless curtain texture; never touches the game's RNG.
float Noise(float u, int cells, uint32_t seed) {
    const float p = u * cells;
    const int i = static_cast<int>(std::floor(p));
    const auto value = [cells, seed](int cell) {
        uint32_t h = static_cast<uint32_t>((cell % cells + cells) % cells) + seed;
        h ^= h >> 16;
        h *= 0x7feb352du;
        h ^= h >> 15;
        h *= 0x846ca68bu;
        h ^= h >> 16;
        return static_cast<float>(h & 65535u) / 65535.0f;
    };
    const float t = Smooth(0.0f, 1.0f, p - std::floor(p));
    return value(i) * (1.0f - t) + value(i + 1) * t;
}

uint8_t Byte(float x) { return static_cast<uint8_t>(std::clamp(x, 0.0f, 255.0f)); }
} // namespace

void AppendMesh(const AuroraConfig &cfg, const AuroraScene::Layout &layout, float seconds, float opacity,
                std::vector<Vertex> &vertices, std::vector<uint16_t> &indices) {
    opacity = Bounded(opacity, 0, 1, 0);
    if (opacity <= 0)
        return;
    const int segments = std::clamp(cfg.ribbonSegments, 16, 128) * 4;
    const float scale = Bounded(cfg.curtainRadius, 100, 1500, 550) / 550;
    const float height = Bounded(cfg.curtainHeight, 20.0f, 500.0f, 140.0f) * 28.0f;
    const float speed = Bounded(cfg.waveSpeed, 0.0f, 5.0f, 1.0f);
    const float time = std::isfinite(seconds) ? std::clamp(seconds, -1e7f, 1e7f) * speed : 0;
    const float variation = Bounded(cfg.variationStrength, 0, 1, 0.65f);
    const float intensity = Bounded(cfg.intensity, 0.0f, 2.5f, 1.0f);
    const int count = std::clamp(layout.count, 0, AuroraScene::MaxPatches);
    if (vertices.size() + count * (segments + 1) * Rows >= 65536)
        return;
    vertices.reserve(vertices.size() + count * (segments + 1) * Rows);
    indices.reserve(indices.size() + count * segments * (Rows - 1) * 6);

    for (int layer = 0; layer < count; ++layer) {
        const auto &patch = layout.patches[layer];
        const auto base = static_cast<uint16_t>(vertices.size());
        const float seed = patch.phase;
        const float localSpeed = 1 + (patch.speed - 1) * variation;
        const float phase = time * 0.035f * localSpeed;
        const float hx = std::cos(patch.heading), hy = std::sin(patch.heading);
        const float pulse = 1 - variation + variation * AuroraScene::Pulse(patch, seconds, speed);
        for (int col = 0; col <= segments; ++col) {
            const float u = static_cast<float>(col) / segments;
            const float along = (u - 0.5f) * patch.length * scale;
            const float across =
                patch.bend * scale * (std::sin(u * 3 * Pi + seed + phase) + 0.30f * std::sin(u * 11 * Pi - phase));
            const float bottom =
                patch.center.z + 350 * std::sin(u * 5 * Pi + seed + phase * 0.6f) + 100 * std::sin(u * 17 * Pi + phase);
            const float rayHeight =
                height * (1 + (patch.height - 1) * variation) * (0.75f + 0.35f * Noise(u, 19, 73u + layer));
            const float endFade = Smooth(0, 0.10f, u) * (1.0f - Smooth(0.88f, 1, u));
            const float foldsAlpha = 0.58f + 0.42f * Noise(u + phase * 0.006f, 23, 193u + layer);
            const float travelling =
                1 - variation * 0.35f * (0.5f + 0.5f * std::sin(u * 6 * Pi - time * 0.12f * localSpeed + seed));
            for (int row = 0; row < Rows; ++row) {
                const float v = static_cast<float>(row) / (Rows - 1);
                const float lean = v * v * patch.bend * scale * 0.5f * std::sin(u * 6 * Pi + seed + phase);
                Vertex vert{};
                vert.position = {patch.center.x * scale + hx * along - hy * (across + lean),
                                 patch.center.y * scale + hy * along + hx * (across + lean), bottom + v * rayHeight};
                const float mix = Smooth(0.18f, 0.95f, v);
                const float tint = patch.hue * variation;
                vert.color = {Byte(cfg.baseColorR * (1 - mix) + cfg.topColorR * mix),
                              Byte((cfg.baseColorG * (1 - mix) + cfg.topColorG * mix) * (1 - tint)),
                              Byte((cfg.baseColorB * (1 - mix) + cfg.topColorB * mix) * (1 + tint)),
                              Byte(230.0f * intensity * opacity * endFade * foldsAlpha * pulse * travelling *
                                   patch.brightness * Smooth(0, 0.12f, v) * (1 - Smooth(0.62f, 1, v)))};
                vert.u = u * (2.4f + variation * seed * 0.1f) + seed + time * 0.004f * localSpeed;
                vert.v = v;
                vertices.push_back(vert);
            }
        }
        for (int col = 0; col < segments; ++col) {
            for (int row = 0; row < Rows - 1; ++row) {
                const auto a = static_cast<uint16_t>(base + col * Rows + row);
                const auto b = static_cast<uint16_t>(a + Rows);
                indices.insert(indices.end(), {a, b, static_cast<uint16_t>(a + 1), static_cast<uint16_t>(a + 1), b,
                                               static_cast<uint16_t>(b + 1)});
            }
        }
    }
}

void BuildCurtainPixels(std::vector<Pixel> &pixels) {
    pixels.resize(TextureWidth * TextureHeight);
    for (int x = 0; x < TextureWidth; ++x) {
        const float u = static_cast<float>(x) / (TextureWidth - 1);
        const float broad = Noise(u, 16, 11);
        const float detail = Noise(u, 127, 31);
        const float fine = Noise(u, 397, 79);
        const float rays = 0.13f + 0.55f * detail * detail + 0.65f * std::pow(fine, 4.0f);
        const float base = 0.035f + 0.045f * Noise(u, 37, 101);
        const float reach = 0.48f + 0.48f * Noise(u, 53, 137);
        for (int y = 0; y < TextureHeight; ++y) {
            const float v = static_cast<float>(y) / (TextureHeight - 1);
            const float h = (v - base) / reach;
            const float edge = Smooth(0, 0.10f, h) * (1 - Smooth(0.60f, 1, h));
            const float core = std::exp(-std::pow((h - 0.14f) / 0.16f, 2.0f));
            const float trail = std::exp(-std::max(0.0f, h) * 1.8f);
            const float emission = edge * ((0.28f + 0.46f * broad) * core + rays * trail);
            pixels[y * TextureWidth + x] = {255, 255, 255, Byte(emission * 380)};
        }
    }
}

float SkyScale(const std::vector<Vertex> &field, Point eye, float farClip) {
    float distanceSq = 1.0f;
    for (const auto &v : field) {
        const float x = v.position.x - eye.x, y = v.position.y - eye.y, z = v.position.z - eye.z;
        distanceSq = std::max(distanceSq, x * x + y * y + z * z);
    }
    return std::min(1.0f, farClip * 0.85f / std::sqrt(distanceSq));
}

Point ProjectToSky(Point world, Point eye, float scale) {
    return {eye.x + (world.x - eye.x) * scale, eye.y + (world.y - eye.y) * scale, eye.z + (world.z - eye.z) * scale};
}
} // namespace AuroraRibbon
