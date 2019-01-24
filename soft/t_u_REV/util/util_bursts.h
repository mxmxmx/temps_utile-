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

namespace util {

  const uint32_t DAMP_CENTER = 30;    // see parameters in APP_CLK
  const uint32_t INITIAL_CENTER = 20; // see parameters in APP_CLK

  const uint64_t damp[DAMP_CENTER + 0x1] 
  {
    0x6666666, // 0.025 
    0xCCCCCCC, // 0.050 
    0x13333333, // 0.075 
    0x19999999, // 0.100 
    0x20000000, // 0.125 
    0x26666666, // 0.150 
    0x2CCCCCCC, // 0.175 
    0x33333333, // 0.200 
    0x39999999, // 0.225 
    0x40000000, // 0.250 
    0x46666666, // 0.275 
    0x4CCCCCCC, // 0.300 
    0x53333333, // 0.325 
    0x59999999, // 0.350 
    0x60000000, // 0.375 
    0x66666666, // 0.400 
    0x6CCCCCCC, // 0.425 
    0x73333333, // 0.450 
    0x79999999, // 0.475 
    0x80000000, // 0.500 
    0x86666666, // 0.525
    0x8CCCCCCC, // 0.550
    0x93333333, // 0.575
    0x99999999, // 0.600
    0xA0000000, // 0.625
    0xA6666666, // 0.650
    0xACCCCCCC, // 0.675
    0xB3333333, // 0.700
    0xB9999999, // 0.725
    0xC0000000, // 0.750
    0xC6666666  // 0.775
  }; // = 2^32 * multiplier
  
  static const uint64_t init[INITIAL_CENTER * 2 + 0x1] =
  {
    0x6666666, // 0.025 
    0xCCCCCCC, // 0.050 
    0x19999999, // 0.100 
    0x26666666, // 0.150 
    0x33333333, // 0.200 
    0x40000000, // 0.250 
    0x4CCCCCCC, // 0.300 
    0x59999999, // 0.350 
    0x66666666, // 0.400 
    0x73333333, // 0.450 
    0x80000000, // 0.500 
    0x8CCCCCCC, // 0.550 
    0x99999999, // 0.600 
    0xA6666666, // 0.650 
    0xB3333333, // 0.700 
    0xC0000000, // 0.750 
    0xCCCCCCCC, // 0.800 
    0xD9999999, // 0.850 
    0xE6666666, // 0.900 
    0xF3333333, // 0.950 
    0x100000000, // 1.000 
    0x10CCCCCCC, // 1.050 
    0x119999999, // 1.100 
    0x126666666, // 1.150 
    0x133333333, // 1.200 
    0x13FFFFFFF, // 1.250 
    0x14CCCCCCC, // 1.300 
    0x159999999, // 1.350 
    0x166666666, // 1.400 
    0x173333333, // 1.450 
    0x17FFFFFFF, // 1.500 
    0x18CCCCCCC, // 1.550 
    0x199999999, // 1.600 
    0x1A6666666, // 1.650 
    0x1B3333333, // 1.700 
    0x1BFFFFFFF, // 1.750 
    0x1CCCCCCCC, // 1.800 
    0x1D9999999, // 1.850 
    0x1E6666666, // 1.900 
    0x1F3333333, // 1.950 
    0x1FFFFFFFF, // 2.000 
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
        if (initial_ == INITIAL_CENTER)
          ticks_to_next_burst_ = frequency_;
        else {
          if (initial_ < INITIAL_CENTER) {
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

        if (damping_ == DAMP_CENTER) {
            return;
        }
        else if (ticks_to_next_burst_ > 0x40) {
          // limit ~ 4ms @ 16k
          if (damping_ > DAMP_CENTER) {
            // decay .. 
            ticks_to_next_burst_ -= multiply_u32xu32_rshift32(ticks_to_next_burst_, damp[damping_ - DAMP_CENTER]);
          }
          else if (damping_ < DAMP_CENTER) {
            // grow ..
            ticks_to_next_burst_ += multiply_u32xu32_rshift32(ticks_to_next_burst_, damp[DAMP_CENTER - damping_]);
          }
        }
        else { 
          // stop
          burst_count_ = burst_size_; 
        }
      }
    };
  };

}; // namespace util


#endif // UTIL_BURSTS_H_
