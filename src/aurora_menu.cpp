#include "aurora_menu.h"
#include "aurora_renderer.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <mod/amlmod.h>
#include <mod/config.h>
#include <plugin-sdk/game_sa/entity/PlayerPed.h>
#include <plugin-sdk/renderware/imgui/imgui_impl_renderware.h>
#include <plugin-sdk/renderware/imgui/imgui_impl_renderware_internal.h>

namespace AuroraMenu {

namespace {

Config *s_pAmlConfig = nullptr;
ImGuiContext *s_ImGuiContext = nullptr;
bool s_ShowWindow = false;
bool s_InitFailed = false;
const char *s_StatusMessage = "";
std::chrono::steady_clock::time_point s_LastFrameTime;

std::atomic<bool> s_UIVisible{false};
std::atomic<bool> s_CaptureScreen{false};
std::atomic<bool> s_ButtonVisible{false};
std::atomic<uint64_t> s_RenderExtent{0};

float UIScale(int height) { return std::clamp(height / 720.0f, 0.75f, 2.0f); }

ImVec4 LauncherBounds(int width, int height) {
    const float scale = UIScale(height);
    const float btnWidth = 92.0f * scale;
    const float btnHeight = 36.0f * scale;
    const float left = 24.0f * scale;
    const float bottom = height - 24.0f * scale;
    return {left, bottom - btnHeight, left + btnWidth, bottom};
}

void DrawControlDialog(AuroraConfig &cfg, float scale, int screenWidth, int screenHeight) {
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

void Render(AuroraConfig &cfg, int screenWidth, int screenHeight) {
    if (screenWidth <= 0 || screenHeight <= 0 || s_InitFailed)
        return;

    // If menu is completely disabled in config, skip
    if (!cfg.enableMenu)
        return;

    s_RenderExtent = (static_cast<uint64_t>(screenWidth) << 32) | static_cast<uint32_t>(screenHeight);
    const auto previous = ImGui::GetCurrentContext();

    if (!s_ImGuiContext) {
        IMGUI_CHECKVERSION();
        s_ImGuiContext = ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().LogFilename = nullptr;
        ImGui::StyleColorsDark();

        if (!ImGui_ImplRenderWare_Init()) {
            ImGui::DestroyContext(s_ImGuiContext);
            s_ImGuiContext = nullptr;
            s_InitFailed = true;
            ImGui::SetCurrentContext(previous);
            return;
        }
        s_LastFrameTime = std::chrono::steady_clock::now();
    }

    ImGui::SetCurrentContext(s_ImGuiContext);
    const auto now = std::chrono::steady_clock::now();
    const float delta = std::chrono::duration<float>(now - s_LastFrameTime).count();
    s_LastFrameTime = now;

    if (ImGui_ImplRenderWare_NewFrame(screenWidth, screenHeight, delta)) {
        const float scale = UIScale(screenHeight);
        auto &style = ImGui::GetStyle();
        style.FontSizeBase = 22.0f * scale;
        style.FramePadding = ImVec2(10.0f * scale, 8.0f * scale);
        ImGui::NewFrame();

        // Optional launcher button (only shown when in-game and enabled in config)
        auto *player = FindPlayerPed ? FindPlayerPed(-1) : nullptr;
        const bool inGame = (player != nullptr);

        if (cfg.showMenuButton && inGame) {
            s_ButtonVisible = true;
            const auto bounds = LauncherBounds(screenWidth, screenHeight);
            const ImVec2 size(bounds.z - bounds.x, bounds.w - bounds.y);
            ImGui::SetNextWindowPos(ImVec2(bounds.x, bounds.y));
            ImGui::SetNextWindowSize(size);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * scale);
            ImGui::Begin("##aurora_launcher", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings);

            if (ImGui::Button("Aurora", size)) {
                s_ShowWindow = !s_ShowWindow;
                s_StatusMessage = "";
            }

            ImGui::End();
            ImGui::PopStyleVar(2);
        } else {
            s_ButtonVisible = false;
        }

        if (s_ShowWindow) {
            DrawControlDialog(cfg, scale, screenWidth, screenHeight);
        }

        ImGui::Render();
        const bool rendered = ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());
        const bool hasActiveUI = s_ShowWindow || s_ButtonVisible.load();
        s_UIVisible = rendered && hasActiveUI;
        s_CaptureScreen = rendered && s_ShowWindow;
        ImGui_ImplRenderWare_SetInputEnabled(s_UIVisible.load());
    } else {
        s_UIVisible = false;
        s_CaptureScreen = false;
        s_ButtonVisible = false;
        ImGui_ImplRenderWare_SetInputEnabled(false);
    }

    ImGui::SetCurrentContext(previous);
}

bool ProcessTouch(int type, int finger, int x, int y, int winWidth, int winHeight) {
    if (!s_UIVisible.load())
        return false;

    const auto extent = s_RenderExtent.load();
    const int width = static_cast<int>(extent >> 32);
    const int height = static_cast<int>(extent & 0xFFFFFFFFu);

    if (width <= 0 || height <= 0)
        return false;

    const auto point = ImGuiRenderWareDetail::MapTouchToRender(x, y, winWidth > 0 ? winWidth : width,
                                                               winHeight > 0 ? winHeight : height, width, height);

    const auto bounds = LauncherBounds(width, height);
    const bool overButton = s_ButtonVisible.load() &&
                            (point.x >= bounds.x && point.x < bounds.z && point.y >= bounds.y && point.y < bounds.w);
    const bool capture = s_CaptureScreen.load() || overButton;

    if (!capture)
        return false;

    return ImGui_ImplRenderWare_ProcessTouch(type, finger, point.x, point.y, capture);
}

bool IsUIVisible() { return s_UIVisible.load(); }

void Shutdown() {
    s_UIVisible = false;
    s_CaptureScreen = false;
    ImGui_ImplRenderWare_SetInputEnabled(false);

    if (s_ImGuiContext) {
        const auto previous = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(s_ImGuiContext);
        ImGui_ImplRenderWare_Shutdown();
        ImGui::DestroyContext(s_ImGuiContext);
        ImGui::SetCurrentContext(previous == s_ImGuiContext ? nullptr : previous);
        s_ImGuiContext = nullptr;
    }
    s_ShowWindow = false;
}

} // namespace AuroraMenu
