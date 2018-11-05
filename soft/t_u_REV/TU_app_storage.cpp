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
#include "TU_app_storage.h"
#include "TU_apps.h"

namespace TU {

void AppStorage::Load()
{
  SERIAL_PRINTLN("AppStorage: LENGTH=%u, NUM_SLOTS=%u, SLOT_SIZE=%u",slot_storage_.LENGTH, slot_storage_.num_slots(), slot_storage_.SLOT_SIZE);
  for (size_t s = 0; s < slot_storage_.num_slots(); ++s) {
    auto &slot = slot_storage_[s];
    if (!slot.CheckCRC()) {
      SERIAL_PRINTLN("Slot %u: id=%02x, valid_length=%u -- CRC check failed, resetting...", s, slot.header.id, slot.header.valid_length);
      slot.Reset();
    } else {
      SERIAL_PRINTLN("Slot %u: id=%02x, valid_length=%u", s, slot.header.id, slot.header.valid_length);
    }
  }
}

}; // namespace TU
