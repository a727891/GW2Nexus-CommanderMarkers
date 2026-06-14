#include "utils/MarkerPlacementHelper.h"

#include <tlhelp32.h>
#include <windows.h>

namespace cm {

namespace {

struct Gw2WindowSearch {
    DWORD pid = 0;
    HWND hwnd = nullptr;
};

BOOL CALLBACK EnumWindowsForPid(HWND hwnd, LPARAM lParam) {
    auto* search = reinterpret_cast<Gw2WindowSearch*>(lParam);
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    if (windowPid != search->pid || !IsWindowVisible(hwnd)) {
        return TRUE;
    }
    if (GetWindow(hwnd, GW_OWNER) != nullptr) {
        return TRUE;
    }
    search->hwnd = hwnd;
    return FALSE;
}

DWORD FindProcessIdByName(const wchar_t* exeName) {
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    DWORD pid = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, exeName) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

HWND FindMainWindowForProcess(DWORD pid) {
    Gw2WindowSearch search{};
    search.pid = pid;
    EnumWindows(EnumWindowsForPid, reinterpret_cast<LPARAM>(&search));
    return search.hwnd;
}

HWND FindGw2MainWindow() {
    DWORD pid = FindProcessIdByName(L"Gw2-64.exe");
    if (pid == 0) {
        pid = FindProcessIdByName(L"Gw2.exe");
    }
    if (pid == 0) {
        return nullptr;
    }
    return FindMainWindowForProcess(pid);
}

}  // namespace

bool TryGetGameClientOrigin(int& left, int& top) {
    left = 0;
    top = 0;

    const HWND hwnd = FindGw2MainWindow();
    if (!hwnd) {
        return false;
    }

    RECT rect{};
    if (!GetClientRect(hwnd, &rect)) {
        return false;
    }

    POINT pt{rect.left, rect.top};
    if (!ClientToScreen(hwnd, &pt)) {
        return false;
    }

    left = pt.x;
    top = pt.y;
    return true;
}

PlacementPoint BlishToPlacementPosition(Vec2f blishCoord, float uiScaleMultiplier) {
    const float x = blishCoord.x * uiScaleMultiplier;
    const float y = blishCoord.y * uiScaleMultiplier;

    int gameLeft = 0;
    int gameTop = 0;
    if (TryGetGameClientOrigin(gameLeft, gameTop)) {
        return {gameLeft + static_cast<int>(x), gameTop + static_cast<int>(y)};
    }

    return {static_cast<int>(x), static_cast<int>(y)};
}

void SetPlacementMousePosition(PlacementPoint placementPosition, bool useScreenCoordinates) {
    (void)useScreenCoordinates;
    SetCursorPos(placementPosition.x, placementPosition.y);
}

PlacementPoint GetPlacementCursorPosition(bool useScreenCoordinates) {
    (void)useScreenCoordinates;
    POINT pt{};
    if (GetCursorPos(&pt)) {
        return {pt.x, pt.y};
    }
    return {};
}

bool UseScreenCoordinatesForPlacement() {
    int left = 0;
    int top = 0;
    return TryGetGameClientOrigin(left, top);
}

}  // namespace cm
