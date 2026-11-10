#include "aurora_blur.h"
#include <cmath>

namespace AuroraBlur {
namespace {
constexpr int Width = AuroraRibbon::TextureWidth, Height = AuroraRibbon::TextureHeight;

void Horizontal(const std::vector<float> &source, std::vector<float> &target, int radius) {
    const float weight = 1.0f / (radius * 2 + 1);
    for (int y = 0; y < Height; ++y) {
        const int row = y * Width;
        float sum = 0;
        for (int i = -radius; i <= radius; ++i)
            sum += source[row + (i + Width) % Width];
        for (int x = 0; x < Width; ++x) {
            target[row + x] = sum * weight;
            sum -= source[row + (x - radius + Width) % Width];
            sum += source[row + (x + radius + 1) % Width];
        }
    }
}

void Vertical(const std::vector<float> &source, std::vector<float> &target, int radius) {
    const float weight = 1.0f / (radius * 2 + 1);
    for (int x = 0; x < Width; ++x) {
        float sum = 0;
        for (int i = 0; i <= radius; ++i)
            sum += source[i * Width + x];
        for (int y = 0; y < Height; ++y) {
            target[y * Width + x] = sum * weight;
            if (y - radius >= 0)
                sum -= source[(y - radius) * Width + x];
            if (y + radius + 1 < Height)
                sum += source[(y + radius + 1) * Width + x];
        }
    }
}
} // namespace

bool EmissionCache::Set(float amount) {
    const int requested =
        static_cast<int>(std::lround((std::isfinite(amount) ? std::clamp(amount, 0.0f, 1.0f) : 0) * 12));
    if (requested == radius)
        return false;
    if (sharp.empty())
        AuroraRibbon::BuildCurtainPixels(sharp);
    pixels = sharp;
    if (requested > 0) {
        a.resize(sharp.size());
        b.resize(sharp.size());
        for (size_t i = 0; i < sharp.size(); ++i)
            a[i] = sharp[i][3];
        // Three separable box filters approximate a Gaussian in O(texture size).
        for (int pass = 0; pass < 3; ++pass) {
            Horizontal(a, b, requested);
            Vertical(b, a, std::max(1, requested / 2));
        }
        for (int y = 0; y < Height; ++y) {
            const float edge = std::min(1.0f, std::min(y, Height - 1 - y) / 3.0f);
            for (int x = 0; x < Width; ++x)
                pixels[y * Width + x][3] = static_cast<uint8_t>(std::clamp(a[y * Width + x] * edge, 0.0f, 255.0f));
        }
    }
    radius = requested;
    pending = true;
    return true;
}
} // namespace AuroraBlur
