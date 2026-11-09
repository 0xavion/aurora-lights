#pragma once

#include "aurora_config.h"

namespace AuroraMenu {

void Init(class Config *amlConfig);
void Render(AuroraConfig &cfg, int screenWidth, int screenHeight);
bool ProcessTouch(int type, int finger, int x, int y, int winWidth, int winHeight);
bool IsUIVisible();
void Shutdown();

} // namespace AuroraMenu
