#pragma once
#include <stdint.h>

namespace safe_nextion {
using Handler = void (*)();

// Undefined weak hooks resolve to nullptr. Never execute them unconditionally.
constexpr bool invokeIfPresent(Handler handler) {
  if(handler == nullptr) return false;
  handler();
  return true;
}

constexpr bool validCommandLength(uint8_t group, uint8_t length) {
  return length != 0 && ((group != 'P' && group != 'T') || length == 2);
}
} // namespace safe_nextion
