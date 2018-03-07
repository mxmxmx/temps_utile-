#include "TU_patterns.h"
#include "TU_patterns_presets.h"
#include "TU_calibration.h"

namespace TU {

    Pattern user_patterns[Patterns::PATTERN_USER_LAST];
    Pattern dummy_pattern;

    /*static*/
    const int Patterns::NUM_PATTERNS = TU::Patterns::PATTERN_USER_LAST + sizeof(TU::patterns) / sizeof(TU::patterns[0]);

    /*static*/
    void Patterns::Init() {
      for (size_t i = 0; i < PATTERN_USER_LAST; ++i)
        memcpy(&user_patterns[i], &TU::patterns[0], sizeof(Pattern));
    }

    void Patterns::Fill() {
      // hack ... fill up patterns if they're still 0 
      int _sum = 0;
     
      for (size_t i = 0; i < PATTERN_USER_LAST; i++) {
        for (size_t j = 0; j < kMaxPatternLength; j++)
        _sum += user_patterns[i].notes[j];
      }
      // still using defaults ?
      if (_sum == 0x0) { 
        for (size_t i = 0; i < PATTERN_USER_LAST; i++) {
          for (size_t j = 0; j < kMaxPatternLength; j++)
           user_patterns[i].notes[j] = TU::calibration_data.dac.calibration_points[0x0][TU::OUTPUTS::kOctaveZero]; 
        }
      }
    }

    /*static*/
    const Pattern &Patterns::GetPattern(int index) {
      if (index < PATTERN_USER_LAST)
        return user_patterns[index];
      else
        return TU::patterns[index - PATTERN_USER_LAST];
    }

    const char* const pattern_names_short[] = {
        "USER1",
        "USER2",
        "USER3",
        "USER4",
        "DEFAULT"
    };

    const char* const pattern_names[] = {
        "User-defined 1",
        "User-defined 2",
        "User-defined 3",
        "User-defined 4",
        "DEFAULT"
    }; 
} // namespace TU
