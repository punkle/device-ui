#pragma once

#include "lvgl.h"

class AppRuntime;

/**
 * Register Berry "ui" module bindings that map to LVGL widget operations.
 * Must be called before runtime->start().
 *
 * @param runtime  The Berry app runtime to register bindings on
 * @param container  The LVGL container object that serves as the app's root
 */
void registerLvglBindings(AppRuntime *runtime, lv_obj_t *container);
