#include "graphics/AppLvglBindings.h"
#include "mapps/AppRuntime.h"
#include "util/ILog.h"
#include <map>
#include <string>
#include <vector>

// Static state for the currently active app runtime (one app at a time)
static AppRuntime *s_activeRuntime = nullptr;
static lv_obj_t *s_rootContainer = nullptr;

// Indirection table: maps small integer IDs to LVGL object pointers.
// Avoids truncating 64-bit pointers to 32-bit int on portduino/native builds.
static std::vector<lv_obj_t *> s_objTable;

static int ptrToId(lv_obj_t *obj)
{
    for (int i = 0; i < (int)s_objTable.size(); i++) {
        if (s_objTable[i] == obj)
            return i;
    }
    int id = (int)s_objTable.size();
    s_objTable.push_back(obj);
    return id;
}

static lv_obj_t *idToPtr(int id)
{
    if (id < 0 || id >= (int)s_objTable.size())
        return nullptr;
    return s_objTable[id];
}

// LVGL click event callback — dispatches to Berry _ui_handle_click
static void lvgl_click_cb(lv_event_t *e)
{
    if (!s_activeRuntime)
        return;
    lv_obj_t *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
    s_activeRuntime->call("_ui_handle_click", {AppValue(ptrToId(obj))});
}

void registerLvglBindings(AppRuntime *runtime, lv_obj_t *container)
{
    s_activeRuntime = runtime;
    s_rootContainer = container;
    s_objTable.clear();

    std::map<std::string, NativeAppFunction> bindings;

    // ui.root() → returns container ID
    bindings["root"] = [](const std::vector<AppValue> &args) -> AppValue {
        return AppValue(ptrToId(s_rootContainer));
    };

    // ui.label(parent, text) → creates label, returns ID
    bindings["label"] = [](const std::vector<AppValue> &args) -> AppValue {
        if (args.size() < 2 || args[0].type != AppValue::Int || args[1].type != AppValue::String)
            return AppValue();
        lv_obj_t *parent = idToPtr(args[0].intVal);
        if (!parent)
            return AppValue();
        lv_obj_t *label = lv_label_create(parent);
        lv_label_set_text(label, args[1].strVal.c_str());
        return AppValue(ptrToId(label));
    };

    // ui.button(parent, text) → creates button with child label, returns button ID
    bindings["button"] = [](const std::vector<AppValue> &args) -> AppValue {
        if (args.size() < 2 || args[0].type != AppValue::Int || args[1].type != AppValue::String)
            return AppValue();
        lv_obj_t *parent = idToPtr(args[0].intVal);
        if (!parent)
            return AppValue();
        lv_obj_t *btn = lv_button_create(parent);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, args[1].strVal.c_str());
        lv_obj_center(label);
        return AppValue(ptrToId(btn));
    };

    // ui.set_pos(obj, x, y)
    bindings["set_pos"] = [](const std::vector<AppValue> &args) -> AppValue {
        if (args.size() < 3 || args[0].type != AppValue::Int || args[1].type != AppValue::Int ||
            args[2].type != AppValue::Int)
            return AppValue();
        lv_obj_t *obj = idToPtr(args[0].intVal);
        if (!obj)
            return AppValue();
        lv_obj_set_pos(obj, args[1].intVal, args[2].intVal);
        return AppValue();
    };

    // ui.set_size(obj, w, h)
    bindings["set_size"] = [](const std::vector<AppValue> &args) -> AppValue {
        if (args.size() < 3 || args[0].type != AppValue::Int || args[1].type != AppValue::Int ||
            args[2].type != AppValue::Int)
            return AppValue();
        lv_obj_t *obj = idToPtr(args[0].intVal);
        if (!obj)
            return AppValue();
        lv_obj_set_size(obj, args[1].intVal, args[2].intVal);
        return AppValue();
    };

    // ui.set_text(obj, text)
    bindings["set_text"] = [](const std::vector<AppValue> &args) -> AppValue {
        if (args.size() < 2 || args[0].type != AppValue::Int || args[1].type != AppValue::String)
            return AppValue();
        lv_obj_t *obj = idToPtr(args[0].intVal);
        if (!obj)
            return AppValue();
        lv_label_set_text(obj, args[1].strVal.c_str());
        return AppValue();
    };

    // ui.on_click(obj) — registers LVGL click handler; Berry callback dispatch via bootstrap
    bindings["on_click"] = [](const std::vector<AppValue> &args) -> AppValue {
        if (args.size() < 1 || args[0].type != AppValue::Int)
            return AppValue();
        lv_obj_t *obj = idToPtr(args[0].intVal);
        if (!obj)
            return AppValue();
        lv_obj_add_event_cb(obj, lvgl_click_cb, LV_EVENT_CLICKED, NULL);
        return AppValue();
    };

    runtime->addBindings("ui", bindings);

    // Custom bootstrap: wraps native _ui_* functions in a Berry class with
    // click callback dispatch via a global handler map
    runtime->setBootstrap("ui",
                          "var _click_handlers = {}\n"
                          "class ui\n"
                          "  static def root() return _ui_root() end\n"
                          "  static def label(p, t) return _ui_label(p, t) end\n"
                          "  static def button(p, t) return _ui_button(p, t) end\n"
                          "  static def set_pos(o, x, y) _ui_set_pos(o, x, y) end\n"
                          "  static def set_size(o, w, h) _ui_set_size(o, w, h) end\n"
                          "  static def set_text(o, t) _ui_set_text(o, t) end\n"
                          "  static def on_click(o, cb)\n"
                          "    _click_handlers[str(o)] = cb\n"
                          "    _ui_on_click(o)\n"
                          "  end\n"
                          "end\n"
                          "def _ui_handle_click(obj_ptr)\n"
                          "  var cb = _click_handlers.find(str(obj_ptr))\n"
                          "  if cb cb() end\n"
                          "end\n");
}
