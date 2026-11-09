#pragma once

#include "aurora_config.h"
#include <array>
#include <vector>

namespace AuroraRibbon {

struct Point {
    float x, y, z;
};
using Pixel = std::array<uint8_t, 4>;
struct Vertex {
    Point position;
    Pixel color;
    float u, v;
};

constexpr int TextureWidth = 1024;
constexpr int TextureHeight = 128;

// Camera-independent world field: looking around never moves the aurora itself.
void BuildMesh(const AuroraConfig &cfg, float timeSec, std::vector<Vertex> &vertices, std::vector<uint16_t> &indices);
void BuildCurtainPixels(std::vector<Pixel> &pixels);
float SkyScale(const std::vector<Vertex> &field, Point eye, float farClip);
Point ProjectToSky(Point world, Point eye, float scale);

} // namespace AuroraRibbon
