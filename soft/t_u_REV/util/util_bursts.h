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

#ifndef UTIL_BURSTS_H_
#define UTIL_BURSTS_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "../extern/dspinst.h"
#include <Arduino.h>

namespace util {

  const uint32_t CENTER = 20; // see parameters in APP_CLK

  const uint64_t damp[CENTER + 0x1] 
  {
    0x6666668, // 0.025 
    0xCCCCCD0, // 0.050 
    0x13333340, // 0.075 
    0x199999A0, // 0.100 
    0x20000000, // 0.125 
    0x26666680, // 0.150 
    0x2CCCCD00, // 0.175 
    0x33333380, // 0.200 
    0x39999A00, // 0.225 
    0x40000080, // 0.250 
    0x46666700, // 0.275 
    0x4CCCCD80, // 0.300 
    0x53333400, // 0.325 
    0x59999A80, // 0.350 
    0x60000100, // 0.375 
    0x66666780, // 0.400 
    0x6CCCCE00, // 0.425 
    0x73333480, // 0.450 
    0x79999B00, // 0.475 
    0x80000100, // 0.500 
    0x86666700, // 0.525 
  }; // = 2^32 * multiplier
  
  static const uint64_t init[CENTER * 2 + 0x1] =
  {
    0x6666668, // 0.025 
    0xCCCCCD0, // 0.050 
    0x199999A0, // 0.100 
    0x26666680, // 0.150 
    0x33333340, // 0.200 
    0x40000000, // 0.250 
    0x4CCCCD00, // 0.300 
    0x59999A00, // 0.350 
    0x66666700, // 0.400 
    0x73333400, // 0.450 
    0x80000100, // 0.500 
    0x8CCCCE00, // 0.550 
    0x99999B00, // 0.600 
    0xA6666800, // 0.650 
    0xB3333500, // 0.700 
    0xC0000200, // 0.750 
    0xCCCCCF00, // 0.800 
    0xD9999C00, // 0.850 
    0xE6666900, // 0.900 
    0xF3333600, // 0.950 
    0x100000000, // 1.000 
    0x10CCCCE00, // 1.050 
    0x119999A00, // 1.100 
    0x126666600, // 1.150 
    0x133333200, // 1.200 
    0x13FFFFE00, // 1.250 
    0x14CCCCA00, // 1.300 
    0x159999600, // 1.350 
    0x166666200, // 1.400 
    0x173332E00, // 1.450 
    0x17FFFFA00, // 1.500 
    0x18CCCC600, // 1.550 
    0x199999200, // 1.600 
    0x1A6665E00, // 1.650 
    0x1B3332A00, // 1.700 
    0x1BFFFF600, // 1.750 
    0x1CCCCC200, // 1.800 
    0x1D9998E00, // 1.850 
    0x1E6665A00, // 1.900 
    0x1F3332600, // 1.950 
    0x1FFFFF200, // 2.000 
  }; // = 2^32 * multiplier

  class Bursts {

  public:

    void Init() {

      burst_size_ = 0x0;
      burst_count_ = burst_size_;
      initial_ = 0x0;
      damping_ = 0x0;
      frequency_ = 0xFFFF;
      ticks_ = 0x0;
    }

    bool new_burst() {

      ticks_ = 0x0;
      // calculate ticks to next burst:
      ticks_to_next_burst(true);
      return true;
    }

    void reset() {
      burst_count_ = 0x0;
    }

    bool process() {

      ticks_++;
      bool next_burst = false;

      if (burst_count_ < burst_size_) {

        if (ticks_ > ticks_to_next_burst_) {

          next_burst = true;
          // calculate ticks to next burst:
          ticks_to_next_burst(false);
        }
      }
      return next_burst;
    }

    void set_density(int32_t density) {
      burst_size_ = density;
    }

    void set_initial(int32_t initial) {
      initial_ = initial;
    }

    void set_frequency(uint32_t frequency) {
      frequency_ = frequency;
    }

    void set_damping(uint32_t damping) {
      damping_ = damping;
    }

    void increment() {
      burst_count_++;
      ticks_ = 0x0;
    }

    uint32_t duty() {
      return (ticks_to_next_burst_ >> 1);
    }

  private:
    int16_t burst_size_;
    int16_t burst_count_;
    uint32_t initial_;
    uint32_t damping_;
    uint32_t frequency_;
    uint32_t ticks_;
    uint32_t ticks_to_next_burst_;

    void ticks_to_next_burst(bool _new) {

      if (_new) {
        // initial frequency:
        if (initial_ == CENTER)
          ticks_to_next_burst_ = frequency_;
        else {
          if (damping_ < CENTER) {
            ticks_to_next_burst_ = multiply_u32xu32_rshift32(frequency_, init[initial_]);
          }
          else {
            // things overflow for x > 1.0 
            uint32_t x = init[initial_] >> 5;
            ticks_to_next_burst_ = multiply_u32xu32_rshift32(frequency_, x) << 5; 
          }
        }
      }
      // subsequent pulses, apply 'damping' (of sorts...)
      else if (burst_count_ < burst_size_) {

        if (damping_ == CENTER)
            return;
        else if (ticks_to_next_burst_ > 0 && (ticks_to_next_burst_ > 0)) {

          if (damping_ > CENTER) {
            // decay .. 
            ticks_to_next_burst_ -= multiply_u32xu32_rshift32(ticks_to_next_burst_, damp[damping_ - CENTER]);
          }
          else if (damping_ < CENTER) {
            // grow ..
            ticks_to_next_burst_ += multiply_u32xu32_rshift32(ticks_to_next_burst_, damp[CENTER - damping_]);
          }
        }   
      }
    };
  };

}; // namespace util


#endif // UTIL_BURSTS_H_
