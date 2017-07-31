/////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017, mxmxmx. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person 
// obtaining a copy of this software and associated documentation 
// files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, merge, 
// publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
/////////////////////////////////////////////////////////////////////////

#ifndef UTIL_PHASE_H_
#define UTIL_PHASE_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "../extern/dspinst.h"

namespace util {

  const int32_t multipliers[] = 
  {   0, 655, 1310, 1965, 2620, 3275, 3930, 4585, 5240, 5895, 
      6550, 7205, 7860, 8515, 9170, 9825, 10480, 11135, 11790, 12445, 
      13100, 13755, 14410, 15065, 15720, 16375, 17030, 17685, 18340, 18995, 
      19650, 20305, 20960, 21615, 22270, 22925, 23580, 24235, 24890, 25545, 
      26200, 26855, 27510, 28165, 28820, 29475, 30130, 30785, 31440, 32095, 
      32750, 33405, 34060, 34715, 35370, 36025, 36680, 37335, 37990, 38645, 
      39300, 39955, 40610, 41265, 41920, 42575, 43230, 43885, 44540, 45195, 
      45850, 46505, 47160, 47815, 48470, 49125, 49780, 50435, 51090, 51745, 
      52400, 53055, 53710, 54365, 55020, 55675, 56330, 56985, 57640, 58295, 
      58950, 59605, 60260, 60915, 61570, 62225, 62880, 63535, 64190, 64845
  };

  class Phase {

  public:

    void Init() {

      swing_ = false;
      now_ = false;
      swing_ticks_ = 0x0;
      phase_offset_ = 0x0;
    }

    bool set_phase(int32_t ticks, int32_t pulsewidth, uint8_t amount, uint8_t trigger) {

      bool swingy_swing = false;

      if (!amount || now_) {
        swingy_swing = false;
        now_ = false;
      }
      else if (trigger) {
      // calculate next phase offset

        int32_t swing_ticks, swing_amount;

        swing_ticks = ticks - 0xFF; // margin
        swing_amount = signed_multiply_32x16b(multipliers[amount], swing_ticks);
        swing_amount = signed_saturate_rshift(swing_amount, 16, 0);

        swing_ticks_ = phase_offset_ = swing_amount;

        if (swing_amount > 0) {
          swing_ = true;
          swingy_swing = true;
        }
        else 
          swingy_swing = false;
      }
      return swingy_swing;
    }

    bool now() {
      return now_;
    }

    bool phase() {
      return swing_;
    }

    int32_t phase_offset() {
      return phase_offset_;
    }

    void clear_phase_offset() {
      phase_offset_ = 0x0;
      swing_ = false;
    }

    bool update() {

      bool _swing = false;

      if (swing_) {
        swing_ticks_--;
        _swing = true;

        if (swing_ticks_ < 0x0) {
          swing_ = _swing = false;
          now_ = true;
        }
      }
      return _swing;
    }

  private:
    int32_t swing_ticks_;
    int32_t phase_offset_;
    bool swing_;
    bool now_;
  };

}; // namespace util


#endif // UTIL_PHASE_H_
