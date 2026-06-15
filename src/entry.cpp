#include "core/AppState.h"
#include "core/Branding.h"
#include "core/FeatureFlags.h"
#include "core/MumbleUtils.h"
#include "ui/BillboardRenderer.h"
#include "ui/DatAssetIconService.h"
#include "ui/MarkersPanel.h"
#include "ui/OptionsPanel.h"
#include "ui/SettingsWindow.h"
#include "ui/MarkerSetEditorWindow.h"
#include "services/RtApiService.h"
#include "ui/QuickAccessService.h"
#include "ui/ScreenMapOverlay.h"
#include "ui/TextureService.h"
#include "utils/InteractBindService.h"

#include <imgui.h>
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include <windows.h>

namespace {

AddonAPI_t* g_api = nullptr;

constexpr const char* kToggleBind = "CM_TOGGLE";

void AddonLoad(AddonAPI_t* api);
void AddonUnload();
void AddonRender();
void AddonOptions();

void RefreshDataLinks(cm::AppState& state) {
    if (!g_api) {
        return;
    }

    state.nexusLink = static_cast<NexusLinkData_t*>(g_api->DataLink_Get(DL_NEXUS_LINK));
    state.mumbleLink = static_cast<Mumble::Data*>(g_api->DataLink_Get(DL_MUMBLE_LINK));
    state.playerIdentity =
        static_cast<const Mumble::Identity*>(g_api->DataLink_Get(DL_MUMBLE_LINK_IDENTITY));
}

void LogWorldReadyOnce(const cm::AppState& state) {
    if (!cm::features::kDebugWorldReadyLog || !g_api) {
        return;
    }

    static bool logged = false;
    if (logged || !cm::IsWorldReady(state.mumbleLink, state.nexusLink)) {
        return;
    }

    logged = true;
    g_api->Log(LOGL_INFO, cm::kLogChannel, "World ready (minimal mode).");
}

void OnGameInteract() {
    auto& state = cm::AppState::Instance();
    if (!state.IsReady()) {
        return;
    }

    state.mapWatch.OnInteractKey(state.mumbleLink, state.nexusLink, state.ltMode,
                                 state.nexusLink ? state.nexusLink->Scaling : 1.0f);
}

void OnToggle(const char*, bool isRelease) {
    if (isRelease || !cm::features::kQuickAccess) {
        return;
    }

    cm::QuickAccessService::OnShortcutActivated(cm::AppState::Instance());
}

void AddonLoad(AddonAPI_t* api) {
    g_api = api;
    api->Log(LOGL_INFO, cm::kLogChannel, "AddonLoad starting (minimal baseline).");

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(api->ImguiContext));
    ImGui::SetAllocatorFunctions(
        reinterpret_cast<void* (*)(size_t, void*)>(api->ImguiMalloc),
        reinterpret_cast<void (*)(void*, void*)>(api->ImguiFree));

    cm::AppState::Instance().PrepareLoad(api);

    api->GUI_Register(RT_Render, AddonRender);
    api->GUI_Register(RT_OptionsRender, AddonOptions);

    if (cm::features::kQuickAccess) {
        Keybind_t defaultBind{};
        api->InputBinds_RegisterWithStruct(kToggleBind, OnToggle, defaultBind);
    }

    if (cm::features::kRegisterInputBind) {
        cm::InteractBindService::Register(api, OnGameInteract);
    }

    if (cm::features::kMarkersPanel) {
        cm::MarkersPanel::RegisterInput(api);
    }

    api->Log(LOGL_INFO, cm::kLogChannel, "Loaded.");
}

void AddonUnload() {
    if (!g_api) {
        return;
    }

    g_api->GUI_Deregister(AddonRender);
    g_api->GUI_Deregister(AddonOptions);

    if (cm::features::kQuickAccess) {
        g_api->InputBinds_Deregister(kToggleBind);
    }

    if (cm::features::kRegisterInputBind) {
        cm::InteractBindService::Unregister(g_api);
    }

    if (cm::features::kMarkersPanel) {
        cm::MarkersPanel::UnregisterInput(g_api);
    }

    if (cm::features::kQuickAccess) {
        cm::QuickAccessService::Unregister(g_api);
    }
    if (cm::features::kDatAssetIcons) {
        cm::DatAssetIconService::Shutdown();
    }
    if (cm::features::kDeferredInit) {
        cm::TextureService::Shutdown();
    }

    cm::AppState::Instance().Shutdown();
    g_api->Log(LOGL_INFO, cm::kLogChannel, "Unloaded.");
    g_api = nullptr;
}

void AddonRender() {
    if (!g_api) {
        return;
    }

    auto& state = cm::AppState::Instance();
    RefreshDataLinks(state);

    LogWorldReadyOnce(state);

    if (!cm::features::kDeferredInit) {
        return;
    }

    if (!cm::IsWorldReady(state.mumbleLink, state.nexusLink)) {
        return;
    }

    state.ProcessDeferredInit();
    if (!state.IsReady()) {
        return;
    }

    state.ProcessBackgroundNotices();

    cm::RtApiService::Tick();

    if (cm::features::kBackgroundSync) {
        state.ProcessPendingCommunitySync();
        state.ProcessPendingMapRefresh();
    }

    if (cm::features::kQuickAccess) {
        state.ProcessRenderInit();
        cm::QuickAccessService::ProcessFrame();
        cm::QuickAccessService::SyncVisibility(g_api, state);
    }

    if (cm::features::kMapWatch) {
        if (state.mumbleLink && state.nexusLink) {
            state.mapWatch.Update(state.mumbleLink, state.nexusLink, state.ltMode,
                                  state.nexusLink->Scaling);
        }
        state.placementService.Tick();
    }

    if (cm::features::kDatAssetIcons) {
        cm::DatAssetIconService::ProcessDownloads();
    }

    // World overlays first (sent to display back), settings window last so it stays on top.
    if (cm::features::kMarkersPanel) {
        cm::MarkersPanel::Render(state);
    }
    if (cm::features::kScreenMapOverlay) {
        cm::ScreenMapOverlay::Render(state);
    }
    if (cm::features::kBillboards) {
        cm::BillboardRenderer::Render(state);
    }

    if (cm::features::kOptionsPanel && cm::SettingsWindow::IsOpen()) {
        state.EnsureOptionsReady();
        if (cm::features::kDeferredInit &&
            cm::IsWorldReady(state.mumbleLink, state.nexusLink)) {
            state.ProcessDeferredInit();
        }
        cm::SettingsWindow::Render(state);
    }

    if (cm::features::kOptionsPanel && cm::MarkerSetEditorWindow::IsOpen()) {
        state.EnsureOptionsReady();
        cm::MarkerSetEditorWindow::Render(state);
    }
}

void AddonOptions() {
    if (!g_api) {
        return;
    }

    auto& state = cm::AppState::Instance();
    RefreshDataLinks(state);

    if (cm::features::kOptionsPanel) {
        state.EnsureOptionsReady();
        if (cm::features::kDeferredInit &&
            cm::IsWorldReady(state.mumbleLink, state.nexusLink)) {
            state.ProcessDeferredInit();
        }
        cm::OptionsPanel::RenderNexusConfigEntry(state);
        return;
    }

    ImGui::TextUnformatted(cm::kDisplayName);
    ImGui::Separator();
    ImGui::TextUnformatted("Minimal debug build — all features disabled.");
    ImGui::TextUnformatted("See docs/DEBUGGING.md to re-enable incrementally.");
}

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
        def.Author = cm::kAuthor;
        def.Description = cm::kDescription;
        def.Load = AddonLoad;
        def.Unload = AddonUnload;
        def.Flags = AF_None;
        def.Provider = UP_None;
        initialized = true;
    }
    return &def;
}
