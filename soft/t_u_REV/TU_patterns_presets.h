#ifndef TU_PATTERNS_PRESETS_H_
#define TU_PATTERNS_PRESETS_H_

#include <Arduino.h>

namespace TU {

  struct Pattern {
    int16_t span;
    size_t num_slots;
    int16_t slots[16];
  };


  const Pattern patterns[] = {
    // Off
    { 0, 0, { } },
    // Semitones
    { 12 << 7, 12, { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1} }
  
  };
}
#endif // TU_PATTERNS__PRESETS_H_
