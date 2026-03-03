#include "graphics/AppHost.h"
#include "graphics/AppLvglBindings.h"
#include "mapps/MeshEventHandler.h"
#include "util/ILog.h"

// --- AppHost ---

AppHost::AppHost() {}

void AppHost::discoverApps()
{
    appInfos.clear();
    appLibraryIndices.clear();

    if (!appLib) {
        ILOG_INFO("[AppHost] No AppLibrary set");
        return;
    }

    const auto &entries = appLib->getApps();
    ILOG_INFO("[AppHost] AppLibrary has %u app(s) total", (unsigned)entries.size());
    for (int i = 0; i < (int)entries.size(); i++) {
        const MappManifest &manifest = entries[i].manifest;

        // Only include apps with "mui" entry point
        if (manifest.getEntryFile("mui").empty()) {
            ILOG_INFO("[AppHost] Skipping app '%s' (slug='%s'): no 'mui' entry point (entries:",
                      manifest.name.c_str(), manifest.slug.c_str());
            for (const auto &ep : manifest.entries) {
                ILOG_INFO("[AppHost]   '%s' -> '%s'", ep.first.c_str(), ep.second.c_str());
            }
            if (manifest.entries.empty()) {
                ILOG_INFO("[AppHost]   (none)");
            }
            continue;
        }

        AppInfo info;
        info.name = manifest.name;
        info.version = manifest.version;
        info.author = manifest.author;
        info.slug = manifest.slug;
        info.permissions = manifest.permissions;

        // Derive trust/approval status from AppStatus
        AppStatus status = entries[i].status;
        info.signatureValid = (status != AppStatus::Unsigned && status != AppStatus::SignatureFailed &&
                               status != AppStatus::Discovered);
        info.trusted = (status == AppStatus::SignatureApproved || status == AppStatus::Ready);
        info.permissionsApproved = (status == AppStatus::Ready);

        ILOG_INFO("[AppHost] Found app: '%s' v%s by %s (status=%d)", info.name.c_str(), info.version.c_str(),
                  info.author.c_str(), (int)status);

        appInfos.push_back(std::move(info));
        appLibraryIndices.push_back(i);
    }

    ILOG_INFO("[AppHost] Discovered %u app(s) with 'mui' entry", (unsigned)appInfos.size());
}

bool AppHost::launchApp(int index, lv_obj_t *container)
{
    if (!appLib || index < 0 || index >= (int)appInfos.size())
        return false;

    const std::string &slug = appInfos[index].slug;

    ILOG_DEBUG("[AppHost] Creating mui runtime for '%s'", slug.c_str());

    // Create mui runtime via AppLibrary
    AppRuntime *runtime = appLib->createRuntime(slug, "mui");
    if (!runtime) {
        ILOG_WARN("[AppHost] Failed to create mui runtime for '%s'", slug.c_str());
        return false;
    }

    // Stop any existing MUI runtime
    if (activeRuntime && activeRuntime->isRunning()) {
        activeRuntime->stop();
    }

    ILOG_DEBUG("[AppHost] Registering LVGL bindings for '%s'", slug.c_str());
    registerLvglBindings(runtime, container);

    ILOG_DEBUG("[AppHost] Starting mui Berry runtime for '%s'", slug.c_str());
    if (!runtime->start()) {
        std::string err = runtime->getLastError();
        ILOG_ERROR("[AppHost] Failed to start mui runtime for '%s': %s", slug.c_str(),
                   err.empty() ? "(no details)" : err.c_str());
        appLib->stopRuntime(slug, "mui");
        return false;
    }

    runtime->call("init");

    activeRuntime = runtime;
    activeAppIndex = index;
    activeAppSlug = slug;

    // Clear stale state notifications — the fresh MUI init() reads current
    // state from app_state directly.
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        stateNotifications.clear();
    }

    // Register as global handler to receive handleStateChanged() notifications
    setGlobalMeshEventHandler(this);

    ILOG_INFO("[AppHost] Launched mui for '%s'", appInfos[index].name.c_str());
    return true;
}

void AppHost::stopApp(lv_obj_t *container)
{
    // Stop only the MUI (UI) runtime — handler persists in firmware's AppModule
    if (activeRuntime) {
        activeRuntime->stop();
        if (appLib && activeAppIndex >= 0 && activeAppIndex < (int)appInfos.size()) {
            appLib->stopRuntime(appInfos[activeAppIndex].slug, "mui");
        }
        activeRuntime = nullptr;
    }

    activeAppSlug.clear();

    // Deregister global handler
    if (getGlobalMeshEventHandler() == this)
        setGlobalMeshEventHandler(nullptr);

    // Clear stale state notifications (MUI is gone, no consumer)
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        stateNotifications.clear();
    }

    if (container) {
        lv_obj_clean(container);
    }
}

bool AppHost::isAppRunning() const
{
    return activeRuntime && activeRuntime->isRunning();
}

bool AppHost::approveAndLaunch(int index, lv_obj_t *container)
{
    if (!appLib || index < 0 || index >= (int)appInfos.size())
        return false;

    int libIndex = appLibraryIndices[index];

    // Approve developer signature
    if (!appLib->approveSignature(libIndex)) {
        ILOG_WARN("[AppHost] approveSignature failed for '%s'", appInfos[index].slug.c_str());
        return false;
    }

    // Approve permissions
    if (!appLib->approvePermissions(libIndex)) {
        ILOG_WARN("[AppHost] approvePermissions failed for '%s'", appInfos[index].slug.c_str());
        return false;
    }

    // Update cached info
    appInfos[index].trusted = true;
    appInfos[index].permissionsApproved = true;

    // Trigger handler start in firmware via app-ready callback
    AppReadyCallback cb = getAppReadyCallback();
    if (cb) {
        cb(appInfos[index].slug);
    }

    return launchApp(index, container);
}

// --- MeshEventHandler: state change notification (Router thread) ---

void AppHost::handleStateChanged(const std::string &appSlug, const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (stateNotifications.size() >= MAX_STATE_NOTIFICATIONS)
        stateNotifications.pop_front(); // drop oldest
    stateNotifications.push_back({appSlug, key, value});
}

// --- Tick: drain state notifications on UI thread ---

void AppHost::tick()
{
    if (!activeRuntime || activeAppSlug.empty())
        return;

    // Swap under lock for minimal contention
    std::deque<StateNotification> pending;
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        pending.swap(stateNotifications);
    }

    for (const auto &notif : pending) {
        if (notif.appSlug == activeAppSlug) {
            activeRuntime->call("on_state_changed", {AppValue(notif.key), AppValue(notif.value)});
        }
    }
}
