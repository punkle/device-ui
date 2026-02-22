#ifdef HAS_APP_FRAMEWORK

#include "berry/BerryRuntime.h"
#include "berry/BerryLVGL.h"
extern "C" {
#include "be_exec.h"
}
#include "util/ILog.h"

#include <cstdarg>
#include <cstring>

#ifdef ARCH_ESP32
#include <Esp.h>
#endif

#if defined(ARCH_PORTDUINO)
#include "PortduinoFS.h"
extern fs::FS &fileSystem;
#elif defined(HAS_SD_MMC)
#include "SD_MMC.h"
extern fs::SDMMCFS &SDFs;
#elif defined(HAS_SDCARD)
#include "SdFat.h"
extern SdFs SDFs;
#endif

BerryRuntime::BerryRuntime() : vm(nullptr), appContainer(nullptr), instructionCount(0) {}

BerryRuntime::~BerryRuntime()
{
    destroyVM();
}

void BerryRuntime::init()
{
    createVM();
}

void BerryRuntime::createVM()
{
    if (vm) {
        destroyVM();
    }
#ifdef ARCH_ESP32
    ILOG_DEBUG("[Berry] createVM: free heap before be_vm_new = %u", ESP.getFreeHeap());
#else
    ILOG_DEBUG("[Berry] createVM: allocating Berry VM");
#endif
    vm = be_vm_new();
    if (!vm) {
        ILOG_ERROR("[Berry] createVM: be_vm_new returned NULL! Out of memory?");
        return;
    }
#ifdef ARCH_ESP32
    ILOG_DEBUG("[Berry] createVM: free heap after be_vm_new = %u", ESP.getFreeHeap());
#endif
    ILOG_DEBUG("[Berry] createVM: setting obs hook");
    be_set_obs_hook(vm, obsHook);
    ILOG_DEBUG("[Berry] createVM: registering modules");
    registerModules();
    ILOG_DEBUG("[Berry] createVM: done");
}

void BerryRuntime::destroyVM()
{
    ILOG_DEBUG("[Berry] destroyVM: freeing %u event callbacks", (unsigned)eventDataList.size());
    // Free tracked event callback data
    for (auto *data : eventDataList) {
        delete data;
    }
    eventDataList.clear();

    if (vm) {
        ILOG_DEBUG("[Berry] destroyVM: deleting VM");
#ifdef ARCH_ESP32
        ILOG_DEBUG("[Berry] destroyVM: free heap before delete = %u", ESP.getFreeHeap());
#endif
        be_vm_delete(vm);
        vm = nullptr;
#ifdef ARCH_ESP32
        ILOG_DEBUG("[Berry] destroyVM: free heap after delete = %u", ESP.getFreeHeap());
#endif
    }
    ILOG_DEBUG("[Berry] destroyVM: done");
}

// Callback for be_execprotected — registers modules inside an exception handler
static void registerModulesProtected(bvm *vm, void *data)
{
    BerryRuntime *runtime = static_cast<BerryRuntime *>(data);

    // Register LVGL binding functions
    berry_lvgl_register(vm);

    // Store runtime pointer as a global so bindings can access it
    be_pushcomptr(vm, runtime);
    be_setglobal(vm, "_berry_runtime");
    be_pop(vm, 1);
}

void BerryRuntime::registerModules()
{
    if (!vm)
        return;

    int result = be_execprotected(vm, registerModulesProtected, this);
    if (result != 0) {
        ILOG_ERROR("[Berry] registerModules: failed with error %d (likely out of memory)", result);
    }
}

bool BerryRuntime::launchApp(const AppManifest &app, lv_obj_t *container)
{
    ILOG_DEBUG("[Berry] launchApp: starting '%s'", app.name.c_str());
#ifdef ARCH_ESP32
    ILOG_DEBUG("[Berry] launchApp: free heap = %u, largest free block = %u", ESP.getFreeHeap(),
               ESP.getMaxAllocHeap());
#endif

    appContainer = container;
    instructionCount = 0;

    // Recreate VM for clean state
    ILOG_DEBUG("[Berry] launchApp: creating VM");
    createVM();

    if (!vm) {
        ILOG_ERROR("[Berry] launchApp: VM creation failed, aborting");
        return false;
    }

    // Set the app container as a global in Berry
    ILOG_DEBUG("[Berry] launchApp: setting app container global");
    be_pushcomptr(vm, container);
    be_setglobal(vm, "_app_container");
    be_pop(vm, 1);

    const char *source = nullptr;
    std::string fileSource;

    if (app.embeddedSource) {
        source = app.embeddedSource;
        ILOG_DEBUG("[Berry] launchApp: using embedded source (%u bytes)", (unsigned)strlen(source));
    } else {
        // Read from SD card
        std::string path = app.sdPath + app.entry;
        ILOG_DEBUG("[Berry] launchApp: reading source from SD: %s", path.c_str());
        fileSource = readFile(path.c_str());
        if (fileSource.empty()) {
            ILOG_ERROR("[Berry] launchApp: failed to read app source: %s", path.c_str());
            return false;
        }
        source = fileSource.c_str();
        ILOG_DEBUG("[Berry] launchApp: read %u bytes from SD", (unsigned)fileSource.size());
    }

#ifdef ARCH_ESP32
    ILOG_DEBUG("[Berry] launchApp: free heap before load = %u", ESP.getFreeHeap());
#endif

    // Load and execute the Berry script
    ILOG_DEBUG("[Berry] launchApp: calling be_loadbuffer");
    int result = be_loadbuffer(vm, "app", source, strlen(source));
    if (result != 0) {
        ILOG_ERROR("[Berry] launchApp: load error: %s", be_tostring(vm, -1));
        be_pop(vm, 1);
        return false;
    }
    ILOG_DEBUG("[Berry] launchApp: be_loadbuffer OK");

#ifdef ARCH_ESP32
    ILOG_DEBUG("[Berry] launchApp: free heap before exec = %u", ESP.getFreeHeap());
#endif

    ILOG_DEBUG("[Berry] launchApp: calling be_pcall");
    result = be_pcall(vm, 0);
    if (result != 0) {
        ILOG_ERROR("[Berry] launchApp: exec error: %s", be_tostring(vm, -1));
        be_pop(vm, 1);
        return false;
    }
    be_pop(vm, 1);

#ifdef ARCH_ESP32
    ILOG_DEBUG("[Berry] launchApp: completed, free heap = %u", ESP.getFreeHeap());
#endif
    ILOG_DEBUG("[Berry] launchApp: '%s' launched successfully", app.name.c_str());
    return true;
}

void BerryRuntime::stopApp()
{
    // Clean all LVGL children from the app container
    if (appContainer) {
        lv_obj_clean(appContainer);
        appContainer = nullptr;
    }

    destroyVM();
    instructionCount = 0;
}

std::string BerryRuntime::readFile(const char *path)
{
#if defined(ARCH_PORTDUINO)
    fs::FS &fs = fileSystem;
    File file = fs.open(path, FILE_READ);
    if (!file)
        return "";

    size_t size = file.size();
    std::string content;
    content.resize(size);
    file.read((uint8_t *)content.data(), size);
    file.close();
    return content;
#elif defined(HAS_SD_MMC)
    fs::FS &fs = SDFs;
    File file = fs.open(path, FILE_READ);
    if (!file)
        return "";

    size_t size = file.size();
    std::string content;
    content.resize(size);
    file.read((uint8_t *)content.data(), size);
    file.close();
    return content;
#elif defined(HAS_SDCARD)
    FsFile file = SDFs.open(path, O_RDONLY);
    if (!file)
        return "";

    size_t size = file.size();
    std::string content;
    content.resize(size);
    file.read((uint8_t *)content.data(), size);
    file.close();
    return content;
#else
    return "";
#endif
}

void BerryRuntime::obsHook(bvm *vm, int event, ...)
{
    // Only handle VM heartbeat for instruction counting
    // Other events (GC_START, GC_END, STACK_RESIZE_START, PCALL_ERROR)
    // must not touch the Berry stack as VM state may be inconsistent
    if (event != BE_OBS_VM_HEARTBEAT) {
        return;
    }

    // Get runtime pointer from VM globals
    be_getglobal(vm, "_berry_runtime");
    if (!be_iscomptr(vm, -1)) {
        be_pop(vm, 1);
        return;
    }
    BerryRuntime *runtime = static_cast<BerryRuntime *>(be_tocomptr(vm, -1));
    be_pop(vm, 1);

    runtime->instructionCount++;
    if (runtime->instructionCount > MAX_INSTRUCTIONS) {
        be_raise(vm, "runtime_error", "script exceeded instruction limit");
    }
}

#endif // HAS_APP_FRAMEWORK
