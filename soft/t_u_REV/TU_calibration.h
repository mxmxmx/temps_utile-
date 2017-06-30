#ifndef TU_CALIBRATION_H_
#define TU_CALIBRATION_H_

#include "TU_ADC.h"
#include "TU_config.h"
#include "TU_outputs.h"
#include "src/util_pagestorage.h"
#include "util/EEPROMStorage.h"

//#define VERBOSE_LUT
#ifdef VERBOSE_LUT
#define LUT_PRINTF(fmt, ...) serial_printf(fmt, ##__VA_ARGS__)
#else
#define LUT_PRINTF(x, ...) do {} while (0)
#endif

//#define CALIBRATION_LOAD_LEGACY

namespace TU {

enum EncoderConfig : uint32_t {
  ENCODER_CONFIG_NORMAL,
  ENCODER_CONFIG_R_REVERSED,
  ENCODER_CONFIG_L_REVERSED,
  ENCODER_CONFIG_LR_REVERSED,
  ENCODER_CONFIG_LAST
};

enum CalibrationFlags {
  CALIBRATION_FLAG_ENCODER_MASK = 0x3
};

struct CalibrationData {
  static constexpr uint32_t FOURCC = FOURCC<'C', 'A', 'L', 1>::value;

  OUTPUTS::CalibrationData dac;
  ADC::CalibrationData adc;

  uint8_t display_offset;
  uint32_t flags;
  uint32_t reserved0;
  uint32_t reserved1;

  EncoderConfig encoder_config() const {
    return static_cast<EncoderConfig>(flags & CALIBRATION_FLAG_ENCODER_MASK);
  }

  EncoderConfig next_encoder_config() {
    uint32_t raw_config = ((flags & CALIBRATION_FLAG_ENCODER_MASK) + 1) % ENCODER_CONFIG_LAST;
    flags = (flags & ~CALIBRATION_FLAG_ENCODER_MASK) | raw_config;
    return static_cast<EncoderConfig>(raw_config);
  }
};

typedef PageStorage<EEPROMStorage, EEPROM_CALIBRATIONDATA_START, EEPROM_CALIBRATIONDATA_END, CalibrationData> CalibrationStorage;

extern CalibrationData calibration_data;

}; // namespace TU

// Forward declarations for screwy build system
struct CalibrationStep;
struct CalibrationState;

#endif // TU_CALIBRATION_H_
