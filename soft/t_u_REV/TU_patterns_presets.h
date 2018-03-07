#ifndef TU_PATTERNS_PRESETS_H_
#define TU_PATTERNS_PRESETS_H_

#include "Arduino.h"

namespace TU {
  
  struct Pattern {
    // size and length is stored in app settings
    int16_t notes[16];
  };
  
  const Pattern patterns[] = {
    
    // default
    { 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0 }
  
  };
}
#endif // TU_PATTERNS__PRESETS_H_
