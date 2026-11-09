#pragma once

#include "aurora_config.h"

namespace AuroraMenu {

void Init(class Config *amlConfig);
// Body of the ImGui render callback: the launcher button and the controls window.
void Draw(AuroraConfig &cfg);

} // namespace AuroraMenu
