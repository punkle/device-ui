#pragma once

#ifdef HAS_APP_FRAMEWORK

#include "berry.h"

// Register HTTP and app capability bindings in the Berry VM
void berry_http_register(bvm *vm);

// Individual binding functions
int berry_http_get(bvm *vm);
int berry_http_post(bvm *vm);
int berry_http_supported(bvm *vm);
int berry_app_has_capability(bvm *vm);

#endif // HAS_APP_FRAMEWORK
