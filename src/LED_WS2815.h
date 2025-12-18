#pragma once

#include <Arduino.h>
#include <NeoPixelBus.h>
#include <NeoPixelBrightnessBus.h>
#include <stdlib.h>
#include <math.h>

#define DATA_PIN 10
#define NUM_LEDS 180

static NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> ledStrip(NUM_LEDS, DATA_PIN);

struct LedPatternDef {
    const char* name;
    void (*render)(uint8_t frame, const RgbColor &accent);
};

constexpr size_t LED_PATTERN_COUNT = 24;
static LedPatternDef LED_PATTERNS[LED_PATTERN_COUNT];
static bool ledPatternsInitialized = false;

static unsigned long ledLastUpdate = 0;
static unsigned long ledNextPatternSwitch = 0;
static uint8_t ledFrame = 0;
static String ledLastPatternName = "";
static bool ledAutoplayState = false;
static bool ledLastPowerState = false;

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

static inline void fadeStrip(uint8_t amount){
    for(uint16_t i = 0; i < NUM_LEDS; ++i){
        RgbColor current = ledStrip.GetPixelColor(i);
        uint8_t r = (current.R > amount) ? (current.R - amount) : 0;
        uint8_t g = (current.G > amount) ? (current.G - amount) : 0;
        uint8_t b = (current.B > amount) ? (current.B - amount) : 0;
        ledStrip.SetPixelColor(i, RgbColor(r, g, b));
    }
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


static inline void fillColorWipe(uint8_t frame, const RgbColor &accent){
    uint16_t pos = frame % (NUM_LEDS + 5);
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    for(uint16_t i = 0; i < pos && i < NUM_LEDS; ++i){
        ledStrip.SetPixelColor(i, accent);
    }
}

static inline void fillTheaterChase(uint8_t frame, const RgbColor &accent){
    uint8_t offset = frame % 3;
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    for(uint16_t i = offset; i < NUM_LEDS; i += 3){
        ledStrip.SetPixelColor(i, accent);
    }
}

static inline void fillScanner(uint8_t frame, const RgbColor &accent){
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    uint16_t head = frame % ((NUM_LEDS - 1) * 2);
    if(head >= NUM_LEDS) head = (NUM_LEDS - 1) * 2 - head;
    for(int16_t i = -3; i <= 3; ++i){
        int16_t idx = head + i;
        if(idx < 0 || idx >= NUM_LEDS) continue;
        uint8_t brightness = 255 - abs(i) * 64;
        RgbColor scaled((accent.R * brightness) / 255, (accent.G * brightness) / 255, (accent.B * brightness) / 255);
        ledStrip.SetPixelColor(idx, scaled);
    }
}

static inline void fillSparkle(uint8_t frame, const RgbColor &accent){
    (void)frame;
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    for(uint8_t i = 0; i < 8; ++i){
        uint16_t idx = random(NUM_LEDS);
        ledStrip.SetPixelColor(idx, accent);
    }
}

static inline void fillTwinkle(uint8_t frame, const RgbColor &accent){
    (void)frame;
    for(uint16_t i = 0; i < NUM_LEDS; ++i){
        uint8_t sparkle = random(0, 100) < 10 ? random(128, 255) : 0;
        RgbColor base(
            static_cast<uint8_t>((accent.R * 128) / 255),
            static_cast<uint8_t>((accent.G * 128) / 255),
            static_cast<uint8_t>((accent.B * 128) / 255));
        RgbColor mixed(
            (base.R + sparkle) > 255 ? 255 : base.R + sparkle,
            (base.G + sparkle) > 255 ? 255 : base.G + sparkle,
            (base.B + sparkle) > 255 ? 255 : base.B + sparkle);
        ledStrip.SetPixelColor(i, mixed);
    }
}

static inline void fillConfetti(uint8_t frame){
    fadeStrip(4);
    uint16_t idx = random(NUM_LEDS);
    ledStrip.SetPixelColor(idx, wheelColor(frame + idx));
}

static inline void fillWaves(uint8_t frame){
    for(uint16_t i = 0; i < NUM_LEDS; ++i){
        float wave = sinf((i + frame) * 0.2f) * 127 + 128;
        ledStrip.SetPixelColor(i, RgbColor(wave, 255 - wave, (wave / 2)));
    }
}

static inline void fillBreathe(uint8_t frame, const RgbColor &accent){
    float phase = (sin(frame * 0.1f) + 1.0f) * 0.5f;
    uint8_t brightness = static_cast<uint8_t>(phase * 255);
    RgbColor scaled(
        (accent.R * brightness) / 255,
        (accent.G * brightness) / 255,
        (accent.B * brightness) / 255);
    ledStrip.ClearTo(scaled);
}

static inline void fillFirefly(uint8_t frame){
    fadeStrip(10);
    for(uint8_t i = 0; i < 4; ++i){
        uint16_t idx = random(NUM_LEDS);
        uint8_t glow = (sin(frame * 0.2f + idx) + 1.0f) * 127;
        ledStrip.SetPixelColor(idx, RgbColor(glow, glow, glow));
    }
}

static inline void fillRipple(uint8_t frame, const RgbColor &accent){
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    uint16_t center = (frame * 2) % NUM_LEDS;
    for(uint16_t i = 0; i < NUM_LEDS; ++i){
        uint16_t dist = abs((int)center - (int)i);
        uint8_t intensity = dist < 40 ? max(0, 255 - dist * 6) : 0;
        RgbColor scaled((accent.R * intensity) / 255, (accent.G * intensity) / 255, (accent.B * intensity) / 255);
        ledStrip.SetPixelColor(i, scaled);
    }
}

static inline void fillDots(uint8_t frame){
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    for(uint8_t i = 0; i < 12; ++i){
        uint16_t idx = (frame * (i + 1) + i * 7) % NUM_LEDS;
        ledStrip.SetPixelColor(idx, wheelColor(idx + frame * 5));
    }
}

static inline void fillGradient(uint8_t frame, const RgbColor &accent){
    for(uint16_t i = 0; i < NUM_LEDS; ++i){
        float t = static_cast<float>(i) / (NUM_LEDS - 1);
        uint8_t r = (accent.R * (1.0f - t)) + (wheelColor(frame).R * t);
        uint8_t g = (accent.G * (1.0f - t)) + (wheelColor(frame).G * t);
        uint8_t b = (accent.B * (1.0f - t)) + (wheelColor(frame).B * t);
        ledStrip.SetPixelColor(i, RgbColor(r, g, b));
    }
}

static inline void fillMeteor(uint8_t frame){
    fadeStrip(20);
    uint16_t head = (frame * 2) % NUM_LEDS;
    for(uint8_t i = 0; i < 10; ++i){
        int16_t idx = head - i;
        if(idx < 0) idx += NUM_LEDS;
        uint8_t brightness = 255 - i * 25;
        RgbColor color = wheelColor(frame + i * 8);
        RgbColor scaled(
            (color.R * brightness) / 255,
            (color.G * brightness) / 255,
            (color.B * brightness) / 255);
        ledStrip.SetPixelColor(idx, scaled);
    }
}

static inline void fillJuggle(uint8_t frame){
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    for(uint8_t i = 0; i < 8; ++i){
        uint16_t pos = (frame * (i + 1)) % NUM_LEDS;
        ledStrip.SetPixelColor(pos, wheelColor(frame + i * 32));
    }
}

static inline void fillAurora(uint8_t frame){
    for(uint16_t i = 0; i < NUM_LEDS; ++i){
        float wave1 = sin((i * 0.12f) + frame * 0.08f) * 0.5f + 0.5f;
        float wave2 = sin((i * 0.07f) - frame * 0.05f) * 0.5f + 0.5f;
        uint8_t r = static_cast<uint8_t>(wave1 * 120);
        uint8_t g = static_cast<uint8_t>(wave2 * 200);
        uint8_t b = static_cast<uint8_t>((wave1 * 0.5f + wave2 * 0.5f) * 255);
        ledStrip.SetPixelColor(i, RgbColor(r, g, b));
    }
}

static inline void fillCandy(uint8_t frame){
    for(uint16_t i = 0; i < NUM_LEDS; ++i){
        bool stripe = ((i + frame) / 4) % 2 == 0;
        ledStrip.SetPixelColor(i, stripe ? RgbColor(255, 80, 120) : RgbColor(40, 0, 70));
    }
}

static inline void fillTwirl(uint8_t frame){
    for(uint16_t i = 0; i < NUM_LEDS; ++i){
        uint8_t hue = (i * 5 + frame * 3) & 0xFF;
        ledStrip.SetPixelColor(i, wheelColor(hue));
    }
}

static inline void initPatterns(){
    if(ledPatternsInitialized) return;
    LedPatternDef defs[LED_PATTERN_COUNT] = {
        {"rainbow", [](uint8_t f, const RgbColor &){ fillRainbowFrame(f); }},
        {"pulse", [](uint8_t f, const RgbColor &accent){ fillPulseFrame(f, accent); }},
        {"chase", [](uint8_t f, const RgbColor &){ fillChaseFrame(f); }},
        {"comet", [](uint8_t f, const RgbColor &){ fillCometFrame(f); }},
        {"color_wipe", [](uint8_t f, const RgbColor &accent){ fillColorWipe(f, accent); }},
        {"theater_chase", [](uint8_t f, const RgbColor &accent){ fillTheaterChase(f, accent); }},
        {"scanner", [](uint8_t f, const RgbColor &accent){ fillScanner(f, accent); }},
        {"sparkle", [](uint8_t f, const RgbColor &accent){ fillSparkle(f, accent); }},
        {"twinkle", [](uint8_t f, const RgbColor &accent){ fillTwinkle(f, accent); }},
        {"confetti", [](uint8_t f, const RgbColor &){ fillConfetti(f); }},
        {"waves", [](uint8_t f, const RgbColor &){ fillWaves(f); }},
        {"breathe", [](uint8_t f, const RgbColor &accent){ fillBreathe(f, accent); }},
        {"firefly", [](uint8_t f, const RgbColor &){ fillFirefly(f); }},
        {"ripple", [](uint8_t f, const RgbColor &accent){ fillRipple(f, accent); }},
        {"dots", [](uint8_t f, const RgbColor &){ fillDots(f); }},
        {"gradient", [](uint8_t f, const RgbColor &accent){ fillGradient(f, accent); }},
        {"meteor", [](uint8_t f, const RgbColor &){ fillMeteor(f); }},
        {"juggle", [](uint8_t f, const RgbColor &){ fillJuggle(f); }},
        {"aurora", [](uint8_t f, const RgbColor &){ fillAurora(f); }},
        {"candy", [](uint8_t f, const RgbColor &){ fillCandy(f); }},
        {"twirl", [](uint8_t f, const RgbColor &){ fillTwirl(f); }},
        {"sparkle_trails", [](uint8_t f, const RgbColor &accent){ fillPulseFrame(f, accent); fillSparkle(f, accent); }},
        {"neon_flow", [](uint8_t f, const RgbColor &accent){ fillGradient(f, accent); fillConfetti(f); }},
        {"calm_sea", [](uint8_t f, const RgbColor &){ fillWaves(f); }}
    };
    for(size_t i = 0; i < LED_PATTERN_COUNT; ++i) LED_PATTERNS[i] = defs[i];
    ledPatternsInitialized = true;
}


static inline unsigned long effectiveAutoplayDelay(){
    int duration = (LedAutoplayDuration > 1) ? LedAutoplayDuration : 1;
    return static_cast<unsigned long>(duration) * 1000UL;
}

static inline void scheduleNextPattern(){
    ledNextPatternSwitch = millis() + effectiveAutoplayDelay();
}

static inline void updatePatternFromName(){
    initPatterns();
    String normalized = LedPattern;
    normalized.trim();
    size_t target = 0;
    bool found = false;
    for(size_t i = 0; i < LED_PATTERN_COUNT; ++i){
        if(normalized.equalsIgnoreCase(LED_PATTERNS[i].name)){
            normalized = LED_PATTERNS[i].name;
            target = i;
            found = true;
            break;
        }
    }
    if(!found){
        normalized = LED_PATTERNS[0].name;
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
    if(currentPatternIndex < 0 || currentPatternIndex >= static_cast<int>(LED_PATTERN_COUNT)){
        currentPatternIndex = 0;
    }
    LED_PATTERNS[currentPatternIndex].render(frame, accent);
    
}

void setup_WS2815(){
    initPatterns();
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
                if(ledLastPowerState){
            ledStrip.SetBrightness(0);
            ledStrip.ClearTo(RgbColor(0, 0, 0));
            ledStrip.Show();
            ledLastPowerState = false;
        }
        ledFrame = 0;
        ledLastUpdate = millis();
        scheduleNextPattern();
        ledLastPowerState = true;
    }

    if(!ledLastPowerState){
        ledStrip.SetBrightness(constrain(new_bright, 0, 255));
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
        LedPattern = LED_PATTERNS[currentPatternIndex].name;
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
