#pragma once
#include "aurora_ribbon.h"

namespace AuroraBlur {
class EmissionCache {
  public:
    bool Set(float amount);
    const std::vector<AuroraRibbon::Pixel> &Pixels() const { return pixels; }
    bool NeedsUpload() const { return pending; }
    void MarkUploaded() { pending = false; }

  private:
    int radius = -1;
    bool pending = false;
    std::vector<AuroraRibbon::Pixel> sharp, pixels;
    std::vector<float> a, b;
};
} // namespace AuroraBlur
