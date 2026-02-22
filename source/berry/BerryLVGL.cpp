#ifdef HAS_APP_FRAMEWORK

#include "berry/BerryLVGL.h"
#include "berry/BerryRuntime.h"
#include "util/ILog.h"

#include "berry.h"
#include "lvgl.h"

#include <cstdio>
#include <cstring>

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

// Helper: get app container from VM globals
static lv_obj_t *getAppContainer(bvm *vm)
{
    be_getglobal(vm, "_app_container");
    lv_obj_t *obj = nullptr;
    if (be_iscomptr(vm, -1)) {
        obj = static_cast<lv_obj_t *>(be_tocomptr(vm, -1));
    }
    be_pop(vm, 1);
    return obj;
}

// Helper: get parent from arg or default to app container
static lv_obj_t *getParent(bvm *vm, int argIndex)
{
    if (be_top(vm) >= argIndex && be_iscomptr(vm, argIndex)) {
        return static_cast<lv_obj_t *>(be_tocomptr(vm, argIndex));
    }
    return getAppContainer(vm);
}

// ui.root() -> comptr
int berry_ui_root(bvm *vm)
{
    lv_obj_t *container = getAppContainer(vm);
    if (container) {
        be_pushcomptr(vm, container);
    } else {
        be_pushnil(vm);
    }
    be_return(vm);
}

// ui.label(parent, text) -> comptr
int berry_ui_label(bvm *vm)
{
    int argc = be_top(vm);
    lv_obj_t *parent = getParent(vm, 1);
    if (!parent) {
        be_pushnil(vm);
        be_return(vm);
    }

    lv_obj_t *label = lv_label_create(parent);

    // Find text argument: could be arg 1 (if no parent) or arg 2
    const char *text = nullptr;
    if (argc >= 2 && be_isstring(vm, 2)) {
        text = be_tostring(vm, 2);
    } else if (argc >= 1 && be_isstring(vm, 1)) {
        text = be_tostring(vm, 1);
    }

    if (text) {
        lv_label_set_text(label, text);
    }

    be_pushcomptr(vm, label);
    be_return(vm);
}

// ui.button(parent, text) -> comptr
int berry_ui_button(bvm *vm)
{
    int argc = be_top(vm);
    lv_obj_t *parent = getParent(vm, 1);
    if (!parent) {
        be_pushnil(vm);
        be_return(vm);
    }

    lv_obj_t *btn = lv_button_create(parent);

    // Find text argument
    const char *text = nullptr;
    if (argc >= 2 && be_isstring(vm, 2)) {
        text = be_tostring(vm, 2);
    } else if (argc >= 1 && be_isstring(vm, 1)) {
        text = be_tostring(vm, 1);
    }

    if (text) {
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_center(label);
    }

    be_pushcomptr(vm, btn);
    be_return(vm);
}

// ui.container(parent) -> comptr
int berry_ui_container(bvm *vm)
{
    lv_obj_t *parent = getParent(vm, 1);
    if (!parent) {
        be_pushnil(vm);
        be_return(vm);
    }

    lv_obj_t *cont = lv_obj_create(parent);
    be_pushcomptr(vm, cont);
    be_return(vm);
}

// ui.set_size(obj, w, h)
int berry_ui_set_size(bvm *vm)
{
    int argc = be_top(vm);
    if (argc < 3 || !be_iscomptr(vm, 1) || !be_isint(vm, 2) || !be_isint(vm, 3)) {
        be_pushnil(vm);
        be_return(vm);
    }

    lv_obj_t *obj = static_cast<lv_obj_t *>(be_tocomptr(vm, 1));
    int w = be_toint(vm, 2);
    int h = be_toint(vm, 3);
    lv_obj_set_size(obj, w, h);
    be_pushnil(vm);
    be_return(vm);
}

// ui.set_pos(obj, x, y)
int berry_ui_set_pos(bvm *vm)
{
    int argc = be_top(vm);
    if (argc < 3 || !be_iscomptr(vm, 1) || !be_isint(vm, 2) || !be_isint(vm, 3)) {
        be_pushnil(vm);
        be_return(vm);
    }

    lv_obj_t *obj = static_cast<lv_obj_t *>(be_tocomptr(vm, 1));
    int x = be_toint(vm, 2);
    int y = be_toint(vm, 3);
    lv_obj_set_pos(obj, x, y);
    be_pushnil(vm);
    be_return(vm);
}

// ui.set_text(obj, text)
int berry_ui_set_text(bvm *vm)
{
    int argc = be_top(vm);
    if (argc < 2 || !be_iscomptr(vm, 1) || !be_isstring(vm, 2)) {
        be_pushnil(vm);
        be_return(vm);
    }

    lv_obj_t *obj = static_cast<lv_obj_t *>(be_tocomptr(vm, 1));
    const char *text = be_tostring(vm, 2);

    // Check if this is a label or has a child label (button case)
    if (lv_obj_check_type(obj, &lv_label_class)) {
        lv_label_set_text(obj, text);
    } else {
        // Try to find child label
        uint32_t childCount = lv_obj_get_child_count(obj);
        for (uint32_t i = 0; i < childCount; i++) {
            lv_obj_t *child = lv_obj_get_child(obj, i);
            if (lv_obj_check_type(child, &lv_label_class)) {
                lv_label_set_text(child, text);
                break;
            }
        }
    }

    be_pushnil(vm);
    be_return(vm);
}

// Helper: parse hex color string "#RRGGBB" or integer
static lv_color_t parseColor(bvm *vm, int argIndex)
{
    if (be_isint(vm, argIndex)) {
        uint32_t val = be_toint(vm, argIndex);
        return lv_color_hex(val);
    } else if (be_isstring(vm, argIndex)) {
        const char *str = be_tostring(vm, argIndex);
        if (str[0] == '#') {
            uint32_t val = strtoul(str + 1, nullptr, 16);
            return lv_color_hex(val);
        }
    }
    return lv_color_black();
}

// ui.set_style_bg_color(obj, color)
int berry_ui_set_style_bg_color(bvm *vm)
{
    int argc = be_top(vm);
    if (argc < 2 || !be_iscomptr(vm, 1)) {
        be_pushnil(vm);
        be_return(vm);
    }

    lv_obj_t *obj = static_cast<lv_obj_t *>(be_tocomptr(vm, 1));
    lv_color_t color = parseColor(vm, 2);
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    be_pushnil(vm);
    be_return(vm);
}

// ui.set_style_text_color(obj, color)
int berry_ui_set_style_text_color(bvm *vm)
{
    int argc = be_top(vm);
    if (argc < 2 || !be_iscomptr(vm, 1)) {
        be_pushnil(vm);
        be_return(vm);
    }

    lv_obj_t *obj = static_cast<lv_obj_t *>(be_tocomptr(vm, 1));
    lv_color_t color = parseColor(vm, 2);
    lv_obj_set_style_text_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    be_pushnil(vm);
    be_return(vm);
}

// Global callback counter for unique naming
static int berry_callback_counter = 0;

// LVGL event callback that invokes a Berry closure
static void berry_click_event_cb(lv_event_t *e)
{
    BerryEventData *data = static_cast<BerryEventData *>(lv_event_get_user_data(e));
    if (!data || !data->vm)
        return;

    bvm *vm = data->vm;

    // Retrieve the stored function using its unique global name
    char name[32];
    snprintf(name, sizeof(name), "_cb_%d", data->funcRef);
    be_getglobal(vm, name);

    if (be_isfunction(vm, -1) || be_isclosure(vm, -1)) {
        int result = be_pcall(vm, 0);
        if (result != 0) {
            ILOG_ERROR("Berry callback error: %s", be_tostring(vm, -1));
        }
        be_pop(vm, 1);
    } else {
        be_pop(vm, 1);
    }
}

// ui.on_click(obj, closure)
int berry_ui_on_click(bvm *vm)
{
    int argc = be_top(vm);
    if (argc < 2 || !be_iscomptr(vm, 1)) {
        be_pushnil(vm);
        be_return(vm);
    }

    lv_obj_t *obj = static_cast<lv_obj_t *>(be_tocomptr(vm, 1));

    // Store the closure as a uniquely-named global variable
    int cbId = berry_callback_counter++;
    char name[32];
    snprintf(name, sizeof(name), "_cb_%d", cbId);
    be_pushvalue(vm, 2); // push the closure
    be_setglobal(vm, name);
    be_pop(vm, 1);

    // Create event data
    BerryRuntime *runtime = getRuntime(vm);
    BerryEventData *data = new BerryEventData{vm, cbId};
    if (runtime) {
        runtime->eventDataList.push_back(data);
    }

    lv_obj_add_event_cb(obj, berry_click_event_cb, LV_EVENT_CLICKED, data);

    be_pushnil(vm);
    be_return(vm);
}

void berry_lvgl_register(bvm *vm)
{
    ILOG_DEBUG("[Berry] berry_lvgl_register: registering native functions");
    // Register each native function as a global with _ui_ prefix
    be_regfunc(vm, "_ui_root", berry_ui_root);
    be_regfunc(vm, "_ui_label", berry_ui_label);
    be_regfunc(vm, "_ui_button", berry_ui_button);
    be_regfunc(vm, "_ui_container", berry_ui_container);
    be_regfunc(vm, "_ui_set_size", berry_ui_set_size);
    be_regfunc(vm, "_ui_set_pos", berry_ui_set_pos);
    be_regfunc(vm, "_ui_set_text", berry_ui_set_text);
    be_regfunc(vm, "_ui_set_style_bg_color", berry_ui_set_style_bg_color);
    be_regfunc(vm, "_ui_set_style_text_color", berry_ui_set_style_text_color);
    be_regfunc(vm, "_ui_on_click", berry_ui_on_click);
    ILOG_DEBUG("[Berry] berry_lvgl_register: native functions registered");

    // Create the 'ui' module as a Berry class with static-like methods
    // by executing a small bootstrap script
    static const char bootstrap[] =
        "class ui\n"
        "  static def root() return _ui_root() end\n"
        "  static def label(p, t) return _ui_label(p, t) end\n"
        "  static def button(p, t) return _ui_button(p, t) end\n"
        "  static def container(p) return _ui_container(p) end\n"
        "  static def set_size(o, w, h) return _ui_set_size(o, w, h) end\n"
        "  static def set_pos(o, x, y) return _ui_set_pos(o, x, y) end\n"
        "  static def set_text(o, t) return _ui_set_text(o, t) end\n"
        "  static def set_style_bg_color(o, c) return _ui_set_style_bg_color(o, c) end\n"
        "  static def set_style_text_color(o, c) return _ui_set_style_text_color(o, c) end\n"
        "  static def on_click(o, f) return _ui_on_click(o, f) end\n"
        "end\n";

    ILOG_DEBUG("[Berry] berry_lvgl_register: loading bootstrap script");
    int result = be_loadbuffer(vm, "ui_bootstrap", bootstrap, strlen(bootstrap));
    if (result == 0) {
        ILOG_DEBUG("[Berry] berry_lvgl_register: executing bootstrap script");
        be_pcall(vm, 0);
        be_pop(vm, 1);
        ILOG_DEBUG("[Berry] berry_lvgl_register: bootstrap complete");
    } else {
        ILOG_ERROR("[Berry] berry_lvgl_register: bootstrap load failed: %s", be_tostring(vm, -1));
        be_pop(vm, 1);
    }
}

#endif // HAS_APP_FRAMEWORK
