#pragma once
#include <stdint.h>

// Pure state machine: no Arduino String, heap, network or Flash access.
namespace lighting_restart {
enum class Mode : uint32_t { Off, On, Auto, AutoTimerOff, Timer, Unknown };
struct Channel { uint32_t mode; uint32_t output; uint32_t command; };
struct Record {
  uint32_t magic;
  uint32_t version;
  uint32_t sequence;
  Channel channels[2];
  uint32_t checksum;
};
constexpr uint32_t kMagic = 0x4C475231;
constexpr uint32_t kVersion = 1;
constexpr uint32_t mix(uint32_t hash, uint32_t value) {
  for(unsigned i = 0; i < 4; ++i) { hash = (hash ^ (value & 255U)) * 16777619U; value >>= 8; }
  return hash;
}
constexpr uint32_t checksum(const Record &r) {
  uint32_t hash = mix(mix(mix(2166136261U, kMagic), kVersion), r.sequence);
  for(const auto &c : r.channels) hash = mix(mix(mix(hash, c.mode), c.output), c.command);
  return hash;
}
constexpr bool valid(const Record &r) {
  if(r.magic != kMagic || r.version != kVersion || r.checksum != checksum(r)) return false;
  for(const auto &c : r.channels) if(c.mode > uint32_t(Mode::Unknown) || c.output > 1) return false;
  return true;
}
constexpr bool newer(uint32_t a, uint32_t b) { return int32_t(a - b) > 0; }
constexpr int newest(const Record &a, const Record &b) {
  return valid(a) ? (valid(b) && newer(b.sequence, a.sequence) ? 1 : 0) : (valid(b) ? 1 : -1);
}
constexpr bool automatic(Mode mode) { return mode == Mode::Auto || mode == Mode::AutoTimerOff || mode == Mode::Timer; }
constexpr Mode recoverFromReadback(Mode configured, bool explicitIntent, bool actual) {
  // Without a verified warm-reset intent, stale NVS must not toggle a manual relay.
  return !explicitIntent && !automatic(configured) ? (actual ? Mode::On : Mode::Off) : configured;
}
constexpr const char *name(Mode mode) {
  return mode == Mode::Off ? "off" : mode == Mode::On ? "on" : mode == Mode::Auto ? "auto" :
         mode == Mode::AutoTimerOff ? "auto_timer_off" : mode == Mode::Timer ? "timer" : "unknown";
}
constexpr bool evaluate(Mode mode, bool held, bool readbackSeen, bool timeReady,
                        bool window, bool sensorReady, int lumen, int darkOn, int brightOff) {
  if(!readbackSeen || mode == Mode::Unknown) return held;
  if(mode == Mode::Off) return false;
  if(mode == Mode::On) return true;
  if(mode == Mode::Timer) return timeReady ? window : held;
  if(mode == Mode::AutoTimerOff) {
    if(!timeReady) return held;
    if(!window) return false;
    return held || (sensorReady && lumen <= darkOn);
  }
  if(!sensorReady) return held;
  if(!held && lumen <= darkOn) return true;
  if(held && lumen >= brightOff) return false;
  return held;
}
} // namespace lighting_restart
