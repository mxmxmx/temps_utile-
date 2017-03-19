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

namespace util {

  const int32_t MAX_SOURCES = 16;

  struct Packet
  {
    uint32_t ON_interval;
    uint32_t OFF_interval;
    int8_t PcktSize;
    uint32_t coeff;
    bool state;
  };

  // computes (((uint64_t)a[31:0] * (uint64_t)b[31:0]) >> 32)
  static inline uint32_t multiply_u32xu32_rshift32(uint32_t a, uint32_t b) __attribute__((always_inline));
  static inline uint32_t multiply_u32xu32_rshift32(uint32_t a, uint32_t b)
  {
  #if defined(KINETISK)
    uint32_t out, tmp;
    asm volatile("umull %0, %1, %2, %3" : "=r" (tmp), "=r" (out) : "r" (a), "r" (b));
    return out;
  #elif defined(KINETISL)
    return 0; // TODO....
  #endif
  }

  class Bursts {

  public:

    void Init() {
      sources_ = 0x1;
      available_sources_ = sources_;
      max_interval_ = 0xFFFF;
      density_ = 0xFF;
      frequency_ = 0xFFFF;
    }

    bool Clock(uint32_t ticks) {

      bool on_state = false;
      uint8_t cnt = 0x0;

      for (int i = 0; i < available_sources_; i++) {

        Packet *packet;
        packet = &burst_[i];

        int8_t _available_packets = packet->PcktSize;

        if (_available_packets > 0) {

          if (!packet->state && packet->OFF_interval < ticks) {
            packet->PcktSize--;
            next_burst(i);
          }

          if (packet->state && packet->ON_interval < ticks) {
            on_state = true;
            packet->state = false;
          }
        }
        else {
          cnt++;
        }
      }

      available_sources_ -= cnt;

      if (available_sources_ <= 0x0) {

        available_sources_ = sources_;

        for (int i = 0;  i < available_sources_; i++) {
          burst_[i].PcktSize = 0x1 + random(density_ + 0x1);
          burst_[i].coeff = 0x1 + i;
          burst_[i].state = true;
          burst_[i].ON_interval = random(frequency_ << 2);
        }
      }

      return on_state;
    }

    void set_max_interval(uint32_t max_interval) {
      max_interval_ = max_interval;
    }

    void set_density(uint32_t density) {
      density_ = density;
    }

    void set_sources(uint8_t sources) {
      sources_ = sources > MAX_SOURCES ? MAX_SOURCES : sources;
    }

    void set_frequency(uint32_t frequency) {
      frequency_ = frequency;
    }

    void reset() {
      available_sources_ = 0x0;
    }

    void next_burst(uint8_t q) {

      Packet *packet;
      packet = &burst_[q];
     
      if (packet->PcktSize) {
        
        uint32_t _coeff;
        packet->coeff += (packet->coeff);
        
        _coeff = random(packet->coeff) + q;
        packet->ON_interval =  (frequency_ * _coeff) >> 2; // multiply_u32xu32_rshift32(_coeff << 28, frequency_);
        packet->OFF_interval = (frequency_ * random(max_interval_)) >> 4;
        packet->state = true;
      }
    }

  private:
    int16_t sources_;
    int16_t available_sources_;
    uint32_t max_interval_;
    uint32_t density_;
    uint32_t frequency_;
    Packet burst_[MAX_SOURCES];
  };

}; // namespace util


#endif // UTIL_BURSTS_H_
