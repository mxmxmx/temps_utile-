#ifndef TU_PATTERNS_PRESETS_H_
#define TU_PATTERNS_PRESETS_H_

#include <Arduino.h>

namespace TU {

  struct Pattern {
    size_t num_slots;
  };


  const Pattern patterns[] = {
    // default
    { 8 }
  
  };
}
#endif // TU_PATTERNS__PRESETS_H_
