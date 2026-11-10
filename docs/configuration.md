# Configuration

Settings are stored in `AuroraLights.ini` in AML's config directory. Existing settings are retained; missing new keys receive defaults.

## Settings

| Section   | Key                      | Default | Meaning                                                                  |
| --------- | ------------------------ | ------- | ------------------------------------------------------------------------ |
| Placement | Seed                     | 1337    | Deterministic layout seed (0–1,000,000,000).                             |
| Placement | PatchCount               | 3       | Localized aurora groups (1–6).                                           |
| Visuals   | VariationStrength        | 0.65    | Per-group movement, height, brightness and color variation (0–1).        |
| Visuals   | AuroraBlur               | 0.25    | Emission-texture softness (0–1); ImGui displays 0–100%.                  |
| Lighting  | EnableEnvironmentLight   | 1       | Enable regional ambient illumination.                                    |
| Lighting  | EnvironmentLightStrength | 0.12    | Environmental contribution strength (0–1), separate from sky brightness. |

Layouts use fixed world-space centers, never CJ's position or camera rotation. The game calendar selects a layout for each noon-to-noon night: passing midnight preserves placement; the next night changes it. Identical seed and game date reproduce the same placement after restarting. A three-second crossfade introduces a new layout. Further placement requests wait until the ongoing crossfade finishes; the latest values then take effect.

`New Layout` advances the seed. `Aurora Groups`, `Variation`, `Aurora Blur` and environmental lighting can be adjusted in ImGui. Visual changes apply in-game; **Save to AuroraLights.ini** persists them. Replacing a compiled `.so` still requires restarting GTA.

The existing `StartHour`, `EndHour`, `AllWeathers`, `WeatherFilter` and `ZoneFilter` remain appearance filters. Default coverage is all regions and weather, during 21:00–05:00. `AlwaysActive` bypasses time/weather/region filters while keeping the master enable, outdoor and cutscene checks.

Blur affects the aurora's emission texture, not the screen or HUD. Its canonical sharp image is cached; three separable box passes approximate Gaussian softness. GPU textures change only when the quantized blur value changes, at most ten times per second during dragging. Uploads are transactional: a temporary replacement is swapped in only after success, preserving the last good texture on allocation/driver failures and retrying the pending update.

Environment illumination adds a bounded green/cyan ambient contribution based on nearby aurora groups, their brightness and current appearance conditions. World and ped/vehicle/object ambient values are restored before HUD drawing, frame end and shutdown. The integration uses exported game symbols; missing lighting symbols disable illumination without disabling the sky effect. Different game materials can respond differently, so device testing is needed for final strength tuning.
