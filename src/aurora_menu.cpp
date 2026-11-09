#include "aurora_menu.h"
#include "aurora_renderer.h"
#include <algorithm>
#include <mod/config.h>
#include <plugin-sdk/game_sa/entity/PlayerPed.h>
#include <plugin-sdk/renderware/imgui/imgui_impl_renderware.h>

namespace AuroraMenu {

namespace {

Config *s_pAmlConfig = nullptr;
bool s_ShowWindow = false;
const char *s_StatusMessage = "";

float UIScale(float height) { return std::clamp(height / 720.0f, 0.75f, 2.0f); }

void DrawLauncher(float scale, float screenHeight) {
    const ImVec2 size(92.0f * scale, 36.0f * scale);
    ImGui::SetNextWindowPos(ImVec2(24.0f * scale, screenHeight - 24.0f * scale - size.y));
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * scale);
    if (ImGui::Begin("##aurora_launcher", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings)) {
        if (ImGui::Button("Aurora", size)) {
            s_ShowWindow = !s_ShowWindow;
            s_StatusMessage = "";
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void DrawControlDialog(AuroraConfig &cfg, float scale, float screenWidth, float screenHeight) {
    ImGui::SetNextWindowPos(ImVec2(screenWidth * 0.5f, screenHeight * 0.5f), ImGuiCond_FirstUseEver,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(520.0f * scale, screenWidth - 30.0f * scale),
                                    std::min(460.0f * scale, screenHeight - 40.0f * scale)),
                             ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Aurora Lights Controls", &s_ShowWindow, ImGuiWindowFlags_NoCollapse)) {
        const auto &stats = AuroraRenderer::GetStats();
        if (stats.hasRendered) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[DRAW] World aurora (%u verts)",
                               static_cast<unsigned>(stats.vertexCount));
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.2f, 1.0f), "[IDLE] Block Reason: %s",
                               stats.blockReason ? stats.blockReason : "Unknown");
            ImGui::Text("Clock Hr: %d | Area: %d (0=outdoors)", stats.gameHour, stats.currArea);
        }
        ImGui::Separator();

        if (ImGui::Button(cfg.alwaysActive ? "Disable 24/7 Mode (Follow Clock & Zone)"
                                           : "Force Aurora NOW (24/7 Test Mode)",
                          ImVec2(-1, 38.0f * scale))) {
            cfg.alwaysActive = !cfg.alwaysActive;
        }
        ImGui::Separator();

        if (ImGui::BeginTabBar("AuroraTabs")) {
            // Tab 1: Visuals
            if (ImGui::BeginTabItem("Visuals")) {
                ImGui::Checkbox("Master Enable", &cfg.enabled);
                ImGui::Separator();

                ImGui::TextWrapped("Changes apply live. Save to keep them for the next launch.");
                ImGui::SliderFloat("Brightness", &cfg.intensity, 0.0f, 2.5f, "%.2f");
                ImGui::SliderFloat("Wave Speed", &cfg.waveSpeed, 0.0f, 4.0f, "%.2f");
                ImGui::SliderFloat("Curtain Height", &cfg.curtainHeight, 40.0f, 400.0f, "%.0f");
                ImGui::SliderFloat("Curtain Radius", &cfg.curtainRadius, 200.0f, 1200.0f, "%.0f");
                ImGui::SliderInt("Ribbon Segments", &cfg.ribbonSegments, 16, 128);

                ImGui::Separator();
                ImGui::Text("Color Gradient:");

                float baseCol[3] = {cfg.baseColorR / 255.0f, cfg.baseColorG / 255.0f, cfg.baseColorB / 255.0f};
                if (ImGui::ColorEdit3("Base (Root) Color", baseCol)) {
                    cfg.baseColorR = static_cast<uint8_t>(baseCol[0] * 255.0f);
                    cfg.baseColorG = static_cast<uint8_t>(baseCol[1] * 255.0f);
                    cfg.baseColorB = static_cast<uint8_t>(baseCol[2] * 255.0f);
                }

                float topCol[3] = {cfg.topColorR / 255.0f, cfg.topColorG / 255.0f, cfg.topColorB / 255.0f};
                if (ImGui::ColorEdit3("Top (Peak) Color", topCol)) {
                    cfg.topColorR = static_cast<uint8_t>(topCol[0] * 255.0f);
                    cfg.topColorG = static_cast<uint8_t>(topCol[1] * 255.0f);
                    cfg.topColorB = static_cast<uint8_t>(topCol[2] * 255.0f);
                }

                ImGui::EndTabItem();
            }

            // Tab 2: Conditions
            if (ImGui::BeginTabItem("Conditions")) {
                ImGui::Checkbox("Always Active (24/7 Override)", &cfg.alwaysActive);
                ImGui::Separator();

                ImGui::Text("Appearance Time Range:");
                ImGui::SliderInt("Start Hour (Night)", &cfg.startHour, 0, 23);
                ImGui::SliderInt("End Hour (Morning)", &cfg.endHour, 0, 23);

                ImGui::Separator();
                ImGui::Checkbox("All Weathers (Any Weather)", &cfg.allWeathers);

                ImGui::Separator();
                const char *zones[] = {"All San Andreas (Everywhere)",
                                       "Countryside & Mountains (Mount Chiliad, Red County)",
                                       "Desert & Badlands (Bone County)", "Major Cities (LS, SF, LV)"};
                ImGui::Combo("Region / Zone Filter", &cfg.zoneFilter, zones, IM_ARRAYSIZE(zones));

                ImGui::EndTabItem();
            }

            // Tab 3: Settings
            if (ImGui::BeginTabItem("Settings")) {
                ImGui::Checkbox("Show Launcher Button on Screen", &cfg.showMenuButton);
                ImGui::Separator();

                if (ImGui::Button("Save to AuroraLights.ini", ImVec2(-1, 44.0f * scale))) {
                    if (s_pAmlConfig) {
                        AuroraConfig_Save(cfg, s_pAmlConfig);
                        s_StatusMessage = "Settings saved to AuroraLights.ini!";
                    }
                }

                if (ImGui::Button("Reset to Defaults", ImVec2(-1, 44.0f * scale))) {
                    AuroraConfig_SetDefaults(cfg);
                    s_StatusMessage = "Defaults restored!";
                }

                if (ImGui::Button("Close Menu", ImVec2(-1, 44.0f * scale))) {
                    s_ShowWindow = false;
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        if (s_StatusMessage && s_StatusMessage[0]) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "%s", s_StatusMessage);
        }
    }
    ImGui::End();
}

} // namespace

void Init(Config *amlConfig) { s_pAmlConfig = amlConfig; }

// The backend owns the ImGui context, frame loop and touch routing; this only draws.
void Draw(AuroraConfig &cfg) {
    if (!cfg.enableMenu)
        return;

    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    const float scale = UIScale(screen.y);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f * scale, 8.0f * scale));

    // The launcher only shows in-game; an open controls window also stays over the pause menu.
    if (cfg.showMenuButton && FindPlayerPed && FindPlayerPed(-1))
        DrawLauncher(scale, screen.y);
    if (s_ShowWindow)
        DrawControlDialog(cfg, scale, screen.x, screen.y);

    ImGui::PopStyleVar();
}

} // namespace AuroraMenu
