#pragma once
#include "aurora_config.h"

namespace AuroraLighting {
void Init(const AuroraConfig *cfg);
void Restore();
bool Available();
} // namespace AuroraLighting
