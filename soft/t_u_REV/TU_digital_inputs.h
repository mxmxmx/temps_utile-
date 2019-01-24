#ifndef TU_DIGITAL_INPUTS_H_
#define TU_DIGITAL_INPUTS_H_

#include <stdint.h>
#include "TU_config.h"
#include "TU_core.h"

namespace TU {

enum DigitalInput {
  DIGITAL_INPUT_1,
  DIGITAL_INPUT_2,
  DIGITAL_INPUT_LAST
};

#define DIGITAL_INPUT_MASK(x) (0x1 << (x))

static constexpr uint32_t DIGITAL_INPUT_1_MASK = DIGITAL_INPUT_MASK(DIGITAL_INPUT_1);
static constexpr uint32_t DIGITAL_INPUT_2_MASK = DIGITAL_INPUT_MASK(DIGITAL_INPUT_2);

class DigitalInputs {
public:

  static void Init();

  static void Scan();
  
  static void Clear();

  // @return mask of all pins clocked since last call, reset state
  static inline uint32_t clocked() {
    return clocked_mask_;
  }

  // @return mask if pin clocked since last call and reset state
  template <DigitalInput input> static inline uint32_t clocked() {
    return clocked_mask_ & (0x1 << input);
  }

  // @return mask if pin clocked since last call, reset state
  static inline uint32_t clocked(DigitalInput input) {
    return clocked_mask_ & (0x1 << input);
  }

  template <DigitalInput input> static inline bool read_immediate() {
    return !digitalReadFast(input);
  }

  static inline bool read_immediate(DigitalInput input) {
    return !digitalReadFast(input);
  }

  template <DigitalInput input> static inline void clock() {
    clocked_[input] = 1;
  }

  static inline uint8_t global_div_TR1() {
    return global_divisor_TR1_;
  }
  
  static inline void set_global_div_TR1(uint8_t divisor) {
    global_divisor_TR1_ = divisor;
  }

  static inline bool master_clock() {
    return master_clock_TR1_;
  }

  static inline void set_master_clock(bool v) {
    master_clock_TR1_ = v;
  }

private:

  static uint32_t clocked_mask_;
  static volatile uint32_t clocked_[DIGITAL_INPUT_LAST];
  static uint8_t global_divisor_TR1_;
  static bool master_clock_TR1_;

  template <DigitalInput input>
  static uint32_t ScanInput() {
    if (clocked_[input]) {
      clocked_[input] = 0;
      return DIGITAL_INPUT_MASK(input);
    } else {
      return 0;
    }
  }
};

// Helper class for visualizing digital inputs with decay
// Uses 4 bits for decay
class DigitalInputDisplay {
public:
  static constexpr uint32_t kDisplayTime = TU_CORE_ISR_FREQ / 8;
  static constexpr uint32_t kPhaseInc = (0xf << 28) / kDisplayTime;

  void Init() {
    phase_ = 0;
  }

  void Update(uint32_t ticks, bool clocked) {
    uint32_t phase_inc = ticks * kPhaseInc;
    if (clocked) {
      phase_ = 0xffffffff;
    } else {
      uint32_t phase = phase_;
      if (phase) {
        if (phase < phase_inc)
          phase_ = 0;
        else
          phase_ = phase - phase_inc;
      }
    }
  }

  uint8_t getState() const {
    return phase_ >> 28;
  }

private:
  uint32_t phase_;
};

};

#endif // TU_DIGITAL_INPUTS_H_
