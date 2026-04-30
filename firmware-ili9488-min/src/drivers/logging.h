#pragma once

#include <Arduino.h>

// ============================================================
// Arduino-style logging macros
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

// Note: ESP32 Arduino framework already defines __FILENAME__ in assert.h
// We use a different name to avoid redefinition warnings
#ifndef SRC_FILE
#define SRC_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#if LOG_ENABLED

// Error level (always printed if LOG_ENABLED)
#define LOG_E(fmt, ...) do { \
  Serial.printf("[E][%s:%d] " fmt "\r\n", SRC_FILE, __LINE__, ##__VA_ARGS__); \
} while(0)

// Warn level
#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_W(fmt, ...) do { \
  Serial.printf("[W][%s:%d] " fmt "\r\n", SRC_FILE, __LINE__, ##__VA_ARGS__); \
} while(0)
#else
#define LOG_W(fmt, ...) do {} while(0)
#endif

// Info level
#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_I(fmt, ...) do { \
  Serial.printf("[I][%s:%d] " fmt "\r\n", SRC_FILE, __LINE__, ##__VA_ARGS__); \
} while(0)
#else
#define LOG_I(fmt, ...) do {} while(0)
#endif

// Debug level
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_D(fmt, ...) do { \
  Serial.printf("[D][%s:%d] " fmt "\r\n", SRC_FILE, __LINE__, ##__VA_ARGS__); \
} while(0)
#else
#define LOG_D(fmt, ...) do {} while(0)
#endif

// Verbose level
#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
#define LOG_V(fmt, ...) do { \
  Serial.printf("[V][%s:%d] " fmt "\r\n", SRC_FILE, __LINE__, ##__VA_ARGS__); \
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
