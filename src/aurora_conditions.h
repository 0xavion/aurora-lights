#pragma once

#include "aurora_config.h"

namespace AuroraConditions {

/**
 * Initializes symbols needed for condition checking.
 */
void Init();

/**
 * Returns a human-readable string explaining why the aurora is or isn't rendering.
 */
const char *GetLastBlockReason();

/**
 * Determines whether the Aurora curtain should be rendered.
 * @param cfg The active aurora configuration.
 * @param camX Current camera/player X.
 * @param camY Current camera/player Y.
 * @param camZ Current camera/player Z.
 * @param areaCode Game interior area code (0 = outside).
 * @param outAlphaMult Output scalar multiplier for smooth fade transitions.
 * @return True if aurora should be rendered.
 */
bool ShouldRender(const AuroraConfig &cfg, float camX, float camY, float camZ, int areaCode, float &outAlphaMult);

} // namespace AuroraConditions
