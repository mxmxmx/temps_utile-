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
  slot_storage_.Load();

  SERIAL_PRINTLN("AppStorage: LENGTH=%u, NUM_SLOTS=%u, SLOT_SIZE=%u",slot_storage_.LENGTH, slot_storage_.num_slots(), slot_storage_.SLOT_SIZE);
  for (size_t s = 0; s < slot_storage_.num_slots(); ++s) {
    CheckSlot(s);
  }
}

void AppStorage::CheckSlot(size_t slot_index)
{
  auto &slot = slot_storage_[slot_index];
  auto &slot_info = slots_[slot_index];

  slot_info.id = slot.header.id;
  if (!slot.header.id || !slot.header.valid_length) {
    SERIAL_PRINTLN("Slot %u: id=%04x, valid_length=%u -- Empty", slot_index, slot.header.id, slot.header.valid_length);
    slot_info.state = SLOT_STATE::EMPTY;
    return;
  }

  slot_info.state = SLOT_STATE::CORRUPT;
  if (!slot.CheckCRC()) {
    SERIAL_PRINTLN("Slot %u: id=%04x, valid_length=%u -- CRC check failed", slot_index, slot.header.id, slot.header.valid_length);
    return;
  }

  SERIAL_PRINTLN("Slot %u: id=%04x, valid_length=%u", slot_index, slot.header.id, slot.header.valid_length);
  auto app = apps::find(slot_info.id);
  if (!app) {
    SERIAL_PRINTLN("Slot %u: id=%04x -- App not found!", slot_index, slot.header.id);
    return;
  }
  SERIAL_PRINTLN("Slot %u: id=%04x found app '%s'", slot_index, slot.header.id, app->name);

  if (slot.header.version != app->storage_version) {
    SERIAL_PRINTLN("Slot %u: id=%04x -- version mismatch, expected %04x, got %04x", slot_index, slot.header.id, app->storage_version, slot.header.version);
  }

  size_t expected_length = app->storageSize();
  if (slot.header.valid_length != expected_length) {
    SERIAL_PRINTLN("Slot %u: id=%04x -- storage length mismatch, expected %u, got %u", slot_index, slot.header.id, expected_length, slot.header.valid_length);
    return;
  }

  slot_info.state = SLOT_STATE::OK;
}

bool AppStorage::SaveAppToSlot(const App *app, size_t slot_index)
{
  SERIAL_PRINTLN("Saving %04x '%s' to slot %u", app->id, app->name, slot_index);
  auto &slot = slot_storage_[slot_index];

  slot.header.id = app->id;
  slot.header.version = app->storage_version;
  slot.header.valid_length = app->Save(slot.data);
  slot.header.crc = slot.CalcCRC();
  slot_storage_.Write(slot_index);

  auto &slot_info = slots_[slot_index];
  slot_info.id = app->id;
  slot_info.state = SLOT_STATE::OK;
  return true;
}

bool AppStorage::LoadAppFromSlot(const App *app, size_t slot_index) const
{
  SERIAL_PRINTLN("Loading %04x '%s' from slot %u", app->id, app->name, slot_index);
  auto &slot = slot_storage_[slot_index];
  if (app->id != slot.header.id || app->storage_version != slot.header.version)
    return false;

  app->Reset();
  size_t len = app->Restore(slot.data);
  if (len != slot.header.valid_length) {
    SERIAL_PRINTLN("Load %04x from slot %u, restored %u but expected %u bytes", app->id, slot_index, len, slot.header.valid_length);
    return false;
  } else {
    return true;
  }
}

}; // namespace TU
