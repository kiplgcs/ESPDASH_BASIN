#pragma once
#include "../lib/Easy Nextion Library/src/SafeNextionDispatch.h"

namespace nextion_dispatch_tests {
constexpr void definedHandler() {}
static_assert(!safe_nextion::invokeIfPresent(nullptr), "Undefined hooks must be skipped");
static_assert(safe_nextion::invokeIfPresent(definedHandler), "Defined hooks must still run");
constexpr bool allFrameLengths() {
  for(unsigned group = 0; group < 256; ++group) {
    for(unsigned length = 0; length < 256; ++length) {
      const bool expected = length != 0 && ((group != 'P' && group != 'T') || length == 2);
      if(safe_nextion::validCommandLength(group, length) != expected) return false;
    }
  }
  return true;
}
static_assert(allFrameLengths(), "Reject incomplete/oversized P/T frames without dispatch");
} // namespace nextion_dispatch_tests
