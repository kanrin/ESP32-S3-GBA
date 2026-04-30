#pragma once

#include "esp_log.h"

// ============================================================
// ESP-IDF style logging macros
// Usage:
//   LOG_I("System initialized");
//   LOG_W("Low memory: %d bytes free", freeHeap);
//   LOG_E("Failed to mount SD card");
// ============================================================

// Enable/disable logging globally
#ifndef LOG_ENABLED
#define LOG_ENABLED 1
#endif

// Log level configuration
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_VERBOSE 4

// Note: ESP-IDF's ESP_LOGx macros already include file/line info via TAG
// We use a custom tag for our application
#define APP_TAG "GBA"

#if LOG_ENABLED

// Error level (always printed if LOG_ENABLED)
#define LOG_E(fmt, ...) do { \
  ESP_LOGE(APP_TAG, fmt, ##__VA_ARGS__); \
} while(0)

// Warn level
#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_W(fmt, ...) do { \
  ESP_LOGW(APP_TAG, fmt, ##__VA_ARGS__); \
} while(0)
#else
#define LOG_W(fmt, ...) do {} while(0)
#endif

// Info level
#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_I(fmt, ...) do { \
  ESP_LOGI(APP_TAG, fmt, ##__VA_ARGS__); \
} while(0)
#else
#define LOG_I(fmt, ...) do {} while(0)
#endif

// Debug level
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_D(fmt, ...) do { \
  ESP_LOGD(APP_TAG, fmt, ##__VA_ARGS__); \
} while(0)
#else
#define LOG_D(fmt, ...) do {} while(0)
#endif

// Verbose level
#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
#define LOG_V(fmt, ...) do { \
  ESP_LOGV(APP_TAG, fmt, ##__VA_ARGS__); \
} while(0)
#else
#define LOG_V(fmt, ...) do {} while(0)
#endif

#else // LOG_ENABLED == 0

#define LOG_E(fmt, ...) do {} while(0)
#define LOG_W(fmt, ...) do {} while(0)
#define LOG_I(fmt, ...) do {} while(0)
#define LOG_D(fmt, ...) do {} while(0)
#define LOG_V(fmt, ...) do {} while(0)

#endif // LOG_ENABLED
