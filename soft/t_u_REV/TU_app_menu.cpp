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

#include <Arduino.h>
#include "TU_app_menu.h"
#include "TU_ui.h"
#include "TU_apps.h"

static constexpr uint32_t kBlinkTicks = 200;

namespace TU {

void AppMenu::Page::Init(const char *n, int start, int end)
{
  name = n;
  cursor.Init(start, end);
}

void AppMenu::Init()
{
  current_page_ = APPS_PAGE;
  pages_[LOAD_PAGE].Init("Load", 0, app_storage.num_slots() - 1);
  pages_[SAVE_PAGE].Init("Save", 0, app_storage.num_slots() - 1);
  pages_[APPS_PAGE].Init("Apps", 0, app_switcher.num_available_apps() - 1);

  debug_display_ = false;
  first_ = true;
  slot_armed_ = 0;
  action_aborted_ = false;

  update_enabled_settings();
  pages_[CONF_PAGE].Init("Conf", 0, GLOBAL_CONFIG_SETTING_LAST - 1);
  pages_[CONF_PAGE].cursor.AdjustEnd(num_enabled_settings() - 1);
}

void AppMenu::Resume()
{
  if (first_) {
    size_t last_slot_index = app_switcher.last_slot_index();
    if (last_slot_index < app_storage.num_slots()) {
      pages_[LOAD_PAGE].cursor.Scroll(last_slot_index);
      pages_[SAVE_PAGE].cursor.Scroll(last_slot_index);
    }
    first_ = false;
  }

  pages_[CONF_PAGE].cursor.set_editing(false);
  debug_display_ = false;
  slot_armed_ = 0;
  action_aborted_ = false;
}

void AppMenu::Tick()
{
  uint32_t t = ui.ticks();

  if (ui.read_immediate(CONTROL_BUTTON_R) && !action_aborted_) {
    if (t - ui.button_press_time(CONTROL_BUTTON_R) >= Ui::kLongPressTicks && !slot_armed_) {
      slot_armed_ = 1;
      ticks_ = t;
    }
  } else {
    slot_armed_ = 0;
  }

  if (slot_armed_) {
    if (t - ticks_ > kBlinkTicks) {
      ++slot_armed_;
      ticks_ = t;
    }
  }
}

void AppMenu::Draw() const
{
  using TitleBar = menu::TitleBar<menu::kDefaultMenuStartX, 4, 2>;

  TitleBar::Draw();
  int column = 0;
  for (auto &page : pages_) {
    TitleBar::SetColumn(column++);
    graphics.print(page.name);
  }
  TitleBar::Selected(current_page_);

  if (APPS_PAGE == current_page_)
    DrawAppsPage();
  else if (CONF_PAGE == current_page_)
    DrawConfPage();
  else
    DrawSlotsPage(current_page_);
}

void AppMenu::DrawAppsPage() const
{
  menu::SettingsListItem item;
  item.x = 0;
  item.y = menu::CalcLineY(0);

  auto &cursor = pages_[APPS_PAGE].cursor;
  for (int current = cursor.first_visible();
       current <= cursor.last_visible();
       ++current, item.y += menu::kMenuLineH) {
    auto app_desc = app_switcher.app_desc(current);

    if (current == cursor.cursor_pos()) {
      item.selected = slot_armed_ < 8 || slot_armed_ & 0x1;
    } else {
      item.selected = false;
    }

    item.SetPrintPos();
    graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);  
    graphics.print(app_desc->name);
    if (app_switcher.current_app_id() == app_desc->id)
       graphics.drawBitmap8(item.x + 2, item.y + 1, 4, bitmap_indicator_4x8);
    item.DrawCustom();
  }
}

void AppMenu::DrawSlotsPage(PAGE page) const
{
  menu::SettingsListItem item;
  item.x = 0;
  item.y = menu::CalcLineY(0);

  int last_slot_index = app_switcher.last_slot_index();

  auto &cursor = pages_[page].cursor;
  for (int current = cursor.first_visible();
       current <= cursor.last_visible();
       ++current, item.y += menu::kMenuLineH) {

    auto &slot_info = app_storage[current];
    //auto app = app_switcher.find(slot_info.id);

    if (current == cursor.cursor_pos()) {
      if (LOAD_PAGE == page && !slot_info.loadable())
        item.selected = true;
      else
        item.selected = slot_armed_ < 4 || slot_armed_ & 0x1;
    } else {
      item.selected = false;
    }
    item.SetPrintPos();

    if (current == last_slot_index)
      graphics.drawBitmap8(item.x + 2, item.y + 1, 4, bitmap_indicator_4x8);

    if (SLOT_STATE::CORRUPT == slot_info.state)
      graphics.print('!');
    else
      graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);

    if (!debug_display_) {
      if (SLOT_STATE::EMPTY != slot_info.state)
      // for the time being ...
        graphics.printf("PRESET   >      #%d", current + 0x1); //graphics.printf(app ? app->name : "???? (%02x)", app->id);
      else
        graphics.print("(empty)");
    } else {
      auto &slot = app_storage.storage_slot(current);
      graphics.printf("%04x %04x %u", slot_info.id, slot.header.version, slot.header.valid_length);
    }

    item.DrawCustom();
  }
}

void AppMenu::DrawConfPage() const
{
  auto &cursor = pages_[CONF_PAGE].cursor;

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int setting = enabled_setting_at(settings_list.Next(list_item));
    list_item.DrawDefault(global_config.get_value(setting), global_config.value_attr(setting));
  } 
}

AppMenu::Action AppMenu::HandleEvent(const UI::Event &event)
{
  // This is a bit of a hack to allow "aborting" the R button action.
  // Any other event whil it is pressed will cause it to be ignored until it is
  // released, then repressed. Ideally, this might be a feature of the UI; ;the
  // IgnoreButton feature isn't quite sufficient in this case.
  if (event.mask & CONTROL_BUTTON_R && event.control != CONTROL_BUTTON_R) {
    action_aborted_ = true;
  }

  auto &current_page_cursor = pages_[current_page_].cursor;

  if (UI::EVENT_ENCODER == event.type) {
    if (CONTROL_ENCODER_L == event.control) {
      int page = current_page_ + event.value;
      CONSTRAIN(page, 0, PAGE_LAST - 1);
      current_page_ = static_cast<PAGE>(page);
      current_page_cursor.Scroll(-app_storage.num_slots()); // cursor is a bit erratic, so jump back to line 0
    } else if (CONTROL_ENCODER_R == event.control) {
      if (CONF_PAGE == current_page_ && current_page_cursor.editing()) {
        GLOBAL_CONFIG_SETTING setting = enabled_setting_at(current_page_cursor.cursor_pos());
        if (global_config.change_value(setting, event.value))
          global_config.Apply();
      } else {
        current_page_cursor.Scroll(event.value);
      }
    }
  } else {
    if (CONTROL_BUTTON_DOWN == event.control)
      return { current_page_, ACTION_EXIT, 0 };
 
    if (CONTROL_BUTTON_R == event.control) {
      if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
        if (!action_aborted_) {
          switch(current_page_) {
          case LOAD_PAGE: return { current_page_, ACTION_LOAD, current_page_cursor.cursor_pos() };
          case SAVE_PAGE: return { current_page_, ACTION_SAVE, current_page_cursor.cursor_pos() };
          case APPS_PAGE: return { current_page_, ACTION_INIT, current_page_cursor.cursor_pos() };
          default: break;
          }
        }
        action_aborted_ = false;
      } else {
        switch (current_page_) {
        case LOAD_PAGE:
        case SAVE_PAGE: break; // debug_display_ = !debug_display_; break;
        case APPS_PAGE: return { current_page_, ACTION_SWITCH, current_page_cursor.cursor_pos() }; break;
        case CONF_PAGE: current_page_cursor.toggle_editing(); break;
        default: break;
        }
      }
    }
  }
  return { current_page_, ACTION_NONE, 0 };
}

void AppMenu::update_enabled_settings()
{
  GLOBAL_CONFIG_SETTING *settings = enabled_settings_;
  *settings++ = GLOBAL_CONFIG_SETTING_DIV1;
  *settings++ = GLOBAL_CONFIG_SETTING_SLAVE_TR1;
  num_enabled_settings_ = settings - enabled_settings_;
}

};
