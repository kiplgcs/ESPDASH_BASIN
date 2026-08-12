#pragma once

// This private header is included by wifi_manager.h after PreferencesStorageKey,
// the NVS namespaces, prefs and the prefs lock have been declared.

#include <Arduino.h>
#include <Preferences.h>
#include <cstring>
#include <type_traits>

using PersistenceJobId = uint32_t;

enum class PersistenceJobState : uint8_t {
  Unknown = 0,
  Pending,
  Writing,
  Succeeded,
  Failed,
  Superseded,
  Rejected
};

enum class PersistenceError : uint8_t {
  None = 0,
  InvalidKey,
  QueueBusy,
  QueueFull,
  WorkerStartFailed,
  WorkerNotRunning,
  ProducerTrackingFull,
  PreferencesBeginFailed,
  PreferencesPutFailed,
  ReadbackFailed,
  PriorJobFailed
};

struct PersistenceJobInfo {
  PersistenceJobId id = 0;
  PersistenceJobState state = PersistenceJobState::Unknown;
  PersistenceError error = PersistenceError::None;
  PersistenceJobId supersededBy = 0;
  uint32_t updatedAtMs = 0;

  bool known() const { return id != 0 && state != PersistenceJobState::Unknown; }
  bool terminal() const {
    return state == PersistenceJobState::Succeeded ||
           state == PersistenceJobState::Failed ||
           state == PersistenceJobState::Superseded ||
           state == PersistenceJobState::Rejected;
  }
  bool succeeded() const { return state == PersistenceJobState::Succeeded; }
};

struct PersistenceSubmission {
  PersistenceJobId jobId = 0;
  bool accepted = false;
  bool completedSynchronously = false;
  bool succeeded = false;
  PersistenceError error = PersistenceError::None;
};

struct PersistenceQueueStats {
  uint32_t accepted = 0;
  uint32_t coalesced = 0;
  uint32_t rejected = 0;
  uint32_t succeeded = 0;
  uint32_t failed = 0;
  uint32_t unchanged = 0;
  uint16_t pending = 0;
  bool workerRunning = false;
  bool runtimeRequired = false;
  bool workerStartAttempted = false;
  bool workerStartFailed = false;
};

struct PersistenceFlushResult {
  bool drained = false;
  uint32_t succeeded = 0;
  uint32_t failed = 0;
  uint32_t rejected = 0;
  PersistenceError error = PersistenceError::None;
};

inline const char *persistenceJobStateName(PersistenceJobState state) {
  switch(state){
    case PersistenceJobState::Pending: return "pending";
    case PersistenceJobState::Writing: return "writing";
    case PersistenceJobState::Succeeded: return "succeeded";
    case PersistenceJobState::Failed: return "failed";
    case PersistenceJobState::Superseded: return "superseded";
    case PersistenceJobState::Rejected: return "rejected";
    default: return "unknown";
  }
}

inline const char *persistenceErrorName(PersistenceError error) {
  switch(error){
    case PersistenceError::InvalidKey: return "invalid_key";
    case PersistenceError::QueueBusy: return "queue_busy";
    case PersistenceError::QueueFull: return "queue_full";
    case PersistenceError::WorkerStartFailed: return "worker_start_failed";
    case PersistenceError::WorkerNotRunning: return "worker_not_running";
    case PersistenceError::ProducerTrackingFull: return "producer_tracking_full";
    case PersistenceError::PreferencesBeginFailed: return "preferences_begin_failed";
    case PersistenceError::PreferencesPutFailed: return "preferences_put_failed";
    case PersistenceError::ReadbackFailed: return "readback_failed";
    case PersistenceError::PriorJobFailed: return "prior_job_failed";
    default: return "none";
  }
}

namespace deferred_preferences_detail {

// 64 jobs cover the largest existing multi-key action (RS485 "all off" writes
// more than 30 values) while keeping the producer side strictly bounded.
inline constexpr size_t queueCapacity = 64;
inline constexpr size_t resultCapacity = 160;
inline constexpr size_t producerCapacity = 16;
inline constexpr TickType_t submitLockWait = pdMS_TO_TICKS(3);
inline constexpr TickType_t inspectLockWait = pdMS_TO_TICKS(20);

enum class Domain : uint8_t { Value, Button, Barrier };
enum class ValueType : uint8_t { Int, Float, String };

struct Value {
  ValueType type = ValueType::Int;
  int intValue = 0;
  float floatValue = 0.0f;
  String stringValue;
};

struct QueueJob {
  bool used = false;
  bool writing = false;
  PersistenceJobId id = 0;
  uint32_t order = 0;
  Domain domain = Domain::Value;
  TaskHandle_t producer = nullptr;
  uint32_t producerBatch = 0;
  bool predecessorFailed = false;
  String key;
  Value value;
};

struct ProducerBatchToken {
  TaskHandle_t producer = nullptr;
  uint32_t batch = 0;
  bool valid = false;
};

struct ProducerState {
  TaskHandle_t producer = nullptr;
  uint32_t currentBatch = 1;
  bool currentBatchFailed = false;
};

struct WriteResult {
  bool success = false;
  bool unchanged = false;
  PersistenceError error = PersistenceError::None;
};

inline SemaphoreHandle_t queueMutex = nullptr;
inline TaskHandle_t workerHandle = nullptr;
inline volatile bool workerRunning = false;
inline volatile bool runtimeRequired = false;
inline volatile bool workerStartAttempted = false;
inline volatile bool workerStartFailed = false;
inline portMUX_TYPE producerMux = portMUX_INITIALIZER_UNLOCKED;
inline QueueJob jobs[queueCapacity];
inline PersistenceJobInfo results[resultCapacity];
inline ProducerState producers[producerCapacity];
inline size_t resultCursor = 0;
inline PersistenceJobId nextJobId = 1;
inline uint32_t nextOrder = 1;
inline PersistenceQueueStats counters;
inline bool synchronousPredecessorFailure = false;
inline volatile uint32_t unlockedRejectedCount = 0;

inline void ensureQueueMutex() {
  // Called synchronously during setup before the worker is enabled. Once the
  // handle is published every runtime caller only reads it.
  if(queueMutex == nullptr) queueMutex = xSemaphoreCreateMutex();
}

inline bool takeQueueLock(TickType_t waitTicks) {
  ensureQueueMutex();
  return queueMutex != nullptr && xSemaphoreTake(queueMutex, waitTicks) == pdTRUE;
}

inline void giveQueueLock() {
  if(queueMutex != nullptr) xSemaphoreGive(queueMutex);
}

inline bool loadFlag(const volatile bool *flag) {
  return __atomic_load_n(flag, __ATOMIC_ACQUIRE);
}

inline void storeFlag(volatile bool *flag, bool value) {
  __atomic_store_n(flag, value, __ATOMIC_RELEASE);
}

inline ProducerBatchToken currentProducerBatch() {
  ProducerBatchToken token;
  token.producer = xTaskGetCurrentTaskHandle();
  if(token.producer == nullptr) return token;

  portENTER_CRITICAL(&producerMux);
  ProducerState *freeSlot = nullptr;
  for(auto &state : producers){
    if(state.producer == token.producer){
      token.batch = state.currentBatch;
      token.valid = true;
      portEXIT_CRITICAL(&producerMux);
      return token;
    }
    if(state.producer == nullptr && freeSlot == nullptr) freeSlot = &state;
  }
  if(freeSlot != nullptr){
    freeSlot->producer = token.producer;
    freeSlot->currentBatch = 1;
    freeSlot->currentBatchFailed = false;
    token.batch = freeSlot->currentBatch;
    token.valid = true;
  }
  portEXIT_CRITICAL(&producerMux);
  return token;
}

inline bool markProducerBatchFailed(const ProducerBatchToken &token) {
  if(!token.valid) return false;
  bool marked = false;
  portENTER_CRITICAL(&producerMux);
  for(auto &state : producers){
    if(state.producer == token.producer && state.currentBatch == token.batch){
      state.currentBatchFailed = true;
      marked = true;
      break;
    }
  }
  portEXIT_CRITICAL(&producerMux);
  return marked;
}

inline bool closeProducerBatch(const ProducerBatchToken &token,
                               bool &predecessorFailed) {
  predecessorFailed = true; // A lost producer context must fail closed.
  if(!token.valid) return false;
  bool closed = false;
  portENTER_CRITICAL(&producerMux);
  for(auto &state : producers){
    if(state.producer == token.producer && state.currentBatch == token.batch){
      predecessorFailed = state.currentBatchFailed;
      state.currentBatchFailed = false;
      ++state.currentBatch;
      if(state.currentBatch == 0) state.currentBatch = 1;
      closed = true;
      break;
    }
  }
  portEXIT_CRITICAL(&producerMux);
  return closed;
}

inline bool terminalState(PersistenceJobState state) {
  return state == PersistenceJobState::Succeeded ||
         state == PersistenceJobState::Failed ||
         state == PersistenceJobState::Superseded ||
         state == PersistenceJobState::Rejected;
}

inline PersistenceJobInfo *findResultLocked(PersistenceJobId id) {
  if(id == 0) return nullptr;
  for(auto &result : results){
    if(result.id == id) return &result;
  }
  return nullptr;
}

inline PersistenceJobInfo *resultSlotLocked(PersistenceJobId id) {
  if(PersistenceJobInfo *existing = findResultLocked(id)) return existing;
  for(auto &result : results){
    if(result.id == 0) return &result;
  }
  for(size_t offset = 0; offset < resultCapacity; ++offset){
    const size_t index = (resultCursor + offset) % resultCapacity;
    if(terminalState(results[index].state)){
      resultCursor = (index + 1) % resultCapacity;
      return &results[index];
    }
  }
  return nullptr; // Active results are never evicted.
}

inline void setResultLocked(PersistenceJobId id, PersistenceJobState state,
                            PersistenceError error = PersistenceError::None,
                            PersistenceJobId supersededBy = 0) {
  PersistenceJobInfo *slot = resultSlotLocked(id);
  if(slot == nullptr) return;
  slot->id = id;
  slot->state = state;
  slot->error = error;
  slot->supersededBy = supersededBy;
  slot->updatedAtMs = millis();
}

inline PersistenceJobId issueJobIdLocked() {
  PersistenceJobId id = nextJobId++;
  if(nextJobId == 0) nextJobId = 1;
  if(id == 0) id = nextJobId++;
  return id;
}

inline uint32_t issueOrderLocked() {
  uint32_t order = nextOrder++;
  if(nextOrder == 0) nextOrder = 1;
  if(order == 0) order = nextOrder++;
  return order;
}

inline bool orderBefore(uint32_t left, uint32_t right) {
  return static_cast<int32_t>(left - right) < 0;
}

inline bool sameFloatBits(float left, float right) {
  uint32_t leftBits = 0;
  uint32_t rightBits = 0;
  memcpy(&leftBits, &left, sizeof(leftBits));
  memcpy(&rightBits, &right, sizeof(rightBits));
  return leftBits == rightBits;
}

inline bool storedTypeMatches(Preferences &preferences, const char *key,
                              ValueType type) {
  const PreferenceType storedType = preferences.getType(key);
  if(type == ValueType::Int) return storedType == PT_I32;
  if(type == ValueType::Float){
    return storedType == PT_BLOB && preferences.getBytesLength(key) == sizeof(float);
  }
  return storedType == PT_STR;
}

inline WriteResult writeNow(Domain domain, const String &key, const Value &value) {
  WriteResult result;
  if(domain == Domain::Barrier) return result; // Barrier completion is handled by the FIFO worker.
  const PreferencesStorageKey storageKey(key.c_str());
  if(!storageKey.valid()){
    Serial.println("[NVS] deferred write rejected: empty key");
    result.error = PersistenceError::InvalidKey;
    return result;
  }

  const char *nameSpace = domain == Domain::Button
      ? storageKey.nameSpace(buttonPreferencesNamespace, longButtonPreferencesNamespace)
      : storageKey.nameSpace(valuePreferencesNamespace, longValuePreferencesNamespace);

  lockPrefs();
  if(!prefs.begin(nameSpace, false)){
    unlockPrefs();
    logPreferencesError("begin deferred write", nameSpace, storageKey);
    result.error = PersistenceError::PreferencesBeginFailed;
    return result;
  }

  bool alreadyEqual = false;
  if(prefs.isKey(storageKey.storageKey()) &&
     storedTypeMatches(prefs, storageKey.storageKey(), value.type)){
    if(value.type == ValueType::Int){
      alreadyEqual = prefs.getInt(storageKey.storageKey(), value.intValue ^ 0x5a5a5a5a) == value.intValue;
    } else if(value.type == ValueType::Float){
      alreadyEqual = sameFloatBits(prefs.getFloat(storageKey.storageKey(), 0.0f), value.floatValue);
    } else {
      alreadyEqual = prefs.getString(storageKey.storageKey(), String()) == value.stringValue;
    }
  }

  size_t written = 0;
  bool putSucceeded = true;
  if(!alreadyEqual){
    if(value.type == ValueType::Int){
      written = prefs.putInt(storageKey.storageKey(), value.intValue);
      putSucceeded = written == sizeof(int32_t);
    } else if(value.type == ValueType::Float){
      written = prefs.putFloat(storageKey.storageKey(), value.floatValue);
      putSucceeded = written == sizeof(float);
    } else {
      written = prefs.putString(storageKey.storageKey(), value.stringValue.c_str());
      putSucceeded = written > 0 ||
          (value.stringValue.length() == 0 && prefs.isKey(storageKey.storageKey()));
    }
  }

  bool verified = putSucceeded && prefs.isKey(storageKey.storageKey()) &&
                  storedTypeMatches(prefs, storageKey.storageKey(), value.type);
  if(verified){
    if(value.type == ValueType::Int){
      verified = prefs.getInt(storageKey.storageKey(), value.intValue ^ 0x5a5a5a5a) == value.intValue;
    } else if(value.type == ValueType::Float){
      verified = sameFloatBits(prefs.getFloat(storageKey.storageKey(), 0.0f), value.floatValue);
    } else {
      verified = prefs.getString(storageKey.storageKey(), String()) == value.stringValue;
    }
  }

  prefs.end();
  unlockPrefs();

  result.unchanged = alreadyEqual;
  result.success = verified;
  if(!putSucceeded) result.error = PersistenceError::PreferencesPutFailed;
  else if(!verified) result.error = PersistenceError::ReadbackFailed;
  if(!result.success){
    logPreferencesError(result.error == PersistenceError::PreferencesPutFailed
                            ? "deferred write" : "deferred readback",
                        nameSpace, storageKey);
  }
  return result;
}

inline void finishJob(const QueueJob &completedJob, const WriteResult &writeResult) {
  if(!takeQueueLock(portMAX_DELAY)) return;
  if(!writeResult.success && completedJob.domain != Domain::Barrier){
    bool assignedToBarrier = false;
    for(auto &job : jobs){
      if(job.used && job.domain == Domain::Barrier &&
         job.producer == completedJob.producer &&
         job.producerBatch == completedJob.producerBatch){
        job.predecessorFailed = true;
        assignedToBarrier = true;
        break;
      }
    }
    if(!assignedToBarrier){
      const ProducerBatchToken token{completedJob.producer,
                                     completedJob.producerBatch, true};
      markProducerBatchFailed(token);
    }
  }
  for(auto &job : jobs){
    if(job.used && job.id == completedJob.id){
      job.used = false;
      job.writing = false;
      job.id = 0;
      job.order = 0;
      job.producer = nullptr;
      job.producerBatch = 0;
      job.predecessorFailed = false;
      job.key = String();
      job.value.stringValue = String();
      break;
    }
  }
  if(writeResult.success){
    ++counters.succeeded;
    if(writeResult.unchanged) ++counters.unchanged;
    setResultLocked(completedJob.id, PersistenceJobState::Succeeded);
  } else {
    ++counters.failed;
    setResultLocked(completedJob.id, PersistenceJobState::Failed, writeResult.error);
  }
  giveQueueLock();
}

inline bool takeNextJob(QueueJob &destination) {
  if(!takeQueueLock(portMAX_DELAY)) return false;
  QueueJob *selected = nullptr;
  for(auto &job : jobs){
    if(!job.used || job.writing) continue;
    if(selected == nullptr || orderBefore(job.order, selected->order)) selected = &job;
  }
  if(selected == nullptr){
    giveQueueLock();
    return false;
  }
  selected->writing = true;
  setResultLocked(selected->id, PersistenceJobState::Writing);
  destination = *selected;
  giveQueueLock();
  return true;
}

inline void workerTask(void *) {
  for(;;){
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    QueueJob job;
    while(takeNextJob(job)){
      WriteResult result;
      if(job.domain == Domain::Barrier){
        result.success = !job.predecessorFailed;
        if(!result.success) result.error = PersistenceError::PriorJobFailed;
      } else {
        result = writeNow(job.domain, job.key, job.value);
      }
      finishJob(job, result);
      job = QueueJob();
      taskYIELD();
    }
  }
}

inline PersistenceSubmission rejectSubmission(PersistenceError error) {
  PersistenceSubmission submission;
  submission.error = error;
  if(takeQueueLock(submitLockWait)){
    submission.jobId = issueJobIdLocked();
    ++counters.rejected;
    setResultLocked(submission.jobId, PersistenceJobState::Rejected, error);
    giveQueueLock();
  } else {
    __atomic_add_fetch(&unlockedRejectedCount, 1U, __ATOMIC_RELAXED);
  }
  return submission;
}

inline PersistenceSubmission submit(Domain domain, const char *key, const Value &value) {
  PersistenceSubmission submission;
  const String ownedKey = key ? String(key) : String();

  if(!loadFlag(&workerRunning)){
    const bool runtimeIsRequired = loadFlag(&runtimeRequired);
    const bool startWasAttempted = loadFlag(&workerStartAttempted);
    if(runtimeIsRequired || startWasAttempted){
      return rejectSubmission(loadFlag(&workerStartFailed)
                                  ? PersistenceError::WorkerStartFailed
                                  : PersistenceError::WorkerNotRunning);
    }

    if(!takeQueueLock(portMAX_DELAY)) return submission;
    submission.jobId = issueJobIdLocked();
    submission.accepted = true;
    submission.completedSynchronously = true;
    ++counters.accepted;
    setResultLocked(submission.jobId, PersistenceJobState::Writing);
    giveQueueLock();

    WriteResult writeResult;
    if(domain == Domain::Barrier){
      writeResult.success = !synchronousPredecessorFailure;
      if(!writeResult.success) writeResult.error = PersistenceError::PriorJobFailed;
      synchronousPredecessorFailure = false;
    } else {
      writeResult = writeNow(domain, ownedKey, value);
    }
    submission.succeeded = writeResult.success;
    submission.error = writeResult.error;
    if(!writeResult.success && domain != Domain::Barrier) synchronousPredecessorFailure = true;
    if(takeQueueLock(portMAX_DELAY)){
      if(writeResult.success){
        ++counters.succeeded;
        if(writeResult.unchanged) ++counters.unchanged;
        setResultLocked(submission.jobId, PersistenceJobState::Succeeded);
      } else {
        ++counters.failed;
        setResultLocked(submission.jobId, PersistenceJobState::Failed, writeResult.error);
      }
      giveQueueLock();
    }
    return submission;
  }

  const ProducerBatchToken producerToken = currentProducerBatch();
  if(!producerToken.valid){
    return rejectSubmission(PersistenceError::ProducerTrackingFull);
  }

  if(!takeQueueLock(submitLockWait)){
    markProducerBatchFailed(producerToken);
    __atomic_add_fetch(&unlockedRejectedCount, 1U, __ATOMIC_RELAXED);
    submission.error = PersistenceError::QueueBusy;
    return submission;
  }

  submission.jobId = issueJobIdLocked();
  QueueJob *slot = nullptr;
  for(auto &job : jobs){
    if(domain != Domain::Barrier && job.used && !job.writing &&
       job.domain == domain && job.key == ownedKey &&
       job.producer == producerToken.producer &&
       job.producerBatch == producerToken.batch){
      slot = &job;
      setResultLocked(job.id, PersistenceJobState::Superseded,
                      PersistenceError::None, submission.jobId);
      ++counters.coalesced;
      break;
    }
  }
  if(slot == nullptr){
    for(auto &job : jobs){
      if(!job.used){
        slot = &job;
        break;
      }
    }
  }

  if(slot == nullptr){
    ++counters.rejected;
    markProducerBatchFailed(producerToken);
    submission.error = PersistenceError::QueueFull;
    setResultLocked(submission.jobId, PersistenceJobState::Rejected,
                    PersistenceError::QueueFull);
    giveQueueLock();
    return submission;
  }

  slot->used = true;
  slot->writing = false;
  slot->id = submission.jobId;
  slot->order = issueOrderLocked();
  slot->domain = domain;
  slot->producer = producerToken.producer;
  slot->producerBatch = producerToken.batch;
  if(domain == Domain::Barrier){
    bool predecessorFailed = true;
    closeProducerBatch(producerToken, predecessorFailed);
    slot->predecessorFailed = predecessorFailed;
  } else {
    slot->predecessorFailed = false;
  }
  slot->key = ownedKey;
  slot->value = value;
  submission.accepted = true;
  ++counters.accepted;
  setResultLocked(submission.jobId, PersistenceJobState::Pending);
  giveQueueLock();

  TaskHandle_t handle = workerHandle;
  if(handle != nullptr) xTaskNotifyGive(handle);
  return submission;
}

template <typename T> inline Value makeValue(T value) {
  static_assert(std::is_same<T, float>::value || std::is_same<T, int>::value ||
                std::is_same<T, String>::value,
                "deferred persistence supports only float, int and String");
  Value stored;
  if constexpr (std::is_same<T, float>::value){
    stored.type = ValueType::Float;
    stored.floatValue = value;
  } else if constexpr (std::is_same<T, int>::value){
    stored.type = ValueType::Int;
    stored.intValue = value;
  } else {
    stored.type = ValueType::String;
    stored.stringValue = value;
  }
  return stored;
}

inline uint16_t pendingCountLocked() {
  uint16_t count = 0;
  for(const auto &job : jobs){
    if(job.used) ++count;
  }
  return count;
}

} // namespace deferred_preferences_detail

inline void persistenceRequireRuntimeWorker() {
  deferred_preferences_detail::storeFlag(
      &deferred_preferences_detail::runtimeRequired, true);
}

inline bool persistenceStartWorker(uint32_t stackSize = 4096,
                                   UBaseType_t priority = 1) {
  using namespace deferred_preferences_detail;
  // Crossing this boundary is irreversible for this boot. If task creation
  // fails, runtime callers must see a rejected job instead of committing Flash
  // synchronously from async_tcp or another latency-sensitive task.
  storeFlag(&runtimeRequired, true);
  storeFlag(&workerStartAttempted, true);
  storeFlag(&workerStartFailed, false);
  ensureQueueMutex();
  if(queueMutex == nullptr){
    storeFlag(&workerStartFailed, true);
    Serial.println("[NVS] failed to create deferred persistence mutex");
    return false;
  }
  if(loadFlag(&workerRunning)) return true;

  TaskHandle_t createdHandle = nullptr;
  const BaseType_t created = xTaskCreate(workerTask, "NvsPersist", stackSize,
                                         nullptr, priority, &createdHandle);
  if(created != pdPASS || createdHandle == nullptr){
    storeFlag(&workerStartFailed, true);
    Serial.println("[NVS] failed to start deferred persistence worker");
    return false;
  }
  workerHandle = createdHandle;
  storeFlag(&workerRunning, true);
  Serial.printf("[NVS] deferred persistence worker started | queue=%u\n",
                static_cast<unsigned>(queueCapacity));
  return true;
}

inline bool persistenceWorkerRunning() {
  return deferred_preferences_detail::loadFlag(
      &deferred_preferences_detail::workerRunning);
}

inline bool persistenceRuntimeRequired() {
  return deferred_preferences_detail::loadFlag(
      &deferred_preferences_detail::runtimeRequired);
}

inline bool persistenceWorkerStartAttempted() {
  return deferred_preferences_detail::loadFlag(
      &deferred_preferences_detail::workerStartAttempted);
}

inline bool persistenceWorkerStartFailed() {
  return deferred_preferences_detail::loadFlag(
      &deferred_preferences_detail::workerStartFailed);
}

inline PersistenceJobInfo persistenceJobStatus(PersistenceJobId id) {
  using namespace deferred_preferences_detail;
  PersistenceJobInfo result;
  if(!takeQueueLock(inspectLockWait)) return result;
  if(PersistenceJobInfo *stored = findResultLocked(id)) result = *stored;
  giveQueueLock();
  return result;
}

inline PersistenceQueueStats persistenceQueueStats() {
  using namespace deferred_preferences_detail;
  PersistenceQueueStats result;
  if(!takeQueueLock(inspectLockWait)) return result;
  result = counters;
  result.rejected += __atomic_load_n(&unlockedRejectedCount, __ATOMIC_RELAXED);
  result.pending = pendingCountLocked();
  result.workerRunning = loadFlag(&workerRunning);
  result.runtimeRequired = loadFlag(&runtimeRequired);
  result.workerStartAttempted = loadFlag(&workerStartAttempted);
  result.workerStartFailed = loadFlag(&workerStartFailed);
  giveQueueLock();
  return result;
}

inline PersistenceFlushResult persistenceFlushDetailed(uint32_t timeoutMs = 5000) {
  using namespace deferred_preferences_detail;
  PersistenceFlushResult result;
  if(loadFlag(&runtimeRequired) && !loadFlag(&workerRunning)){
    result.failed = 1;
    result.error = loadFlag(&workerStartFailed)
                       ? PersistenceError::WorkerStartFailed
                       : PersistenceError::WorkerNotRunning;
    return result;
  }
  const PersistenceQueueStats before = persistenceQueueStats();
  const uint32_t startedAt = millis();
  for(;;){
    if(takeQueueLock(inspectLockWait)){
      const bool idle = pendingCountLocked() == 0;
      giveQueueLock();
      if(idle){
        result.drained = true;
        break;
      }
    }
    if(static_cast<uint32_t>(millis() - startedAt) >= timeoutMs) break;
    vTaskDelay(pdMS_TO_TICKS(2) > 0 ? pdMS_TO_TICKS(2) : 1);
  }
  const PersistenceQueueStats after = persistenceQueueStats();
  result.succeeded = after.succeeded - before.succeeded;
  result.failed = after.failed - before.failed;
  result.rejected = after.rejected - before.rejected;
  return result;
}

inline bool persistenceFlush(uint32_t timeoutMs = 5000) {
  const PersistenceFlushResult result = persistenceFlushDetailed(timeoutMs);
  return result.drained && result.failed == 0 && result.rejected == 0;
}

inline PersistenceSubmission saveButtonStateTrackedResult(const char *key, int value) {
  return deferred_preferences_detail::submit(
      deferred_preferences_detail::Domain::Button, key,
      deferred_preferences_detail::makeValue<int>(value));
}

inline PersistenceSubmission persistenceBarrierTrackedResult() {
  deferred_preferences_detail::Value markerValue;
  return deferred_preferences_detail::submit(
      deferred_preferences_detail::Domain::Barrier, nullptr, markerValue);
}

inline PersistenceJobId persistenceBarrierTracked() {
  return persistenceBarrierTrackedResult().jobId;
}

inline PersistenceJobId saveButtonStateTracked(const char *key, int value) {
  return saveButtonStateTrackedResult(key, value).jobId;
}

inline bool saveButtonState(const char *key, int value) {
  const PersistenceSubmission result = saveButtonStateTrackedResult(key, value);
  return result.accepted && (!result.completedSynchronously || result.succeeded);
}

inline int loadButtonState(const char *key, int def = 0) {
  const PreferencesStorageKey storageKey(key);
  if(!storageKey.valid()){
    Serial.println("[NVS] button read rejected: empty key");
    return def;
  }
  const char *nameSpace = storageKey.nameSpace(buttonPreferencesNamespace,
                                                longButtonPreferencesNamespace);
  lockPrefs();
  if(!prefs.begin(nameSpace, true)){
    unlockPrefs();
    return def;
  }
  const int value = prefs.getInt(storageKey.storageKey(), def);
  prefs.end();
  unlockPrefs();
  return value;
}

template <typename T> inline PersistenceSubmission saveValueTrackedResult(const char *key, T value) {
  return deferred_preferences_detail::submit(
      deferred_preferences_detail::Domain::Value, key,
      deferred_preferences_detail::makeValue<T>(value));
}

template <typename T> inline PersistenceJobId saveValueTracked(const char *key, T value) {
  return saveValueTrackedResult<T>(key, value).jobId;
}

template <typename T> inline bool saveValue(const char *key, T value);

template <typename T> inline T loadValue(const char *key, T def) {
  static_assert(std::is_same<T, float>::value || std::is_same<T, int>::value ||
                std::is_same<T, String>::value,
                "loadValue supports only float, int and String");
  const PreferencesStorageKey storageKey(key);
  if(!storageKey.valid()){
    Serial.println("[NVS] value read rejected: empty key");
    return def;
  }
  const char *nameSpace = storageKey.nameSpace(valuePreferencesNamespace,
                                                longValuePreferencesNamespace);
  lockPrefs();
  const bool opened = prefs.begin(nameSpace, true);
  T value = def;
  if(opened && prefs.isKey(storageKey.storageKey())){
    if constexpr (std::is_same<T, float>::value)
      value = prefs.getFloat(storageKey.storageKey(), def);
    else if constexpr (std::is_same<T, int>::value)
      value = prefs.getInt(storageKey.storageKey(), def);
    else
      value = prefs.getString(storageKey.storageKey(), def);
    prefs.end();
    unlockPrefs();
    return value;
  }
  if(opened) prefs.end();
  unlockPrefs();
  saveValue<T>(key, value); // During setup this remains a synchronous default write.
  return value;
}

template <typename T> inline bool saveValue(const char *key, T value) {
  const PersistenceSubmission result = saveValueTrackedResult<T>(key, value);
  return result.accepted && (!result.completedSynchronously || result.succeeded);
}
