#include "remote_crash_diagnostics.h"
#include <esp_attr.h>
#include <esp_system.h>
#include <esp_private/panic_internal.h>
#include <xtensa/xtensa_context.h>
#include <soc/soc.h>
#include <soc/soc_memory_layout.h>
#include <ArduinoJson.h>
#include <nvs.h>
#include <EasyNextionLibrary.h>

extern EasyNex myNex;

extern "C" void __real_esp_panic_handler(panic_info_t *) __attribute__((noreturn));

#if REMOTE_CRASH_DIAGNOSTICS
namespace remote_crash {
constexpr uint32_t kMagic = 0x50414E31;
constexpr unsigned kStackWords = 64;
struct CrashRecord {
  uint32_t magic, version, core, exception, pc, sp, ra, cause, address;
  uint32_t stage[2], uptime[2], stackCount;
  uint32_t firmwareMd5[8];
  uint32_t stack[kStackWords];
  uint32_t checksum;
};
static_assert(sizeof(CrashRecord) == 87 * sizeof(uint32_t), "Panic record must have a stable word-only layout");
RTC_NOINIT_ATTR CrashRecord retainedCrash;
struct Progress { volatile uint32_t stage; volatile uint32_t uptime; };
DRAM_ATTR Progress progress[2] = {};
DRAM_ATTR volatile uint32_t captureStarted = 0;
DRAM_ATTR uint32_t runningMd5[8] = {};
inline bool available = false;

// All panic-time operations stay in internal memory. No Flash writes, heap,
// String, serial output, FreeRTOS locks or network access in the wrapper.
inline __attribute__((always_inline)) uint32_t mixWord(uint32_t hash, uint32_t word) {
  for(unsigned i = 0; i < 4; ++i) { hash = (hash ^ (word & 255U)) * 16777619U; word >>= 8; }
  return hash;
}
uint32_t IRAM_ATTR recordChecksum(const volatile CrashRecord *r) {
  // Word accesses also work in RTC memory that cannot be read byte-wise.
  uint32_t hash = mixWord(2166136261U, r->version);
  hash = mixWord(hash, r->core); hash = mixWord(hash, r->exception);
  hash = mixWord(hash, r->pc); hash = mixWord(hash, r->sp); hash = mixWord(hash, r->ra);
  hash = mixWord(hash, r->cause); hash = mixWord(hash, r->address);
  for(unsigned i = 0; i < 2; ++i) hash = mixWord(hash, r->stage[i]);
  for(unsigned i = 0; i < 2; ++i) hash = mixWord(hash, r->uptime[i]);
  hash = mixWord(hash, r->stackCount);
  for(unsigned i = 0; i < 8; ++i) hash = mixWord(hash, r->firmwareMd5[i]);
  for(unsigned i = 0; i < kStackWords; ++i) hash = mixWord(hash, r->stack[i]);
  return hash;
}
void IRAM_ATTR capture(panic_info_t *info) {
  if(!info || captureStarted) return;
  captureStarted = 1;
  volatile CrashRecord *record = &retainedCrash;
  record->magic = 0;
  record->version = 1;
  record->core = info->core;
  record->exception = g_panic_abort ? uint32_t(PANIC_EXCEPTION_ABORT) : uint32_t(info->exception);
  record->pc = reinterpret_cast<uint32_t>(info->addr);
  record->sp = 0; record->ra = 0; record->cause = 0; record->address = 0;
  record->stackCount = 0;
  for(unsigned i = 0; i < 2; ++i) {
    record->stage[i] = progress[i].stage;
    record->uptime[i] = progress[i].uptime;
  }
  for(unsigned i = 0; i < 8; ++i) record->firmwareMd5[i] = runningMd5[i];
  for(unsigned i = 0; i < kStackWords; ++i) record->stack[i] = 0;
  const uintptr_t frameAddress = reinterpret_cast<uintptr_t>(info->frame);
  if((frameAddress & 3U) == 0 && frameAddress >= SOC_DRAM_LOW &&
     frameAddress <= SOC_DRAM_HIGH - sizeof(XtExcFrame)) {
    const volatile XtExcFrame *frame = static_cast<const volatile XtExcFrame *>(info->frame);
    record->pc = frame->pc;
    record->sp = frame->a1;
    record->ra = frame->a0;
    record->cause = frame->exccause;
    record->address = frame->excvaddr;
    const uintptr_t stack = uint32_t(frame->a1);
    if((stack & 3U) == 0 && stack >= SOC_DRAM_LOW && stack <= SOC_DRAM_HIGH - 4) {
      const unsigned count = min(unsigned(kStackWords), unsigned((SOC_DRAM_HIGH - stack) / 4));
      const volatile uint32_t *words = reinterpret_cast<const volatile uint32_t *>(stack);
      for(unsigned i = 0; i < count; ++i) record->stack[i] = words[i];
      record->stackCount = count;
    }
  }
  record->checksum = recordChecksum(record);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  record->magic = kMagic;
}
void begin() {
  const String md5 = ESP.getSketchMD5();
  for(unsigned i = 0; i < 8 && md5.length() == 32; ++i) {
    runningMd5[i] = 0;
    for(unsigned j = 0; j < 4; ++j) runningMd5[i] |= uint32_t(uint8_t(md5[4*i+j])) << (8*j);
  }
  const auto reset = esp_reset_reason();
  const bool warm = reset == ESP_RST_PANIC || reset == ESP_RST_SW || reset == ESP_RST_INT_WDT ||
                    reset == ESP_RST_TASK_WDT || reset == ESP_RST_WDT;
  available = warm && retainedCrash.magic == kMagic && retainedCrash.version == 1 &&
              retainedCrash.stackCount <= kStackWords && retainedCrash.checksum == recordChecksum(&retainedCrash);
  if(!available) retainedCrash.magic = 0;
}
bool hasRecord() { return available; }
void checkpoint(uint32_t stage) {
  const unsigned core = xPortGetCoreID();
  if(core < 2) { progress[core].stage = stage; progress[core].uptime = millis(); }
}
String hexAddress(uint32_t address) {
  char value[11]; snprintf(value, sizeof(value), "0x%08lx", static_cast<unsigned long>(address));
  return String(value);
}
String json() {
  JsonDocument doc;
  doc["enabled"] = true;
  doc["build"] = __DATE__ " " __TIME__;
  doc["sketch_md5"] = String(reinterpret_cast<const char *>(runningMd5), 32);
  doc["reset_reason_code"] = unsigned(esp_reset_reason());
  doc["crash_available"] = available;
  JsonObject nextion = doc["nextion_rx"].to<JsonObject>();
  nextion["rejected_count"] = myNex.rejectedCommandCount;
  nextion["last_length"] = myNex.lastRejectedLength;
  nextion["last_group"] = myNex.lastRejectedGroup;
  nextion["last_payload"] = myNex.lastRejectedPayload;
  nextion["last_reason"] = myNex.lastRejectedReason;
  nvs_stats_t stats = {};
  const esp_err_t nvsError = nvs_get_stats(nullptr, &stats);
  JsonObject storage = doc["nvs_stats"].to<JsonObject>();
  storage["error"] = esp_err_to_name(nvsError);
  if(nvsError == ESP_OK) {
    storage["used_entries"] = stats.used_entries;
    storage["free_entries"] = stats.free_entries;
    storage["total_entries"] = stats.total_entries;
    storage["namespace_count"] = stats.namespace_count;
  }
  if(available) {
    doc["crash_sketch_md5"] = String(reinterpret_cast<const char *>(retainedCrash.firmwareMd5), 32);
    doc["core"] = retainedCrash.core;
    doc["exception"] = retainedCrash.exception;
    doc["pc"] = hexAddress(retainedCrash.pc);
    doc["sp"] = hexAddress(retainedCrash.sp);
    doc["return_address"] = hexAddress(retainedCrash.ra);
    doc["exception_cause"] = retainedCrash.cause;
    doc["fault_address"] = hexAddress(retainedCrash.address);
    JsonArray stages = doc["last_stages"].to<JsonArray>();
    JsonArray uptimes = doc["last_uptime_ms"].to<JsonArray>();
    for(unsigned i = 0; i < 2; ++i) { stages.add(retainedCrash.stage[i]); uptimes.add(retainedCrash.uptime[i]); }
    JsonArray stack = doc["raw_stack_words"].to<JsonArray>();
    for(unsigned i = 0; i < retainedCrash.stackCount; ++i) stack.add(hexAddress(retainedCrash.stack[i]));
    doc["stack_note"] = "Raw stack candidates, not an unwound backtrace. Decode with matching firmware.elf.";
  }
  String result; serializeJson(doc, result); return result;
}
} // namespace remote_crash
#endif

extern "C" void IRAM_ATTR __wrap_esp_panic_handler(panic_info_t *info) {
#if REMOTE_CRASH_DIAGNOSTICS
  remote_crash::capture(info);
#endif
  __real_esp_panic_handler(info);
}
