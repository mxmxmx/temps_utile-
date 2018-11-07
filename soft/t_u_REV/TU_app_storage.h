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
#ifndef TU_APPS_STORAGE_H_
#define TU_APPS_STORAGE_H_

#include "src/util_misc.h"
#include "src/util_slot_storage.h"
#include "util/EEPROMStorage.h"
#include "TU_config.h"

namespace TU {

struct App;

enum class SLOT_STATE {
  EMPTY,
  CORRUPT,
  OK
};

struct SlotInfo {
  uint16_t id = 0;
  SLOT_STATE state = SLOT_STATE::EMPTY;

  bool loadable() const {
    return id && SLOT_STATE::OK == state;
  }
};

// Augment the base slots with some helpful wrapperonis
class AppStorage {
public:
  static constexpr size_t kNumSlots = 4;
  using SlotStorage = util::SlotStorage<EEPROMStorage, EEPROM_APPDATA_START, EEPROM_APPDATA_END, kNumSlots>;

  void Load();

  inline size_t num_slots() const {
    return slot_storage_.num_slots();
  }

  const SlotInfo & operator [](size_t i) const {
    return slots_[i];
  }

  const SlotStorage::Slot & storage_slot(size_t i) const {
    return slot_storage_[i];
  }

  bool SaveAppToSlot(const App *app, size_t slot_index);
  bool LoadAppFromSlot(const App *app, size_t slot_index) const;

private:

  SlotStorage slot_storage_;
  std::array<SlotInfo, kNumSlots> slots_;

  void CheckSlot(size_t slot_index);
};

extern AppStorage app_storage;

}; // namespace TU

#endif // TU_APPS_STORAGE_H_
