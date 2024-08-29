#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/debug/Log.hpp>
#include <hyprland/src/render/decorations/CHyprGroupBarDecoration.hpp>
#include <format>

inline HANDLE PHANDLE = nullptr;

std::vector<int> workspaces;

namespace Monocle {

template <typename... Args>
void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
    auto msg = std::vformat(fmt.get(), std::make_format_args(args...));
    Debug::log(level, "[Monocle] {}", msg);
}

std::vector<CWindow*> getWindowsOnWorkspace() {
    std::vector<CWindow*> windows = {};

    for (auto& w : g_pCompositor->m_vWindows) {
        int workspaceID = w->workspaceID();
        int currentWorkspace = g_pCompositor->m_pLastMonitor->activeWorkspaceID();
        if (workspaceID == currentWorkspace)
            windows.push_back(w.get());
    }

    return windows;
}

void moveWindowIntoGroup(CWindow* pWindow, CWindow* pWindowInDirection) {
    if (pWindow->m_sGroupData.deny)
        return;

    g_pLayoutManager->getCurrentLayout()->onWindowRemoved(pWindow); // This removes groupped property!

    static auto USECURRPOS = CConfigValue<Hyprlang::INT>("group:insert_after_current");
    pWindowInDirection     = *USECURRPOS ? pWindowInDirection : pWindowInDirection->getGroupTail();

    pWindowInDirection->insertWindowToGroup(pWindow);
    pWindowInDirection->setGroupCurrent(pWindow);
    pWindow->updateWindowDecos();
    g_pLayoutManager->getCurrentLayout()->recalculateWindow(pWindow);
    g_pCompositor->focusWindow(pWindow);
    g_pCompositor->warpCursorTo(pWindow->middle());

    if (!pWindow->getDecorationByType(DECORATION_GROUPBAR))
        pWindow->addWindowDeco(std::make_unique<CHyprGroupBarDecoration>(pWindow));
}

void moveIntoGroup(std::string args) {
    char        arg = args[0];

    static auto PIGNOREGROUPLOCK = CConfigValue<Hyprlang::INT>("binds:ignore_group_lock");

    if (!*PIGNOREGROUPLOCK && g_pKeybindManager->m_bGroupsLocked)
        return;

    if (!isDirection(args)) {
        Debug::log(ERR, "Cannot move into group in direction {}, unsupported direction. Supported: l,r,u/t,d/b", arg);
        return;
    }

    const auto PWINDOW = g_pCompositor->m_pLastWindow;

    if (!PWINDOW || PWINDOW->m_bIsFloating || PWINDOW->m_sGroupData.deny)
        return;

    auto PWINDOWINDIR = g_pCompositor->getWindowInDirection(PWINDOW, arg);

    if (!PWINDOWINDIR || !PWINDOWINDIR->m_sGroupData.pNextWindow)
        return;

    // Do not move window into locked group if binds:ignore_group_lock is false
    if (!*PIGNOREGROUPLOCK && (PWINDOWINDIR->getGroupHead()->m_sGroupData.locked || (PWINDOW->m_sGroupData.pNextWindow && PWINDOW->getGroupHead()->m_sGroupData.locked)))
        return;

    moveWindowIntoGroup(PWINDOW, PWINDOWINDIR);
}

}

void monocleOn(std::string arg) {
    const auto currentWindow = g_pCompositor->m_pLastWindow;

    int currentWorkspace = g_pCompositor->m_pLastMonitor->activeWorkspaceID();
    workspaces.push_back(currentWorkspace);

    std::vector<CWindow*> windows = Monocle::getWindowsOnWorkspace();
    auto firstWindow = windows[0];
    if (!firstWindow->m_sGroupData.pNextWindow)
        firstWindow->createGroup();

    for (size_t i = 1; i < windows.size(); i++) {
        auto window1 = windows[i-1];
        auto window2 = windows[i];
        g_pCompositor->focusWindow(window2);
        Monocle::moveWindowIntoGroup(window2, window1);
    }

    g_pCompositor->focusWindow(currentWindow);
}

void monocleOff(std::string arg) {
    int currentWorkspace = g_pCompositor->m_pLastMonitor->activeWorkspaceID();
    size_t toRemove = -1;
    for (size_t i = 0; i < workspaces.size(); i++) {
        if (workspaces[i] == currentWorkspace)
            toRemove = i;
    }
    if (toRemove != -1)
        workspaces.erase(workspaces.begin() + toRemove);

    if (g_pCompositor->m_pLastWindow->m_sGroupData.pNextWindow)
        HyprlandAPI::invokeHyprctlCommand("dispatch", "togglegroup");
}

void monocleToggle(std::string arg) {
    int currentWorkspace = g_pCompositor->m_pLastMonitor->activeWorkspaceID();
    if (std::find(workspaces.begin(), workspaces.end(), currentWorkspace) != workspaces.end()) 
        monocleOff("");
    else 
        monocleOn("");
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    // ALWAYS add this to your plugins. It will prevent random crashes coming from
    // mismatched header versions.
    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[MyPlugin] Mismatched headers! Can't proceed.",
                                     CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[MyPlugin] Version mismatch");
    }

    HyprlandAPI::addDispatcher(PHANDLE, "monocle:on", monocleOn);
    HyprlandAPI::addDispatcher(PHANDLE, "monocle:off", monocleOff);
    HyprlandAPI::addDispatcher(PHANDLE, "monocle:toggle", monocleToggle);

    return {"MyPlugin", "An amazing plugin that is going to change the world!", "Me", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    // ...
}
