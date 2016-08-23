#ifndef TU_PATTERNS_H_
#define TU_PATTERNS_H_

#include "TU_patterns_presets.h"


namespace TU {

typedef TU::Pattern Pattern;

static constexpr int kMaxPatternLength = 16;
static constexpr int kMinPatternLength = 4;

class Patterns {
public:

  static const int NUM_PATTERNS;

  enum {
    PATTERN_USER_0,
    PATTERN_USER_1,
    PATTERN_USER_2,
    PATTERN_USER_3,
    PATTERN_USER_LAST, 
    PATTERN_DEFAULT,
    PATTERN_NONE = PATTERN_USER_LAST,
  };

  static void Init();
  static const Pattern &GetPattern(int index);
};


extern const char *const pattern_names[];
extern const char *const pattern_names_short[];
extern Pattern user_patterns[TU::Patterns::PATTERN_USER_LAST];
extern Pattern dummy_pattern;

};

#endif // TU_PATTERNS_H_
