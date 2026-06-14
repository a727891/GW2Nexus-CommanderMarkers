#include "core/AppState.h"
#include "core/Branding.h"
#include "core/MumbleUtils.h"
#include "ui/BillboardRenderer.h"
#include "ui/MarkersPanel.h"
#include "ui/OptionsPanel.h"
#include "ui/QuickAccessService.h"
#include "ui/ScreenMapOverlay.h"
#include "ui/TextureService.h"

#include <imgui.h>
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include <windows.h>

namespace {

AddonAPI_t* g_api = nullptr;
float g_lastFrameTime = 0.0f;

constexpr const char* kInteractBind = "CM_INTERACT";

void AddonLoad(AddonAPI_t* api);
void AddonUnload();
void AddonRender();
void AddonOptions();

void OnInteract(const char*, bool isRelease) {
    if (isRelease) return;
    auto& state = cm::AppState::Instance();
    state.mapWatch.OnInteractKey(state.mumbleLink, state.nexusLink, state.ltMode,
                                 state.nexusLink ? state.nexusLink->Scaling : 1.0f);
}

void AddonLoad(AddonAPI_t* api) {
    g_api = api;
    api->Log(LOGL_INFO, cm::kLogChannel, "AddonLoad starting.");

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(api->ImguiContext));
    ImGui::SetAllocatorFunctions(
        reinterpret_cast<void* (*)(size_t, void*)>(api->ImguiMalloc),
        reinterpret_cast<void (*)(void*, void*)>(api->ImguiFree));

    auto& state = cm::AppState::Instance();
    state.Initialize(api);
    cm::TextureService::Initialize(api, state.addonDir);

    api->GUI_Register(RT_Render, AddonRender);
    api->GUI_Register(RT_OptionsRender, AddonOptions);
    api->InputBinds_RegisterWithString(kInteractBind, OnInteract, "F");

    cm::QuickAccessService::Register(api, state);
    cm::QuickAccessService::SyncVisibility(api, state);

    if (!state.requiredBindsOk) {
        api->GUI_SendAlert(
            "Squad marker keybinds are not set in Guild Wars 2. "
            "Open Options → Controls and assign squad marker binds for Commander Markers to work.");
    }

    api->Log(LOGL_INFO, cm::kLogChannel, "Loaded.");
}

void AddonUnload() {
    if (!g_api) return;
    g_api->GUI_Deregister(AddonRender);
    g_api->GUI_Deregister(AddonOptions);
    g_api->InputBinds_Deregister(kInteractBind);
    cm::QuickAccessService::Unregister(g_api);
    cm::TextureService::Shutdown();
    cm::AppState::Instance().Shutdown();
    g_api->Log(LOGL_INFO, cm::kLogChannel, "Unloaded.");
    g_api = nullptr;
}

void AddonRender() {
    auto& state = cm::AppState::Instance();

    state.ProcessPendingCommunitySync();
    state.ProcessPendingMapRefresh();

    if (state.mumbleLink && state.nexusLink) {
        state.mapWatch.Update(state.mumbleLink, state.nexusLink, state.ltMode,
                              state.nexusLink->Scaling);
    }

    state.placementService.Tick();

    cm::MarkersPanel::Render(state);
    cm::ScreenMapOverlay::Render(state);
    cm::BillboardRenderer::Render(state);
}

void AddonOptions() { cm::OptionsPanel::Render(cm::AppState::Instance()); }

}  // namespace

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }

extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef() {
    static AddonDefinition_t def{};
    static bool initialized = false;
    if (!initialized) {
        def.Signature = -2024061401;
        def.APIVersion = NEXUS_API_VERSION;
        def.Name = cm::kDisplayName;
        def.Version = {1, 0, 0, 0};
        def.Author = "Soeed";
        def.Description = cm::kDescription;
        def.Load = AddonLoad;
        def.Unload = AddonUnload;
        def.Flags = AF_None;
        def.Provider = UP_None;
        initialized = true;
    }
    return &def;
}
