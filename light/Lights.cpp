/*
 * SPDX-FileCopyrightText: 2018-2024 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "LightService"

#include "Lights.h"

#include <android-base/logging.h>

#include <fstream>

#define LEDS            "/sys/class/leds/"

#define BUTTON1_LED     LEDS "button-backlight1/"
#define BUTTON_LED      LEDS "button-backlight/"
#define LCD_LED         LEDS "lcd-backlight/"
#define WHITE_LED       LEDS "white/"

#define BLINK           "blink"
#define BRIGHTNESS      "brightness"
#define DUTY_PCTS       "duty_pcts"
#define PAUSE_HI        "pause_hi"
#define PAUSE_LO        "pause_lo"
#define RAMP_STEP_MS    "ramp_step_ms"
#define START_IDX       "start_idx"

#define MAX_LED_BRIGHTNESS    255
#define MAX_LCD_BRIGHTNESS    4095

/*
 * 8 duty percent steps.
 */
#define RAMP_STEPS 8
/*
 * Each step will stay on for 50ms by default.
 */
#define RAMP_STEP_DURATION 50
/*
 * Each value represents a duty percent (0 - 100) for the led pwm.
 */
static int32_t BRIGHTNESS_RAMP[RAMP_STEPS] = {0, 12, 25, 37, 50, 72, 85, 100};

namespace {
/*
 * Write value to path and close file.
 */
static void set(std::string path, std::string value) {
    std::ofstream file(path);

    if (!file.is_open()) {
        LOG(WARNING) << "failed to write " << value.c_str() << " to " << path.c_str();
        return;
    }

    file << value;
}

static void set(std::string path, int value) {
    set(path, std::to_string(value));
}

static uint32_t getBrightness(const HwLightState& state) {
    uint32_t alpha, red, green, blue;

    /*
     * Extract brightness from AARRGGBB.
     */
    alpha = (state.color >> 24) & 0xFF;
    red = (state.color >> 16) & 0xFF;
    green = (state.color >> 8) & 0xFF;
    blue = state.color & 0xFF;

    /*
     * Scale RGB brightness if Alpha brightness is not 0xFF.
     */
    if (alpha != 0xFF) {
        red = red * alpha / 0xFF;
        green = green * alpha / 0xFF;
        blue = blue * alpha / 0xFF;
    }

    return (77 * red + 150 * green + 29 * blue) >> 8;
}

static inline uint32_t scaleBrightness(uint32_t brightness, uint32_t maxBrightness) {
    return brightness * maxBrightness / 0xFF;
}

static inline uint32_t getScaledBrightness(const HwLightState& state, uint32_t maxBrightness) {
    return scaleBrightness(getBrightness(state), maxBrightness);
}

static void handleBacklight(const HwLightState& state) {
    uint32_t brightness = getScaledBrightness(state, MAX_LCD_BRIGHTNESS);
    set(LCD_LED BRIGHTNESS, brightness);
}

static void handleButtons(const HwLightState& state) {
    uint32_t brightness = getScaledBrightness(state, MAX_LED_BRIGHTNESS);
    set(BUTTON_LED BRIGHTNESS, brightness);
    set(BUTTON1_LED BRIGHTNESS, brightness);
}

/*
 * Scale each value of the brightness ramp according to the
 * brightness of the color.
 */
static std::string getScaledRamp(uint32_t brightness) {
    std::string ramp, pad;

    for (auto const& step : BRIGHTNESS_RAMP) {
        ramp += pad + std::to_string(step * brightness / 0xFF);
        pad = ",";
    }

    return ramp;
}

static void handleNotification(const HwLightState& state) {
    uint32_t whiteBrightness = getScaledBrightness(state, MAX_LED_BRIGHTNESS);

    /* Disable blinking */
    set(WHITE_LED BLINK, 0);

    if (state.flashMode == FlashMode::TIMED) {
        /*
         * If the flashOnMs duration is not long enough to fit ramping up
         * and down at the default step duration, step duration is modified
         * to fit.
         */
        int32_t stepDuration = RAMP_STEP_DURATION;
        int32_t pauseHi = state.flashOnMs - (stepDuration * RAMP_STEPS * 2);
        int32_t pauseLo = state.flashOffMs;

        if (pauseHi < 0) {
            stepDuration = state.flashOnMs / (RAMP_STEPS * 2);
            pauseHi = 0;
        }

        /* White */
        set(WHITE_LED START_IDX, 0 * RAMP_STEPS);
        set(WHITE_LED DUTY_PCTS, getScaledRamp(whiteBrightness));
        set(WHITE_LED PAUSE_LO, pauseLo);
        set(WHITE_LED PAUSE_HI, pauseHi);
        set(WHITE_LED RAMP_STEP_MS, stepDuration);

        /* Enable blinking */
        set(WHITE_LED BLINK, 1);
    } else {
        set(WHITE_LED BRIGHTNESS, whiteBrightness);
    }
}

static inline bool isLit(const HwLightState& state) {
    return state.color & 0x00ffffff;
}

/* Keep sorted in the order of importance. */
static std::vector<LightBackend> backends = {
    { LightType::ATTENTION, handleNotification },
    { LightType::NOTIFICATIONS, handleNotification },
    { LightType::BATTERY, handleNotification },
    { LightType::BACKLIGHT, handleBacklight },
    { LightType::BUTTONS, handleButtons },
};

}  // anonymous namespace

namespace aidl {
namespace android {
namespace hardware {
namespace light {

ndk::ScopedAStatus Lights::setLightState(int id, const HwLightState& state) {
    LightStateHandler handler = nullptr;
    LightType type = static_cast<LightType>(id);
    bool handled = false;

    /* Lock global mutex until light state is updated. */
    std::lock_guard<std::mutex> lock(globalLock);

    /* Update the cached state value for the current type. */
    for (LightBackend& backend : backends) {
        if (backend.type == type) {
            backend.state = state;
            handler = backend.handler;
        }
    }

    /* If no handler has been found, then the type is not supported. */
    if (!handler) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    /* Light up the type with the highest priority that matches the current handler. */
    for (LightBackend& backend : backends) {
        if (handler == backend.handler && isLit(backend.state)) {
            handler(backend.state);
            handled = true;
            break;
        }
    }

    /* If no type has been lit up, then turn off the hardware. */
    if (!handled) {
        handler(state);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* lights) {
    int i = 0;

    for (const LightBackend& backend : backends) {
        HwLight hwLight;
        hwLight.id = (int) backend.type;
        hwLight.type = backend.type;
        hwLight.ordinal = i;
        lights->push_back(hwLight);
        i++;
    }

    return ndk::ScopedAStatus::ok();
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
