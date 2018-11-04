// Copyright (c) 2016 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and assTUiated dTUumentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include <Arduino.h>
#include "APP_CLK.h"
#include "TU_patterns.h"
#include "TU_global_config.h"
#include "TU_ui.h"
#include "TU_apps_storage.h"

#include "src/util_misc.h"
#include "src/util_pagestorage.h"
#include "util/util_macros.h"

static constexpr TU::App available_apps[] = {
  INSTATIATE_APP('C','L', "6xclocks", CLOCKS),
};

static constexpr int NUM_AVAILABLE_APPS = ARRAY_SIZE(available_apps);
static constexpr int DEFAULT_APP_INDEX = 0;
static constexpr uint16_t DEFAULT_APP_ID = available_apps[DEFAULT_APP_INDEX].id;

namespace TU {

// Global settings are stored separately to actual app setings.
// The theory is that they might not change as often.
struct GlobalState {
  static constexpr uint32_t FOURCC = FOURCC<'T','U','X',2>::value;

  bool encoders_enable_acceleration;
  bool reserved0;

  uint16_t current_app_id;
  TU::Pattern user_patterns[TU::Patterns::PATTERN_USER_LAST];
};

// App settings are packed into a single blob of binary data; each app's chunk
// gets its own header with id and the length of the entire chunk. This makes
// this a bit more flexible during development.
// Chunks are aligned on 2-byte boundaries for arbitrary reasons (thankfully M4
// allows unaligned access...)

struct AppData {
  static constexpr uint32_t FOURCC = FOURCC<'T','U','A',4>::value;

  static constexpr size_t kAppDataSize = EEPROM_APPDATA_BINARY_SIZE;
  char data[kAppDataSize];
  size_t used;
};

typedef PageStorage<EEPROMStorage, EEPROM_GLOBALSTATE_START, EEPROM_GLOBALSTATE_END, GlobalState> GlobalStateStorage;
typedef PageStorage<EEPROMStorage, EEPROM_APPDATA_START, EEPROM_APPDATA_END, AppData> AppDataStorage;

GlobalState global_state;
GlobalStateStorage global_state_storage;

AppData app_settings;
AppDataStorage app_data_storage;


void save_global_state() {
  SERIAL_PRINTLN("Saving global settings...");

  memcpy(global_state.user_patterns, TU::user_patterns, sizeof(TU::user_patterns));
  
  global_state_storage.Save(global_state);
  SERIAL_PRINTLN("Saved global settings in page_index %d", global_state_storage.page_index());
}

static void save_app_data() {
  SERIAL_PRINTLN("Saving app data... (%u bytes available)", TU::AppData::kAppDataSize);

  app_settings.used = 0;
  char *data = app_settings.data;
  char *data_end = data + TU::AppData::kAppDataSize;

  size_t start_app = random(NUM_AVAILABLE_APPS);
  for (size_t i = 0; i < NUM_AVAILABLE_APPS; ++i) {
    const auto &app = available_apps[(start_app + i) % NUM_AVAILABLE_APPS];
    size_t storage_size = app.storageSize() + sizeof(ChunkHeader);
    if (storage_size & 1) ++storage_size; // Align chunks on 2-byte boundaries
    if (storage_size > sizeof(ChunkHeader) && app.Save) {
      if (data + storage_size > data_end) {
        SERIAL_PRINTLN("*********************");
        SERIAL_PRINTLN("%s: CANNOT BE SAVED, NOT ENOUGH SPACE FOR %u BYTES, %u BYTES AVAILABLE OF %u BYTES TOTAL", app.name, storage_size, data_end - data, AppData::kAppDataSize);
        SERIAL_PRINTLN("*********************");
        continue;
      }

      ChunkHeader *chunk = reinterpret_cast<ChunkHeader *>(data);
      chunk->id = app.id;
      chunk->length = storage_size;
      size_t used = app.Save(chunk + 1);
      SERIAL_PRINTLN("* %s (%02x) : Saved %u bytes... (%u)", app.name, app.id, used, storage_size);

      app_settings.used += chunk->length;
      data += chunk->length;
    }
  }
  SERIAL_PRINTLN("App settings used: %u/%u", app_settings.used, EEPROM_APPDATA_BINARY_SIZE);
  app_data_storage.Save(app_settings);
  SERIAL_PRINTLN("Saved app settings in page_index %d", app_data_storage.page_index());
}

static void restore_app_data() {
  SERIAL_PRINTLN("Restoring app data from page_index %d, used=%u", app_data_storage.page_index(), app_settings.used);

  const char *data = app_settings.data;
  const char *data_end = data + app_settings.used;
  size_t restored_bytes = 0;

  while (data < data_end) {
    const ChunkHeader *chunk = reinterpret_cast<const ChunkHeader *>(data);
    if (data + chunk->length > data_end) {
      SERIAL_PRINTLN("Uh oh, app chunk length %u exceeds available data left (%u)", chunk->length, data_end - data);
      break;
    }

    const App *app = apps::find(chunk->id);
    if (!app) {
      SERIAL_PRINTLN("App %02x not found, ignoring chunk...", app->id);
      if (!chunk->length)
        break;
      data += chunk->length;
      continue;
    }
    size_t expected_length = app->storageSize() + sizeof(ChunkHeader);
    if (expected_length & 0x1) ++expected_length;
    if (chunk->length != expected_length) {
      SERIAL_PRINTLN("* %s (%02x): chunk length %u != %u (storageSize=%u), skipping...", app->name, chunk->id, chunk->length, expected_length, app->storageSize());
      data += chunk->length;
      continue;
    }

    size_t used = 0;
    if (app->Restore) {
      const size_t len = chunk->length - sizeof(ChunkHeader);
      used = app->Restore(chunk + 1);
      SERIAL_PRINTLN("* %s (%02x): Restored %u from %u (chunk length %u)...", app->name, chunk->id, used, len, chunk->length);
    }
    restored_bytes += chunk->length;
    data += chunk->length;
  }

  SERIAL_PRINTLN("App data restored: %u, expected %u", restored_bytes, app_settings.used);
}

static bool SaveAppToSlot(const App *app, size_t slot_index)
{
  SERIAL_PRINTLN("Save %02x to slot %u", app->id, slot_index);

  auto &slot = apps::slot_storage[slot_index];
  slot.Reset();
  slot.id = app->id;
  slot.valid_length = app->Save(slot.data);
  slot.crc = slot.CalcCRC();

  apps::slot_storage.Write(slot_index);
  return true;
}

namespace apps {

const App *current_app = &available_apps[DEFAULT_APP_INDEX];
SlotStorage slot_storage;

void set_current_app(int index) {
  current_app = &available_apps[index];
  global_state.current_app_id = current_app->id;
}

const App *find(uint16_t id) {
  for (auto &app : available_apps)
    if (app.id == id) return &app;
  return nullptr;
}

int index_of(uint16_t id) {
  int i = 0;
  for (const auto &app : available_apps) {
    if (app.id == id) return i;
    ++i;
  }
  return i;
}

size_t num_available_apps() {
  return NUM_AVAILABLE_APPS;
}

const App *app_desc(size_t index) {
  return &available_apps[index];
}

uint16_t current_app_id() {
  return current_app->id;
}

void Init(bool reset_settings) {

  global_config.Init();
  for (auto &app : available_apps)
    app.Init();

  SERIAL_PRINTLN("SlotStorage: LENGTH=%u, NUM_SLOTS=%u, SLOT_SIZE=%u", slot_storage.LENGTH, slot_storage.num_slots(), slot_storage.SLOT_SIZE);
  slot_storage.Load();
  for (size_t s = 0; s < slot_storage.num_slots(); ++s) {
    auto &slot = slot_storage[s];
    if (!slot.CheckCRC()) {
      SERIAL_PRINTLN("Slot %u: id=%02x, valid_length=%u -- CRC check failed, resetting...", s, slot.id, slot.valid_length);
      slot.Reset();
    } else {
      SERIAL_PRINTLN("Slot %u: id=%02x, valid_length=%u", s, slot.id, slot.valid_length);
    }
  }

  global_state.current_app_id = DEFAULT_APP_ID;
  global_state.encoders_enable_acceleration = TU_ENCODERS_ENABLE_ACCELERATION_DEFAULT;
  global_state.reserved0 = false;

  if (reset_settings) {
    if (ui.ConfirmReset()) {
      SERIAL_PRINTLN("Erasing EEPROM settings...");
      EEPtr d = EEPROM_GLOBALSTATE_START;
      size_t len = EEPROMStorage::LENGTH - EEPROM_GLOBALSTATE_START;
      while (len--)
        *d++ = 0;
      SERIAL_PRINTLN("...done");
      SERIAL_PRINTLN("Skipping loading of global/app settings, using defaults...");
      global_state_storage.Init();
      app_data_storage.Init();
    } else {
      reset_settings = false;
    }
  }

  if (!reset_settings) {
    SERIAL_PRINTLN("Loading global settings: struct size is %u, PAGESIZE=%u, PAGES=%u, LENGTH=%u",
                  sizeof(GlobalState),
                  GlobalStateStorage::PAGESIZE,
                  GlobalStateStorage::PAGES,
                  GlobalStateStorage::LENGTH);

    if (!global_state_storage.Load(global_state)) {
      SERIAL_PRINTLN("Settings not loaded or invalid, using defaults...");
    } else {
      SERIAL_PRINTLN("Loaded settings from page_index %d, current_app_id is %02x",
                    global_state_storage.page_index(),global_state.current_app_id);
      memcpy(user_patterns, global_state.user_patterns, sizeof(user_patterns));
    }
    
    SERIAL_PRINTLN("Encoder acceleration: %s", global_state.encoders_enable_acceleration ? "enabled" : "disabled");
    ui.encoders_enable_acceleration(global_state.encoders_enable_acceleration);

    SERIAL_PRINTLN("Loading app data: struct size is %u, PAGESIZE=%u, PAGES=%u, LENGTH=%u",
                  sizeof(AppData),
                  AppDataStorage::PAGESIZE,
                  AppDataStorage::PAGES,
                  AppDataStorage::LENGTH);

    if (!app_data_storage.Load(app_settings)) {
      SERIAL_PRINTLN("App data not loaded, using defaults...");
    } else {
      restore_app_data();
    }
    global_config.Apply();
  }

  int current_app_index = apps::index_of(global_state.current_app_id);
  if (current_app_index < 0 || current_app_index >= NUM_AVAILABLE_APPS) {
    SERIAL_PRINTLN("App id %02x not found, using default...", global_state.current_app_id);
    global_state.current_app_id = DEFAULT_APP_INDEX;
    current_app_index = DEFAULT_APP_INDEX;
  }

  set_current_app(current_app_index);
  current_app->HandleAppEvent(APP_EVENT_RESUME);

  delay(100);
}

}; // namespace apps

static void draw_save_message(uint8_t c) {
  GRAPHICS_BEGIN_FRAME(true);
  uint8_t _size = c % 120;
  graphics.drawRect(63 - (_size >> 1), 31 - (_size >> 2), _size, _size >> 1);  
  GRAPHICS_END_FRAME();
}

void Ui::RunAppMenu() {

  SetButtonIgnoreMask();

  apps::current_app->HandleAppEvent(APP_EVENT_SUSPEND);

  menu::ScreenCursor<5> cursor;
  cursor.Init(0, NUM_AVAILABLE_APPS - 1);
  cursor.Scroll(apps::index_of(global_state.current_app_id));

  bool change_app = false;
  bool save = false;
  bool exit = false;
  while (!exit && idle_time() < APP_SELECTION_TIMEOUT_MS) {

    while (event_queue_.available()) {
      UI::Event event = event_queue_.PullEvent();
      if (IgnoreEvent(event))
        continue;

      if (CONTROL_BUTTON_L == event.control) {
        ui.DebugStats();
      } else {
        auto action = app_menu_.HandleEvent(event);
        if (AppMenu::ACTION_EXIT == action.type) {
          exit = true;
        } else if (AppMenu::ACTION_SAVE == action.type) {
          if (SaveAppToSlot(apps::current_app, action.index))
            exit = true;
        }
      }
    }

    GRAPHICS_BEGIN_FRAME(true);
    app_menu_.Draw();
    GRAPHICS_END_FRAME();
  }

  event_queue_.Flush();
  event_queue_.Poke();

  CORE::app_isr_enabled = false;
  delay(1);
/*
  if (change_app) {
    apps::set_current_app(cursor.cursor_pos());
    if (save) {
      save_global_state();
      save_app_data();
      int cnt = 0x0;
      while(idle_time() < SETTINGS_SAVE_TIMEOUT_MS)
        draw_save_message((cnt++) >> 4);
    }
  }
*/
  TU::ui.encoders_enable_acceleration(global_state.encoders_enable_acceleration);

  // Restore state
  apps::current_app->HandleAppEvent(APP_EVENT_RESUME);
  CORE::app_isr_enabled = true;
}

bool Ui::ConfirmReset() {

  SetButtonIgnoreMask();

  bool done = false;
  bool confirm = false;

  do {
    while (event_queue_.available()) {
      UI::Event event = event_queue_.PullEvent();
      if (IgnoreEvent(event))
        continue;
      if (CONTROL_BUTTON_R == event.control) {
        confirm = true;
        done = true;
      } else if (CONTROL_BUTTON_L == event.control) {
        confirm = false;
        done = true;
      }
    }

    GRAPHICS_BEGIN_FRAME(true);
    weegfx::coord_t y = menu::CalcLineY(0);
    graphics.setPrintPos(menu::kIndentDx, y);
    graphics.print("[L] EXIT");
    y += menu::kMenuLineH;

    graphics.setPrintPos(menu::kIndentDx, y);
    graphics.print("[R] RESET SETTINGS" );
    y += menu::kMenuLineH;
    graphics.setPrintPos(menu::kIndentDx, y);
    graphics.print("    AND ERASE EEPROM");
    GRAPHICS_END_FRAME();

  } while (!done);

  return confirm;
}

}; // namespace TU
