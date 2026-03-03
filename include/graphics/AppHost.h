#pragma once

#include "lvgl.h"
#include "mapps/AppLibrary.h"
#include "mapps/AppRuntime.h"
#include "mapps/MeshEventHandler.h"

#include <deque>
#include <mutex>
#include <string>
#include <vector>

/**
 * @brief AppHost — LVGL wrapper over the shared AppLibrary.
 *
 * Delegates app discovery, verification, and trust to AppLibrary (from mapps).
 * Manages LVGL-specific concerns: Berry runtimes with LVGL bindings,
 * container lifecycle, and the "mui" entry point filter.
 *
 * Handler runtimes are managed by firmware's AppModule — AppHost only
 * receives state-change notifications via handleStateChanged() and
 * forwards them to the MUI runtime on the UI thread.
 */
class AppHost : public MeshEventHandler
{
  public:
    AppHost();

    // Set the shared AppLibrary (injected from firmware via init chain)
    void setAppLibrary(AppLibrary *lib) { appLib = lib; }

    // Build the app info list from AppLibrary (filters to "mui" entry point only)
    void discoverApps();

    struct AppInfo {
        std::string name;
        std::string version;
        std::string author;
        std::string slug;
        std::vector<std::string> permissions;
        bool signatureValid;
        bool trusted;
        bool permissionsApproved;
    };

    // App list (only apps with "mui" entry point)
    const std::vector<AppInfo> &getApps() const { return appInfos; }

    // Launch app at index into the given LVGL container
    bool launchApp(int index, lv_obj_t *container);

    // Stop the currently running app, clean container
    void stopApp(lv_obj_t *container);

    // Whether an app is currently running
    bool isAppRunning() const;

    // Approve signature + permissions, persist trust, then launch
    bool approveAndLaunch(int index, lv_obj_t *container);

    // Call each frame from the LVGL task handler to drain state
    // notification queue.
    void tick();

    // MeshEventHandler: called from Router thread when a handler updates app_state
    void handleStateChanged(const std::string &appSlug, const std::string &key, const std::string &value) override;

  private:
    AppLibrary *appLib = nullptr;
    std::vector<AppInfo> appInfos;
    std::vector<int> appLibraryIndices; // maps appInfos index -> AppLibrary index
    AppRuntime *activeRuntime = nullptr; // mui runtime
    int activeAppIndex = -1;
    std::string activeAppSlug;

    // State notification queue (Router thread → UI thread, protected by stateMutex)
    static constexpr size_t MAX_STATE_NOTIFICATIONS = 32;
    struct StateNotification {
        std::string appSlug;
        std::string key;
        std::string value;
    };
    std::mutex stateMutex;
    std::deque<StateNotification> stateNotifications;
};
