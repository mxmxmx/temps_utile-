#ifndef TU_OUTPUTS_H_
#define TU_OUTPUTS_H_

#include <stdint.h>
#include <string.h>
#include "TU_config.h"
#include "util/util_math.h"
#include "util/util_macros.h"

extern void set_Output1(uint8_t data);
extern void set_Output2(uint8_t data);
extern void set_Output3(uint8_t data);
extern void set_Output4(uint16_t data);
extern void set_Output5(uint8_t data);
extern void set_Output6(uint8_t data);

extern const int8_t _DAC_CHANNEL;

enum CLOCK_CHANNEL {
  CLOCK_CHANNEL_1,
  CLOCK_CHANNEL_2,
  CLOCK_CHANNEL_3,
  CLOCK_CHANNEL_4,
  CLOCK_CHANNEL_5,
  CLOCK_CHANNEL_6,
  CLOCK_CHANNEL_LAST
};

const uint8_t NUM_CHANNELS = 6;
const uint8_t NUM_DACS = 1;

namespace TU {

class OUTPUTS {
public:
  static constexpr size_t kHistoryDepth = 8;
  static constexpr uint16_t MAX_VALUE = 4095;  // DAC fullscale 
  static constexpr int CALIBRATION_POINTS = 1; // 0v

  struct CalibrationData {
    uint16_t calibrated_Zero[NUM_DACS][CALIBRATION_POINTS];
  };

  static void Init(CalibrationData *calibration_data);

  static void set_all(uint32_t value) {
    for (int i = CLOCK_CHANNEL_1; i < CLOCK_CHANNEL_LAST; ++i)
      values_[i] = USAT16(value);
  }

  template <CLOCK_CHANNEL channel>
  static void set(uint32_t value) {
    values_[channel] = USAT16(value);
  }

  static void set(CLOCK_CHANNEL channel, uint32_t value) {
    values_[channel] = USAT16(value);
  }

  template <CLOCK_CHANNEL channel>
  static void setState(uint32_t value) {
    states_[channel] = USAT16(value);
  }

  static void setState(CLOCK_CHANNEL channel, uint32_t value) {
    states_[channel] = USAT16(value);
  }

  static uint32_t value(size_t index) {
    return values_[index];
  }

  static uint32_t state(size_t index) {
    return states_[index];
  }

  static uint32_t get_zero_offset(CLOCK_CHANNEL channel) {
    
    if (channel == _DAC_CHANNEL) 
      return calibration_data_->calibrated_Zero[0][0];
    else 
      return 0x0;
  }


  static void Update() {

    set_Output1(values_[CLOCK_CHANNEL_1]);
    set_Output2(values_[CLOCK_CHANNEL_2]);
    set_Output3(values_[CLOCK_CHANNEL_3]);
    set_Output4(values_[CLOCK_CHANNEL_4]);
    set_Output5(values_[CLOCK_CHANNEL_5]);
    set_Output6(values_[CLOCK_CHANNEL_6]);
    
    size_t tail = history_tail_;
    history_[CLOCK_CHANNEL_4][tail] = values_[CLOCK_CHANNEL_4];
    history_tail_ = (tail + 1) % kHistoryDepth;
  }

  template <CLOCK_CHANNEL channel>
  static void getHistory(uint16_t *dst){
    size_t head = (history_tail_ + 1) % kHistoryDepth;

    size_t count = kHistoryDepth - head;
    const uint16_t *src = history_[channel] + head;
    while (count--)
      *dst++ = *src++;

    count = head;
    src = history_[channel];
    while (count--)
      *dst++ = *src++;
  }

private:
  static CalibrationData *calibration_data_;
  static uint32_t values_[CLOCK_CHANNEL_LAST];
  static uint32_t states_[CLOCK_CHANNEL_LAST];
  static uint16_t history_[NUM_CHANNELS][kHistoryDepth];
  static volatile size_t history_tail_;
};

}; // namespace TU

#endif // TU_OUTPUTS_H_
