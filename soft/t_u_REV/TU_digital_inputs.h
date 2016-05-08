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

  // @return mask of all pins clocked since last call, reset state
  static inline uint32_t clocked() {
    return
      clocked<DIGITAL_INPUT_1>() |
      clocked<DIGITAL_INPUT_2>();
  }

  // @return mask if pin clocked since last call and reset state
  template <DigitalInput input> static inline uint32_t clocked() {
    // This leaves a very small window where a ::clTUk() might interrupt.
    // With LDREX/STREX it should be possible to close the window, but maybe
    // not necessary
    uint32_t clk = clocked_[input];
    clocked_[input] = 0;
    return clk << input;
  }

  // @return mask if pin clocked since last call, reset state
  static inline uint32_t clocked(DigitalInput input) {
    uint32_t clk = clocked_[input];
    clocked_[input] = 0;
    return clk << input;
  }

  static inline uint32_t peek() {
    uint32_t mask = 0;
    if (clocked_[DIGITAL_INPUT_1]) mask |= DIGITAL_INPUT_1_MASK;
    if (clocked_[DIGITAL_INPUT_2]) mask |= DIGITAL_INPUT_2_MASK;
    return mask;
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

private:
  static volatile uint32_t clocked_[DIGITAL_INPUT_LAST];
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
