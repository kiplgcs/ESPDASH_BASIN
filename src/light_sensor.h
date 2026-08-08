#pragma once

#include <Arduino.h>

inline void updateLumenSensor(unsigned long intervalMs = 250){
  static unsigned long lastSampleAt = 0;
  const unsigned long now = millis();
  if(lastSampleAt != 0 && now - lastSampleAt < intervalMs) return;
  lastSampleAt = now;

  constexpr uint8_t lumenAdcPin = 3;
  constexpr uint8_t sampleCount = 21;
  constexpr uint8_t selectedSampleIndex = sampleCount / 5; // Нижний устойчивый квантиль: режет редкие выбросы вверх без реакции на одиночный провал.
  constexpr int sensorSupplyMv = 3300;
  constexpr int sensorBrightMv = 2800; // По фактической схеме ниже напряжение = ярче.
  constexpr int sensorDarkMv = 3300;   // При затемнении сопротивление падает, напряжение GPIO3 растет: верх шкалы = темно.
  constexpr int adcSaturationRaw = 4085;
  int rawSamples[sampleCount];
  int milliVoltSamples[sampleCount];
  uint32_t rawSum = 0;
  uint32_t milliVoltSum = 0;

  (void)analogRead(lumenAdcPin); // Первый замер после переключения ADC не используем.
  delayMicroseconds(120);

  for(uint8_t i = 0; i < sampleCount; ++i){
    int raw = analogRead(lumenAdcPin);
    int milliVolts = analogReadMilliVolts(lumenAdcPin);
    raw = constrain(raw, 0, 4095);
    milliVolts = constrain(milliVolts, 0, sensorSupplyMv);
    rawSamples[i] = raw;
    milliVoltSamples[i] = milliVolts;
    rawSum += static_cast<uint32_t>(raw);
    milliVoltSum += static_cast<uint32_t>(milliVolts);
    delayMicroseconds(120);
  }

  for(uint8_t i = 1; i < sampleCount; ++i){
    const int rawValue = rawSamples[i];
    const int milliVoltValue = milliVoltSamples[i];
    int8_t j = static_cast<int8_t>(i) - 1;
    while(j >= 0 && rawSamples[j] > rawValue){ rawSamples[j + 1] = rawSamples[j]; --j; }
    rawSamples[j + 1] = rawValue;
    j = static_cast<int8_t>(i) - 1;
    while(j >= 0 && milliVoltSamples[j] > milliVoltValue){ milliVoltSamples[j + 1] = milliVoltSamples[j]; --j; }
    milliVoltSamples[j + 1] = milliVoltValue;
  }

  const int selectedRaw = rawSamples[selectedSampleIndex];
  const int selectedCalibratedMilliVolts = milliVoltSamples[selectedSampleIndex];
  const int selectedMilliVolts = constrain((selectedRaw * sensorSupplyMv + 2047) / 4095, 0, sensorSupplyMv);
  static int filteredMilliVolts = -1;
  if(filteredMilliVolts < 0) filteredMilliVolts = selectedMilliVolts;
  else filteredMilliVolts = (filteredMilliVolts + selectedMilliVolts + 1) / 2;

  Lumen_Ul_Raw = static_cast<int>(rawSum / sampleCount);
  Lumen_Ul_AverageMilliVolts = static_cast<int>(milliVoltSum / sampleCount);
  Lumen_Ul_MinMilliVolts = milliVoltSamples[0];
  Lumen_Ul_MaxMilliVolts = milliVoltSamples[sampleCount - 1];
  Lumen_Ul_FilteredMilliVolts = constrain(filteredMilliVolts, 0, sensorSupplyMv);
  Lumen_Ul_MilliVolts = Lumen_Ul_FilteredMilliVolts;
  Lumen_Ul_SampleCount = sampleCount;
  Lumen_Ul_AdcSaturated = rawSamples[sampleCount - 1] >= adcSaturationRaw;
  Lumen_Ul_InputOutOfExpectedRange = Lumen_Ul_AdcSaturated || Lumen_Ul_FilteredMilliVolts >= 3250;
  Lumen_Ul_LastSampleAt = now;

  const int percentRangeMv = max(1, sensorDarkMv - sensorBrightMv);
  const int percentMilliVolts = constrain(Lumen_Ul_MilliVolts, sensorBrightMv, sensorDarkMv);
  Lumen_Ul = constrain(((sensorDarkMv - percentMilliVolts) * 100 + percentRangeMv / 2) / percentRangeMv, 0, 100);

  static unsigned long lastTextUpdateAt = 0;
  if(lastTextUpdateAt != 0 && now - lastTextUpdateAt < 1000) return;
  lastTextUpdateAt = now;

  const float voltage = static_cast<float>(Lumen_Ul_MilliVolts) / 1000.0f;
  const String voltageText = String(voltage, 2);
  InfoString2 = "Освещенность = " + String(Lumen_Ul) + " % (" + voltageText + " В)";
  LightSensorLiveInfo = "Сейчас: " + String(Lumen_Ul) + "%, " + voltageText +
                        " В на GPIO3. Для вашей схемы затемнение повышает напряжение: " +
                        String(sensorBrightMv) + " мВ = 100%, " + String(sensorDarkMv) + " мВ = 0%.";

  String estimatedLdr = "не рассчитывается";
  if(Lumen_Ul_FilteredMilliVolts > 0 && Lumen_Ul_FilteredMilliVolts < sensorSupplyMv){
    const float ldrOhms = 10000.0f * static_cast<float>(sensorSupplyMv - Lumen_Ul_FilteredMilliVolts) /
                          static_cast<float>(Lumen_Ul_FilteredMilliVolts);
    estimatedLdr = String(ldrOhms / 1000.0f, 2) + " кОм";
  }
  LightSensorDiagnosticInfo = String("GPIO3 ADC: выбран нижний устойчивый raw ") + String(selectedRaw) +
                              " = " + String(selectedMilliVolts) + " мВ по шкале 0..3.3 В; калиброванное ADC " +
                              String(selectedCalibratedMilliVolts) + " мВ; фильтр " +
                              String(Lumen_Ul_FilteredMilliVolts) + " мВ; среднее " +
                              String(Lumen_Ul_AverageMilliVolts) + " мВ; диапазон пачки " +
                              String(Lumen_Ul_MinMilliVolts) + ".." + String(Lumen_Ul_MaxMilliVolts) +
                              " мВ; raw среднее " + String(Lumen_Ul_Raw) +
                              ", raw выбран/min/max " + String(selectedRaw) + "/" + String(rawSamples[0]) +
                              "/" + String(rawSamples[sampleCount - 1]) +
                              ". Расчетное сопротивление LDR при подтяжке 10 кОм к GND: " + estimatedLdr +
                              ". Шкала освещенности: " + String(sensorBrightMv) + " мВ = 100%, " +
                              String(sensorDarkMv) + " мВ = 0%.";
  if(Lumen_Ul_AdcSaturated){
    LightSensorDiagnosticInfo += " ADC у верхнего предела: небольшие изменения выше этой зоны ESP32-S3 может не различать. Для точного измерения до 3.3 В нужен внешний делитель, чтобы GPIO3 не подходил к насыщению ADC.";
  }
}
