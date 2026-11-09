#pragma once
#include <Arduino.h>
#include <esp_system.h>
#include "wifi_manager.h"
#include "lighting_restart_core.h"

namespace lighting_restart {
// Two commit slots: an interrupted write cannot destroy the preceding intent.
RTC_NOINIT_ATTR Record retained[2];
inline portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
inline Channel channels[2] = {{uint32_t(Mode::Unknown), 0, 1}, {uint32_t(Mode::Unknown), 0, 1}};
inline bool readbackSeen[2] = {false, false};
inline bool explicitIntent[2] = {false, false};
inline bool bootUiLoading = true;
inline uint32_t recordSequence = 0;
inline int recordSlot = -1;
inline const char *bootSource[2] = {"readback_pending", "readback_pending"};
inline const char *const storageKeys[2] = {"SetYardLight", "SetFenceLight"};
inline PersistenceJobId pendingJob[2] = {0, 0};
inline uint32_t queuedCommand[2] = {0, 0};
inline uint32_t persistedCommand[2] = {0, 0};
inline uint32_t retryAt[2] = {0, 0};
inline const char *persistenceState[2] = {"pending", "pending"};

inline Mode parse(String value) {
  value.trim(); value.toLowerCase();
  if(value == "off") return Mode::Off;
  if(value == "on") return Mode::On;
  if(value == "auto") return Mode::Auto;
  if(value == "auto_timer_off") return Mode::AutoTimerOff;
  if(value == "timer") return Mode::Timer;
  return Mode::Unknown;
}
inline Channel snapshot(unsigned channel) {
  portENTER_CRITICAL(&mux);
  const Channel value = channels[channel];
  portEXIT_CRITICAL(&mux);
  return value;
}
inline Mode mode(unsigned channel) { return Mode(snapshot(channel).mode); }
inline bool ready(unsigned channel) {
  portENTER_CRITICAL(&mux);
  const bool value = readbackSeen[channel] && channels[channel].mode != uint32_t(Mode::Unknown);
  portEXIT_CRITICAL(&mux);
  return value;
}
// Caller owns mux. No allocation, FreeRTOS wait or Flash write in this function.
inline void commitRetainedLocked() {
  Record next = {kMagic, kVersion, ++recordSequence, {channels[0], channels[1]}, 0};
  next.checksum = checksum(next);
  const int target = recordSlot == 0 ? 1 : 0;
  volatile Record *destination = &retained[target];
  destination->magic = 0;
  destination->version = next.version;
  destination->sequence = next.sequence;
  for(unsigned i = 0; i < 2; ++i) {
    destination->channels[i].mode = next.channels[i].mode;
    destination->channels[i].output = next.channels[i].output;
    destination->channels[i].command = next.channels[i].command;
  }
  destination->checksum = next.checksum;
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  destination->magic = kMagic;
  recordSlot = target;
}
inline Mode readStoredMode(unsigned channel) {
  String value;
  lockPrefs();
  if(prefs.begin(valuePreferencesNamespace, true)) {
    if(prefs.isKey(storageKeys[channel]) && prefs.getType(storageKeys[channel]) == PT_STR)
      value = prefs.getString(storageKeys[channel], "");
    prefs.end();
  }
  unlockPrefs();
  // Unlike loadValue(), a failed read never writes a default OFF over the key.
  return parse(value);
}
inline void begin() {
  const esp_reset_reason_t reset = esp_reset_reason();
  const bool warm = reset == ESP_RST_SW || reset == ESP_RST_PANIC || reset == ESP_RST_INT_WDT ||
                    reset == ESP_RST_TASK_WDT || reset == ESP_RST_WDT;
  recordSlot = warm ? newest(retained[0], retained[1]) : -1;
  if(recordSlot >= 0) recordSequence = retained[recordSlot].sequence;
  for(unsigned i = 0; i < 2; ++i) {
    if(recordSlot >= 0 && retained[recordSlot].channels[i].mode != uint32_t(Mode::Unknown) &&
       retained[recordSlot].channels[i].command != 0) {
      channels[i] = retained[recordSlot].channels[i];
      bootSource[i] = "rtc_last_intent";
      explicitIntent[i] = true;
    } else {
      const Mode saved = readStoredMode(i);
      channels[i] = {uint32_t(saved), saved == Mode::On ? 1U : 0U, 0};
      bootSource[i] = saved == Mode::Unknown ? "readback_pending" : "nvs";
    }
    if(channels[i].mode == uint32_t(Mode::On)) channels[i].output = 1;
    if(channels[i].mode == uint32_t(Mode::Off)) channels[i].output = 0;
  }
  portENTER_CRITICAL(&mux);
  commitRetainedLocked();
  portEXIT_CRITICAL(&mux);
}
inline void request(unsigned channel, Mode requested) {
  if(requested == Mode::Unknown || bootUiLoading) return;
  portENTER_CRITICAL(&mux);
  Channel &state = channels[channel];
  explicitIntent[channel] = true;
  state.mode = uint32_t(requested);
  // An automatic mode starts from the existing output, never from a false default.
  if(requested == Mode::On) state.output = 1;
  if(requested == Mode::Off) state.output = 0;
  if(++state.command == 0) ++state.command;
  commitRetainedLocked();
  portEXIT_CRITICAL(&mux);
}
inline void observe(unsigned channel, bool actual) {
  portENTER_CRITICAL(&mux);
  if(!readbackSeen[channel]) {
    Channel &state = channels[channel];
    const Mode recovered = recoverFromReadback(Mode(state.mode), explicitIntent[channel], actual);
    if(!explicitIntent[channel] && !automatic(Mode(state.mode))) {
      state.mode = uint32_t(recovered);
      state.output = actual ? 1 : 0;
      bootSource[channel] = "relay_readback_no_trusted_intent";
    }
    if(automatic(Mode(state.mode))) state.output = actual ? 1 : 0;
    if(state.command == 0) state.command = 1;
    explicitIntent[channel] = true;
    readbackSeen[channel] = true;
    commitRetainedLocked();
  }
  portEXIT_CRITICAL(&mux);
}
inline bool update(unsigned channel, const Channel &observed, bool desired) {
  portENTER_CRITICAL(&mux);
  Channel &state = channels[channel];
  // A result computed before a newer manual command must not overwrite that command.
  if(state.command == observed.command && state.output != uint32_t(desired)) {
    state.output = desired ? 1 : 0;
    commitRetainedLocked();
  }
  const bool output = state.output != 0;
  portEXIT_CRITICAL(&mux);
  return output;
}
inline void servicePersistence() {
  // One network-task owner submits/checks all durable mode writes in order.
  for(unsigned i = 0; i < 2; ++i) {
    const Channel current = snapshot(i);
    if(current.mode == uint32_t(Mode::Unknown) || current.command == 0) continue;
    if(pendingJob[i]) {
      const PersistenceJobInfo job = persistenceJobStatus(pendingJob[i]);
      if(job.known() && !job.terminal()) continue;
      if(job.succeeded()) { persistedCommand[i] = queuedCommand[i]; persistenceState[i] = "succeeded"; }
      else { persistenceState[i] = persistenceErrorName(job.error); retryAt[i] = millis() + 5000; }
      pendingJob[i] = 0;
    }
    if(current.command == persistedCommand[i] || int32_t(millis() - retryAt[i]) < 0) continue;
    const auto submission = saveValueTrackedResult<String>(storageKeys[i], String(name(Mode(current.mode))));
    if(submission.accepted) {
      pendingJob[i] = submission.jobId;
      queuedCommand[i] = current.command;
      persistenceState[i] = "pending";
    } else {
      persistenceState[i] = persistenceErrorName(submission.error);
      retryAt[i] = millis() + 5000;
    }
  }
}
} // namespace lighting_restart
