/********************************************************************
 * Berry configuration for device-ui (embedded targets)
 * Based on berry-lang/berry default/berry_conf.h
 ********************************************************************/
#ifndef BERRY_CONF_H
#define BERRY_CONF_H

#include <assert.h>

#ifndef BE_DEBUG
#define BE_DEBUG                        0
#endif

/* Use 32-bit int on embedded (saves RAM vs long long) */
#define BE_INTGER_TYPE                  0

/* Use single-precision float on embedded */
#define BE_USE_SINGLE_FLOAT             1

#define BE_USE_PRECOMPILED_OBJECT       1
#define BE_DEBUG_RUNTIME_INFO           1
#define BE_DEBUG_VAR_INFO               0

#define BE_USE_PERF_COUNTERS            1
#define BE_VM_OBSERVABILITY_SAMPLING    20

#define BE_STACK_TOTAL_MAX              2000
#define BE_STACK_FREE_MIN               10
#define BE_STACK_START                  50
#define BE_CONST_SEARCH_SIZE            50
#define BE_USE_STR_HASH_CACHE           0

/* Disable filesystem in Berry - we handle file I/O ourselves */
#define BE_USE_FILE_SYSTEM              0

#define BE_USE_SCRIPT_COMPILER          1
#define BE_USE_BYTECODE_SAVER           0
#define BE_USE_BYTECODE_LOADER          0
#define BE_USE_SHARED_LIB               0
#define BE_USE_OVERLOAD_HASH            1
#define BE_USE_DEBUG_HOOK               0
#define BE_USE_DEBUG_GC                 0
#define BE_USE_DEBUG_STACK              0

/* Modules: enable only what we need to minimize flash */
#define BE_USE_STRING_MODULE            1
#define BE_USE_JSON_MODULE              1
#define BE_USE_MATH_MODULE              1
#define BE_USE_TIME_MODULE              0
#define BE_USE_OS_MODULE                0
#define BE_USE_GLOBAL_MODULE            1
#define BE_USE_SYS_MODULE               0
#define BE_USE_DEBUG_MODULE             0
#define BE_USE_GC_MODULE                0
#define BE_USE_SOLIDIFY_MODULE          0
#define BE_USE_INTROSPECT_MODULE        0
#define BE_USE_STRICT_MODULE            0

/* Memory management */
#define BE_EXPLICIT_ABORT               abort
#define BE_EXPLICIT_EXIT                exit

#if defined(BOARD_HAS_PSRAM) && defined(ARCH_ESP32)
/* Route Berry allocations to PSRAM (prefer SPIRAM, fallback to default) */
#include <esp_heap_caps.h>
static inline void *berry_psram_malloc(size_t size) {
    void *p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) p = malloc(size); /* fallback to internal if PSRAM fails */
    return p;
}
static inline void *berry_psram_realloc(void *ptr, size_t size) {
    if (!ptr) return berry_psram_malloc(size); /* NULL ptr = malloc */
    /* Use MALLOC_CAP_8BIT to allow realloc across heap regions (SPIRAM or internal) */
    void *p = heap_caps_realloc(ptr, size, MALLOC_CAP_8BIT);
    if (!p && size > 0) p = realloc(ptr, size); /* fallback to default */
    return p;
}
#define BE_EXPLICIT_MALLOC              berry_psram_malloc
#define BE_EXPLICIT_FREE                free
#define BE_EXPLICIT_REALLOC             berry_psram_realloc
#else
#define BE_EXPLICIT_MALLOC              malloc
#define BE_EXPLICIT_FREE                free
#define BE_EXPLICIT_REALLOC             realloc
#endif

#define be_assert(expr)                 assert(expr)

#endif
