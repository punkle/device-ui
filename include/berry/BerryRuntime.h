#pragma once

#ifdef HAS_APP_FRAMEWORK

#include "berry.h"
#include "lvgl.h"
#include <string>
#include <vector>

struct AppManifest {
    std::string name;
    std::string version;
    std::string author;
    std::string entry;
    const char *embeddedSource; // non-null for built-in apps, nullptr for SD apps
    std::string sdPath;         // SD card path for SD apps, empty for built-in
    bool isBuiltin;
    std::string nativeId;       // non-empty for native (non-Berry) apps, e.g. "map"
    const uint8_t *iconData;    // embedded PNG icon data (nullptr if none)
    uint32_t iconDataSize;      // size of iconData in bytes
    std::string iconPath;       // filesystem path for SD app icons
    std::vector<std::string> capabilities; // declared capabilities (e.g. "http-client")
};

// Generated built-in apps registry entry (used by generated_builtin_apps.cpp)
struct BuiltinAppEntry {
    const char *name;
    const char *version;
    const char *author;
    const char *entry;
    const char *source;
    bool isBuiltin;
    const uint8_t *iconData;
    uint32_t iconDataSize;
    const char *capabilities; // comma-separated capability list, or nullptr
};

// Provided by generated_builtin_apps.cpp
extern const BuiltinAppEntry *getBuiltinApps(int &count);

// Event callback tracking for cleanup
struct BerryEventData {
    bvm *vm;
    int funcRef;
};

class BerryRuntime
{
  public:
    BerryRuntime();
    ~BerryRuntime();

    void init();
    bool launchApp(const AppManifest &app, lv_obj_t *container);
    void stopApp();

    lv_obj_t *getAppContainer() const { return appContainer; }
    bvm *getVM() const { return vm; }
    bool hasCapability(const std::string &cap) const;

    // Tracked event data for cleanup (accessible to LVGL bindings)
    std::vector<BerryEventData *> eventDataList;

  private:
    void createVM();
    void destroyVM();
    void registerModules();
    std::string readFile(const char *path);

    static void obsHook(bvm *vm, int event, ...);

    bvm *vm;
    lv_obj_t *appContainer;
    int instructionCount;
    std::vector<std::string> currentCapabilities;

    static const int MAX_INSTRUCTIONS = 100000;         // runaway protection limit
    static const int MAX_INSTRUCTIONS_NETWORK = 500000; // extended limit for HTTP + JSON parsing
};

#endif // HAS_APP_FRAMEWORK
