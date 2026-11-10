# Configuration

Aurora Lights stores its settings in `AuroraLights.ini` in AML's config directory. The plugin creates missing keys at startup and keeps existing valid values.

## Editing and saving

- **In-game:** tap **Aurora** at the bottom-left of the screen. Changes apply live. Open **Settings → Save to AuroraLights.ini** to keep them for the next launch.
- **In a text editor:** fully close GTA, edit the file, then restart the game. There is no live file reload; saving from an open menu can overwrite external edits with the current in-memory settings.
- **Reset:** **Reset to Defaults** changes the current settings immediately. Press **Save to AuroraLights.ini** afterward if you want the reset to persist.

Use `1` for on and `0` for off. Text such as `true` is not supported by the boolean parser. Use a decimal point for fractional values, such as `0.65`, and keep section and key names as shown below.

Numeric values outside the listed ranges are clamped at startup. Non-finite floating-point values fall back to their defaults. Malformed text does not necessarily use the default: the numeric parser can read it as zero, so use valid numbers. The plugin writes the loaded, bounded settings back to the file.

## Default configuration

```ini
[General]
Enable=1
EnableMenu=1
ShowMenuButton=1

[Conditions]
AlwaysActive=0
StartHour=21
EndHour=5
AllWeathers=1
WeatherFilter=-1
ZoneFilter=0

[Visuals]
Intensity=1.0
WaveSpeed=1.0
CurtainHeight=140.0
CurtainRadius=550.0
RibbonSegments=64
BaseColorR=0
BaseColorG=240
BaseColorB=160
TopColorR=140
TopColorG=50
TopColorB=200
VariationStrength=0.65
AuroraBlur=0.25

[Placement]
Seed=1337
PatchCount=3

[Lighting]
EnableEnvironmentLight=1
EnvironmentLightStrength=0.12
```

These defaults show three aurora groups outdoors from 21:00 until before 05:00, with no region or weather restriction.

## General

| Key              | Default | Values     | Effect                                                                                                     |
| ---------------- | ------- | ---------- | ---------------------------------------------------------------------------------------------------------- |
| `Enable`         | `1`     | `0` or `1` | Master switch for the sky effect and its environmental lighting. The menu remains usable when this is off. |
| `EnableMenu`     | `1`     | `0` or `1` | Enables the launcher and control window. Turning this off leaves the aurora controlled by the INI file.    |
| `ShowMenuButton` | `1`     | `0` or `1` | Shows the **Aurora** launcher. Hiding it does not close an already-open control window.                    |

The launcher appears once a player is loaded. An open control window can also remain visible over the pause menu.

If you hide the launcher and then close the window, there is no alternate menu shortcut. Fully close GTA, set `EnableMenu=1` and `ShowMenuButton=1` in the INI file, and restart to restore it.

## Conditions

| Key             | Default | Values                                   | Effect                                                                                                                          |
| --------------- | ------- | ---------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| `AlwaysActive`  | `0`     | `0` or `1`                               | Bypasses the hour, weather, and region filters. Available through **Always Active (24/7 Override)** or the force-aurora button. |
| `StartHour`     | `21`    | Integer, `0`–`23`                        | First eligible in-game hour, included in the window.                                                                            |
| `EndHour`       | `5`     | Integer, `0`–`23`                        | Hour at which the effect stops, excluded from the window.                                                                       |
| `AllWeathers`   | `1`     | `0` or `1`                               | Ignores `WeatherFilter` when on. Turning it off enables the single-ID filter if that ID is nonnegative.                         |
| `WeatherFilter` | `-1`    | Integer; see [weather IDs](#weather-ids) | One required weather ID. Any negative value disables the weather restriction. This key is edited through the INI file.          |
| `ZoneFilter`    | `0`     | Integer, `0`–`3`                         | Restricts appearance by region. Available through **Region / Zone Filter**.                                                     |

### Time and overrides

- `StartHour=21`, `EndHour=5`: 21:00–04:59, crossing midnight.
- `StartHour=8`, `EndHour=18`: 08:00–17:59 on the same day.
- Equal start and end hours produce an empty window. Use `AlwaysActive=1` for 24-hour appearance instead.
- During the first and last eligible hours, brightness ramps with the game minutes, with a 35% minimum. The effect stops when `EndHour` is reached.

`AlwaysActive` still requires `Enable=1`, an outdoor area, a loaded player, and a usable scene camera. The renderer also skips cutscenes. It does not change the game's clock or weather.

### Weather IDs

To require a particular weather, set **both** `AllWeathers=0` and a nonnegative `WeatherFilter`. For example, `AllWeathers=0` with `WeatherFilter=9` permits foggy San Fierro weather. Leaving `WeatherFilter=-1` still permits all weather, even with `AllWeathers=0`.

The filter reads the game's incoming weather ID during transitions, falling back to the old weather ID if the incoming weather symbol is unavailable. It checks the weather type, not the apparent cloud cover. If neither weather symbol is available, this filter does not block rendering.

The built-in IDs are defined in [Weather.h](../plugin-sdk/game_sa/other/Weather.h):

| ID   | Weather type                      |
| ---- | --------------------------------- |
| `0`  | Extra sunny, Los Santos           |
| `1`  | Sunny, Los Santos                 |
| `2`  | Extra sunny with smog, Los Santos |
| `3`  | Sunny with smog, Los Santos       |
| `4`  | Cloudy, Los Santos                |
| `5`  | Sunny, San Fierro                 |
| `6`  | Extra sunny, San Fierro           |
| `7`  | Cloudy, San Fierro                |
| `8`  | Rainy, San Fierro                 |
| `9`  | Foggy, San Fierro                 |
| `10` | Sunny, Las Venturas               |
| `11` | Extra sunny, Las Venturas         |
| `12` | Cloudy, Las Venturas              |
| `13` | Extra sunny, countryside          |
| `14` | Sunny, countryside                |
| `15` | Cloudy, countryside               |
| `16` | Rainy, countryside                |
| `17` | Extra sunny, desert               |
| `18` | Sunny, desert                     |
| `19` | Sandstorm, desert                 |
| `20` | Underwater                        |
| `21` | Extra colours 1                   |
| `22` | Extra colours 2                   |

The location in a weather name describes the weather preset. `ZoneFilter` independently controls where the aurora is allowed. The parser accepts one integer, not a list such as `8,9,16`; an ID that never occurs will keep the aurora hidden while filtering is enabled.

### Region filter

| `ZoneFilter` | Menu selection               | Coverage                                                            |
| ------------ | ---------------------------- | ------------------------------------------------------------------- |
| `0`          | All San Andreas (Everywhere) | No geographic restriction. Interiors remain excluded.               |
| `1`          | Countryside & Mountains      | Areas outside the approximate city and desert boundaries.           |
| `2`          | Desert & Badlands            | Approximate desert region, including Bone County and Tierra Robada. |
| `3`          | Major Cities                 | Approximate Los Santos, San Fierro, and Las Venturas regions.       |

Regions are estimated from camera coordinates using broad rectangular boundaries. They do not follow the game's named-zone borders exactly. The filter controls whether auroras render from that region; it does not move the aurora groups into it.

## Visuals

| Key                 | Default | INI range           | Effect / menu control                                                                                               |
| ------------------- | ------- | ------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `Intensity`         | `1.0`   | `0.0`–`2.5`         | Overall brightness, shown as **Brightness**. Zero also removes the environmental light contribution.                |
| `WaveSpeed`         | `1.0`   | `0.0`–`5.0`         | Speed of curtain movement, texture scrolling, and brightness pulses. **Wave Speed** in the menu goes up to `4.0`.   |
| `CurtainHeight`     | `140.0` | `20.0`–`500.0`      | Vertical curtain extent. The menu range is `40.0`–`400.0`.                                                          |
| `CurtainRadius`     | `550.0` | `100.0`–`1500.0`    | Horizontal scale of group positions and curtain width; `550.0` is the baseline. The menu range is `200.0`–`1200.0`. |
| `RibbonSegments`    | `64`    | Integer, `16`–`128` | Curtain mesh detail. Higher values create more geometry.                                                            |
| `BaseColorR`        | `0`     | Integer, `0`–`255`  | Red channel at the base of the curtain. Part of **Base (Root) Color**.                                              |
| `BaseColorG`        | `240`   | Integer, `0`–`255`  | Green channel at the base.                                                                                          |
| `BaseColorB`        | `160`   | Integer, `0`–`255`  | Blue channel at the base.                                                                                           |
| `TopColorR`         | `140`   | Integer, `0`–`255`  | Red channel toward the top. Part of **Top (Peak) Color**.                                                           |
| `TopColorG`         | `50`    | Integer, `0`–`255`  | Green channel toward the top.                                                                                       |
| `TopColorB`         | `200`   | Integer, `0`–`255`  | Blue channel toward the top.                                                                                        |
| `VariationStrength` | `0.65`  | `0.0`–`1.0`         | Extra differences in group height, speed, tint, and brightness pulses. Shown as **Variation**.                      |
| `AuroraBlur`        | `0.25`  | `0.0`–`1.0`         | Softness of the aurora's emission texture. Shown as **Aurora Blur**, from 0% to 100%.                               |

### Shape, movement, and color

`CurtainHeight` changes the height of the curtain above its lower edge, not the altitude of its group center. `CurtainRadius` scales the horizontal layout around the map origin; it is not a distance from CJ. A larger value widens the curtains and moves their centers farther apart. These are shape controls, not literal measurements in metres.

`WaveSpeed=0` stops the animated movement and pulsing, but date- or seed-driven layout transitions can still occur. `VariationStrength=0` removes the extra height, speed, tint, and pulse variation; groups still retain different positions, shapes, and baseline brightness.

The base and top RGB values form a gradient. The defaults give a green/cyan base and a purple top. Variation can shift each group's tint slightly. Setting both ends to black makes the curtain invisible even with brightness enabled.

### Blur

`AuroraBlur=0` uses the original sharp texture; `1` gives the strongest available smoothing. The menu displays percentages, so 25% is stored as `0.25`. The blur radius is rounded to one of 13 levels, so small slider movements may look identical.

Blur applies to the aurora texture, not the screen, scenery, or HUD. The sharp source is cached, and three separable box-filter passes approximate Gaussian softness. The renderer updates the GPU texture only when the blur level changes, with uploads limited to ten attempts per second. A failed replacement keeps the last working texture and retries later.

## Placement

| Key          | Default | Values                    | Effect / menu control                                                                                             |
| ------------ | ------- | ------------------------- | ----------------------------------------------------------------------------------------------------------------- |
| `Seed`       | `1337`  | Integer, `0`–`1000000000` | Starting seed for deterministic placement. **New Layout** advances it by one, wrapping to zero after the maximum. |
| `PatchCount` | `3`     | Integer, `1`–`6`          | Number of aurora groups, shown as **Aurora Groups**. Higher counts add coverage and geometry.                     |

Groups have fixed world-space centers. They do not follow CJ or rotate with the camera. The seed, group count, and in-game calendar determine their arrangement. Using the same values and game date reproduces that layout after restarting; the animation phase can differ.

A night runs from noon to noon for layout selection. Midnight keeps the existing layout; noon selects the next one. This selection is separate from the appearance hours in `[Conditions]`. In 24/7 mode, you can see the layout change during the day.

Changes to the seed, group count, or night trigger a three-second crossfade. If another change arrives during that fade, the current transition finishes first, then the latest requested values take effect. **New Layout** does not save automatically; use **Save to AuroraLights.ini** to retain the new seed.

## Lighting

| Key                        | Default | Values      | Effect / menu control                                                                                                    |
| -------------------------- | ------- | ----------- | ------------------------------------------------------------------------------------------------------------------------ |
| `EnableEnvironmentLight`   | `1`     | `0` or `1`  | Enables the ambient contribution to scenery, objects, peds, and vehicles. Shown as **Environment Illumination**.         |
| `EnvironmentLightStrength` | `0.12`  | `0.0`–`1.0` | Scales that contribution, shown as **Environment Light Strength**. Zero removes it while leaving the sky effect visible. |

Lighting depends on the camera's distance from each fixed group, the group's brightness, the appearance fade, `Intensity`, and the base RGB color. The strength setting is separate from sky brightness, but it still follows those factors. A black base color contributes no environmental light even if the top color is bright.

The combined contribution is capped at `0.35` per channel before adding it to the existing ambient values. Raising the strength can reach that cap rather than continuing to brighten the scene. Material response varies across game builds and graphics mods; start near the default and adjust in-game.

This uses the game's ambient lighting, without adding point lights or dynamic shadows or editing `timecyc.dat`. The original ambient values are restored before HUD drawing, frame end, and shutdown.

If the required game symbols or lighting hook are unavailable, the menu reports **Environment illumination is unavailable on this game build (see AML log)**. The sky effect can still run.

## In-game controls

| Control                               | What it does                                                                                                             |
| ------------------------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| **Aurora**                            | Opens or closes the control window.                                                                                      |
| **Force Aurora NOW (24/7 Test Mode)** | Turns on `AlwaysActive`. The same button switches back to following the filters when 24/7 mode is active.                |
| **Visuals** tab                       | Master enable, brightness, movement, shape, groups, layout seed, colors, blur, and environmental lighting.               |
| **Conditions** tab                    | 24/7 override, appearance hours, all-weather toggle, and region filter. Edit a specific `WeatherFilter` in the INI file. |
| **Settings** tab                      | Launcher visibility, saving, resetting, and closing the menu.                                                            |
| **Save to AuroraLights.ini**          | Writes the current settings, including the seed and 24/7 override.                                                       |
| **Reset to Defaults**                 | Restores the values in the example above for the current session. Save afterward to keep them.                           |
| **Close Menu**                        | Closes the control window without saving settings.                                                                       |

`EnableMenu` is INI-only. `Seed` is also editable directly in the INI file; the menu changes it through **New Layout**.

The status at the top shows **[DRAW]** when a draw was submitted, or **[IDLE]** with the current block reason. It also shows the night layout and group count. A submitted draw does not guarantee visibility: the groups may be outside the current view, or the chosen brightness or colors may make them invisible.

## Performance and troubleshooting

For less geometry, lower `PatchCount` or `RibbonSegments` first. Each group uses `9 × (4 × RibbonSegments + 1)` vertices. The defaults use 6,939 vertices once the layout settles; a crossfade can draw both layouts at once. Blur is recalculated when its quantized level changes, not on every frame.

| Symptom                                              | What to check                                                                                                                                                                                  |
| ---------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| No aurora appears                                    | Read the menu's **[IDLE] Block Reason**. Check `Enable=1`, go outdoors, and try the 24/7 button. Look around the sky; the groups occupy fixed areas.                                           |
| 24/7 mode still shows nothing                        | Check for an interior, cutscene, unloaded player, missing scene camera, or renderer error. Also check that `Intensity` is above zero and the colors are not black.                             |
| It disappears outside certain hours                  | Check `StartHour` and `EndHour`. Equal values disable the time window; `AlwaysActive=1` bypasses it.                                                                                           |
| Weather filtering seems ineffective                  | Set `AllWeathers=0` and a valid, nonnegative `WeatherFilter`. `-1` disables the restriction. During a weather transition, the incoming weather ID is used.                                     |
| A named neighborhood seems to be in the wrong region | Region boundaries are approximate. Set `ZoneFilter=0` to remove the geographic restriction.                                                                                                    |
| The menu button is missing                           | Check `EnableMenu=1` and `ShowMenuButton=1`, then restart after editing the INI file. A player must be loaded for the launcher to appear. Check the AML log for an ImGui initialization error. |
| Settings revert after restarting                     | Use **Save to AuroraLights.ini**. Resetting defaults and choosing a new layout do not save by themselves.                                                                                      |
| Editing the file has no immediate effect             | The file is read at startup. Close GTA before editing and restart afterward.                                                                                                                   |
| A slider cannot reach the INI value                  | The menu has narrower ranges for wave speed, curtain height, and radius. See the [Visuals table](#visuals) for both sets of limits.                                                            |
| Blur changes seem delayed                            | Small adjustments can share one blur level. Uploads are rate-limited, and failed uploads retain the previous texture while retrying. Check the AML log if the delay persists.                  |
| Lighting is too strong or too weak                   | Adjust `EnvironmentLightStrength`, then check `Intensity` and the base color. Other lighting mods and game materials can affect the result.                                                    |
| Lighting is unavailable                              | Check the `AuroraLights` messages in the AML log for missing symbols or a hook failure. The sky effect has separate requirements.                                                              |
| Lower frame rate with many groups                    | Reduce `PatchCount` and `RibbonSegments`. Layout transitions temporarily render old and new groups together.                                                                                   |

Replacing the plugin `.so` also requires a full game restart. Installation and build instructions are in the [README](../README.md).
