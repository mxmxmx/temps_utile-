#include "TU_patterns.h"
#include "TU_patterns_presets.h"

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
