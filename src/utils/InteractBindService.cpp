#include "utils/InteractBindService.h"

#include "utils/GameScanCode.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

namespace cm {
namespace {

constexpr const char* kLegacyInteractBind = "CM_INTERACT";
constexpr const char* kInputBindUpdatedEvent = "EV_INPUTBIND_UPDATED";

struct KeyboardBind {
    bool alt = false;
    bool ctrl = false;
    bool shift = false;
    unsigned short scanCode = 0;
};

struct InteractBinds {
    KeyboardBind primary;
    KeyboardBind secondary;
};

AddonAPI_t* g_api = nullptr;
InteractBindService::Handler g_handler;
InteractBinds g_binds{};
bool g_registered = false;

int ParseIntAttribute(const std::string& block, const char* name) {
    const std::string key = std::string(name) + "=\"";
    const std::size_t start = block.find(key);
    if (start == std::string::npos) {
        return -1;
    }
    const std::size_t valueStart = start + key.size();
    const std::size_t valueEnd = block.find('"', valueStart);
    if (valueEnd == std::string::npos) {
        return -1;
    }
    return std::atoi(block.substr(valueStart, valueEnd - valueStart).c_str());
}

std::string DeviceAttribute(const std::string& block, const char* name) {
    const std::string key = std::string(name) + "=\"";
    const std::size_t start = block.find(key);
    if (start == std::string::npos) {
        return {};
    }
    const std::size_t valueStart = start + key.size();
    const std::size_t valueEnd = block.find('"', valueStart);
    if (valueEnd == std::string::npos) {
        return {};
    }
    return block.substr(valueStart, valueEnd - valueStart);
}

KeyboardBind ParseKeyboardBind(const std::string& block, const char* modName,
                               const char* deviceName, const char* buttonName) {
    KeyboardBind bind{};
    const std::string device = DeviceAttribute(block, deviceName);
    if (device.empty() || device == "None") {
        return bind;
    }
    if (device != "Keyboard") {
        return bind;
    }

    const int mods = ParseIntAttribute(block, modName);
    if (mods >= 0) {
        bind.alt = (mods & 0b0100) != 0;
        bind.ctrl = (mods & 0b0010) != 0;
        bind.shift = (mods & 0b0001) != 0;
    }

    const int button = ParseIntAttribute(block, buttonName);
    if (button >= 0) {
        bind.scanCode = GameScanCodeToScanCode(static_cast<std::uint16_t>(button));
    }
    return bind;
}

void ApplyDefaultInteractBind() {
    g_binds.primary = KeyboardBind{};
    g_binds.primary.scanCode = GameScanCodeToScanCode(70);  // Default interact: F
    g_binds.secondary = KeyboardBind{};
}

bool ParseInteractBindsFromXml(const std::filesystem::path& path, InteractBinds& out) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    const std::string needle = "id=\"65\"";
    const std::size_t actionPos = content.find(needle);
    if (actionPos == std::string::npos) {
        return false;
    }

    const std::size_t blockEnd = content.find("/>", actionPos);
    if (blockEnd == std::string::npos) {
        return false;
    }

    const std::string block = content.substr(actionPos, blockEnd - actionPos);
    out.primary = ParseKeyboardBind(block, "mod", "device", "button");
    out.secondary = ParseKeyboardBind(block, "mod2", "device2", "button2");
    return out.primary.scanCode != 0 || out.secondary.scanCode != 0;
}

std::vector<std::filesystem::path> CandidateInputPaths(AddonAPI_t* api) {
    std::vector<std::filesystem::path> paths;
    if (!api) {
        return paths;
    }

    if (api->Paths_GetCommonDirectory) {
        const std::filesystem::path commonDir = api->Paths_GetCommonDirectory();
        paths.push_back(commonDir / "GameBinds.xml");
        paths.push_back(commonDir / "Input.xml");
    }

    if (api->Paths_GetGameDirectory) {
        const std::filesystem::path gameDir = api->Paths_GetGameDirectory();
        paths.push_back(gameDir / "Input.xml");
        paths.push_back(gameDir / ".." / "Input.xml");
    }

    return paths;
}

void ReloadInteractBinds() {
    ApplyDefaultInteractBind();

    if (!g_api) {
        return;
    }

    for (const auto& path : CandidateInputPaths(g_api)) {
        InteractBinds parsed{};
        if (ParseInteractBindsFromXml(path, parsed)) {
            g_binds = parsed;
            if (g_binds.primary.scanCode == 0 && g_binds.secondary.scanCode != 0) {
                g_binds.primary = g_binds.secondary;
                g_binds.secondary = KeyboardBind{};
            }
            return;
        }
    }
}

bool ModifiersMatch(const KeyboardBind& bind) {
    const bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    return bind.alt == alt && bind.ctrl == ctrl && bind.shift == shift;
}

bool BindMatchesKey(const KeyboardBind& bind, unsigned short scanCode) {
    return bind.scanCode != 0 && bind.scanCode == scanCode && ModifiersMatch(bind);
}

void OnInputBindingsUpdated(void*) { ReloadInteractBinds(); }

UINT InteractWndProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (!g_handler) {
        return uMsg;
    }

    switch (uMsg) {
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            if (ImGui::GetCurrentContext()) {
                ImGuiIO& io = ImGui::GetIO();
                if (io.WantCaptureKeyboard) {
                    break;
                }
            }

            if (lParam & (1u << 30)) {
                break;  // Ignore key repeat.
            }

            const unsigned short scanCode =
                static_cast<unsigned short>((lParam >> 16) & 0xFF);
            if (!BindMatchesKey(g_binds.primary, scanCode) &&
                !BindMatchesKey(g_binds.secondary, scanCode)) {
                break;
            }

            g_handler();
            break;
        }
        default:
            break;
    }

    return uMsg;
}

}  // namespace

void InteractBindService::Register(AddonAPI_t* api, Handler handler) {
    if (!api || g_registered) {
        return;
    }

    g_api = api;
    g_handler = std::move(handler);

    // Remove legacy addon keybind that captured F without passing it to the game.
    if (api->InputBinds_Deregister) {
        api->InputBinds_Deregister(kLegacyInteractBind);
    }

    ReloadInteractBinds();

    if (api->Events_Subscribe) {
        api->Events_Subscribe(kInputBindUpdatedEvent, OnInputBindingsUpdated);
    }
    if (api->WndProc_Register) {
        api->WndProc_Register(InteractWndProc);
    }

    g_registered = true;
}

const char* InteractBindService::GetPrimaryKeyLabel() {
    static thread_local char label[32];
    const unsigned short scanCode = g_binds.primary.scanCode != 0 ? g_binds.primary.scanCode
                                                                  : static_cast<unsigned short>(0x21);

    const UINT vk = MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK);
    if (vk == 0) {
        return "F";
    }

    const LONG lParam = static_cast<LONG>(scanCode) << 16;
    WCHAR wideName[64]{};
    if (GetKeyNameTextW(lParam, wideName, static_cast<int>(std::size(wideName))) <= 0) {
        return "F";
    }

    const int narrowLen =
        WideCharToMultiByte(CP_UTF8, 0, wideName, -1, label, static_cast<int>(std::size(label)),
                            nullptr, nullptr);
    if (narrowLen <= 1) {
        return "F";
    }

    return label;
}

void InteractBindService::Unregister(AddonAPI_t* api) {
    if (!api || !g_registered) {
        return;
    }

    if (api->Events_Unsubscribe) {
        api->Events_Unsubscribe(kInputBindUpdatedEvent, OnInputBindingsUpdated);
    }
    if (api->WndProc_Deregister) {
        api->WndProc_Deregister(InteractWndProc);
    }
    if (api->InputBinds_Deregister) {
        api->InputBinds_Deregister(kLegacyInteractBind);
    }

    g_handler = {};
    g_api = nullptr;
    g_registered = false;
}

}  // namespace cm
