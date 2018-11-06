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
#include "TU_apps.h"
#include "APP_CLK.h"

#include "TU_patterns.h"
#include "TU_global_config.h"
#include "TU_ui.h"
#include "TU_app_storage.h"

#include "src/util_misc.h"
#include "src/util_pagestorage.h"
#include "util/util_macros.h"

static constexpr TU::App available_apps[] = {
  INSTANTIATE_APP("CL", 0x0101, "6xclocks", CLOCKS),
};

static constexpr int NUM_AVAILABLE_APPS = ARRAY_SIZE(available_apps);
static constexpr int DEFAULT_APP_INDEX = 0;
static constexpr uint16_t DEFAULT_APP_ID = available_apps[DEFAULT_APP_INDEX].id;

namespace TU {

AppStorage app_storage;

// Global settings are stored separately to actual app setings.
// The theory is that they might not change as often.
struct GlobalState {
  static constexpr uint32_t FOURCC = FOURCC<'T','U','X',3>::value;

  bool encoders_enable_acceleration;
  bool reserved0;

  uint16_t current_app_id;
  uint16_t last_slot_index;
};

typedef PageStorage<EEPROMStorage, EEPROM_GLOBALSTATE_START, EEPROM_GLOBALSTATE_END, GlobalState> GlobalStateStorage;

GlobalState global_state;
GlobalStateStorage global_state_storage;

static void SaveGlobalState() {
  SERIAL_PRINTLN("Saving global state...");
  global_state_storage.Save(global_state);

  SERIAL_PRINTLN("Saved global state in page_index %d", global_state_storage.page_index());
}

static bool SaveCurrentAppToSlot(size_t slot_index);
static bool LoadAppFromSlot(size_t slot_index);
static bool LoadAppFromDefaults(size_t app_index);

namespace apps {

const App *current_app = &available_apps[DEFAULT_APP_INDEX];

void SetCurrentApp(int index) {
  current_app = &available_apps[index];
  global_state.current_app_id = current_app->id;
}

void SetCurrentApp(const App *app) {
  current_app = app;
  global_state.current_app_id = current_app->id;
}

const App *find(uint16_t id) {
  for (const auto &app : available_apps)
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

size_t last_slot_index() {
  return global_state.last_slot_index;
}

void Init(bool reset_settings) {

  global_config.Init();
  for (auto &app : available_apps)
    app.Init();

  global_state.encoders_enable_acceleration = TU_ENCODERS_ENABLE_ACCELERATION_DEFAULT;
  global_state.reserved0 = false;
  global_state.current_app_id = DEFAULT_APP_ID;
  global_state.last_slot_index = app_storage.num_slots();

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
      SERIAL_PRINTLN("Loaded settings from page_index %d, current_app_id is %04x",
                    global_state_storage.page_index(),global_state.current_app_id);
    }
    
    SERIAL_PRINTLN("Encoder acceleration: %s", global_state.encoders_enable_acceleration ? "enabled" : "disabled");
    ui.encoders_enable_acceleration(global_state.encoders_enable_acceleration);

    app_storage.Load();
  }
  global_config.Apply();


  // Get last used slot
  // If slot not valid, or app load fails, fall back on defaults
  int current_app_index = apps::index_of(global_state.current_app_id);
  if (current_app_index < 0 || current_app_index >= NUM_AVAILABLE_APPS) {
    SERIAL_PRINTLN("App id %02x not found, using default...", global_state.current_app_id);
    global_state.current_app_id = DEFAULT_APP_INDEX;
    current_app_index = DEFAULT_APP_INDEX;
  }

  SetCurrentApp(current_app_index);
  current_app->HandleAppEvent(APP_EVENT_RESUME);

  delay(100);
}

}; // namespace apps

void Ui::RunAppMenu() {

  SetButtonIgnoreMask();

  apps::current_app->HandleAppEvent(APP_EVENT_SUSPEND);

  app_menu_.Resume();
  bool exit_loop = false;
  while (!exit_loop && idle_time() < APP_SELECTION_TIMEOUT_MS) {

    while (event_queue_.available() && !exit_loop) {
      UI::Event event = event_queue_.PullEvent();
      if (IgnoreEvent(event))
        continue;

      if (CONTROL_BUTTON_L == event.control) {
        ui.DebugStats();
      } else {
        auto action = app_menu_.HandleEvent(event);
        if (AppMenu::ACTION_EXIT == action.type) {
          exit_loop = true;
        } else if (AppMenu::ACTION_SAVE == action.type) {
          if (SaveCurrentAppToSlot(action.index))
            exit_loop = true;
        } else if(AppMenu::ACTION_LOAD == action.type) {
          if (LoadAppFromSlot(action.index))
            exit_loop = true;
        } else if (AppMenu::ACTION_INIT == action.type) {
          LoadAppFromDefaults(action.index);
          exit_loop = true;
        }
      }
    }
    app_menu_.Tick();

    GRAPHICS_BEGIN_FRAME(true);
    app_menu_.Draw();
    GRAPHICS_END_FRAME();
  }

  TU::ui.encoders_enable_acceleration(global_state.encoders_enable_acceleration);
  event_queue_.Flush();
  event_queue_.Poke();

  apps::current_app->HandleAppEvent(APP_EVENT_RESUME);
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

static bool SaveCurrentAppToSlot(size_t slot_index)
{
  app_storage.SaveAppToSlot(apps::current_app, slot_index);
  global_state.last_slot_index = slot_index;
  SaveGlobalState();
  return true;
}

static bool LoadAppFromSlot(size_t slot_index)
{
  auto &slot_info = app_storage[slot_index];
  if (!slot_info.loadable())
    return false;

  auto app = apps::find(slot_info.id);
  if (!app)
    return false;

  CORE::app_isr_enabled = false;
  delay(1);

  bool loaded = false;
  if (app_storage.LoadAppFromSlot(app, slot_index)) {
    apps::SetCurrentApp(app);
    global_state.last_slot_index = slot_index;
    SaveGlobalState();
    loaded = true;
  } else {
    // Defaults?
  }

  CORE::app_isr_enabled = true;
  return loaded;
}

static bool LoadAppFromDefaults(size_t app_index)
{
  CORE::app_isr_enabled = false;
  delay(1);

  global_config.InitDefaults();
  global_config.Apply();

  apps::SetCurrentApp(app_index);
  apps::current_app->Reset();

  CORE::app_isr_enabled = true;
  return true;
}


}; // namespace TU
