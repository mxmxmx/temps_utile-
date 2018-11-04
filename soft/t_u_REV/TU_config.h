#ifndef TU_CONFIG_H_
#define TU_CONFIG_H_

#if F_CPU != 120000000
 #error "Please compile T&U firmware with CPU speed 120MHz"
#endif

// 60us = 16.666...kHz : Works, SPI transfer ends 2uS before next ISR
// 66us = 15.1515...kHz
// 72us = 13.888...kHz
// 100us = 10Khz
static constexpr uint32_t TU_CORE_ISR_FREQ = 16666U;
static constexpr uint32_t TU_CORE_TIMER_RATE = (1000000UL / TU_CORE_ISR_FREQ);
static constexpr uint32_t TU_UI_TIMER_RATE   = 1000UL;

static constexpr int TU_CORE_TIMER_PRIO = 80; //  yet higher
static constexpr int TU_GPIO_ISR_PRIO   = 112; // higher
static constexpr int TU_UI_TIMER_PRIO   = 128; // default

static constexpr unsigned long REDRAW_TIMEOUT_MS = 1;
static constexpr unsigned long SCREENSAVER_TIMEOUT_MS = 25000; // time out menu (in ms)

#define OCTAVES 12      // # octaves
#define SEMITONES (OCTAVES * 12)

static constexpr unsigned long SPLASHSCREEN_DELAY_MS = 1000;
static constexpr unsigned long SPLASHSCREEN_TIMEOUT_MS = 2048;
static constexpr unsigned long SETTINGS_SAVE_TIMEOUT_MS = 1000;
static constexpr unsigned long APP_SELECTION_TIMEOUT_MS = 25000;

namespace TU {
static constexpr size_t kMaxTriggerDelayTicks = 96;
};

#define EEPROM_CALIBRATIONDATA_START 0
#define EEPROM_CALIBRATIONDATA_END 128

#define EEPROM_GLOBALSTATE_START EEPROM_CALIBRATIONDATA_END
#define EEPROM_GLOBALSTATE_END 512

#define EEPROM_APPDATA_START EEPROM_GLOBALSTATE_END
#define EEPROM_APPDATA_END EEPROMStorage::LENGTH

// This is the available space for all apps' settings (\sa TU_apps.ino)
#define EEPROM_APPDATA_BINARY_SIZE (500 - 4)

#define TU_UI_DEBUG
#define TU_UI_SEPARATE_ISR

#define TU_ENCODERS_ENABLE_ACCELERATION_DEFAULT true

#define TU_CALIBRATION_DEFAULT_FLAGS (0)
//#define TU_CALIBRATION_DEFAULT_FLAGS (CALIBRATION_FLAG_ENCODERS_REVERSED)

#endif // TU_CONFIG_H_
