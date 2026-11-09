#pragma once
#include "../src/lighting_restart_core.h"
namespace lighting_restart_tests {
using namespace lighting_restart;
constexpr bool allModesHoldWithoutReadback() {
  for(unsigned m = 0; m <= unsigned(Mode::Unknown); ++m)
    for(unsigned held = 0; held < 2; ++held)
      for(unsigned inputs = 0; inputs < 32; ++inputs)
        if(evaluate(Mode(m), held, false, inputs & 1, inputs & 2, inputs & 4,
                    inputs & 8 ? 0 : 100, 75, 85) != bool(held)) return false;
  return true;
}
constexpr Record record(uint32_t sequence) {
  Record r = {kMagic, kVersion, sequence, {{uint32_t(Mode::On), 1, 7}, {uint32_t(Mode::Off), 0, 8}}, 0};
  r.checksum = checksum(r); return r;
}
constexpr bool rejectsCorruption() {
  auto r = record(1); r.channels[0].output = 0; return !valid(r);
}
static_assert(allModesHoldWithoutReadback(), "Boot cannot change R8/R4 before readback: 384 cases");
static_assert(recoverFromReadback(Mode::Off, false, true) == Mode::On, "Stale NVS OFF cannot turn an ON relay OFF at cold boot");
static_assert(recoverFromReadback(Mode::On, false, false) == Mode::Off, "Stale NVS ON cannot turn an OFF relay ON at cold boot");
static_assert(recoverFromReadback(Mode::Off, true, true) == Mode::Off, "Verified manual OFF intent wins over lagging readback");
static_assert(recoverFromReadback(Mode::On, true, false) == Mode::On, "Verified manual ON intent wins over lagging readback");
static_assert(recoverFromReadback(Mode::Timer, false, true) == Mode::Timer, "Cold boot preserves configured automatic mode");
static_assert(valid(record(1)) && rejectsCorruption(), "RTC record integrity");
static_assert(newest(record(2), record(1)) == 0 && newest(record(1), record(2)) == 1, "Choose newest RTC intent");
static_assert(newest(record(0xffffffffU), record(1)) == 1, "RTC sequence wrap");
static_assert(evaluate(Mode::On, false, true, false, false, false, 100, 75, 85), "Manual ON ignores unavailable time/ADC");
static_assert(!evaluate(Mode::Off, true, true, false, false, false, 0, 75, 85), "Manual OFF remains OFF in darkness");
static_assert(evaluate(Mode::Timer, true, true, false, false, true, 0, 75, 85), "Missing time must not switch a timer OFF");
static_assert(!evaluate(Mode::Timer, false, true, false, true, true, 0, 75, 85), "Missing time must not switch a timer ON");
static_assert(evaluate(Mode::Auto, true, true, true, false, false, 100, 75, 85), "Missing ADC holds ON");
static_assert(!evaluate(Mode::Auto, false, true, true, false, false, 0, 75, 85), "Default dark ADC must not turn ON");
static_assert(evaluate(Mode::Auto, true, true, true, false, true, 80, 75, 85), "Preserve ON in hysteresis band");
static_assert(!evaluate(Mode::Auto, false, true, true, false, true, 80, 75, 85), "Preserve OFF in hysteresis band");
static_assert(evaluate(Mode::Auto, false, true, true, false, true, 0, 75, 85), "Real dark sample turns ON");
static_assert(!evaluate(Mode::Auto, true, true, true, false, true, 100, 75, 85), "Real bright sample turns OFF");
static_assert(evaluate(Mode::Timer, false, true, true, true, false, 0, 75, 85), "Valid timer ON");
static_assert(!evaluate(Mode::Timer, true, true, true, false, false, 0, 75, 85), "Valid timer OFF");
static_assert(evaluate(Mode::AutoTimerOff, true, true, false, false, false, 0, 75, 85), "Hybrid holds without time");
static_assert(!evaluate(Mode::AutoTimerOff, true, true, true, false, true, 0, 75, 85), "Hybrid timer OFF");
} // namespace lighting_restart_tests
