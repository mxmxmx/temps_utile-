#ifndef TU_STRINGS_H_
#define TU_STRINGS_H_

#include <stdint.h>

namespace TU {

  static const int kNumDelayTimes = 8;
  
  namespace Strings {
    extern const char * const seq_playmodes[];
    extern const char * const cv_seq_playmodes[];
    extern const char * const seq_id[];
    extern const char * const trigger_input_names[];
    extern const char * const cv_input_names[];
    extern const char * const no_yes[];
    extern const char * const operators[];
    extern const char * const channel_id[];
    extern const char * const mode[];
    extern const char * const dac_modes[];
    extern const char * const logic_tracking[];
    extern const char * const binary_tracking[];
    extern const char * const encoder_config_strings[];
    extern const char * const pulsewidth_ms[];
    extern const char * const trigger_delay_times[kNumDelayTimes];
    extern const char * const initial_f[];
    extern const char * const damping[];
  };

  extern const uint8_t trigger_delay_ticks[];
};



#endif // TU_STRINGS_H_
