#ifdef HAS_APP_FRAMEWORK

#include "berry/BerryHTTP.h"
#include "berry/BerryRuntime.h"
#include "util/ILog.h"

#include "berry.h"

#include <cstring>

#ifdef ARCH_ESP32
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

// Helper: get BerryRuntime from VM globals
static BerryRuntime *getRuntime(bvm *vm)
{
    be_getglobal(vm, "_berry_runtime");
    BerryRuntime *rt = nullptr;
    if (be_iscomptr(vm, -1)) {
        rt = static_cast<BerryRuntime *>(be_tocomptr(vm, -1));
    }
    be_pop(vm, 1);
    return rt;
}

// Helper: push a Berry map with {"status": int, "body": string}
static void pushResponseMap(bvm *vm, int status, const char *body)
{
    be_newmap(vm);
    // set "status"
    be_pushstring(vm, "status");
    be_pushint(vm, status);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
    // set "body"
    be_pushstring(vm, "body");
    be_pushstring(vm, body);
    be_data_insert(vm, -3);
    be_pop(vm, 2);
    // Wrap raw map into a 'map' class instance so subscript [] works
    be_getbuiltin(vm, "map");
    be_pushvalue(vm, -2);
    be_call(vm, 1);
    be_moveto(vm, -2, -3);
    be_pop(vm, 2);
}

// http.supported() -> bool
int berry_http_supported(bvm *vm)
{
#ifdef ARCH_ESP32
    be_pushbool(vm, btrue);
#else
    be_pushbool(vm, bfalse);
#endif
    be_return(vm);
}

// app.has_capability(name) -> bool
int berry_app_has_capability(bvm *vm)
{
    if (be_top(vm) < 1 || !be_isstring(vm, 1)) {
        be_pushbool(vm, bfalse);
        be_return(vm);
    }
    const char *cap = be_tostring(vm, 1);
    BerryRuntime *rt = getRuntime(vm);
    if (rt && rt->hasCapability(cap)) {
        be_pushbool(vm, btrue);
    } else {
        be_pushbool(vm, bfalse);
    }
    be_return(vm);
}

// http.get(url) -> map {"status": int, "body": string}
int berry_http_get(bvm *vm)
{
    // Check capability
    BerryRuntime *rt = getRuntime(vm);
    if (!rt || !rt->hasCapability("http-client")) {
        be_raise(vm, "permission_error", "app manifest does not declare 'http-client' capability");
    }

#ifdef ARCH_ESP32
    if (be_top(vm) < 1 || !be_isstring(vm, 1)) {
        be_raise(vm, "type_error", "http.get() requires a URL string");
    }
    const char *url = be_tostring(vm, 1);
    ILOG_DEBUG("[Berry HTTP] GET %s", url);

    WiFiClientSecure client;
    client.setInsecure(); // skip certificate verification
    HTTPClient http;

    if (!http.begin(client, url)) {
        ILOG_ERROR("[Berry HTTP] GET begin failed for %s", url);
        pushResponseMap(vm, -1, "connection failed");
        be_return(vm);
    }

    int httpCode = http.GET();
    if (httpCode > 0) {
        String payload = http.getString();
        ILOG_DEBUG("[Berry HTTP] GET response %d, %u bytes", httpCode, payload.length());
        pushResponseMap(vm, httpCode, payload.c_str());
    } else {
        String errMsg = http.errorToString(httpCode);
        ILOG_ERROR("[Berry HTTP] GET failed: %s", errMsg.c_str());
        pushResponseMap(vm, httpCode, errMsg.c_str());
    }
    http.end();
    be_return(vm);
#else
    be_raise(vm, "feature_error", "HTTP client is not available on this platform");
    be_return(vm);
#endif
}

// http.post(url, content_type, body) -> map {"status": int, "body": string}
int berry_http_post(bvm *vm)
{
    // Check capability
    BerryRuntime *rt = getRuntime(vm);
    if (!rt || !rt->hasCapability("http-client")) {
        be_raise(vm, "permission_error", "app manifest does not declare 'http-client' capability");
    }

#ifdef ARCH_ESP32
    if (be_top(vm) < 3 || !be_isstring(vm, 1) || !be_isstring(vm, 2) || !be_isstring(vm, 3)) {
        be_raise(vm, "type_error", "http.post() requires (url, content_type, body) strings");
    }
    const char *url = be_tostring(vm, 1);
    const char *contentType = be_tostring(vm, 2);
    const char *body = be_tostring(vm, 3);
    ILOG_DEBUG("[Berry HTTP] POST %s", url);

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    if (!http.begin(client, url)) {
        ILOG_ERROR("[Berry HTTP] POST begin failed for %s", url);
        pushResponseMap(vm, -1, "connection failed");
        be_return(vm);
    }

    http.addHeader("Content-Type", contentType);
    int httpCode = http.POST((uint8_t *)body, strlen(body));
    if (httpCode > 0) {
        String payload = http.getString();
        ILOG_DEBUG("[Berry HTTP] POST response %d, %u bytes", httpCode, payload.length());
        pushResponseMap(vm, httpCode, payload.c_str());
    } else {
        String errMsg = http.errorToString(httpCode);
        ILOG_ERROR("[Berry HTTP] POST failed: %s", errMsg.c_str());
        pushResponseMap(vm, httpCode, errMsg.c_str());
    }
    http.end();
    be_return(vm);
#else
    be_raise(vm, "feature_error", "HTTP client is not available on this platform");
    be_return(vm);
#endif
}

void berry_http_register(bvm *vm)
{
    ILOG_DEBUG("[Berry] berry_http_register: registering native functions");

    // Register native functions with underscore prefix
    be_regfunc(vm, "_http_get", berry_http_get);
    be_regfunc(vm, "_http_post", berry_http_post);
    be_regfunc(vm, "_http_supported", berry_http_supported);
    be_regfunc(vm, "_app_has_capability", berry_app_has_capability);

    ILOG_DEBUG("[Berry] berry_http_register: native functions registered");

    // Create 'http' and 'app' classes via bootstrap script
    static const char bootstrap[] =
        "class http\n"
        "  static def get(url) return _http_get(url) end\n"
        "  static def post(url, ct, body) return _http_post(url, ct, body) end\n"
        "  static def supported() return _http_supported() end\n"
        "end\n"
        "class app\n"
        "  static def has_capability(name) return _app_has_capability(name) end\n"
        "end\n";

    ILOG_DEBUG("[Berry] berry_http_register: loading bootstrap script");
    int result = be_loadbuffer(vm, "http_bootstrap", bootstrap, strlen(bootstrap));
    if (result == 0) {
        ILOG_DEBUG("[Berry] berry_http_register: executing bootstrap script");
        be_pcall(vm, 0);
        be_pop(vm, 1);
        ILOG_DEBUG("[Berry] berry_http_register: bootstrap complete");
    } else {
        ILOG_ERROR("[Berry] berry_http_register: bootstrap load failed: %s", be_tostring(vm, -1));
        be_pop(vm, 1);
    }
}

#endif // HAS_APP_FRAMEWORK
