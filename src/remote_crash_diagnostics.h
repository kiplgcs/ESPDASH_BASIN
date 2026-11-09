#pragma once
#include <Arduino.h>
#ifndef REMOTE_CRASH_DIAGNOSTICS
#define REMOTE_CRASH_DIAGNOSTICS 0
#endif

// Temporary diagnostics. The lighting restart guard does not depend on this file.
namespace remote_crash {
#if REMOTE_CRASH_DIAGNOSTICS
void begin();
void checkpoint(uint32_t stage);
String json();
bool hasRecord();
#else
inline void begin() {}
inline void checkpoint(uint32_t) {}
inline String json() { return "{\"enabled\":false}"; }
inline bool hasRecord() { return false; }
#endif
} // namespace remote_crash
