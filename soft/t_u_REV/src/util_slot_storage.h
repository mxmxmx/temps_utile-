// Copyright 2018 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
#ifndef UTIL_SLOT_STORAGE_H_
#define UTIL_SLOT_STORAGE_H_

#include <stdint.h>
#include <array>

namespace util {

template <typename STORAGE, size_t BASE_ADDR, size_t END_ADDR, size_t NUM_SLOTS>
class SlotStorage {
public:

  static constexpr size_t LENGTH = END_ADDR - BASE_ADDR;
  static constexpr size_t SLOT_SIZE = ((LENGTH / NUM_SLOTS) + 3) & ~0x03;

  void Load() {
    for (size_t i = 0; i < NUM_SLOTS; ++i)
      STORAGE::read(slot_address(i), slots_[i]);
  }

  size_t num_slots() const {
    return NUM_SLOTS;
  }

  struct Slot {
    bool empty() const {
      return id == 0;
    }

    uint16_t id;
    uint16_t valid_length;
    uint16_t crc;
    uint8_t data[SLOT_SIZE - 3 * sizeof(uint16_t)];

    void Reset() {
      memset(this, 0, sizeof(*this));
    }

    bool CheckCRC() const {
      return crc == CalcCRC();
    }

    uint16_t CalcCRC() const {
      uint16_t c = 0;
      const uint8_t *p = data;
      size_t length = sizeof(data);
      while (length--) {
        c += *p++;
      }
      return c ^ 0xffff;
    }
  };

  Slot & operator [] (size_t index) {
    return slots_[index];
  }

  void Write(size_t slot_index) {
    STORAGE::write(slot_address(slot_index), slots_[slot_index]);
  }

private:

  size_t slot_address(size_t i) {
    return BASE_ADDR + i * SLOT_SIZE;
  }

  std::array<Slot, NUM_SLOTS> slots_;
};

}; // namespace util

#endif // UTIL_SLOT_STORAGE_H_
