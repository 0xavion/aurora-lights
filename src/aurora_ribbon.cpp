#include "aurora_ribbon.h"
#include <cmath>

namespace AuroraRibbon {
namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr int Rows = 9;
constexpr int Layers = 3;

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

void BuildMesh(const AuroraConfig &cfg, float timeSec, std::vector<Vertex> &vertices, std::vector<uint16_t> &indices) {
    vertices.clear();
    indices.clear();
    const int segments = std::clamp(cfg.ribbonSegments, 16, 128) * 4;
    const float radius = Bounded(cfg.curtainRadius, 100.0f, 1500.0f, 550.0f) * 20.0f;
    const float height = Bounded(cfg.curtainHeight, 20.0f, 500.0f, 140.0f) * 28.0f;
    const float speed = Bounded(cfg.waveSpeed, 0.0f, 5.0f, 1.0f);
    const float time = std::isfinite(timeSec) ? timeSec * speed : 0.0f;
    const float phase = time * 0.035f;
    const float intensity = Bounded(cfg.intensity, 0.0f, 2.5f, 1.0f);
    vertices.reserve(Layers * (segments + 1) * Rows);
    indices.reserve(Layers * segments * (Rows - 1) * 6);

    // Fixed San Andreas world coordinates, well above terrain and mountains.
    // Only the render projection is compressed to fit RenderWare's far plane.
    for (int layer = 0; layer < Layers; ++layer) {
        const auto base = static_cast<uint16_t>(vertices.size());
        const float seed = layer * 2.17f;
        const float start = layer == 1 ? 2.75f : (layer == 2 ? -1.0f : -0.45f);
        const float sweep = layer == 2 ? 3.6f : 4.15f;
        const float layerRadius = radius * (layer == 2 ? 0.58f : 1.0f + layer * 0.12f);
        for (int col = 0; col <= segments; ++col) {
            const float u = static_cast<float>(col) / segments;
            const float angle = start + sweep * u;
            const float folds = 0.15f * std::sin(u * 5.0f * Pi + seed + phase) +
                                0.065f * std::sin(u * 13.0f * Pi - seed - phase * 0.7f);
            const float r = layerRadius * (1.0f + folds);
            const float bottom = 4000.0f + layer * 550.0f + 650.0f * std::sin(u * 4.0f * Pi + seed + phase * 0.6f) +
                                 140.0f * std::sin(u * 17.0f * Pi + phase);
            const float rayHeight = height * (0.75f + 0.35f * Noise(u, 19, 73u + layer));
            const float endFade = Smooth(0, 0.10f, u) * (1.0f - Smooth(0.88f, 1, u));
            const float foldsAlpha = 0.58f + 0.42f * Noise(u + phase * 0.006f, 23, 193u + layer);
            for (int row = 0; row < Rows; ++row) {
                const float v = static_cast<float>(row) / (Rows - 1);
                const float lean = v * v * layerRadius * 0.08f * std::sin(u * 6.0f * Pi + seed + phase);
                Vertex vert{};
                vert.position = {(r + lean) * std::cos(angle) + (layer == 2 ? -2000.0f : 0.0f),
                                 (r + lean) * std::sin(angle), bottom + v * rayHeight};
                const float mix = Smooth(0.18f, 0.95f, v);
                vert.color = {Byte(cfg.baseColorR * (1 - mix) + cfg.topColorR * mix),
                              Byte(cfg.baseColorG * (1 - mix) + cfg.topColorG * mix),
                              Byte(cfg.baseColorB * (1 - mix) + cfg.topColorB * mix),
                              Byte(210.0f * intensity * endFade * foldsAlpha * Smooth(0, 0.12f, v) *
                                   (1 - Smooth(0.62f, 1, v)) * (layer == 2 ? 0.60f : 1.0f))};
                vert.u = u * 3.0f + layer * 0.37f + time * 0.004f;
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
