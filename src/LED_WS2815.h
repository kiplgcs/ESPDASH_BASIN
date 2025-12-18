#pragma once

#include <Arduino.h>
#include <NeoPixelBus.h>
#include <NeoPixelBrightnessBus.h>
#include <stdlib.h>

#define DATA_PIN 10
#define NUM_LEDS 180

static NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> ledStrip(NUM_LEDS, DATA_PIN);

constexpr const char* LED_PATTERN_NAMES[] = {"rainbow", "pulse", "chase", "comet"};
constexpr size_t LED_PATTERN_COUNT = sizeof(LED_PATTERN_NAMES) / sizeof(LED_PATTERN_NAMES[0]);

static unsigned long ledLastUpdate = 0;
static unsigned long ledNextPatternSwitch = 0;
static uint8_t ledFrame = 0;
static String ledLastPatternName = "";
static bool ledAutoplayState = false;

static inline RgbColor wheelColor(uint8_t pos){
    if(pos < 85) return RgbColor(pos * 3, 255 - pos * 3, 0);
    if(pos < 170){
        pos -= 85;
        return RgbColor(255 - pos * 3, 0, pos * 3);
    }
    pos -= 170;
    return RgbColor(0, pos * 3, 255 - pos * 3);
}

static inline RgbColor parseHexColor(const String &hexColor){
    String value = hexColor;
    value.trim();
    if(value.startsWith("#")) value = value.substring(1);
    if(value.length() > 6) value = value.substring(value.length() - 6);
    if(value.length() < 6) return RgbColor(0, 0, 0);
    char buffer[7] = {0};
    value.substring(0, 6).toCharArray(buffer, sizeof(buffer));
    uint32_t raw = strtoul(buffer, nullptr, 16);
    return RgbColor((raw >> 16) & 0xFF, (raw >> 8) & 0xFF, raw & 0xFF);
}

static inline void fillRainbowFrame(uint8_t frame){
    for(uint16_t i = 0; i < NUM_LEDS; i++){
        uint8_t position = frame + ((i * 256) / NUM_LEDS);
        ledStrip.SetPixelColor(i, wheelColor(position));
    }
}

static inline void fillPulseFrame(uint8_t frame, const RgbColor &accent){
    uint8_t intensity = (frame < 128) ? frame * 2 : (255 - frame) * 2;
    float ratio = intensity / 255.0f;
    RgbColor scaled(
        static_cast<uint8_t>(accent.R * ratio),
        static_cast<uint8_t>(accent.G * ratio),
        static_cast<uint8_t>(accent.B * ratio));
    ledStrip.ClearTo(scaled);
}

static inline void fillChaseFrame(uint8_t frame){
    constexpr uint8_t segmentLength = 10;
    uint8_t offset = frame / segmentLength;
    RgbColor chaseColor = wheelColor(frame);
    for(uint16_t i = 0; i < NUM_LEDS; i++){
        bool on = ((i + offset) % (segmentLength * 2)) < segmentLength;
        ledStrip.SetPixelColor(i, on ? chaseColor : RgbColor(0, 0, 0));
    }
}

static inline void fillCometFrame(uint8_t frame){
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    int head = (frame * 3) % NUM_LEDS;
    for(uint8_t trail = 0; trail < 16; ++trail){
        int index = (head - trail + NUM_LEDS) % NUM_LEDS;
        uint8_t brightness = (trail < 16) ? (255 - trail * 16) : 0;
        if(brightness == 0) continue;
        RgbColor color = wheelColor(frame + trail * 8);
        RgbColor scaled(
            (color.R * brightness) / 255,
            (color.G * brightness) / 255,
            (color.B * brightness) / 255);
        ledStrip.SetPixelColor(index, scaled);
    }
}

static inline unsigned long effectiveAutoplayDelay(){
    int duration = (LedAutoplayDuration > 1) ? LedAutoplayDuration : 1;
    return static_cast<unsigned long>(duration) * 1000UL;
}

static inline void scheduleNextPattern(){
    ledNextPatternSwitch = millis() + effectiveAutoplayDelay();
}

static inline void updatePatternFromName(){
    String normalized = LedPattern;
    normalized.trim();
    size_t target = 0;
    bool found = false;
    for(size_t i = 0; i < LED_PATTERN_COUNT; ++i){
        if(normalized.equalsIgnoreCase(LED_PATTERN_NAMES[i])){
            normalized = LED_PATTERN_NAMES[i];
            target = i;
            found = true;
            break;
        }
    }
    if(!found){
        normalized = LED_PATTERN_NAMES[0];
        target = 0;
    }
    bool changed = !normalized.equalsIgnoreCase(ledLastPatternName);
    currentPatternIndex = static_cast<int>(target);
    LedPattern = normalized;
    if(changed){
        ledLastPatternName = normalized;
        scheduleNextPattern();
    }
}

static inline void renderPattern(uint8_t frame){
    RgbColor accent = parseHexColor(LEDColor);
    if(accent.R == 0 && accent.G == 0 && accent.B == 0){
        accent = wheelColor(frame);
    }
    switch(currentPatternIndex){
        case 0: fillRainbowFrame(frame); break;
        case 1: fillPulseFrame(frame, accent); break;
        case 2: fillChaseFrame(frame); break;
        case 3: fillCometFrame(frame); break;
        default: fillRainbowFrame(frame); break;
    }
}

void setup_WS2815(){
    ledStrip.Begin();
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    ledStrip.Show();
    ledLastUpdate = millis();
    ledFrame = 0;
    ledLastPatternName = "";
    ledAutoplayState = LedAutoplay;
    updatePatternFromName();
    scheduleNextPattern();
}

void loop_WS2815(){
    if(!Pow_WS2815){
        ledStrip.SetBrightness(0);
        ledStrip.ClearTo(RgbColor(0, 0, 0));
        ledStrip.Show();
        return;
    }

    updatePatternFromName();

    if(ColorRGB){
        ledStrip.SetBrightness(constrain(new_bright, 0, 255));
        ledStrip.ClearTo(parseHexColor(LEDColor));
        ledStrip.Show();
        return;
    }

    if(LedAutoplay != ledAutoplayState){
        ledAutoplayState = LedAutoplay;
        scheduleNextPattern();
    }

    unsigned long now = millis();
    if(LedAutoplay && now >= ledNextPatternSwitch){
        currentPatternIndex = (currentPatternIndex + 1) % static_cast<int>(LED_PATTERN_COUNT);
        LedPattern = LED_PATTERN_NAMES[currentPatternIndex];
        ledLastPatternName = LedPattern;
        scheduleNextPattern();
    }

    if(now - ledLastUpdate < 40){
        return;
    }

    ledLastUpdate = now;
    ledFrame++;

    ledStrip.SetBrightness(constrain(new_bright, 0, 255));
    renderPattern(ledFrame);
    ledStrip.Show();
}
