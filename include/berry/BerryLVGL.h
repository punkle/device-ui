#pragma once

#ifdef HAS_APP_FRAMEWORK

#include "berry.h"

// Register all LVGL bindings as global functions in the Berry VM
void berry_lvgl_register(bvm *vm);

// Individual binding functions
int berry_ui_root(bvm *vm);
int berry_ui_label(bvm *vm);
int berry_ui_button(bvm *vm);
int berry_ui_container(bvm *vm);
int berry_ui_set_size(bvm *vm);
int berry_ui_set_pos(bvm *vm);
int berry_ui_set_text(bvm *vm);
int berry_ui_set_style_bg_color(bvm *vm);
int berry_ui_set_style_text_color(bvm *vm);
int berry_ui_on_click(bvm *vm);

#endif // HAS_APP_FRAMEWORK
